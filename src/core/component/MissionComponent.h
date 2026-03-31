#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/component/IComponent.h"
#include "core/component/MovementComponent.h"
#include "core/component/SensorComponent.h"

namespace fm::core {

enum class MissionExecutionStatus {
    Unassigned,
    Idle,
    Navigating,
    Loitering,
    Tracking,
    Completed,
    Aborted,
};

class MissionComponent final : public IComponent {
public:
    enum class Behavior {
        Unknown,
        Patrol,
        Intercept,
        Escort,
        Orbit,
        Transit,
    };

    struct TaskParameters {
        std::optional<double> interceptCompletionDistanceMeters;
        std::optional<double> orbitRadiusMeters;
        std::optional<bool> orbitClockwise;
        std::optional<double> orbitAcquireToleranceMeters;
        std::optional<double> orbitCompletionToleranceMeters;
        std::optional<double> orbitCompletionHoldSeconds;
        std::optional<double> escortTrailDistanceMeters;
        std::optional<double> escortRightOffsetMeters;
        std::optional<double> escortSlotToleranceMeters;
        std::optional<double> escortCompletionHoldSeconds;
    };

    struct ReplanStep {
        std::string objective;
        std::string behavior;
        std::string targetEntityId;
        double timeoutSeconds {0.0};
        TaskParameters parameters;
    };

    struct ConstraintConfig {
        double timeoutSeconds {0.0};
        std::string prerequisiteEntityId;
        std::string prerequisiteStatus {"completed"};
        std::string replanObjective;
        std::string replanBehavior;
        std::string replanTargetEntityId;
        double replanTimeoutSeconds {0.0};
        std::vector<ReplanStep> replanChain;
    };

    MissionComponent(std::string objective, std::string behavior, std::string targetEntityId)
        : MissionComponent(std::move(objective), std::move(behavior), std::move(targetEntityId), TaskParameters {})
    {
    }

    MissionComponent(std::string objective, std::string behavior, std::string targetEntityId, TaskParameters parameters)
        : objective_(std::move(objective))
        , behavior_(std::move(behavior))
        , behaviorType_(parseBehaviorCode(behavior_))
        , targetEntityId_(std::move(targetEntityId))
        , taskParameters_(std::move(parameters))
    {
    }

    MissionComponent(std::string objective, std::string behavior, std::string targetEntityId, ConstraintConfig constraints)
        : MissionComponent(std::move(objective), std::move(behavior), std::move(targetEntityId), TaskParameters {}, std::move(constraints))
    {
    }

    MissionComponent(std::string objective,
                     std::string behavior,
                     std::string targetEntityId,
                     TaskParameters parameters,
                     ConstraintConfig constraints)
        : objective_(std::move(objective))
        , behavior_(std::move(behavior))
        , behaviorType_(parseBehaviorCode(behavior_))
        , targetEntityId_(std::move(targetEntityId))
        , taskParameters_(std::move(parameters))
        , timeoutSeconds_(constraints.timeoutSeconds)
        , prerequisiteEntityId_(std::move(constraints.prerequisiteEntityId))
        , prerequisiteStatus_(std::move(constraints.prerequisiteStatus))
    {
        if (!constraints.replanBehavior.empty()) {
            replanChain_.push_back({std::move(constraints.replanObjective),
                                    std::move(constraints.replanBehavior),
                                    std::move(constraints.replanTargetEntityId),
                                    constraints.replanTimeoutSeconds});
        }

        for (auto& replanStep : constraints.replanChain) {
            replanChain_.push_back(std::move(replanStep));
        }
    }

    UpdatePhase phase() const override
    {
        return UpdatePhase::General;
    }

    void update(const SimulationUpdateContext& context) override
    {
        refreshExecutionState(&context);
    }

    const std::string& objective() const
    {
        return objective_;
    }

    void setObjective(std::string objective)
    {
        objective_ = std::move(objective);
    }

    const std::string& behavior() const
    {
        return behavior_;
    }

    Behavior behaviorType() const
    {
        return behaviorType_;
    }

