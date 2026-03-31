#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <QString>

#include "app/scenario/ScenarioDefinition.h"
#include "core/simulation/SimulationManager.h"

namespace fm::app {

struct RouteWaypointRenderState {
    std::string name;
    fm::core::Vector2 position;
    double loiterSeconds {0.0};
};

struct EntityRenderState {
    std::string id;
    std::string displayName;
    std::string colorHex;
    std::string side;
    std::string category;
    std::string role;
    std::vector<std::string> tags;
    fm::core::Vector2 position;
    fm::core::Vector2 velocity;
    double headingDegrees {0.0};
    double maxSpeedMetersPerSecond {0.0};
    double maxTurnRateDegreesPerSecond {0.0};
    double sensorRangeMeters {0.0};
    double sensorFieldOfViewDegrees {360.0};
    std::string movementGuidanceMode;
    std::string movementGuidanceTargetName;
    std::string missionObjective;
    std::string missionBehavior;
    std::string missionTargetEntityId;
    std::string missionTaskParametersSummary;
    std::string missionExecutionStatus;
    std::string missionExecutionPhase;
    std::string missionTerminalReason;
    std::string missionPrerequisiteEntityId;
    std::string missionPrerequisiteStatus;
    bool missionActivationSatisfied {false};
    double missionTimeoutSeconds {0.0};
    std::string missionReplanBehavior;
    std::size_t missionRemainingReplanSteps {0};
    std::string missionLastReplanReason;
    std::string missionLastReplanFromBehavior;
    std::string missionLastReplanToBehavior;
    double missionPhaseElapsedSeconds {0.0};
    std::uint64_t missionPhaseEnteredTick {0};
    std::size_t missionCompletedWaypointVisits {0};
    std::size_t missionCompletedRouteCycles {0};
    std::string missionLastCompletedWaypointName;
    std::vector<RouteWaypointRenderState> routeWaypoints;
    std::size_t activeRouteWaypointIndex {0};
    std::string activeRouteWaypointName;
    double routeLoiterSecondsRemaining {0.0};
    bool routeIsLoitering {false};
    std::vector<std::string> detectedTargetIds;
};

struct EntitySnapshot {
    std::string id;
    fm::core::Vector2 position;
    fm::core::Vector2 velocity;
    double headingDegrees {0.0};
    std::vector<std::string> detectedTargetIds;
};

struct SimulationSnapshot {
    std::uint64_t tickCount {0};
    double elapsedSimulationSeconds {0.0};
    std::vector<EntitySnapshot> entities;
};

class SimulationSession {
public:
    explicit SimulationSession(double fixedTimeStepSeconds = 0.01);

    bool loadScenario(const QString& filePath);
    const ScenarioDefinition* scenarioDefinition() const;
    bool updateScenarioDefinition(ScenarioDefinition scenarioDefinition);
    bool addEntityDefinition(EntityDefinition entityDefinition);
    bool removeEntityDefinition(const std::string& entityId);
    bool updateEntityDefinition(const std::string& entityId, EntityDefinition entityDefinition);
    bool updateEntityMissionDefinition(const std::string& entityId, EntityMissionDefinition missionDefinition);
    bool saveScenarioToFile(const QString& filePath);
    void start();
    void pause();
    void stop();
    void tick();
    void step();
    void reset();

    bool hasScenario() const;
    QString currentScenarioPath() const;
    QString currentScenarioName() const;
    QString currentScenarioDescription() const;
    const ScenarioEnvironmentDefinition& currentScenarioEnvironment() const;
    const ScenarioMapBoundsDefinition& currentScenarioMapBounds() const;
    QString lastError() const;
    double fixedTimeStepSeconds() const;
    bool setFixedTimeStepSeconds(double value);

    fm::core::SimulationState state() const;
    double elapsedSimulationSeconds() const;
    std::uint64_t tickCount() const;

    std::vector<EntityRenderState> renderStates() const;
    std::vector<fm::core::Vector2> trajectoryForEntity(const std::string& entityId) const;
    const std::vector<QString>& eventLog() const;
    const std::vector<SimulationSnapshot>& recording() const;
    const SimulationSnapshot* latestSnapshot() const;
    const SimulationSnapshot* snapshotAt(std::size_t index) const;
    std::vector<EntityRenderState> renderStatesAt(std::size_t index) const;

private:
    void appendLog(QString message);
    std::vector<EntityRenderState> buildRenderStates(const SimulationSnapshot& snapshot) const;
    const fm::core::Entity* findEntityById(const std::string& entityId) const;
    void logRenderStateTransitions(const std::vector<EntityRenderState>& currentStates);
    void recordSnapshot();
    void recordTrajectories();
    void rebuildFromScenario();

    ScenarioDefinition scenario_;
    QString currentScenarioPath_;
    QString lastError_;
    fm::core::SimulationManager simulation_;
    std::unordered_map<std::string, std::vector<fm::core::Vector2>> trajectoryHistory_;
    std::vector<EntityRenderState> lastObservedRenderStates_;
    std::vector<QString> eventLog_;
    std::vector<SimulationSnapshot> recording_;
    bool scenarioLoaded_ {false};
};

}  // namespace fm::app