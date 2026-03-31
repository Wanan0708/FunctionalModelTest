#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "core/component/IComponent.h"
#include "core/entity/Entity.h"

namespace fm::core {

class SensorComponent final : public IComponent {
public:
    explicit SensorComponent(double rangeMeters, double fieldOfViewDegrees = 360.0)
        : rangeMeters_(rangeMeters), fieldOfViewDegrees_(fieldOfViewDegrees)
    {
    }

    UpdatePhase phase() const override
    {
        return UpdatePhase::Perception;
    }

    void update(const SimulationUpdateContext& context) override
    {
        detectedTargetIds_.clear();
        if (rangeMeters_ <= 0.0) {
            return;
        }

        const auto rangeSquared = rangeMeters_ * rangeMeters_;
        const double clampedFieldOfViewDegrees = std::clamp(fieldOfViewDegrees_, 0.0, 360.0);
        const bool useFieldOfViewFilter = clampedFieldOfViewDegrees < 359.999;
        const double halfFieldOfViewRadians = clampedFieldOfViewDegrees * 0.5 * kDegreesToRadians;
        const double minimumMagnitudeSquared = 1e-12;
        const double speedSquared = owner().velocity().x * owner().velocity().x + owner().velocity().y * owner().velocity().y;
        const double headingRadians = owner().headingDegrees() * kDegreesToRadians;
        const double forwardX = speedSquared > minimumMagnitudeSquared
            ? owner().velocity().x / std::sqrt(speedSquared)
            : std::cos(headingRadians);
        const double forwardY = speedSquared > minimumMagnitudeSquared
            ? owner().velocity().y / std::sqrt(speedSquared)
            : std::sin(headingRadians);

        for (const auto& entity : context.entities) {
            if (entity == nullptr || entity.get() == &owner()) {
                continue;
            }

            const bool sameKnownSide = !owner().side().empty()
                && !entity->side().empty()
                && entity->side() == owner().side();
            if (sameKnownSide) {
                continue;
            }

            const auto dx = entity->position().x - owner().position().x;
            const auto dy = entity->position().y - owner().position().y;
            const auto distanceSquared = dx * dx + dy * dy;
            if (distanceSquared > rangeSquared) {
                continue;
            }

            if (useFieldOfViewFilter && distanceSquared > minimumMagnitudeSquared) {
                const double distance = std::sqrt(distanceSquared);
                const double targetDirectionX = dx / distance;
                const double targetDirectionY = dy / distance;
                const double dotProduct = std::clamp(forwardX * targetDirectionX + forwardY * targetDirectionY, -1.0, 1.0);
                const double angleOffsetRadians = std::acos(dotProduct);
                if (angleOffsetRadians > halfFieldOfViewRadians) {
                    continue;
                }
            }

            detectedTargetIds_.push_back(entity->id());
        }
    }

    double rangeMeters() const
    {
        return rangeMeters_;
    }

    double fieldOfViewDegrees() const
    {
        return fieldOfViewDegrees_;
    }

    const std::vector<std::string>& detectedTargetIds() const
    {
        return detectedTargetIds_;
    }

private:
    static constexpr double kDegreesToRadians = 3.14159265358979323846 / 180.0;

    double rangeMeters_ {0.0};
    double fieldOfViewDegrees_ {360.0};
    std::vector<std::string> detectedTargetIds_;
};

}  // namespace fm::core