    void setBehavior(std::string behavior)
    {
        behavior_ = std::move(behavior);
        behaviorType_ = parseBehaviorCode(behavior_);
    }

    const std::string& targetEntityId() const
    {
        return targetEntityId_;
    }

    const TaskParameters& taskParameters() const
    {
        return taskParameters_;
    }

    void setTargetEntityId(std::string targetEntityId)
    {
        targetEntityId_ = std::move(targetEntityId);
    }

    MissionExecutionStatus executionStatus() const
    {
        return executionStatus_;
    }

    const std::string& executionPhase() const
    {
        return executionPhase_;
    }

    double phaseElapsedSeconds() const
    {
        return phaseElapsedSeconds_;
    }

    std::uint64_t phaseEnteredTick() const
    {
        return phaseEnteredTick_;
    }

    std::size_t completedWaypointVisits() const
    {
        return completedWaypointVisits_;
    }

    std::size_t completedRouteCycles() const
    {
        return completedRouteCycles_;
    }

    const std::string& lastCompletedWaypointName() const
    {
        return lastCompletedWaypointName_;
    }

    const std::string& terminalReason() const
    {
        return terminalReason_;
    }

    double timeoutSeconds() const
    {
        return timeoutSeconds_;
    }

    const std::string& prerequisiteEntityId() const
    {
        return prerequisiteEntityId_;
    }

    const std::string& prerequisiteStatus() const
    {
        return prerequisiteStatus_;
    }

    const std::string& replanBehavior() const
    {
        return nextReplanStep() != nullptr ? nextReplanStep()->behavior : emptyString();
    }

    const std::string& replanObjective() const
    {
        return nextReplanStep() != nullptr ? nextReplanStep()->objective : emptyString();
    }

    const std::string& replanTargetEntityId() const
    {
        return nextReplanStep() != nullptr ? nextReplanStep()->targetEntityId : emptyString();
    }

    const std::string& lastReplanReason() const
    {
        return lastReplanReason_;
    }

    const std::string& lastReplanFromBehavior() const
    {
        return lastReplanFromBehavior_;
    }

    const std::string& lastReplanToBehavior() const
    {
        return lastReplanToBehavior_;
    }

    std::size_t remainingReplanSteps() const
    {
        return replanChain_.size() - nextReplanIndex_;
    }

    bool activationSatisfied() const
    {
        return activationSatisfied_;
    }

    double interceptCompletionDistanceMeters() const
    {
        return taskParameters_.interceptCompletionDistanceMeters.value_or(kInterceptCompletionDistanceMeters);
    }

    bool hasCustomOrbitRadiusMeters() const
    {
        return taskParameters_.orbitRadiusMeters.has_value();
    }

    double orbitRadiusMeters() const
    {
        return taskParameters_.orbitRadiusMeters.value_or(kDefaultOrbitRadiusMeters);
    }

    bool orbitClockwise() const
    {
        return taskParameters_.orbitClockwise.value_or(true);
    }

    double orbitAcquireToleranceMeters() const
    {
        return taskParameters_.orbitAcquireToleranceMeters.value_or(kOrbitAcquireToleranceMeters);
    }

    double orbitCompletionToleranceMeters() const
    {
        return taskParameters_.orbitCompletionToleranceMeters.value_or(kOrbitCompletionToleranceMeters);
    }

    double orbitCompletionHoldSeconds() const
    {
        return taskParameters_.orbitCompletionHoldSeconds.value_or(kOrbitCompletionHoldSeconds);
    }

    double escortTrailDistanceMeters() const
    {
        return taskParameters_.escortTrailDistanceMeters.value_or(kEscortTrailDistanceMeters);
    }

    double escortRightOffsetMeters() const
    {
        return taskParameters_.escortRightOffsetMeters.value_or(kEscortRightOffsetMeters);
    }

    double escortSlotToleranceMeters() const
    {
        return taskParameters_.escortSlotToleranceMeters.value_or(kEscortSlotToleranceMeters);
    }

    double escortCompletionHoldSeconds() const
    {
        return taskParameters_.escortCompletionHoldSeconds.value_or(kEscortCompletionHoldSeconds);
    }

