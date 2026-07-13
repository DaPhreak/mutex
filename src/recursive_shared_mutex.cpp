#include "phreak_mutex/recursive_shared_mutex.h"

#include <assert.h>
#include <algorithm>
#include <exception>
#include <vector>

namespace {

class recursive_shared_mutex_impl {
public:
    static recursive_shared_mutex_impl& Instance();

    void lock( std::shared_mutex& mutex ) noexcept;
    [[nodiscard]] bool try_lock( std::shared_mutex& mutex ) noexcept;
    void unlock( std::shared_mutex& mutex ) noexcept;
    
    void lock_shared( std::shared_mutex& mutex ) noexcept;
    [[nodiscard]] bool try_lock_shared( std::shared_mutex& mutex ) noexcept;

private:
    using Count = std::make_signed_t<size_t>;
    using Entry = std::pair<const void*,Count>;
    using List  = std::vector<Entry>;
    using Iter  = List::iterator;

    recursive_shared_mutex_impl() = default;
    ~recursive_shared_mutex_impl() noexcept;
    recursive_shared_mutex_impl(const recursive_shared_mutex_impl&)            = delete;
    recursive_shared_mutex_impl& operator=(const recursive_shared_mutex_impl&) = delete;

    Iter   find( const std::shared_mutex& mutex );
    Entry& make( const std::shared_mutex& mutex );

    List mList;
};

recursive_shared_mutex_impl& recursive_shared_mutex_impl::Instance()
{
    thread_local static recursive_shared_mutex_impl res{};

    return res;
}

void recursive_shared_mutex_impl::lock( std::shared_mutex& mutex ) noexcept
{
    if ( auto& entry{ make( mutex ) }; entry.second < 0 ) {
        --entry.second;
    } else {
        if ( entry.second > 0 ) {
            mutex.unlock_shared();
        }
        mutex.lock();
        entry.second = -( entry.second + 1 );
    }
}

bool recursive_shared_mutex_impl::try_lock( std::shared_mutex& mutex ) noexcept
{
    if ( auto it{ find( mutex ) }; it == mList.end() ) {
        if ( mutex.try_lock() ) {
            mList.emplace_back( &mutex, -1 );
            return true;
        }
    } else if ( it->second < 0 ) {
        --it->second;
        return true;
    }
    return false;
}

void recursive_shared_mutex_impl::unlock( std::shared_mutex& mutex ) noexcept
{
    if ( auto it{ find( mutex ) }; it == mList.end() ) {
        std::terminate();
    } else if ( it->second > 0 ) {
        if ( --it->second == 0 ) {
            mutex.unlock_shared();
            mList.erase( std::move( it ) );
        }
    } else if ( ++it->second == 0 ) {
        mutex.unlock();
        mList.erase( std::move( it ) );
    }
}

void recursive_shared_mutex_impl::lock_shared( std::shared_mutex& mutex ) noexcept
{
    if ( auto& entry{ make( mutex ) }; entry.second < 0 ) {
        --entry.second;
    } else if ( ++entry.second == 1 ) {
        mutex.lock_shared();
    }
}

bool recursive_shared_mutex_impl::try_lock_shared( std::shared_mutex& mutex ) noexcept
{
    if ( auto it{ find( mutex ) }; it == mList.end() ) {
        if ( !mutex.try_lock_shared() ) {
            return false;
        }
        mList.emplace_back( &mutex, 1 );
    } else if ( it->second < 0 ) {
        --it->second;
    } else {
        ++it->second;
    }
    return true;
}

recursive_shared_mutex_impl::Iter recursive_shared_mutex_impl::find( const std::shared_mutex& mutex )
{
    if ( auto rit{
        std::find_if( mList.rbegin(), mList.rend(),
        [&] ( const Entry& entry ) { return entry.first == &mutex; } ) };
        rit != mList.rend() )
    {
        return ( ++rit ).base();
    }
    return mList.end(); 
}


recursive_shared_mutex_impl::Entry& recursive_shared_mutex_impl::make( const std::shared_mutex& mutex )
{
    if ( auto rit{
        std::find_if( mList.rbegin(), mList.rend(),
        [&] ( const Entry& entry ) { return entry.first == &mutex; } ) };
        rit != mList.rend() )
    {
        return *rit;
    }
    return mList.emplace_back( &mutex, 0 ); 
}

recursive_shared_mutex_impl::~recursive_shared_mutex_impl() noexcept
{
    assert( mList.empty() );
}

} // namespace

namespace phreak_mutex {

void recursive_shared_mutex::lock() noexcept
{
    recursive_shared_mutex_impl::Instance().lock( mMutex );
}

bool recursive_shared_mutex::try_lock() noexcept
{
    return recursive_shared_mutex_impl::Instance().try_lock( mMutex );
}

void recursive_shared_mutex::unlock() noexcept
{
    recursive_shared_mutex_impl::Instance().unlock( mMutex );
}

void recursive_shared_mutex::lock_shared() noexcept
{
    recursive_shared_mutex_impl::Instance().lock_shared( mMutex );
}

bool recursive_shared_mutex::try_lock_shared() noexcept
{
    return recursive_shared_mutex_impl::Instance().try_lock_shared( mMutex );
}

} // phreak_mutex


