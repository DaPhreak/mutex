#include "recursive_shared_mutex/recursive_shared_mutex.h"

#include <catch2/catch_test_macros.hpp>

#include <thread>

namespace {

TEST_CASE("Lock one thread", "[recursive_shared_mutex]")
{
    mutex::recursive_shared_mutex mutex{};
    std::shared_lock l1{ mutex };
    {
        std::shared_lock l2{ mutex };
        {
            std::scoped_lock l3{ mutex };

            REQUIRE ( mutex.try_lock() );
            REQUIRE ( mutex.try_lock_shared() );
            mutex.unlock();
            mutex.unlock();
        }
    }
}

TEST_CASE("Lock multiple threads", "[recursive_shared_mutex]")
{
    constexpr size_t ThreadNr{100};
    mutex::recursive_shared_mutex mutex{};

    std::vector<std::thread> threads;
    threads.reserve( ThreadNr );
    
    for (size_t i{}; i < ThreadNr ; ++i) {
        threads.emplace_back( [&,i]()
        {
            std::shared_lock l1{ mutex };
            std::this_thread::sleep_for( std::chrono::milliseconds( ThreadNr - i ) );
            {
               std::scoped_lock l2{ mutex };
               std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
            }
            
        } );
    }
    for ( auto& thread: threads ) {
        thread.join();
    }
}

} // ::