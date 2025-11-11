#pragma once

#include "svarog/core/contracts.hpp"

#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>

namespace svarog::execution {

/**
 * @brief Abstract base class for execution contexts in svarogIO.
 *
 * An execution_context represents a context for executing asynchronous operations.
 * It provides a service registry pattern for managing context-specific services,
 * allowing components to share resources within a single execution context.
 *
 * Design decisions:
 * - Service registry uses type-erasure with std::type_index for type-safe storage
 * - Thread-safe access to services via mutex protection
 * - Move-only semantic to prevent accidental copying of execution contexts
 * - Services are stored as shared_ptr<void> for type-erasure
 *
 * Inspired by Boost.Asio's io_context service model.
 *
 * @note All service registry operations are thread-safe.
 * @note Derived classes must implement stop(), restart(), and stopped() methods.
 */
class execution_context {
public:
    /**
     * @brief Default constructor.
     */
    execution_context() = default;

    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup of derived classes and registered services.
     */
    virtual ~execution_context() = default;

    /**
     * @brief Deleted copy constructor (move-only semantic).
     */
    execution_context(const execution_context&) = delete;

    /**
     * @brief Deleted copy assignment operator (move-only semantic).
     */
    execution_context& operator=(const execution_context&) = delete;

    /**
     * @brief Stop the execution context.
     *
     * Signals the context to stop processing. Derived classes must implement
     * this to halt their execution loops and prevent new work from being queued.
     *
     * @post stopped() returns true after this call completes.
     */
    virtual void stop() = 0;

    /**
     * @brief Restart the execution context after it has been stopped.
     *
     * Resets the context state to allow it to be run again. Can only be called
     * when the context is in stopped state.
     *
     * @pre stopped() must return true before calling restart().
     * @post stopped() returns false after restart completes.
     */
    virtual void restart() = 0;

    /**
     * @brief Check if the execution context has been stopped.
     *
     * @return true if the context has been stopped, false otherwise.
     */
    virtual bool stopped() const noexcept = 0;

    /**
     * @brief Retrieve a service from the registry.
     *
     * Returns a reference to the service of the specified type. If the service
     * is not registered, this violates the precondition and will trigger a
     * contract violation in debug builds.
     *
     * Thread-safe: Multiple threads can safely call use_service() concurrently.
     *
     * @tparam Service The type of service to retrieve.
     * @return Reference to the registered service.
     *
     * @pre !stopped() - Cannot use services after the context has been stopped.
     * @pre has_service<Service>() - Service must be registered before use.
     *
     * Example usage:
     * @code
     * // First, register the service
     * ctx.add_service(std::make_shared<MyService>());
     * 
     * // Then use it (precondition: service must exist)
     * auto& my_service = ctx.use_service<MyService>();
     * my_service.do_something();
     * @endcode
     */
    template<typename Service>
    Service& use_service() {
        SVAROG_EXPECTS(!stopped());
        SVAROG_EXPECTS(has_service<Service>());  // Service must be registered
        
        std::lock_guard lock(m_service_registry_mutex);
        return *std::static_pointer_cast<Service>(
            m_service_registry.at(std::type_index(typeid(Service))));
    }

    /**
     * @brief Add a service to the registry.
     *
     * Registers a service instance with the execution context. If a service of
     * the same type is already registered, this will replace it.
     *
     * Thread-safe: Multiple threads can safely call add_service() concurrently.
     *
     * @tparam Service The type of service to register.
     * @param t_service Shared pointer to the service instance.
     *
     * Example usage:
     * @code
     * auto service = std::make_shared<MyService>();
     * ctx.add_service(service);
     * @endcode
     */
    template<typename Service>
    void add_service(std::shared_ptr<Service> t_service) {
        std::lock_guard lock(m_service_registry_mutex);
        m_service_registry.emplace(std::type_index(typeid(Service)), std::move(t_service));
    }

    /**
     * @brief Check if a service is registered.
     *
     * Thread-safe: Multiple threads can safely call has_service() concurrently.
     *
     * @tparam Service The type of service to check.
     * @return true if the service is registered, false otherwise.
     *
     * Example usage:
     * @code
     * if (ctx.has_service<MyService>()) {
     *     auto& service = ctx.use_service<MyService>();
     *     // use service...
     * }
     * @endcode
     */
    template<typename Service>
    bool has_service() const noexcept {
        std::lock_guard lock(m_service_registry_mutex);
        return m_service_registry.contains(
            std::type_index(typeid(Service))
        );
    }

private:
    /// Mutex protecting concurrent access to the service registry
    mutable std::mutex m_service_registry_mutex;

    /// Service registry: maps type_index to type-erased service instances
    /// Services are stored as shared_ptr<void> to allow any type to be registered
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_service_registry;
};

} // namespace svarog::execution
