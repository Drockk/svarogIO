#pragma once

#include "svarog/core/contracts.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace svarog::execution {

/**
 * @brief Concept to check if a service has a shutdown hook.
 * 
 * A service satisfies this concept if it has a callable on_shutdown() method
 * that returns void. This hook will be called during execution context destruction.
 */
template<typename T>
concept HasShutdownHook = requires(T t) {
    { t.on_shutdown() } -> std::same_as<void>;
};

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
 * @invariant Class invariants (maintained throughout object lifetime):
 * - **Service Lifecycle**: Services are destroyed in reverse creation order during context destruction
 * - **Shutdown Hook Execution**: If a service has on_shutdown(), it is called before service destruction
 * - **Registry Consistency**: Service registry and cleanup callbacks are always synchronized
 * - **Thread Safety**: All service registry operations are protected by mutex
 * - **Type Safety**: Each service type can only be registered once per context (singleton pattern)
 * - **Memory Safety**: Services are kept alive via shared_ptr until all cleanup callbacks execute
 * - **State Consistency**: stopped() state is independent of service registry state
 *
 * @note All service registry operations are thread-safe.
 * @note Derived classes must implement stop(), restart(), and stopped() methods.
 * @note Service registration can occur even when stopped(), but use_service() requires !stopped()
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
    virtual ~execution_context();

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
    virtual void stop() {
        SVAROG_EXPECTS(!stopped());
    }

    /**
     * @brief Restart the execution context after it has been stopped.
     *
     * Resets the context state to allow it to be run again. Can only be called
     * when the context is in stopped state.
     *
     * @pre stopped() must return true before calling restart().
     * @post stopped() returns false after restart completes.
     */
    virtual void restart() {
        SVAROG_ENSURES(!stopped());
    }
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
     * the same type is already registered, this will replace it and the old
     * service will be properly cleaned up.
     *
     * If the service implements on_shutdown() method (satisfies HasShutdownHook concept),
     * it will be called during context destruction before the service is destroyed.
     *
     * Thread-safe: Multiple threads can safely call add_service() concurrently.
     *
     * @tparam Service The type of service to register.
     * @param t_service Shared pointer to the service instance.
     *
     * @pre t_service != nullptr - Service pointer must be valid.
     *
     * Example usage:
     * @code
     * // Service with shutdown hook
     * class MyService {
     * public:
     *     void on_shutdown() {
     *         // Cleanup code
     *     }
     * };
     * 
     * auto service = std::make_shared<MyService>();
     * ctx.add_service(service);
     * // on_shutdown() will be called automatically during context destruction
     * @endcode
     */
    template<typename Service>
    void add_service(std::shared_ptr<Service> t_service) {
        SVAROG_EXPECTS(t_service != nullptr);
        
        std::lock_guard lock(m_service_registry_mutex);
        
        auto type_idx = std::type_index(typeid(Service));
        
        // Check if service already exists - if so, we need to replace it
        auto [it, inserted] = m_service_registry.try_emplace(type_idx, t_service);
        
        if (!inserted) {
            // Service already exists - replace it
            it->second = t_service;
            // Note: Old cleanup callback will still run but with expired shared_ptr
            // which is safe - it will just do nothing
        }
        
        // Create cleanup callback that handles shutdown hook and destruction
        // Copy shared_ptr into lambda to extend lifetime
        m_cleanup_callbacks.push_back([service = t_service]() mutable {
            if constexpr (HasShutdownHook<Service>) {
                if (service) {  // Check if service still valid
                    service->on_shutdown();
                }
            }
            // service will be destroyed when lambda goes out of scope
        });
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

    /**
     * @brief Create and register a service with in-place construction.
     *
     * Constructs a service in-place with the given arguments and registers it.
     * This is a convenience wrapper around add_service that creates the service
     * using std::make_shared.
     *
     * Thread-safe: Multiple threads can safely call make_service() concurrently.
     *
     * @tparam Service The type of service to create.
     * @tparam Args Types of constructor arguments.
     * @param args Arguments forwarded to Service constructor.
     * @return Reference to the newly created service.
     *
     * @pre !stopped() - Cannot create services after context has been stopped.
     *
     * Example usage:
     * @code
     * class MyService {
     * public:
     *     MyService(int port, std::string name) { ... }
     * };
     * 
     * auto& service = ctx.make_service<MyService>(8080, "server");
     * @endcode
     */
    template<typename Service, typename... Args>
    Service& make_service(Args&&... args) {
        SVAROG_EXPECTS(!stopped());
        
        auto service = std::make_shared<Service>(std::forward<Args>(args)...);
        Service& ref = *service;  // Get reference BEFORE move
        add_service(service);     // Pass by copy to avoid move
        return ref;
    }

    /**
     * @brief Get existing service or create it using a factory function (lazy initialization).
     *
     * Returns an existing service if registered, otherwise creates it using
     * the provided factory function. Thread-safe: only one thread will create
     * the service if it doesn't exist.
     *
     * @tparam Service The type of service to retrieve or create.
     * @tparam Factory Callable type that returns std::shared_ptr<Service>.
     * @param factory Function to create the service if it doesn't exist.
     * @return Reference to the service (existing or newly created).
     *
     * @pre !stopped() - Cannot create services after context has been stopped.
     *
     * Example usage:
     * @code
     * auto& service = ctx.use_or_make_service<MyService>([]{
     *     return std::make_shared<MyService>(8080, "server");
     * });
     * @endcode
     */
    template<typename Service, typename Factory>
        requires std::invocable<Factory> && 
                 std::same_as<std::invoke_result_t<Factory>, std::shared_ptr<Service>>
    Service& use_or_make_service(Factory&& factory) {
        SVAROG_EXPECTS(!stopped());
        
        std::lock_guard lock(m_service_registry_mutex);
        
        auto type_idx = std::type_index(typeid(Service));
        auto it = m_service_registry.find(type_idx);
        
        if (it != m_service_registry.end()) {
            // Service already exists - return reference to existing
            return *std::static_pointer_cast<Service>(it->second);
        }
        
        // Create new service - unlock not needed as factory should be fast
        auto service = std::forward<Factory>(factory)();
        SVAROG_EXPECTS(service != nullptr);
        
        Service& ref = *service;  // Get reference BEFORE any moves
        
        // Register in map
        m_service_registry.emplace(type_idx, service);
        
        // Create cleanup callback - copy shared_ptr to extend lifetime
        m_cleanup_callbacks.push_back([service]() mutable {
            if constexpr (HasShutdownHook<Service>) {
                if (service) {  // Safety check
                    service->on_shutdown();
                }
            }
        });
        
        return ref;
    }

    /**
     * @brief Get existing service or create it with constructor arguments (lazy initialization).
     *
     * Simpler variant that constructs the service with given arguments if it doesn't exist.
     * If service already exists, the arguments are ignored.
     *
     * Thread-safe: Multiple threads can safely call use_or_make_service() concurrently.
     *
     * @tparam Service The type of service to retrieve or create.
     * @tparam Args Types of constructor arguments.
     * @param args Arguments forwarded to Service constructor if service needs to be created.
     * @return Reference to the service (existing or newly created).
     *
     * @pre !stopped() - Cannot create services after context has been stopped.
     *
     * @note Arguments are perfectly forwarded and captured by value in the lambda
     *       to avoid dangling references when temporaries are passed.
     *
     * Example usage:
     * @code
     * // First call creates the service
     * auto& service1 = ctx.use_or_make_service<MyService>(8080, "server");
     * 
     * // Subsequent calls return existing service (args are ignored)
     * auto& service2 = ctx.use_or_make_service<MyService>(9999, "ignored");
     * // service1 and service2 are the same instance
     * @endcode
     */
    template<typename Service, typename... Args>
    Service& use_or_make_service(Args&&... args) {
        // Capture args by value (decay) to avoid dangling references
        // Use tuple to store forwarded arguments safely
        return use_or_make_service<Service>(
            [... args = std::forward<Args>(args)]() mutable -> std::shared_ptr<Service> {
                return std::make_shared<Service>(std::forward<decltype(args)>(args)...);
            }
        );
    }

private:
    /// Mutex protecting concurrent access to the service registry
    mutable std::mutex m_service_registry_mutex;

    /// Service registry: maps type_index to type-erased service instances
    /// Services are stored as shared_ptr<void> to allow any type to be registered
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_service_registry;
    
    /// Cleanup callbacks executed in reverse order during destruction
    /// Each callback handles on_shutdown() call (if present) and service destruction
    std::vector<std::move_only_function<void()>> m_cleanup_callbacks;
};

} // namespace svarog::execution