    const char* executionStatusCode() const
    {
        switch (executionStatus_) {
        case MissionExecutionStatus::Unassigned:
            return "unassigned";
        case MissionExecutionStatus::Idle:
            return "idle";
        case MissionExecutionStatus::Navigating:
            return "navigating";
        case MissionExecutionStatus::Loitering:
            return "loitering";
        case MissionExecutionStatus::Tracking:
            return "tracking";
        case MissionExecutionStatus::Completed:
            return "completed";
        case MissionExecutionStatus::Aborted:
            return "aborted";
        }

        return "unknown";
    }

    void refreshExecutionState(const SimulationUpdateContext* context = nullptr)
    {
        const double currentPhaseElapsed = context != nullptr
                                               ? std::max(0.0, context->elapsedSimulationSeconds - phaseEnteredSimulationSeconds_)
                                               : phaseElapsedSeconds_;
        if (isTerminalStatus(executionStatus_)) {
            if (context != nullptr) {
                phaseElapsedSeconds_ = currentPhaseElapsed;
            }
            if (auto* movement = owner().getComponent<MovementComponent>(); movement != nullptr) {
                movement->clearMissionGuidance();
            }
            updateRouteProgress();
            return;
        }

        const bool hasMissionAssignment = !objective_.empty() || !behavior_.empty() || !targetEntityId_.empty();
        if (!hasMissionAssignment) {
            applyExecutionState(MissionExecutionStatus::Unassigned, "unassigned", context);
            updateRouteProgress();
            return;
        }

        activationSatisfied_ = isActivationSatisfied(context);
        if (!activationSatisfied_) {
            if (auto* movement = owner().getComponent<MovementComponent>(); movement != nullptr) {
                movement->clearMissionGuidance();
            }
            applyExecutionState(MissionExecutionStatus::Idle, "awaiting-prerequisite", context);
            updateRouteProgress();
            return;
        }

        synchronizeBehaviorGuidance(context);

        if (context != nullptr && timeoutSeconds_ > 0.0) {
            const double activeMissionElapsed = std::max(0.0, context->elapsedSimulationSeconds - activationStartedSimulationSeconds_);
            if (activeMissionElapsed >= timeoutSeconds_) {
                if (applyReplanIfConfigured(context, "timeout-expired")) {
                    updateRouteProgress();
                    return;
                }

                applyExecutionState(MissionExecutionStatus::Aborted, "timeout-expired", context, "timeout-expired");
                updateRouteProgress();
                return;
            }
        }

        const auto* targetEntity = resolveTargetEntity(context);
        if (behaviorType_ == Behavior::Intercept && !targetEntityId_.empty() && context != nullptr && targetEntity == nullptr) {
            if (applyReplanIfConfigured(context, "target-unavailable")) {
                updateRouteProgress();
                return;
            }
            applyExecutionState(MissionExecutionStatus::Aborted, "target-unavailable", context, "target-unavailable");
            updateRouteProgress();
            return;
        }

        if (behaviorType_ == Behavior::Escort && !targetEntityId_.empty() && context != nullptr && targetEntity == nullptr) {
            if (applyReplanIfConfigured(context, "escort-anchor-lost")) {
                updateRouteProgress();
                return;
            }
            applyExecutionState(MissionExecutionStatus::Aborted, "escort-anchor-lost", context, "escort-anchor-lost");
            updateRouteProgress();
            return;
        }

        if (behaviorType_ == Behavior::Orbit && !targetEntityId_.empty() && context != nullptr && targetEntity == nullptr) {
            if (applyReplanIfConfigured(context, "orbit-anchor-lost")) {
                updateRouteProgress();
                return;
            }
            applyExecutionState(MissionExecutionStatus::Aborted, "orbit-anchor-lost", context, "orbit-anchor-lost");
            updateRouteProgress();
            return;
        }

        bool targetDetected = false;
        if (behaviorType_ == Behavior::Intercept && !targetEntityId_.empty()) {
            if (const auto* sensor = owner().getComponent<SensorComponent>(); sensor != nullptr) {
                const auto& detectedTargetIds = sensor->detectedTargetIds();
                targetDetected = std::find(detectedTargetIds.begin(), detectedTargetIds.end(), targetEntityId_) != detectedTargetIds.end();
            }
        }

        if (targetDetected) {
            if (targetEntity != nullptr) {
                const auto offset = Vector2 {
                    targetEntity->position().x - owner().position().x,
                    targetEntity->position().y - owner().position().y,
                };
                const double targetDistance = std::hypot(offset.x, offset.y);
                if (targetDistance <= interceptCompletionDistanceMeters()) {
                    applyExecutionState(MissionExecutionStatus::Completed,
                                        "intercept-complete",
                                        context,
                                        "target-contained");
                    updateRouteProgress();
                    return;
                }
            }

            applyExecutionState(MissionExecutionStatus::Tracking,
                                behaviorType_ == Behavior::Intercept ? "target-intercept-window" : "target-detected",
                                context);
            updateRouteProgress();
            return;
        }

        if (behaviorType_ == Behavior::Escort) {
            if (const auto* movement = owner().getComponent<MovementComponent>(); movement != nullptr && movement->hasGuidanceTarget()) {
                const auto offset = Vector2 {
                    movement->guidanceTargetPosition().x - owner().position().x,
                    movement->guidanceTargetPosition().y - owner().position().y,
                };
                const double distanceToEscortSlot = std::hypot(offset.x, offset.y);
                if (distanceToEscortSlot <= escortSlotToleranceMeters() && executionPhase_ == "escort-station"
                    && context != nullptr && currentPhaseElapsed >= escortCompletionHoldSeconds()) {
                    applyExecutionState(MissionExecutionStatus::Completed,
                                        "escort-complete",
                                        context,
                                        "formation-stable");
                    updateRouteProgress();
                    return;
                }

                applyExecutionState(MissionExecutionStatus::Navigating,
                                    distanceToEscortSlot <= escortSlotToleranceMeters() ? "escort-station" : "escort-rejoin",
                                    context);
                updateRouteProgress();
                return;
            }

            applyExecutionState(MissionExecutionStatus::Idle,
                                targetEntityId_.empty() ? "task-assigned" : "awaiting-escort-anchor",
                                context);
            updateRouteProgress();
            return;
        }

        if (behaviorType_ == Behavior::Orbit) {
            if (const auto* movement = owner().getComponent<MovementComponent>(); movement != nullptr && movement->hasOrbitGuidance()) {
                const auto radial = Vector2 {
                    owner().position().x - movement->orbitCenter().x,
                    owner().position().y - movement->orbitCenter().y,
                };
                const double distanceFromCenter = std::hypot(radial.x, radial.y);
                const double radialError = std::abs(distanceFromCenter - movement->orbitRadiusMeters());
                if (radialError <= orbitCompletionToleranceMeters()
                    && context != nullptr && currentPhaseElapsed >= orbitCompletionHoldSeconds()) {
                    applyExecutionState(MissionExecutionStatus::Completed,
                                        "orbit-complete",
                                        context,
                                        "orbit-established");
                    updateRouteProgress();
                    return;
                }

                applyExecutionState(MissionExecutionStatus::Navigating,
                                    radialError <= orbitAcquireToleranceMeters() ? "orbit-station" : "orbit-acquire",
                                    context);
                updateRouteProgress();
                return;
            }

            applyExecutionState(MissionExecutionStatus::Idle, "awaiting-orbit-anchor", context);
            updateRouteProgress();
            return;
        }

        if (behaviorType_ == Behavior::Transit) {
            if (const auto* movement = owner().getComponent<MovementComponent>(); movement != nullptr && movement->hasRoute()) {
                if (movement->completedRouteCycles() >= 1) {
                    applyExecutionState(MissionExecutionStatus::Completed,
                                        "transit-complete",
                                        context,
                                        "route-finished");
                    updateRouteProgress();
                    return;
                }

                if (movement->isLoitering()) {
                    applyExecutionState(MissionExecutionStatus::Loitering, "holding-at-waypoint", context);
                } else {
                    applyExecutionState(MissionExecutionStatus::Navigating, "transit-to-waypoint", context);
                }

                updateRouteProgress();
                return;
            }

            applyExecutionState(MissionExecutionStatus::Idle, "task-assigned", context);
            updateRouteProgress();
            return;
        }

        if (const auto* movement = owner().getComponent<MovementComponent>();
            movement != nullptr && (movement->hasRoute() || movement->hasGuidanceTarget())) {
            if (movement->hasRoute() && movement->isLoitering()) {
                applyExecutionState(MissionExecutionStatus::Loitering,
                                    behaviorType_ == Behavior::Patrol ? "patrol-hold" : "holding-at-waypoint",
                                    context);
            } else {
                applyExecutionState(MissionExecutionStatus::Navigating,
                                    behaviorType_ == Behavior::Patrol
                                        ? "patrol-transit"
                                        : (behaviorType_ == Behavior::Intercept ? "intercept-transit" : "transit-to-waypoint"),
                                    context);
            }

            updateRouteProgress();
            return;
        }

        applyExecutionState(MissionExecutionStatus::Idle,
                            targetEntityId_.empty() ? "task-assigned" : "searching-for-target",
                            context);
        updateRouteProgress();
    }

private:
    static constexpr double kEscortTrailDistanceMeters = 6.0;
    static constexpr double kEscortRightOffsetMeters = 4.0;
    static constexpr double kEscortSlotToleranceMeters = 1.5;
    static constexpr double kEscortCompletionHoldSeconds = 2.0;
    static constexpr double kOrbitAcquireToleranceMeters = 1.5;
    static constexpr double kOrbitCompletionToleranceMeters = 3.0;
    static constexpr double kDefaultOrbitRadiusMeters = 8.0;
    static constexpr double kOrbitCompletionHoldSeconds = 3.0;
    static constexpr double kInterceptCompletionDistanceMeters = 2.5;

