#include <iostream>
#include <memory>
#include <string>

#include "svarog/execution/execution_context.hpp"

using namespace svarog;

class LoggingService {
public:
    explicit LoggingService(const std::string& name) : m_name(name) {
        std::cout << "[Service] " << m_name << " constructed" << std::endl;
    }

    ~LoggingService() {
        std::cout << "[Service] " << m_name << " destroyed" << std::endl;
    }

    void log(const std::string& message) {
        std::cout << "[" << m_name << "] " << message << std::endl;
    }

    void on_shutdown() {
        std::cout << "[Service] " << m_name << " shutting down..." << std::endl;
    }

private:
    std::string m_name;
};

class ConfigService {
public:
    void set(const std::string& key, const std::string& value) {
        std::cout << "[Config] Set: " << key << " = " << value << std::endl;
    }

    void on_shutdown() {
        std::cout << "[Config] Flushing configuration..." << std::endl;
    }
};

class CustomContext : public execution::execution_context<> {
public:
    CustomContext() {
        std::cout << "[Context] Created" << std::endl;
    }

    ~CustomContext() override {
        std::cout << "[Context] Destroying..." << std::endl;
    }

    void stop() override {
        std::cout << "[Context] Stopping..." << std::endl;
        m_stopped = true;
    }

    void restart() override {
        std::cout << "[Context] Restarting..." << std::endl;
        m_stopped = false;
    }

    bool stopped() const noexcept override {
        return m_stopped;
    }

private:
    bool m_stopped{false};
};

int main() {
    std::cout << "=== Execution Context Example ===" << std::endl;
    std::cout << "Demonstrating service registry pattern" << std::endl;
    std::cout << std::endl;

    {
        CustomContext ctx;

        std::cout << "\n--- Adding Services ---" << std::endl;

        auto logger = std::make_shared<LoggingService>("MainLogger");
        ctx.add_service(logger);

        auto& config = ctx.make_service<ConfigService>();
        config.set("max_connections", "100");

        std::cout << "\n--- Using Services ---" << std::endl;

        auto& logger_ref = ctx.use_service<LoggingService>();
        logger_ref.log("Application started");
        logger_ref.log("Processing data...");

        auto& config_ref = ctx.use_service<ConfigService>();
        config_ref.set("timeout", "30");

        std::cout << "\n--- Checking Service Availability ---" << std::endl;

        if (ctx.has_service<LoggingService>()) {
            std::cout << "LoggingService is available" << std::endl;
        }

        std::cout << "\n--- Lazy Service Creation ---" << std::endl;

        auto& lazy_logger =
            ctx.use_or_make_service<LoggingService>([] { return std::make_shared<LoggingService>("LazyLogger"); });
        lazy_logger.log("This is from lazy-created logger");

        std::cout << "\n--- Context Lifecycle ---" << std::endl;

        std::cout << "Stopped: " << std::boolalpha << ctx.stopped() << std::endl;
        ctx.stop();
        std::cout << "Stopped: " << ctx.stopped() << std::endl;
        ctx.restart();
        std::cout << "Stopped: " << ctx.stopped() << std::endl;

        std::cout << "\n--- Services will be destroyed in reverse order ---" << std::endl;
    }

    std::cout << "\n=== Example Complete ===" << std::endl;
    return 0;
}
