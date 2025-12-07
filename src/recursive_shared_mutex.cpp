#include "mutex/recursive_shared_mutex.h"

#include <assert.h>
#include <vector>

namespace {

class LocalLocks {
public:
    using Counter = std::make_signed_t<size_t>;
    using Entry   = std::pair<const void*,Counter>;
    using List    = std::vector<Entry>;
    using Iter    = List::iterator;

    static LocalLocks& Instance();

    Iter get( const mutex::recursive_shared_mutex* instance );
    void erase( Iter iterator );
private:
    LocalLocks() = default;
    ~LocalLocks() noexcept;
    LocalLocks(const LocalLocks&)            = delete;
    LocalLocks& operator=(const LocalLocks&) = delete;
private:
    List mList;
};

LocalLocks& LocalLocks::Instance()
{
    thread_local static LocalLocks res{};

    return res;
}

LocalLocks::Iter LocalLocks::get( const mutex::recursive_shared_mutex* instance )
{
    if ( auto iterator{
        std::find_if( mList.rbegin(), mList.rend(),
        [&] ( const Entry& entry ) { return entry.first == instance; } ) };
        iterator != mList.rend() )
    {
        return ( ++iterator ).base();
    }
    return mList.emplace( mList.end(), instance, 0 ); 
}

void LocalLocks::erase( Iter iterator )
{
    mList.erase( std::move( iterator ) );
}

LocalLocks::~LocalLocks() noexcept
{
    assert( mList.empty() );
}

} // namespace

namespace mutex {

void recursive_shared_mutex::lock() noexcept
{
    auto it{ LocalLocks::Instance().get( this ) };

    if ( it->second < 0 ) {
        --it->second;
    } else {
        if ( it->second > 0 ) {
            mMutex.unlock_shared();
        }
        mMutex.lock();
        it->second = -( it->second + 1 );
    }
}

bool recursive_shared_mutex::try_lock() noexcept
{
    auto& locks{ LocalLocks::Instance() };
    auto it{ locks.get( this ) };

    if ( it->second < 0 ) {
        --it->second;
        return true;
    } else if ( it->second == 0 ) {
        if ( mMutex.try_lock() ) {
            it->second = -1;
            return true;
        }
        locks.erase( std::move( it ) );
    }
    return false;
}

void recursive_shared_mutex::unlock() noexcept
{
    auto& locks{ LocalLocks::Instance() };
    auto it{ locks.get( this ) };

    assert( it->second != 0 );
    if ( it->second > 0 ) {
        if ( --it->second == 0 ) {
            mMutex.unlock_shared();
            locks.erase( std::move( it ) );
        }
    } else if ( it->second < 0 ) {
        if ( ++it->second == 0 ) {
            mMutex.unlock();
            locks.erase( std::move( it ) );
        }
    } else {
        std::terminate();
    }
}

void recursive_shared_mutex::lock_shared() noexcept
{
    auto it{ LocalLocks::Instance().get( this ) };

    if ( it->second < 0 ) {
        --it->second;
    } else {
        if ( it->second == 0 ) {
            mMutex.lock_shared();
        }
        ++it->second;
    }
}

bool recursive_shared_mutex::try_lock_shared() noexcept
{
    auto& locks{ LocalLocks::Instance() };
    auto it{ locks.get( this ) };

    if ( it->second < 0 ) {
        --it->second;
        return true;
    } else if ( it->second == 0 && !mMutex.try_lock_shared() ) {
        locks.erase( std::move( it ) );
        return false;
    }
    ++it->second;
    return true;
}

} // mutex