    static Behavior parseBehaviorCode(const std::string& value)
    {
        if (value == "patrol") {
            return Behavior::Patrol;
        }

        if (value == "intercept") {
            return Behavior::Intercept;
        }

        if (value == "escort") {
            return Behavior::Escort;
        }

        if (value == "orbit") {
            return Behavior::Orbit;
        }

        if (value == "transit") {
            return Behavior::Transit;
        }

        return Behavior::Unknown;
    }

    static const std::string& emptyString()
    {
        static const std::string value;
        return value;
    }

    bool isTerminalStatus(MissionExecutionStatus status) const
    {
        return status == MissionExecutionStatus::Completed || status == MissionExecutionStatus::Aborted;
    }

    const Entity* resolveTargetEntity(const SimulationUpdateContext* context) const
    {
        if (context == nullptr || targetEntityId_.empty()) {
            return nullptr;
        }

        for (const auto& entity : context->entities) {
            if (entity != nullptr && entity.get() != &owner() && entity->id() == targetEntityId_) {
                return entity.get();
            }
        }

        return nullptr;
    }

    const MissionComponent* resolvePrerequisiteMission(const SimulationUpdateContext* context) const
    {
        if (context == nullptr || prerequisiteEntityId_.empty()) {
            return nullptr;
        }

        for (const auto& entity : context->entities) {
            if (entity == nullptr || entity.get() == &owner() || entity->id() != prerequisiteEntityId_) {
                continue;
            }

            return entity->getComponent<MissionComponent>();
        }

        return nullptr;
    }

