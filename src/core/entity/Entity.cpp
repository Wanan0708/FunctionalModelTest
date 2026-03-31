#include "core/entity/Entity.h"

namespace fm::core {

Entity::Entity(std::string id, std::string displayName)
    : id_(std::move(id)), displayName_(std::move(displayName))
{
}

Entity::~Entity()
{
    clearComponents();
}

const std::string& Entity::id() const
{
    return id_;
}

const std::string& Entity::displayName() const
{
    return displayName_;
}

void Entity::setDisplayName(std::string displayName)
{
    displayName_ = std::move(displayName);
}

const std::string& Entity::side() const
{
    return side_;
}

void Entity::setSide(std::string side)
{
    side_ = std::move(side);
}

const std::string& Entity::category() const
{
    return category_;
}

void Entity::setCategory(std::string category)
{
    category_ = std::move(category);
}

const std::string& Entity::role() const
{
    return role_;
}

void Entity::setRole(std::string role)
{
    role_ = std::move(role);
}

const std::string& Entity::colorHex() const
{
    return colorHex_;
}

void Entity::setColorHex(std::string colorHex)
{
    colorHex_ = std::move(colorHex);
}

const std::vector<std::string>& Entity::tags() const
{
    return tags_;
}

void Entity::setTags(std::vector<std::string> tags)
{
    tags_ = std::move(tags);
}

const Vector2& Entity::position() const
{
    return position_;
}

void Entity::setPosition(Vector2 position)
{
    position_ = position;
}

const Vector2& Entity::velocity() const
{
    return velocity_;
}

void Entity::setVelocity(Vector2 velocity)
{
    velocity_ = velocity;
}

double Entity::headingDegrees() const
{
    return headingDegrees_;
}

void Entity::setHeadingDegrees(double headingDegrees)
{
    headingDegrees_ = headingDegrees;
}

double Entity::maxSpeedMetersPerSecond() const
{
    return maxSpeedMetersPerSecond_;
}

void Entity::setMaxSpeedMetersPerSecond(double value)
{
    maxSpeedMetersPerSecond_ = value;
}

double Entity::maxAccelerationMetersPerSecondSquared() const
{
    return maxAccelerationMetersPerSecondSquared_;
}

void Entity::setMaxAccelerationMetersPerSecondSquared(double value)
{
    maxAccelerationMetersPerSecondSquared_ = value;
}

double Entity::maxDecelerationMetersPerSecondSquared() const
{
    return maxDecelerationMetersPerSecondSquared_;
}

void Entity::setMaxDecelerationMetersPerSecondSquared(double value)
{
    maxDecelerationMetersPerSecondSquared_ = value;
}

double Entity::maxTurnRateDegreesPerSecond() const
{
    return maxTurnRateDegreesPerSecond_;
}

void Entity::setMaxTurnRateDegreesPerSecond(double value)
{
    maxTurnRateDegreesPerSecond_ = value;
}

double Entity::radarCrossSectionSquareMeters() const
{
    return radarCrossSectionSquareMeters_;
}

void Entity::setRadarCrossSectionSquareMeters(double value)
{
    radarCrossSectionSquareMeters_ = value;
}

void Entity::update(UpdatePhase phase, const SimulationUpdateContext& context)
{
    for (const auto& component : components_) {
        if (component->phase() == phase) {
            component->update(context);
        }
    }
}

void Entity::clearComponents()
{
    for (const auto& component : components_) {
        component->onDetach();
    }

    components_.clear();
}

}  // namespace fm::core