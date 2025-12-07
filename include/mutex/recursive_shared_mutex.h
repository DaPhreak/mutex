#pragma once

#include <shared_mutex>

namespace mutex {

class recursive_shared_mutex {
public:
    recursive_shared_mutex()                                         = default;
    recursive_shared_mutex(const recursive_shared_mutex&)            = delete;
    recursive_shared_mutex& operator=(const recursive_shared_mutex&) = delete;

    void lock() noexcept;
    [[nodiscard]] bool try_lock() noexcept;
    void unlock() noexcept;
    
    void lock_shared() noexcept;
    [[nodiscard]] bool try_lock_shared() noexcept;
    void unlock_shared() noexcept
    {
        unlock();
    }
private:
    std::shared_mutex mMutex{};
};

} // namespace mutex