    bool isActivationSatisfied(const SimulationUpdateContext* context)
    {
        if (prerequisiteEntityId_.empty()) {
            if (!activationSatisfied_ && context != nullptr) {
                activationStartedSimulationSeconds_ = context->elapsedSimulationSeconds;
            }
            return true;
        }

        const auto* prerequisiteMission = resolvePrerequisiteMission(context);
        const bool satisfied = prerequisiteMission != nullptr
            && prerequisiteMission->executionStatusCode() == prerequisiteStatus_;
        if (satisfied && !activationSatisfied_ && context != nullptr) {
            activationStartedSimulationSeconds_ = context->elapsedSimulationSeconds;
        }
        return satisfied;
    }

    bool applyReplanIfConfigured(const SimulationUpdateContext* context, const std::string& abortReason)
    {
        const auto* replanStep = nextReplanStep();
        if (replanStep == nullptr) {
            return false;
        }

        lastReplanFromBehavior_ = behavior_;
        lastReplanToBehavior_ = replanStep->behavior;
        objective_ = replanStep->objective.empty() ? objective_ : replanStep->objective;
        behavior_ = replanStep->behavior;
        behaviorType_ = parseBehaviorCode(behavior_);
        targetEntityId_ = replanStep->targetEntityId;
        taskParameters_ = replanStep->parameters;
        timeoutSeconds_ = replanStep->timeoutSeconds;
        ++nextReplanIndex_;
        lastReplanReason_ = abortReason;
        terminalReason_.clear();
        activationStartedSimulationSeconds_ = context != nullptr ? context->elapsedSimulationSeconds : 0.0;
        if (auto* movement = owner().getComponent<MovementComponent>(); movement != nullptr) {
            movement->clearMissionGuidance();
        }
        applyExecutionState(MissionExecutionStatus::Idle, "replanned-after-abort", context);
        return true;
    }

