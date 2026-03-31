#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/component/IComponent.h"
#include "core/math/Vector2.h"

namespace fm::core {

class Entity {
public:
    explicit Entity(std::string id, std::string displayName = {});
    ~Entity();

    const std::string& id() const;
    const std::string& displayName() const;
    void setDisplayName(std::string displayName);

    const std::string& side() const;
    void setSide(std::string side);

    const std::string& category() const;
    void setCategory(std::string category);

    const std::string& role() const;
    void setRole(std::string role);

    const std::string& colorHex() const;
    void setColorHex(std::string colorHex);

    const std::vector<std::string>& tags() const;
    void setTags(std::vector<std::string> tags);

    const Vector2& position() const;
    void setPosition(Vector2 position);

    const Vector2& velocity() const;
    void setVelocity(Vector2 velocity);

    double headingDegrees() const;
    void setHeadingDegrees(double headingDegrees);

    double maxSpeedMetersPerSecond() const;
    void setMaxSpeedMetersPerSecond(double value);

    double maxTurnRateDegreesPerSecond() const;
    void setMaxTurnRateDegreesPerSecond(double value);

    void update(UpdatePhase phase, const SimulationUpdateContext& context);
    void clearComponents();

    template <typename T, typename... Args>
    T& addComponent(Args&&... args)
    {
        static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");

        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        auto* rawPtr = component.get();
        rawPtr->onAttach(*this);
        components_.push_back(std::move(component));
        return *rawPtr;
    }

    template <typename T>
    T* getComponent()
    {
        static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");

        for (const auto& component : components_) {
            if (auto* typed = dynamic_cast<T*>(component.get())) {
                return typed;
            }
        }

        return nullptr;
    }

    template <typename T>
    const T* getComponent() const
    {
        static_assert(std::is_base_of_v<IComponent, T>, "T must derive from IComponent");

        for (const auto& component : components_) {
            if (auto* typed = dynamic_cast<const T*>(component.get())) {
                return typed;
            }
        }

        return nullptr;
    }

private:
    std::string id_;
    std::string displayName_;
    std::string side_;
    std::string category_;
    std::string role_;
    std::string colorHex_;
    std::vector<std::string> tags_;
    Vector2 position_;
    Vector2 velocity_;
    double headingDegrees_ {0.0};
    double maxSpeedMetersPerSecond_ {0.0};
    double maxTurnRateDegreesPerSecond_ {0.0};
    std::vector<std::unique_ptr<IComponent>> components_;
};

}  // namespace fm::core