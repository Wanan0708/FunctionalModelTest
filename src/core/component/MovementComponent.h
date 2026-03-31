#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

#include "core/component/IComponent.h"
#include "core/entity/Entity.h"

namespace fm::core {

struct RouteWaypoint {
    std::string name;
    Vector2 position;
    double loiterSeconds {0.0};
};

enum class MovementGuidanceMode {
    Inertial,
    Route,
    MissionTarget,
    MissionOrbit,
};

inline Vector2 operator-(const Vector2& lhs, const Vector2& rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

inline double dot(const Vector2& lhs, const Vector2& rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

inline double lengthSquared(const Vector2& value)
{
    return dot(value, value);
}

inline Vector2 closestPointOnSegment(const Vector2& point, const Vector2& segmentStart, const Vector2& segmentEnd)
{
    const auto segment = segmentEnd - segmentStart;
    const auto segmentLengthSquared = lengthSquared(segment);
    if (segmentLengthSquared <= 1e-12) {
        return segmentStart;
    }

    const auto toPoint = point - segmentStart;
    const double projection = std::clamp(dot(toPoint, segment) / segmentLengthSquared, 0.0, 1.0);
    return {segmentStart.x + segment.x * projection, segmentStart.y + segment.y * projection};
}

inline double normalizeDegrees(double value)
{
    while (value <= -180.0) {
        value += 360.0;
    }

    while (value > 180.0) {
        value -= 360.0;
    }

    return value;
}

class MovementComponent final : public IComponent {
public:
    MovementComponent() = default;

    MovementComponent(std::vector<RouteWaypoint> route,
                      double preferredSpeedMetersPerSecond,
                      double maxSpeedMetersPerSecond,
                      double maxTurnRateDegreesPerSecond,
                      double arrivalToleranceMeters = 0.5)
        : route_(std::move(route))
        , preferredSpeedMetersPerSecond_(preferredSpeedMetersPerSecond)
        , maxSpeedMetersPerSecond_(maxSpeedMetersPerSecond)
        , maxTurnRateDegreesPerSecond_(maxTurnRateDegreesPerSecond)
        , arrivalToleranceMeters_(arrivalToleranceMeters)
    {
    }

    UpdatePhase phase() const override
    {
        return UpdatePhase::Movement;
    }

    const std::vector<RouteWaypoint>& route() const
    {
        return route_;
    }

    bool hasRoute() const
    {
        return !route_.empty();
    }

    std::size_t currentWaypointIndex() const
    {
        return route_.empty() ? 0 : currentWaypointIndex_ % route_.size();
    }

    const RouteWaypoint* activeWaypoint() const
    {
        if (route_.empty()) {
            return nullptr;
        }

        return &route_[currentWaypointIndex()];
    }

    double remainingLoiterSeconds() const
    {
        return remainingLoiterSeconds_;
    }

    bool isLoitering() const
    {
        return remainingLoiterSeconds_ > 0.0;
    }

    MovementGuidanceMode guidanceMode() const
    {
        if (orbitGuidanceActive_) {
            return MovementGuidanceMode::MissionOrbit;
        }

        if (guidanceTargetActive_) {
            return MovementGuidanceMode::MissionTarget;
        }

        if (!route_.empty()) {
            return MovementGuidanceMode::Route;
        }

        return MovementGuidanceMode::Inertial;
    }

    bool hasGuidanceTarget() const
    {
        return guidanceTargetActive_;
    }

    const Vector2& guidanceTargetPosition() const
    {
        return guidanceTargetPosition_;
    }

    const std::string& guidanceTargetLabel() const
    {
        return guidanceTargetLabel_;
    }

    void setGuidanceTarget(Vector2 position, std::string label)
    {
        orbitGuidanceActive_ = false;
        guidanceTargetActive_ = true;
        guidanceTargetPosition_ = position;
        guidanceTargetLabel_ = std::move(label);
    }

    void setOrbitGuidance(Vector2 center, double radiusMeters, std::string label, bool clockwise = true)
    {
        guidanceTargetActive_ = false;
        orbitGuidanceActive_ = true;
        orbitCenter_ = center;
        orbitRadiusMeters_ = radiusMeters;
        orbitGuidanceLabel_ = std::move(label);
        orbitClockwise_ = clockwise;
    }

    void clearGuidanceTarget()
    {
        guidanceTargetActive_ = false;
        guidanceTargetLabel_.clear();
    }

    void clearOrbitGuidance()
    {
        orbitGuidanceActive_ = false;
        orbitGuidanceLabel_.clear();
    }

    void clearMissionGuidance()
    {
        clearGuidanceTarget();
        clearOrbitGuidance();
    }

    bool hasOrbitGuidance() const
    {
        return orbitGuidanceActive_;
    }

    const Vector2& orbitCenter() const
    {
        return orbitCenter_;
    }

    double orbitRadiusMeters() const
    {
        return orbitRadiusMeters_;
    }

    const std::string& orbitGuidanceLabel() const
    {
        return orbitGuidanceLabel_;
    }

    std::size_t totalWaypointVisits() const
    {
        return totalWaypointVisits_;
    }

    std::size_t completedRouteCycles() const
    {
        return completedRouteCycles_;
    }

    std::size_t lastReachedWaypointIndex() const
    {
        return lastReachedWaypointIndex_;
    }

    const RouteWaypoint* lastReachedWaypoint() const
    {
        if (route_.empty() || lastReachedWaypointIndex_ == kNoWaypointIndex) {
            return nullptr;
        }

        return &route_[lastReachedWaypointIndex_];
    }

    void update(const SimulationUpdateContext& context) override
    {
        auto& entity = owner();

        if (!guidanceTargetActive_ && route_.empty()) {
            entity.setPosition(entity.position() + entity.velocity() * context.deltaSeconds);
            return;
        }

        constexpr int integrationSubsteps = 12;
        const double substepDeltaSeconds = context.deltaSeconds / static_cast<double>(integrationSubsteps);
        for (int substepIndex = 0; substepIndex < integrationSubsteps; ++substepIndex) {
            if (orbitGuidanceActive_) {
                updateOrbitGuidanceMotion(entity, substepDeltaSeconds);
                continue;
            }

            if (guidanceTargetActive_) {
                updateGuidanceTargetMotion(entity, substepDeltaSeconds);
                continue;
            }

            if (remainingLoiterSeconds_ > 0.0) {
                remainingLoiterSeconds_ = std::max(0.0, remainingLoiterSeconds_ - substepDeltaSeconds);
                entity.setVelocity({0.0, 0.0});
                if (remainingLoiterSeconds_ > 0.0) {
                    continue;
                }

                currentWaypointIndex_ = (currentWaypointIndex_ + 1) % route_.size();
            }

            const double speedCap = maxSpeedMetersPerSecond_ > 0.0
                                        ? maxSpeedMetersPerSecond_
                                        : preferredSpeedMetersPerSecond_;
            const double desiredSpeed = std::clamp(preferredSpeedMetersPerSecond_, 0.0, speedCap);
            const double speed = desiredSpeed > 0.0 ? desiredSpeed : speedCap;
            if (speed <= 0.0) {
                entity.setVelocity({0.0, 0.0});
                continue;
            }

            for (std::size_t guard = 0; guard < route_.size(); ++guard) {
                const auto& targetWaypoint = route_[currentWaypointIndex_];
                const auto offset = Vector2 {
                    targetWaypoint.position.x - entity.position().x,
                    targetWaypoint.position.y - entity.position().y,
                };
                const auto distanceToTarget = std::hypot(offset.x, offset.y);
                if (distanceToTarget <= arrivalToleranceMeters_) {
                    entity.setPosition(targetWaypoint.position);
                    entity.setVelocity({0.0, 0.0});
                    registerWaypointArrival(currentWaypointIndex_);
                    remainingLoiterSeconds_ = std::max(0.0, targetWaypoint.loiterSeconds);
                    if (remainingLoiterSeconds_ > 0.0) {
                        break;
                    }

                    advanceToNextWaypoint();
                    continue;
                }

                const double desiredHeadingDegrees = std::atan2(offset.y, offset.x) * 180.0 / 3.14159265358979323846;
                const double currentHeadingDegrees = normalizeDegrees(entity.headingDegrees());
                const double deltaHeadingDegrees = normalizeDegrees(desiredHeadingDegrees - currentHeadingDegrees);
                const double maxHeadingStepDegrees = maxTurnRateDegreesPerSecond_ > 0.0
                                                        ? maxTurnRateDegreesPerSecond_ * substepDeltaSeconds
                                                        : std::abs(deltaHeadingDegrees);
                const double appliedHeadingStep = std::clamp(deltaHeadingDegrees,
                                                             -maxHeadingStepDegrees,
                                                             maxHeadingStepDegrees);
                const double newHeadingDegrees = normalizeDegrees(currentHeadingDegrees + appliedHeadingStep);
                const double newHeadingRadians = newHeadingDegrees * 3.14159265358979323846 / 180.0;
                const double turnSeverity = std::clamp(std::abs(deltaHeadingDegrees) / 90.0, 0.0, 1.0);
                const double turnSpeedFactor = 1.0 - 0.45 * turnSeverity;
                const double maneuverSpeed = speed * turnSpeedFactor;
                const auto newVelocity = Vector2 {std::cos(newHeadingRadians) * maneuverSpeed, std::sin(newHeadingRadians) * maneuverSpeed};
                const auto moveDistance = maneuverSpeed * substepDeltaSeconds;
                const auto currentPosition = entity.position();
                const auto nextPosition = currentPosition + newVelocity * substepDeltaSeconds;
                const auto closestPoint = closestPointOnSegment(targetWaypoint.position, currentPosition, nextPosition);
                const auto closestOffset = targetWaypoint.position - closestPoint;
                const auto closestDistance = std::sqrt(lengthSquared(closestOffset));
                const double arrivalWindow = std::max(arrivalToleranceMeters_, moveDistance * 0.5);

                entity.setHeadingDegrees(newHeadingDegrees);
                entity.setVelocity(newVelocity);
                if (distanceToTarget <= arrivalToleranceMeters_ || closestDistance <= arrivalWindow) {
                    entity.setPosition(closestPoint);
                    entity.setVelocity({0.0, 0.0});
                    registerWaypointArrival(currentWaypointIndex_);
                    remainingLoiterSeconds_ = std::max(0.0, targetWaypoint.loiterSeconds);
                    if (remainingLoiterSeconds_ > 0.0) {
                        break;
                    }

                    advanceToNextWaypoint();
                    continue;
                }

                entity.setPosition(nextPosition);
                break;
            }
        }
    }

private:
    static constexpr std::size_t kNoWaypointIndex = std::numeric_limits<std::size_t>::max();

    void updateGuidanceTargetMotion(Entity& entity, double deltaSeconds)
    {
        const double speedCap = maxSpeedMetersPerSecond_ > 0.0
                                    ? maxSpeedMetersPerSecond_
                                    : preferredSpeedMetersPerSecond_;
        const double desiredSpeed = std::clamp(preferredSpeedMetersPerSecond_, 0.0, speedCap);
        const double speed = desiredSpeed > 0.0 ? desiredSpeed : speedCap;
        if (speed <= 0.0) {
            entity.setVelocity({0.0, 0.0});
            return;
        }

        const auto offset = Vector2 {
            guidanceTargetPosition_.x - entity.position().x,
            guidanceTargetPosition_.y - entity.position().y,
        };
        const auto distanceToTarget = std::hypot(offset.x, offset.y);
        if (distanceToTarget <= arrivalToleranceMeters_) {
            entity.setPosition(guidanceTargetPosition_);
            entity.setVelocity({0.0, 0.0});
            return;
        }

        const double desiredHeadingDegrees = std::atan2(offset.y, offset.x) * 180.0 / 3.14159265358979323846;
        const double currentHeadingDegrees = normalizeDegrees(entity.headingDegrees());
        const double deltaHeadingDegrees = normalizeDegrees(desiredHeadingDegrees - currentHeadingDegrees);
        const double maxHeadingStepDegrees = maxTurnRateDegreesPerSecond_ > 0.0
                                                ? maxTurnRateDegreesPerSecond_ * deltaSeconds
                                                : std::abs(deltaHeadingDegrees);
        const double appliedHeadingStep = std::clamp(deltaHeadingDegrees,
                                                     -maxHeadingStepDegrees,
                                                     maxHeadingStepDegrees);
        const double newHeadingDegrees = normalizeDegrees(currentHeadingDegrees + appliedHeadingStep);
        const double newHeadingRadians = newHeadingDegrees * 3.14159265358979323846 / 180.0;
        const double turnSeverity = std::clamp(std::abs(deltaHeadingDegrees) / 90.0, 0.0, 1.0);
        const double turnSpeedFactor = 1.0 - 0.45 * turnSeverity;
        const double maneuverSpeed = speed * turnSpeedFactor;
        const auto newVelocity = Vector2 {std::cos(newHeadingRadians) * maneuverSpeed, std::sin(newHeadingRadians) * maneuverSpeed};
        const auto currentPosition = entity.position();
        const auto nextPosition = currentPosition + newVelocity * deltaSeconds;
        const auto closestPoint = closestPointOnSegment(guidanceTargetPosition_, currentPosition, nextPosition);
        const auto closestOffset = guidanceTargetPosition_ - closestPoint;
        const auto closestDistance = std::sqrt(lengthSquared(closestOffset));
        const double arrivalWindow = std::max(arrivalToleranceMeters_, maneuverSpeed * deltaSeconds * 0.5);

        entity.setHeadingDegrees(newHeadingDegrees);
        entity.setVelocity(newVelocity);
        if (closestDistance <= arrivalWindow) {
            entity.setPosition(closestPoint);
            entity.setVelocity({0.0, 0.0});
            return;
        }

        entity.setPosition(nextPosition);
    }

    void updateOrbitGuidanceMotion(Entity& entity, double deltaSeconds)
    {
        const double speedCap = maxSpeedMetersPerSecond_ > 0.0
                                    ? maxSpeedMetersPerSecond_
                                    : preferredSpeedMetersPerSecond_;
        const double desiredSpeed = std::clamp(preferredSpeedMetersPerSecond_, 0.0, speedCap);
        const double speed = desiredSpeed > 0.0 ? desiredSpeed : speedCap;
        if (speed <= 0.0) {
            entity.setVelocity({0.0, 0.0});
            return;
        }

        auto radial = Vector2 {entity.position().x - orbitCenter_.x, entity.position().y - orbitCenter_.y};
        double distanceFromCenter = std::hypot(radial.x, radial.y);
        const double desiredRadius = orbitRadiusMeters_ > 0.0 ? orbitRadiusMeters_ : std::max(6.0, distanceFromCenter);
        if (distanceFromCenter <= 1e-6) {
            radial = {desiredRadius, 0.0};
            distanceFromCenter = desiredRadius;
        }

        const Vector2 radialUnit {radial.x / distanceFromCenter, radial.y / distanceFromCenter};
        const Vector2 tangentUnit = orbitClockwise_
                                        ? Vector2 {radialUnit.y, -radialUnit.x}
                                        : Vector2 {-radialUnit.y, radialUnit.x};
        const double radialError = distanceFromCenter - desiredRadius;
        const double radialCorrectionGain = std::clamp(radialError / std::max(desiredRadius, 1.0), -0.75, 0.75);
        Vector2 desiredDirection {
            tangentUnit.x - radialUnit.x * radialCorrectionGain,
            tangentUnit.y - radialUnit.y * radialCorrectionGain,
        };

        const double desiredDirectionLength = std::hypot(desiredDirection.x, desiredDirection.y);
        if (desiredDirectionLength <= 1e-9) {
            desiredDirection = tangentUnit;
        } else {
            desiredDirection.x /= desiredDirectionLength;
            desiredDirection.y /= desiredDirectionLength;
        }

        const double desiredHeadingDegrees = std::atan2(desiredDirection.y, desiredDirection.x) * 180.0 / 3.14159265358979323846;
        const double currentHeadingDegrees = normalizeDegrees(entity.headingDegrees());
        const double deltaHeadingDegrees = normalizeDegrees(desiredHeadingDegrees - currentHeadingDegrees);
        const double maxHeadingStepDegrees = maxTurnRateDegreesPerSecond_ > 0.0
                                                ? maxTurnRateDegreesPerSecond_ * deltaSeconds
                                                : std::abs(deltaHeadingDegrees);
        const double appliedHeadingStep = std::clamp(deltaHeadingDegrees,
                                                     -maxHeadingStepDegrees,
                                                     maxHeadingStepDegrees);
        const double newHeadingDegrees = normalizeDegrees(currentHeadingDegrees + appliedHeadingStep);
        const double newHeadingRadians = newHeadingDegrees * 3.14159265358979323846 / 180.0;
        const double turnSeverity = std::clamp(std::abs(deltaHeadingDegrees) / 90.0, 0.0, 1.0);
        const double turnSpeedFactor = 1.0 - 0.35 * turnSeverity;
        const double maneuverSpeed = speed * turnSpeedFactor;
        const auto newVelocity = Vector2 {std::cos(newHeadingRadians) * maneuverSpeed, std::sin(newHeadingRadians) * maneuverSpeed};

        entity.setHeadingDegrees(newHeadingDegrees);
        entity.setVelocity(newVelocity);
        entity.setPosition(entity.position() + newVelocity * deltaSeconds);
    }

    void registerWaypointArrival(std::size_t waypointIndex)
    {
        lastReachedWaypointIndex_ = waypointIndex;
        ++totalWaypointVisits_;
        if (!route_.empty() && waypointIndex + 1 == route_.size()) {
            ++completedRouteCycles_;
        }
    }

    void advanceToNextWaypoint()
    {
        if (route_.empty()) {
            return;
        }

        currentWaypointIndex_ = (currentWaypointIndex_ + 1) % route_.size();
    }

    std::vector<RouteWaypoint> route_;
    double preferredSpeedMetersPerSecond_ {0.0};
    double maxSpeedMetersPerSecond_ {0.0};
    double maxTurnRateDegreesPerSecond_ {180.0};
    double arrivalToleranceMeters_ {0.5};
    std::size_t currentWaypointIndex_ {0};
    double remainingLoiterSeconds_ {0.0};
    std::size_t totalWaypointVisits_ {0};
    std::size_t completedRouteCycles_ {0};
    std::size_t lastReachedWaypointIndex_ {kNoWaypointIndex};
    bool guidanceTargetActive_ {false};
    Vector2 guidanceTargetPosition_;
    std::string guidanceTargetLabel_;
    bool orbitGuidanceActive_ {false};
    Vector2 orbitCenter_;
    double orbitRadiusMeters_ {0.0};
    bool orbitClockwise_ {true};
    std::string orbitGuidanceLabel_;
};

}  // namespace fm::core