    const ReplanStep* nextReplanStep() const
    {
        return nextReplanIndex_ < replanChain_.size() ? &replanChain_[nextReplanIndex_] : nullptr;
    }

    void synchronizeBehaviorGuidance(const SimulationUpdateContext* context)
    {
        auto* movement = owner().getComponent<MovementComponent>();
        if (movement == nullptr) {
            return;
        }

        if (behaviorType_ == Behavior::Orbit) {
            if (context == nullptr) {
                movement->clearMissionGuidance();
                return;
            }

            const auto anchor = resolveOrbitAnchor(*movement, *context);
            movement->setOrbitGuidance(anchor.position, anchor.radiusMeters, anchor.label, orbitClockwise());
            return;
        }

        if ((behaviorType_ != Behavior::Intercept && behaviorType_ != Behavior::Escort) || targetEntityId_.empty() || context == nullptr) {
            movement->clearMissionGuidance();
            return;
        }

        for (const auto& entity : context->entities) {
            if (entity == nullptr || entity.get() == &owner() || entity->id() != targetEntityId_) {
                continue;
            }

            if (behaviorType_ == Behavior::Intercept) {
                movement->setGuidanceTarget(entity->position(), entity->displayName().empty() ? entity->id() : entity->displayName());
            } else {
                const auto anchorPosition = computeEscortAnchor(*entity);
                movement->setGuidanceTarget(anchorPosition,
                                           (entity->displayName().empty() ? entity->id() : entity->displayName()) + " escort slot");
            }
            return;
        }

        movement->clearMissionGuidance();
    }

    struct OrbitAnchor {
        Vector2 position;
        double radiusMeters {kDefaultOrbitRadiusMeters};
        std::string label;
    };

    OrbitAnchor resolveOrbitAnchor(const MovementComponent& movement, const SimulationUpdateContext& context) const
    {
        if (!targetEntityId_.empty()) {
            for (const auto& entity : context.entities) {
                if (entity == nullptr || entity->id() != targetEntityId_) {
                    continue;
                }

                if (taskParameters_.orbitRadiusMeters.has_value()) {
                    return {entity->position(), std::max(0.0, *taskParameters_.orbitRadiusMeters), entity->displayName().empty() ? entity->id() : entity->displayName()};
                }

                const auto offset = Vector2 {
                    owner().position().x - entity->position().x,
                    owner().position().y - entity->position().y,
                };
                const double radius = std::max(kDefaultOrbitRadiusMeters, std::hypot(offset.x, offset.y));
                return {entity->position(), radius, entity->displayName().empty() ? entity->id() : entity->displayName()};
            }
        }

        if (taskParameters_.orbitRadiusMeters.has_value()) {
            return {owner().position(), std::max(0.0, *taskParameters_.orbitRadiusMeters), "orbit-anchor"};
        }

        if (!movement.route().empty()) {
            Vector2 center;
            for (const auto& waypoint : movement.route()) {
                center.x += waypoint.position.x;
                center.y += waypoint.position.y;
            }

            center.x /= static_cast<double>(movement.route().size());
            center.y /= static_cast<double>(movement.route().size());

            double radiusSum = 0.0;
            for (const auto& waypoint : movement.route()) {
                const auto offset = Vector2 {waypoint.position.x - center.x, waypoint.position.y - center.y};
                radiusSum += std::hypot(offset.x, offset.y);
            }

            const double averageRadius = radiusSum / static_cast<double>(movement.route().size());
            return {center, std::max(kDefaultOrbitRadiusMeters, averageRadius), "orbit-anchor"};
        }

        return {owner().position(), kDefaultOrbitRadiusMeters, "orbit-anchor"};
    }

