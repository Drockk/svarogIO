#pragma once

#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <ranges>
#include <typeindex>
#include <vector>

#include "svarog/core/contracts.hpp"

#if __cpp_lib_flat_map >= 202207L
    #include <flat_map>
    template <typename K, typename V>
    using default_service_map = std::flat_map<K, V>;
#else
    #include <unordered_map>
    template <typename K, typename V>
    using default_service_map = std::unordered_map<K, V>;
#endif

namespace svarog::execution {

template <typename T>
concept HasShutdownHook = requires(T t) {
    { t.on_shutdown() } -> std::same_as<void>;
};

template <typename Container>
concept ServiceContainer = requires(Container t_container, std::type_index t_key, std::any t_value) {
    typename Container::key_type;
    typename Container::mapped_type;
    { t_container.find(t_key) };
    { t_container.insert_or_assign(t_key, t_value) };
    { t_container.contains(t_key) } -> std::convertible_to<bool>;
    { t_container.erase(t_key) };
};

template<ServiceContainer Container = default_service_map<std::type_index, std::shared_ptr<void>>>
class execution_context {
public:
    execution_context() = default;
    virtual ~execution_context() {
        // Execute cleanup callbacks in reverse order using ranges::reverse_view
        // This ensures proper shutdown hook calls and destruction order
        for (auto& cleanup : std::ranges::reverse_view(m_cleanup_callbacks)) {
            cleanup();
        }
    }

    execution_context(const execution_context&) = delete;
    execution_context& operator=(const execution_context&) = delete;

    virtual void stop() = 0;
    virtual void restart() = 0;
    virtual bool stopped() const noexcept = 0;

    template <typename Service>
    Service& use_service() {
        SVAROG_EXPECTS(!stopped());
        SVAROG_EXPECTS(has_service<Service>());

        std::lock_guard lock(m_service_registry_mutex);
        return *std::static_pointer_cast<Service>(m_service_registry.at(std::type_index(typeid(Service))));
    }

    template <typename Service>
    void add_service(std::shared_ptr<Service> t_service) {
        SVAROG_EXPECTS(t_service != nullptr);

        std::lock_guard lock(m_service_registry_mutex);

        auto type_idx = std::type_index(typeid(Service));

        auto [it, inserted] = m_service_registry.try_emplace(type_idx, t_service);

        if (!inserted) {
            it->second = t_service;
        }

        m_cleanup_callbacks.push_back([service = t_service]() mutable {
            if constexpr (HasShutdownHook<Service>) {
                if (service) {
                    service->on_shutdown();
                }
            }
        });
    }

    template <typename Service>
    bool has_service() const noexcept {
        std::lock_guard lock(m_service_registry_mutex);
        return m_service_registry.contains(std::type_index(typeid(Service)));
    }

    template <typename Service, typename... Args>
    Service& make_service(Args&&... args) {
        SVAROG_EXPECTS(!stopped());

        auto service = std::make_shared<Service>(std::forward<Args>(args)...);
        Service& ref = *service;
        add_service(service);
        return ref;
    }

    template <typename Service, typename Factory>
        requires std::invocable<Factory> && std::same_as<std::invoke_result_t<Factory>, std::shared_ptr<Service>>
    Service& use_or_make_service(Factory&& factory) {
        SVAROG_EXPECTS(!stopped());

        std::lock_guard lock(m_service_registry_mutex);

        auto type_idx = std::type_index(typeid(Service));
        auto it = m_service_registry.find(type_idx);

        if (it != m_service_registry.end()) {
            return *std::static_pointer_cast<Service>(it->second);
        }

        auto service = std::forward<Factory>(factory)();
        SVAROG_EXPECTS(service != nullptr);

        Service& ref = *service;

        m_service_registry.emplace(type_idx, service);

        m_cleanup_callbacks.push_back([service]() mutable {
            if constexpr (HasShutdownHook<Service>) {
                if (service) {
                    service->on_shutdown();
                }
            }
        });

        return ref;
    }

    template <typename Service, typename... Args>
    Service& use_or_make_service(Args&&... args) {
        return use_or_make_service<Service>(
            [... args = std::forward<Args>(args)]() mutable -> std::shared_ptr<Service> {
                return std::make_shared<Service>(std::forward<decltype(args)>(args)...);
            });
    }

private:
    mutable std::mutex m_service_registry_mutex;
    Container m_service_registry;
    std::vector<std::move_only_function<void()>> m_cleanup_callbacks;
};

}  // namespace svarog::execution
