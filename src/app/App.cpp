#include "App.hpp"
#include "StatusOverlayView.hpp"
#include "platform/Keys.hpp"
#include "render/Colors.hpp"
#include <cstdio>


namespace gv {
    
namespace {
        
constexpr uint32_t kFrameUs = 33333;      // ~30 FPS
constexpr uint32_t kBatteryPollUs = 2000000; // 2 seconds
constexpr uint32_t kAppSerialUs = 1000000; // 1 second

inline void updateXipFooter(gv::StatusOverlay& overlay,
                            bool enabled,
                            const gv::XipProfileStats& stats) {
    if (!enabled) {
        overlay.clearFooterLeft();
        return;
    }

    char buf[24];
    std::snprintf(
        buf, sizeof(buf),
        "XIP %u%% M%u",
        unsigned(stats.hitPercent),
        unsigned(stats.misses)
    );

    overlay.setFooterLeft(buf, gv::color::Gray);
}
    
struct BatteryFooterCache {
    uint32_t accumUs = kBatteryPollUs; // force immediate first update
    uint8_t level = 0;
    bool charging = false;
    bool valid = false;
};

inline void updateBatteryFooter(gv::StatusOverlay& overlay,
                         const gv::IPlatform& platform,
                         BatteryFooterCache& cache,
                         uint32_t dtUs) {
    cache.accumUs += dtUs;
    if (cache.valid && cache.accumUs < kBatteryPollUs) {
        return;
    }
    cache.accumUs = 0;
    
    const uint8_t level = platform.batteryLevelPercent();
    const bool charging = platform.batteryCharging();
    
    if (cache.valid && cache.level == level && cache.charging == charging) {
        return;
    }
    
    cache.level = level;
    cache.charging = charging;
    cache.valid = true;
    
    uint16_t color = gv::color::Green;
    if (level < 10) {
        color = gv::color::Red;
    } else if (level < 30) {
        color = gv::color::Yellow;
    }
    
    char buf[32];
    if (charging) {
        std::snprintf(buf, sizeof(buf), "Batt Charging: %u%%", unsigned(level));
    } else {
        std::snprintf(buf, sizeof(buf), "Batt: %u%%", unsigned(level));
    }
    
    overlay.setFooterRight(buf, color);
}

inline bool keyToChar(uint8_t key, char& out) {
    if (key >= 'A' && key <= 'Z') {
        out = static_cast<char>(key);
        return true;
    }
    if (key >= 'a' && key <= 'z') {
        out = static_cast<char>(key);
        return true;
    }
    if (key >= '0' && key <= '9') {
        out = static_cast<char>(key);
        return true;
    }
    if (key == KEY_SPACE) {
        out = ' ';
        return true;
    }
    if (key == '-' || key == '_') {
        out = static_cast<char>(key);
        return true;
    }
    return false;
}

} // anon

int App::run(IPlatform* platform) {
    plat_ = platform;
    init(*plat_);

    uint32_t accumUs = 0;
    uint32_t appSerialAccumUs = 0;
    BatteryFooterCache batteryCache{};

    while (true) {
        uint32_t dtUs = plat_->dtUs();
        if (dtUs > 250000) dtUs = 250000;
        accumUs += dtUs;

        if (accumUs < kFrameUs) {
            continue;
        }

        plat_->input().update();
        InputState in = pollInput();

        if (in.overlayTogglePressed) {
            statusOverlay_.toggleVisible();
        }

        accumUs -= kFrameUs;

        if (currentState_) {
            currentState_->update(*this, in, kFrameUs);
            auto& display = plat_->display();

            updateBatteryFooter(statusOverlay_, *plat_, batteryCache, kFrameUs);

            if (currentState_->rendersDirectly()) {
                updateXipFooter(statusOverlay_, xipProfilingEnabled_, xipProfileStats_);
                currentState_->renderDirect(*this, display);
                continue;
            }

            if (xipProfilingEnabled_) {
                xipProfileBeginFrame();
            }

            display.beginFrame();
            currentState_->render(*this, frame_);
            if (xipProfilingEnabled_) {
                xipProfileStats_ = xipProfileEndFrame();
            } else {
                xipProfileStats_ = XipProfileStats{};
            }

            if (plat_->serialOutputEnabled()) {
                appSerialAccumUs += kFrameUs;
                if (appSerialAccumUs >= kAppSerialUs) {
                    appSerialAccumUs = 0;
                    emitAppSerial();
                }
            }

            updateXipFooter(statusOverlay_, xipProfilingEnabled_, xipProfileStats_);

            StatusOverlayView::appendTo(frame_, statusOverlay_, display.width(), display.height());
            display.draw(frame_);
            display.endFrame();
        }
    }

    return 0;
}

std::size_t App::unlockedLevelCount() const {
#ifdef GV3D_TESTING
    return kLevelCount;
#else
    return unlockedLevelCount_;
#endif
}

bool App::isLevelUnlocked(std::size_t i) const {
#ifdef GV3D_TESTING
    return i < kLevelCount;
#else
    return i < unlockedLevelCount_;
#endif
}

const char* App::levelName(std::size_t i) const {
    if (i >= kLevelCount) return "";
    return kLevels[i].name;
}

const char* App::levelPath(std::size_t i) const {
    if (i >= kLevelCount) return "";
    return kLevels[i].path;
}

int App::levelPercentComplete(std::size_t levelIndex) const {
    if (!hasActiveSave()) return 0;
    if (levelIndex >= kLevelCount) return 0;

    return int(saveData_.levelPercent(activeSaveIndex_, selectedDifficulty_, levelIndex));
}

void App::setSelectedDifficulty(Difficulty difficulty) {
    selectedDifficulty_ = difficulty;

    if (hasActiveSave()) {
        (void)saveData_.setSelectedDifficulty(activeSaveIndex_, selectedDifficulty_);
        syncRuntimeProgressFromActiveSave();
        (void)saveSaves();
    }
}

bool App::loadSaves() {
    if (!plat_) return false;

    if (!saveData_.load(plat_->fs())) {
        return false;
    }

    if (saveData_.hasLastPlayed()) {
        activeSaveIndex_ = saveData_.lastPlayedIndex();
        selectedDifficulty_ = saveData_.selectedDifficulty(activeSaveIndex_);
        syncRuntimeProgressFromActiveSave();
    } else {
        clearActiveSaveState();
    }

    return true;
}

bool App::saveSaves() const {
    if (!plat_) return false;
    return saveData_.save(plat_->fs());
}

bool App::createNewSave(const char* name) {
    uint8_t index = SaveData::kNoSelection;
    if (!saveData_.createEntry(name, index)) {
        return false;
    }

    activeSaveIndex_ = index;
    selectedDifficulty_ = saveData_.selectedDifficulty(index);
    saveData_.setLastPlayedIndex(index);
    syncRuntimeProgressFromActiveSave();
    return saveSaves();
}

bool App::selectSave(std::size_t index) {
    if (index >= saveData_.entryCount()) {
        return false;
    }

    activeSaveIndex_ = static_cast<uint8_t>(index);
    selectedDifficulty_ = saveData_.selectedDifficulty(activeSaveIndex_);

    if (!saveData_.setLastPlayedIndex(activeSaveIndex_)) {
        return false;
    }

    syncRuntimeProgressFromActiveSave();
    return saveSaves();
}

bool App::deleteSave(std::size_t index) {
    if (index >= saveData_.entryCount()) {
        return false;
    }

    if (!saveData_.deleteEntry(index)) {
        return false;
    }

    if (saveData_.hasLastPlayed()) {
        activeSaveIndex_ = saveData_.lastPlayedIndex();
        selectedDifficulty_ = saveData_.selectedDifficulty(activeSaveIndex_);
        syncRuntimeProgressFromActiveSave();
    } else {
        clearActiveSaveState();
    }

    return saveSaves();
}

bool App::canContinue() const {
    return hasAnySave() && saveData_.hasLastPlayed();
}

bool App::continueGame() {
    if (!canContinue()) {
        return false;
    }

    const uint8_t idx = saveData_.lastPlayedIndex();
    if (idx >= saveData_.entryCount()) {
        return false;
    }

    activeSaveIndex_ = idx;
    syncRuntimeProgressFromActiveSave();

    const std::size_t frontier =
        (unlockedLevelCount_ > 0) ? (unlockedLevelCount_ - 1) : 0;

    return startLevel(frontier);
}

void App::syncRuntimeProgressFromActiveSave() {
    if (!hasActiveSave()) {
        clearActiveSaveState();
        return;
    }

#ifdef GV3D_TESTING
    unlockedLevelCount_ = kLevelCount;
#else
    std::size_t unlocked = saveData_.unlockedCount(activeSaveIndex_, selectedDifficulty_);
    if (unlocked < 1) unlocked = 1;
    if (unlocked > kLevelCount) unlocked = kLevelCount;
    unlockedLevelCount_ = unlocked;
#endif

    selectedLevel_ = (unlockedLevelCount_ > 0) ? (unlockedLevelCount_ - 1) : 0;
    if (selectedLevel_ >= kLevelCount) {
        selectedLevel_ = kLevelCount - 1;
    }
}

bool App::saveCurrentProgress() {
    if (!hasActiveSave()) return false;
    if (selectedLevel_ >= kLevelCount) return false;

    const uint8_t progress = static_cast<uint8_t>(game_.progressPercent());
    const uint8_t oldProgress = static_cast<uint8_t>(
        saveData_.levelPercent(activeSaveIndex_, selectedDifficulty_, selectedLevel_)
    );

    if (progress <= oldProgress) {
        return saveSaves();
    }

    if (!saveData_.setLevelPercent(activeSaveIndex_, selectedDifficulty_, selectedLevel_, progress)) {
        return false;
    }

    return saveSaves();
}

bool App::saveLevelCompletion(std::size_t levelIndex) {
    if (!hasActiveSave()) return false;
    if (levelIndex >= kLevelCount) return false;

    if (!saveData_.setLevelPercent(activeSaveIndex_, selectedDifficulty_, levelIndex, 100)) {
        return false;
    }

#ifndef GV3D_TESTING
    const uint8_t unlocked = saveData_.unlockedCount(activeSaveIndex_, selectedDifficulty_);
    const std::size_t frontier = (unlocked > 0) ? (unlocked - 1) : 0;

    if (levelIndex == frontier && unlocked < kLevelCount) {
        if (!saveData_.setUnlockedCount(
                activeSaveIndex_,
                selectedDifficulty_,
                static_cast<uint8_t>(unlocked + 1))) {
            return false;
        }
    }
#endif

    syncRuntimeProgressFromActiveSave();
    return saveSaves();
}

uint8_t App::collectedStarsForLevel(std::size_t levelIndex) const {
    if (!hasActiveSave()) return 0;
    if (levelIndex >= kLevelCount) return 0;

    return uint8_t(saveData_.levelStars(activeSaveIndex_, selectedDifficulty_, levelIndex) & 0x07u);
}

bool App::collectStar(std::size_t levelIndex, uint8_t starIndex) {
    if (!hasActiveSave()) return false;
    if (levelIndex >= kLevelCount) return false;
    if (starIndex >= 3) return false;

    const uint8_t oldMask = uint8_t(
        saveData_.levelStars(activeSaveIndex_, selectedDifficulty_, levelIndex) & 0x07u
    );

    if (!saveData_.collectLevelStar(activeSaveIndex_, selectedDifficulty_, levelIndex, starIndex)) {
        return false;
    }

    const uint8_t newMask = uint8_t(
        saveData_.levelStars(activeSaveIndex_, selectedDifficulty_, levelIndex) & 0x07u
    );

    if (newMask == oldMask) {
        return true;
    }

    return saveSaves();
}

void App::changeState(IAppState& next) {
    if (currentState_ == &next) {
        return;
    }

    if (currentState_) {
        currentState_->onExit(*this);
    }

    currentState_ = &next;
    currentState_->onEnter(*this);
}

void App::showTitle() {
    changeState(titleState_);
}

void App::showHomeMenu() {
    changeState(homeMenuState_);
}

void App::showNewGame() {
    changeState(newGameState_);
}

void App::showSavedGames() {
    changeState(savedGamesState_);
}

void App::showOptions() {
    changeState(optionsState_);
}

void App::showLevelSelect() {
    changeState(levelSelectState_);
}

void App::showPlaying() {
    changeState(playingState_);
}

bool App::startLevel(std::size_t levelIndex) {
    if (!plat_) return false;
    if (levelIndex >= kLevelCount) return false;
    if (!isLevelUnlocked(levelIndex)) {
        statusOverlay_.addWarning("Level locked");
        return false;
    }

    game_.reset();
    game_.setFileSystem(&plat_->fs());
    game_.setLevelIndex(levelIndex);
    game_.setDifficulty(selectedDifficulty_);

    const LevelEntry& e = kLevels[levelIndex];
    if (!game_.loadLevel(e.path)) {
        statusOverlay_.addError("Failed to load level");
        statusOverlay_.addInfo(e.path);
        return false;
    }

    if (hasActiveSave() && levelIndex < SaveData::kLevelCount) {
        game_.setCollectedStarsMask(
            saveData_.levelStars(activeSaveIndex_, selectedDifficulty_, levelIndex)
        );
    } else {
        game_.setCollectedStarsMask(0);
    }

    selectedLevel_ = levelIndex;
    showPlaying();
    return true;
}

void App::clearActiveSaveState() {
    activeSaveIndex_ = SaveData::kNoSelection;
    selectedDifficulty_ = Difficulty::Rookie;
    unlockedLevelCount_ = 1;
    selectedLevel_ = 0;
}

void App::emitAppSerial() const {
    char buf[128]{};
    const char* stateName = currentState_ ? currentState_->name() : "None";
    if (xipProfilingEnabled()) {
        const auto& x = xipProfileStats();
        std::snprintf(
            buf, sizeof(buf),
            "APP State: %s Level: %u XIP: %u%% A:%u M:%u",
            stateName,
            unsigned(selectedLevel() + 1),
            unsigned(x.hitPercent),
            unsigned(x.accesses),
            unsigned(x.misses)
        );
    } else {
        std::snprintf(
            buf, sizeof(buf),
            "APP State: %s Level: %u",
            stateName,
            unsigned(selectedLevel() + 1)
        );
    }

    std::printf("%s\n", buf);
}

void App::init(IPlatform& platform) {
    plat_->init();
    (void)plat_->fs().init();

    w_ = plat_->display().width();
    h_ = plat_->display().height();

    game_.reset();
    game_.setFileSystem(&platform.fs());

    statusOverlay_.clear();
    clearActiveSaveState();

    if (!loadSaves()) {
        statusOverlay_.addWarning("Could not load saves");
    }

    showTitle();
}

InputState App::pollInput() const {
    const IInput& kb = plat_->input();

    InputState in{};
    in.thrust        = kb.down(KEY_SPACE);
    in.thrustPressed = kb.pressed(KEY_SPACE);

    in.upPressed    = kb.pressed(KEY_UP);
    in.downPressed  = kb.pressed(KEY_DOWN);
    in.leftPressed  = kb.pressed(KEY_LEFT);
    in.rightPressed = kb.pressed(KEY_RIGHT);

    in.confirm = kb.pressed(KEY_ENTER) || kb.pressed(KEY_RETURN);
    in.back = kb.pressed(KEY_ESC);
    in.pausePressed = kb.pressed(KEY_POWER);
    in.overlayTogglePressed = kb.pressed(KEY_F1);

    in.backspacePressed = kb.pressed(KEY_BACKSPACE);
    in.deletePressed = kb.pressed(KEY_DEL);

    for (uint16_t key = 0x20; key <= 0x7E; ++key) {
        if (kb.pressed(static_cast<uint8_t>(key))) {
            char c = '\0';
            if (keyToChar(static_cast<uint8_t>(key), c)) {
                in.typedChar = c;
                break;
            }
        }
    }

    return in;
}

} // namespace gv