    Vector2 computeEscortAnchor(const Entity& target) const
    {
        const double headingRadians = target.headingDegrees() * 3.14159265358979323846 / 180.0;
        const auto forward = Vector2 {std::cos(headingRadians), std::sin(headingRadians)};
        const auto right = Vector2 {std::sin(headingRadians), -std::cos(headingRadians)};
        return {
            target.position().x - forward.x * escortTrailDistanceMeters() + right.x * escortRightOffsetMeters(),
            target.position().y - forward.y * escortTrailDistanceMeters() + right.y * escortRightOffsetMeters(),
        };
    }

    void updateRouteProgress()
    {
        if (const auto* movement = owner().getComponent<MovementComponent>(); movement != nullptr) {
            completedWaypointVisits_ = movement->totalWaypointVisits();
            completedRouteCycles_ = movement->completedRouteCycles();
            if (const auto* lastWaypoint = movement->lastReachedWaypoint(); lastWaypoint != nullptr) {
                lastCompletedWaypointName_ = lastWaypoint->name;
            }
            return;
        }

        completedWaypointVisits_ = 0;
        completedRouteCycles_ = 0;
        lastCompletedWaypointName_.clear();
    }

    void applyExecutionState(MissionExecutionStatus nextStatus,
                             std::string nextPhase,
                             const SimulationUpdateContext* context,
                             std::string terminalReason = {})
    {
        const bool changed = nextStatus != executionStatus_ || nextPhase != executionPhase_;
        executionStatus_ = nextStatus;
        executionPhase_ = std::move(nextPhase);
        terminalReason_ = std::move(terminalReason);

        if (context == nullptr) {
            phaseElapsedSeconds_ = 0.0;
            phaseEnteredTick_ = 0;
            return;
        }

        if (changed) {
            phaseEnteredTick_ = context->tickCount;
            phaseEnteredSimulationSeconds_ = context->elapsedSimulationSeconds;
        }

        phaseElapsedSeconds_ = std::max(0.0, context->elapsedSimulationSeconds - phaseEnteredSimulationSeconds_);
    }

    std::string objective_;
    std::string behavior_;
    Behavior behaviorType_ {Behavior::Unknown};
    std::string targetEntityId_;
    TaskParameters taskParameters_;
    double timeoutSeconds_ {0.0};
    std::string prerequisiteEntityId_;
    std::string prerequisiteStatus_ {"completed"};
    std::vector<ReplanStep> replanChain_;
    std::size_t nextReplanIndex_ {0};
    MissionExecutionStatus executionStatus_ {MissionExecutionStatus::Unassigned};
    std::string executionPhase_ {"unassigned"};
    double phaseEnteredSimulationSeconds_ {0.0};
    double activationStartedSimulationSeconds_ {0.0};
    double phaseElapsedSeconds_ {0.0};
    std::uint64_t phaseEnteredTick_ {0};
    std::size_t completedWaypointVisits_ {0};
    std::size_t completedRouteCycles_ {0};
    std::string lastCompletedWaypointName_;
    std::string terminalReason_;
    std::string lastReplanReason_;
    std::string lastReplanFromBehavior_;
    std::string lastReplanToBehavior_;
    bool activationSatisfied_ {false};
};

}  // namespace fm::core