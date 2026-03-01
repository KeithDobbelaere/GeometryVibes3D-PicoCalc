#pragma once

namespace gv {
    
struct InputState {
    bool thrust = false;

    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;

    bool confirm = false;        // "pressed this frame"
    bool back = false;           // "pressed this frame"
    bool pausePressed = false;   // "pressed this frame"
};

} // namespace gv