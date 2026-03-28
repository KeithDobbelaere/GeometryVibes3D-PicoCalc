#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace gv {

template <typename T, std::size_t Capacity>
class StaticQueue {
public:
    using value_type = T;
    using size_type = std::size_t;

    class iterator {
    public:
        constexpr iterator(StaticQueue* q, size_type i) : q_(q), i_(i) {}

        constexpr T& operator*() const { return (*q_)[i_]; }
        constexpr T* operator->() const { return &(*q_)[i_]; }

        constexpr iterator& operator++() {
            ++i_;
            return *this;
        }

        constexpr bool operator==(const iterator& other) const {
            return q_ == other.q_ && i_ == other.i_;
        }

        constexpr bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

    private:
        StaticQueue* q_ = nullptr;
        size_type i_ = 0;
    };

    class const_iterator {
    public:
        constexpr const_iterator(const StaticQueue* q, size_type i) : q_(q), i_(i) {}

        constexpr const T& operator*() const { return (*q_)[i_]; }
        constexpr const T* operator->() const { return &(*q_)[i_]; }

        constexpr const_iterator& operator++() {
            ++i_;
            return *this;
        }

        constexpr bool operator==(const const_iterator& other) const {
            return q_ == other.q_ && i_ == other.i_;
        }

        constexpr bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }

    private:
        const StaticQueue* q_ = nullptr;
        size_type i_ = 0;
    };

public:
    constexpr StaticQueue() = default;

    constexpr size_type size() const { return size_; }
    static constexpr size_type capacity() { return Capacity; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr bool full() const { return size_ == Capacity; }

    constexpr void clear() {
        head_ = 0;
        size_ = 0;
    }

    constexpr T& operator[](size_type i) { return data_[physicalIndex(i)]; }
    constexpr const T& operator[](size_type i) const { return data_[physicalIndex(i)]; }

    constexpr T& front() { return (*this)[0]; }
    constexpr const T& front() const { return (*this)[0]; }

    constexpr T& back() { return (*this)[size_ - 1]; }
    constexpr const T& back() const { return (*this)[size_ - 1]; }

    constexpr iterator begin() { return iterator(this, 0); }
    constexpr const_iterator begin() const { return const_iterator(this, 0); }
    constexpr const_iterator cbegin() const { return const_iterator(this, 0); }

    constexpr iterator end() { return iterator(this, size_); }
    constexpr const_iterator end() const { return const_iterator(this, size_); }
    constexpr const_iterator cend() const { return const_iterator(this, size_); }

    // Appends at the back. If full, overwrites the oldest item.
    constexpr bool push_back(const T& value) {
        if (size_ < Capacity) {
            data_[physicalIndex(size_)] = value;
            ++size_;
        } else {
            data_[head_] = value;
            head_ = nextIndex(head_);
        }
        return true;
    }

    // Appends at the back. If full, overwrites the oldest item.
    constexpr bool push_back(T&& value) {
        if (size_ < Capacity) {
            data_[physicalIndex(size_)] = std::move(value);
            ++size_;
        } else {
            data_[head_] = std::move(value);
            head_ = nextIndex(head_);
        }
        return true;
    }

    // Appends at the back. If full, overwrites the oldest item.
    template <typename... Args>
    constexpr bool emplace_back(Args&&... args) {
        if (size_ < Capacity) {
            data_[physicalIndex(size_)] = T{std::forward<Args>(args)...};
            ++size_;
        } else {
            data_[head_] = T{std::forward<Args>(args)...};
            head_ = nextIndex(head_);
        }
        return true;
    }

    constexpr void pop_front() {
        if (size_ > 0) {
            head_ = nextIndex(head_);
            --size_;
        }
    }

private:
    constexpr size_type nextIndex(size_type i) const {
        ++i;
        if (i == Capacity) {
            i = 0;
        }
        return i;
    }

    constexpr size_type physicalIndex(size_type logicalIndex) const {
        size_type i = head_ + logicalIndex;
        if (i >= Capacity) {
            i -= Capacity;
        }
        return i;
    }

private:
    std::array<T, Capacity> data_{};
    size_type head_ = 0;
    size_type size_ = 0;
};

} // namespace gv