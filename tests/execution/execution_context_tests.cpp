#include <catch2/catch_test_macros.hpp>

#include <svarog/execution/execution_context.hpp>
#include <memory>
#include <thread>
#include <vector>

// Mock execution_context for testing (cannot instantiate abstract class)
class mock_execution_context : public svarog::execution::execution_context {
public:
    void stop() override { stopped_ = true; }
    void restart() override { stopped_ = false; }
    bool stopped() const noexcept override { return stopped_; }
    
private:
    bool stopped_ = false;
};

// Simple test service
struct test_service {
    int value;
    explicit test_service(int v) : value(v) {}
};

// Service with shutdown hook
struct service_with_hook {
    static inline int shutdown_count = 0;
    int id;
    
    explicit service_with_hook(int i) : id(i) {}
    
    void on_shutdown() {
        ++shutdown_count;
    }
    
    static void reset() { shutdown_count = 0; }
};

TEST_CASE("execution_context service registry", "[execution_context]") {
    SECTION("Single service registration") {
        mock_execution_context context;
        
        auto service = std::make_shared<test_service>(42);
        context.add_service(service);
        
        REQUIRE(context.has_service<test_service>());
        auto& retrieved = context.use_service<test_service>();
        REQUIRE(retrieved.value == 42);
    }

    SECTION("Multiple service types") {
        mock_execution_context context;
        
        struct service_a { int x = 1; };
        struct service_b { int y = 2; };
        
        context.make_service<service_a>();
        context.make_service<service_b>();
        
        REQUIRE(context.has_service<service_a>());
        REQUIRE(context.has_service<service_b>());
        REQUIRE(context.use_service<service_a>().x == 1);
        REQUIRE(context.use_service<service_b>().y == 2);
    }

    SECTION("Service singleton per context") {
        mock_execution_context context;
        
        auto& service1 = context.make_service<test_service>(100);
        auto& service2 = context.use_service<test_service>();
        
        // Should be the same instance
        REQUIRE(&service1 == &service2);
        REQUIRE(service2.value == 100);
    }
}

TEST_CASE("execution_context service lifecycle", "[execution_context]") {
    SECTION("Services destroyed with context") {
        service_with_hook::reset();
        
        {
            mock_execution_context context;
            context.make_service<service_with_hook>(1);
            context.make_service<service_with_hook>(2);
        } // Destructor calls shutdown hooks
        
        REQUIRE(service_with_hook::shutdown_count == 2);
    }
    
    SECTION("Shutdown hooks called in reverse order") {
        service_with_hook::reset();
        static std::vector<int> order;
        
        struct tracked_service {
            int id;
            explicit tracked_service(int i) : id(i) {}
            void on_shutdown() { order.push_back(id); }
        };
        
        order.clear();
        {
            mock_execution_context context;
            context.make_service<tracked_service>(1);
            context.make_service<tracked_service>(2);
            context.make_service<tracked_service>(3);
        }
        
        // Should be destroyed in reverse order: 3, 2, 1
        REQUIRE(order.size() == 3);
        REQUIRE(order[0] == 3);
        REQUIRE(order[1] == 2);
        REQUIRE(order[2] == 1);
    }
}

TEST_CASE("execution_context lazy initialization", "[execution_context]") {
    mock_execution_context context;
    
    SECTION("use_or_make_service creates if not exists") {
        REQUIRE_FALSE(context.has_service<test_service>());
        
        auto& service = context.use_or_make_service<test_service>(200);
        
        REQUIRE(context.has_service<test_service>());
        REQUIRE(service.value == 200);
    }
    
    SECTION("use_or_make_service returns existing") {
        auto& service1 = context.use_or_make_service<test_service>(300);
        auto& service2 = context.use_or_make_service<test_service>(999);
        
        // Same instance, ignores second args
        REQUIRE(&service1 == &service2);
        REQUIRE(service2.value == 300);
    }
    
    SECTION("use_or_make_service with factory") {
        auto& service = context.use_or_make_service<test_service>([]{
            return std::make_shared<test_service>(500);
        });
        
        REQUIRE(service.value == 500);
    }
}

TEST_CASE("execution_context service replacement", "[execution_context]") {
    mock_execution_context context;
    
    auto service1 = std::make_shared<test_service>(100);
    context.add_service(service1);
    REQUIRE(context.use_service<test_service>().value == 100);
    
    // Replace with new service
    auto service2 = std::make_shared<test_service>(200);
    context.add_service(service2);
    REQUIRE(context.use_service<test_service>().value == 200);
}

TEST_CASE("execution_context thread safety", "[execution_context]") {
    mock_execution_context context;
    
    SECTION("Concurrent add_service") {
        constexpr int num_threads = 10;
        std::vector<std::thread> threads;
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&context, i]() {
                auto service = std::make_shared<test_service>(i);
                context.add_service(service);
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        REQUIRE(context.has_service<test_service>());
    }
    
    SECTION("Concurrent use_service") {
        context.make_service<test_service>(42);
        
        constexpr int num_threads = 10;
        std::vector<std::thread> threads;
        std::atomic<int> success_count{0};
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&context, &success_count]() {
                auto& service = context.use_service<test_service>();
                if (service.value == 42) {
                    ++success_count;
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        REQUIRE(success_count == num_threads);
    }
}

TEST_CASE("execution_context stopped state", "[execution_context]") {
    mock_execution_context context;
    
    REQUIRE_FALSE(context.stopped());
    
    context.stop();
    REQUIRE(context.stopped());
    
    context.restart();
    REQUIRE_FALSE(context.stopped());
}
