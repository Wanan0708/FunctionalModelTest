#include "app/services/SimulationSession.h"

#include <unordered_map>
#include <stdexcept>
#include <utility>

#include "app/scenario/ScenarioLoader.h"
#include "core/component/MissionComponent.h"
#include "core/component/MovementComponent.h"
#include "core/component/SensorComponent.h"
#include "core/entity/Entity.h"

namespace fm::app {

SimulationSession::SimulationSession(double fixedTimeStepSeconds)
    : simulation_(fixedTimeStepSeconds)
{
    appendLog(QStringLiteral("仿真会话已创建。"));
}

bool SimulationSession::loadScenario(const QString& filePath)
{
    const auto result = ScenarioLoader::loadFromFile(filePath);
    if (!result.ok()) {
        lastError_ = result.errorMessage;
        appendLog(QStringLiteral("场景加载失败: %1").arg(lastError_));
        return false;
    }

    scenario_ = result.scenario;
    currentScenarioPath_ = filePath;
    lastError_.clear();
    scenarioLoaded_ = true;
    rebuildFromScenario();
    appendLog(QStringLiteral("场景加载成功: %1, 实体数: %2")
                  .arg(currentScenarioName())
                  .arg(scenario_.entities.size()));
    return true;
}

const ScenarioDefinition* SimulationSession::scenarioDefinition() const
{
    return scenarioLoaded_ ? &scenario_ : nullptr;
}

bool SimulationSession::updateScenarioDefinition(ScenarioDefinition scenarioDefinition)
{
    if (!scenarioLoaded_) {
        lastError_ = QStringLiteral("尚未加载场景，无法更新场景属性。");
        appendLog(QStringLiteral("场景编辑失败: %1").arg(lastError_));
        return false;
    }

    if (scenarioDefinition.name.empty()) {
        lastError_ = QStringLiteral("场景名称不能为空。");
        appendLog(QStringLiteral("场景编辑失败: %1").arg(lastError_));
        return false;
    }

    scenario_ = std::move(scenarioDefinition);
    simulation_.stop();
    rebuildFromScenario();
    appendLog(QStringLiteral("场景定义已更新: %1").arg(currentScenarioName()));
    return true;
}

bool SimulationSession::addEntityDefinition(EntityDefinition entityDefinition)
{
    if (!scenarioLoaded_) {
        lastError_ = QStringLiteral("尚未加载场景，无法新增实体。");
        appendLog(QStringLiteral("实体新增失败: %1").arg(lastError_));
        return false;
    }

    if (entityDefinition.identity.id.empty()) {
        lastError_ = QStringLiteral("新增实体必须包含唯一 ID。");
        appendLog(QStringLiteral("实体新增失败: %1").arg(lastError_));
        return false;
    }

    for (const auto& entity : scenario_.entities) {
        if (entity.identity.id == entityDefinition.identity.id) {
            lastError_ = QStringLiteral("实体 ID 已存在: %1").arg(QString::fromStdString(entityDefinition.identity.id));
            appendLog(QStringLiteral("实体新增失败: %1").arg(lastError_));
            return false;
        }
    }

    scenario_.entities.push_back(std::move(entityDefinition));
    simulation_.stop();
    rebuildFromScenario();
    appendLog(QStringLiteral("实体已新增: %1").arg(QString::fromStdString(scenario_.entities.back().identity.id)));
    return true;
}

bool SimulationSession::removeEntityDefinition(const std::string& entityId)
{
    if (!scenarioLoaded_) {
        lastError_ = QStringLiteral("尚未加载场景，无法删除实体。");
        appendLog(QStringLiteral("实体删除失败: %1").arg(lastError_));
        return false;
    }

    for (auto it = scenario_.entities.begin(); it != scenario_.entities.end(); ++it) {
        if (it->identity.id != entityId) {
            continue;
        }

        scenario_.entities.erase(it);
        simulation_.stop();
        rebuildFromScenario();
        appendLog(QStringLiteral("实体已删除: %1").arg(QString::fromStdString(entityId)));
        return true;
    }

    lastError_ = QStringLiteral("未找到实体: %1").arg(QString::fromStdString(entityId));
    appendLog(QStringLiteral("实体删除失败: %1").arg(lastError_));
    return false;
}

bool SimulationSession::updateEntityDefinition(const std::string& entityId, EntityDefinition entityDefinition)
{
    if (!scenarioLoaded_) {
        lastError_ = QStringLiteral("尚未加载场景，无法更新实体。");
        appendLog(QStringLiteral("实体编辑失败: %1").arg(lastError_));
        return false;
    }

    for (auto& entity : scenario_.entities) {
        if (entity.identity.id != entityId) {
            continue;
        }

        entityDefinition.identity.id = entity.identity.id;
        entityDefinition.identity.tags = entity.identity.tags;
        entityDefinition.mission = entity.mission;
        entity = std::move(entityDefinition);
        simulation_.stop();
        rebuildFromScenario();
        appendLog(QStringLiteral("实体定义已更新: %1").arg(QString::fromStdString(entityId)));
        return true;
    }

    lastError_ = QStringLiteral("未找到实体: %1").arg(QString::fromStdString(entityId));
    appendLog(QStringLiteral("实体编辑失败: %1").arg(lastError_));
    return false;
}

bool SimulationSession::updateEntityMissionDefinition(const std::string& entityId, EntityMissionDefinition missionDefinition)
{
    if (!scenarioLoaded_) {
        lastError_ = QStringLiteral("尚未加载场景，无法更新任务。");
        appendLog(QStringLiteral("任务编辑失败: %1").arg(lastError_));
        return false;
    }

    for (auto& entity : scenario_.entities) {
        if (entity.identity.id != entityId) {
            continue;
        }

        entity.mission = std::move(missionDefinition);
        simulation_.stop();
        rebuildFromScenario();
        appendLog(QStringLiteral("任务已更新: %1").arg(QString::fromStdString(entityId)));
        return true;
    }

    lastError_ = QStringLiteral("未找到实体: %1").arg(QString::fromStdString(entityId));
    appendLog(QStringLiteral("任务编辑失败: %1").arg(lastError_));
    return false;
}

bool SimulationSession::saveScenarioToFile(const QString& filePath)
{
    if (!scenarioLoaded_) {
        lastError_ = QStringLiteral("尚未加载场景，无法保存。");
        appendLog(QStringLiteral("场景保存失败: %1").arg(lastError_));
        return false;
    }

    QString errorMessage;
    if (!ScenarioLoader::saveToFile(scenario_, filePath, errorMessage)) {
        lastError_ = errorMessage;
        appendLog(QStringLiteral("场景保存失败: %1").arg(lastError_));
        return false;
    }

    currentScenarioPath_ = filePath;
    lastError_.clear();
    appendLog(QStringLiteral("场景已保存: %1").arg(filePath));
    return true;
}

void SimulationSession::start()
{
    if (!scenarioLoaded_) {
        appendLog(QStringLiteral("开始失败: 尚未加载场景。"));
        return;
    }

    simulation_.start();
    appendLog(QStringLiteral("仿真开始运行。"));
}

void SimulationSession::pause()
{
    simulation_.pause();
    appendLog(QStringLiteral("仿真已暂停。"));
}

void SimulationSession::stop()
{
    simulation_.stop();
    appendLog(QStringLiteral("仿真已停止。"));
}

void SimulationSession::tick()
{
    if (simulation_.state() == fm::core::SimulationState::Running) {
        simulation_.advanceOneTick();
        recordTrajectories();
        recordSnapshot();
        logRenderStateTransitions(renderStates());
    }
}

void SimulationSession::step()
{
    if (!scenarioLoaded_) {
        appendLog(QStringLiteral("单步执行失败: 尚未加载场景。"));
        return;
    }

    simulation_.advanceOneTick();
    recordTrajectories();
    recordSnapshot();
    logRenderStateTransitions(renderStates());
    appendLog(QStringLiteral("单步执行完成: Tick=%1, 仿真时间=%2 s")
                  .arg(tickCount())
                  .arg(elapsedSimulationSeconds(), 0, 'f', 2));
}

void SimulationSession::reset()
{
    if (!scenarioLoaded_) {
        simulation_.stop();
        appendLog(QStringLiteral("重置请求已忽略: 尚未加载场景。"));
        return;
    }

    rebuildFromScenario();
    appendLog(QStringLiteral("场景已重置。"));
}

bool SimulationSession::hasScenario() const
{
    return scenarioLoaded_;
}

QString SimulationSession::currentScenarioPath() const
{
    return currentScenarioPath_;
}

QString SimulationSession::currentScenarioName() const
{
    return QString::fromStdString(scenario_.name);
}

QString SimulationSession::currentScenarioDescription() const
{
    return QString::fromStdString(scenario_.description);
}

const ScenarioEnvironmentDefinition& SimulationSession::currentScenarioEnvironment() const
{
    return scenario_.environment;
}

const ScenarioMapBoundsDefinition& SimulationSession::currentScenarioMapBounds() const
{
    return scenario_.mapBounds;
}

QString SimulationSession::lastError() const
{
    return lastError_;
}

double SimulationSession::fixedTimeStepSeconds() const
{
    return simulation_.fixedTimeStepSeconds();
}

bool SimulationSession::setFixedTimeStepSeconds(double value)
{
    try {
        simulation_.setFixedTimeStepSeconds(value);
        appendLog(QStringLiteral("固定时间步已更新为 %1 s。")
                      .arg(simulation_.fixedTimeStepSeconds(), 0, 'f', 3));
        return true;
    } catch (const std::invalid_argument&) {
        appendLog(QStringLiteral("固定时间步更新失败: 参数必须大于 0。"));
        return false;
    }
}

fm::core::SimulationState SimulationSession::state() const
{
    return simulation_.state();
}

double SimulationSession::elapsedSimulationSeconds() const
{
    return simulation_.elapsedSimulationSeconds();
}

std::uint64_t SimulationSession::tickCount() const
{
    return simulation_.tickCount();
}

std::vector<EntityRenderState> SimulationSession::renderStates() const
{
    const auto* snapshot = latestSnapshot();
    return snapshot != nullptr ? buildRenderStates(*snapshot) : std::vector<EntityRenderState> {};
}

std::vector<fm::core::Vector2> SimulationSession::trajectoryForEntity(const std::string& entityId) const
{
    const auto it = trajectoryHistory_.find(entityId);
    if (it == trajectoryHistory_.end()) {
        return {};
    }

    return it->second;
}

const std::vector<QString>& SimulationSession::eventLog() const
{
    return eventLog_;
}

const std::vector<SimulationSnapshot>& SimulationSession::recording() const
{
    return recording_;
}

const SimulationSnapshot* SimulationSession::latestSnapshot() const
{
    if (recording_.empty()) {
        return nullptr;
    }

    return &recording_.back();
}

const SimulationSnapshot* SimulationSession::snapshotAt(std::size_t index) const
{
    if (index >= recording_.size()) {
        return nullptr;
    }

    return &recording_[index];
}

std::vector<EntityRenderState> SimulationSession::renderStatesAt(std::size_t index) const
{
    const auto* snapshot = snapshotAt(index);
    return snapshot != nullptr ? buildRenderStates(*snapshot) : std::vector<EntityRenderState> {};
}

void SimulationSession::appendLog(QString message)
{
    constexpr std::size_t maxEntries = 200;

    message = QStringLiteral("[T=%1s | Tick=%2] %3")
                  .arg(elapsedSimulationSeconds(), 0, 'f', 2)
                  .arg(tickCount())
                  .arg(message);

    eventLog_.push_back(std::move(message));
    if (eventLog_.size() > maxEntries) {
        eventLog_.erase(eventLog_.begin(), eventLog_.begin() + static_cast<std::ptrdiff_t>(eventLog_.size() - maxEntries));
    }
}

std::vector<EntityRenderState> SimulationSession::buildRenderStates(const SimulationSnapshot& snapshot) const
{
    std::vector<EntityRenderState> states;
    states.reserve(snapshot.entities.size());

    for (const auto& entitySnapshot : snapshot.entities) {
        const EntityDefinition* definition = nullptr;
        for (const auto& scenarioEntity : scenario_.entities) {
            if (scenarioEntity.identity.id == entitySnapshot.id) {
                definition = &scenarioEntity;
                break;
            }
        }

        std::string displayName = entitySnapshot.id;
        std::string colorHex = "#4C956C";
        std::string side;
        std::string category;
        std::string role;
        std::vector<std::string> tags;
        double preferredSpeedMetersPerSecond = 0.0;
        std::string missionObjective;
        std::string missionBehavior;
        std::string missionTargetEntityId;
        std::string missionTaskParametersSummary;
        std::string missionExecutionStatus;
        std::string missionExecutionPhase;
        std::string missionTerminalReason;
        std::string missionPrerequisiteEntityId;
        std::string missionPrerequisiteStatus;
        bool missionActivationSatisfied = false;
        double missionTimeoutSeconds = 0.0;
        std::string missionReplanBehavior;
        std::size_t missionRemainingReplanSteps = 0;
        std::string missionLastReplanReason;
        std::string missionLastReplanFromBehavior;
        std::string missionLastReplanToBehavior;
        double missionPhaseElapsedSeconds = 0.0;
        std::uint64_t missionPhaseEnteredTick = 0;
        std::size_t missionCompletedWaypointVisits = 0;
        std::size_t missionCompletedRouteCycles = 0;
        std::string missionLastCompletedWaypointName;
        std::vector<RouteWaypointRenderState> routeWaypoints;
        std::size_t activeRouteWaypointIndex = 0;
        std::string activeRouteWaypointName;
        double routeLoiterSecondsRemaining = 0.0;
        bool routeIsLoitering = false;
        double maxSpeedMetersPerSecond = 0.0;
        double maxAccelerationMetersPerSecondSquared = 0.0;
        double maxDecelerationMetersPerSecondSquared = 0.0;
        double maxTurnRateDegreesPerSecond = 0.0;
        double radarCrossSectionSquareMeters = 1.0;
        bool sensorEnabled = false;
        std::string sensorType;
        double sensorRangeMeters = 0.0;
        double sensorFieldOfViewDegrees = 360.0;
        double radarPeakTransmitPowerWatts = 0.0;
        double radarAntennaGainDecibels = 0.0;
        double radarCenterFrequencyHertz = 0.0;
        double radarSignalBandwidthHertz = 0.0;
        double radarNoiseFigureDecibels = 0.0;
        double radarSystemLossDecibels = 0.0;
        double radarRequiredSnrDecibels = 13.0;
        double radarProcessingGainDecibels = 0.0;
        double radarScanRateHertz = 0.0;
        double radarReceiverTemperatureKelvin = 290.0;
        std::string movementGuidanceMode = "inertial";
        std::string movementGuidanceTargetName;

        if (definition != nullptr) {
            displayName = definition->identity.displayName.empty() ? definition->identity.id : definition->identity.displayName;
            colorHex = definition->identity.colorHex.empty() ? colorHex : definition->identity.colorHex;
            side = definition->identity.side;
            category = definition->identity.category;
            role = definition->identity.role;
            tags = definition->identity.tags;
            preferredSpeedMetersPerSecond = definition->kinematics.preferredSpeedMetersPerSecond;
            maxSpeedMetersPerSecond = definition->kinematics.maxSpeedMetersPerSecond;
            maxAccelerationMetersPerSecondSquared = definition->kinematics.maxAccelerationMetersPerSecondSquared;
            maxDecelerationMetersPerSecondSquared = definition->kinematics.maxDecelerationMetersPerSecondSquared;
            maxTurnRateDegreesPerSecond = definition->kinematics.maxTurnRateDegreesPerSecond;
            radarCrossSectionSquareMeters = definition->signature.radarCrossSectionSquareMeters;
            sensorEnabled = definition->sensor.enabled;
            sensorType = definition->sensor.type;
            sensorRangeMeters = definition->sensor.rangeMeters;
            sensorFieldOfViewDegrees = definition->sensor.fieldOfViewDegrees;
            radarPeakTransmitPowerWatts = definition->sensor.radar.peakTransmitPowerWatts;
            radarAntennaGainDecibels = definition->sensor.radar.antennaGainDecibels;
            radarCenterFrequencyHertz = definition->sensor.radar.centerFrequencyHertz;
            radarSignalBandwidthHertz = definition->sensor.radar.signalBandwidthHertz;
            radarNoiseFigureDecibels = definition->sensor.radar.noiseFigureDecibels;
            radarSystemLossDecibels = definition->sensor.radar.systemLossDecibels;
            radarRequiredSnrDecibels = definition->sensor.radar.requiredSnrDecibels;
            radarProcessingGainDecibels = definition->sensor.radar.processingGainDecibels;
            radarScanRateHertz = definition->sensor.radar.scanRateHertz;
            radarReceiverTemperatureKelvin = definition->sensor.radar.receiverTemperatureKelvin;
        }

        if (const auto* entity = findEntityById(entitySnapshot.id); entity != nullptr) {
            displayName = entity->displayName().empty() ? entity->id() : entity->displayName();
            colorHex = entity->colorHex().empty() ? colorHex : entity->colorHex();
            side = entity->side();
            category = entity->category();
            role = entity->role();
            tags = entity->tags();
            maxSpeedMetersPerSecond = entity->maxSpeedMetersPerSecond();
            maxAccelerationMetersPerSecondSquared = entity->maxAccelerationMetersPerSecondSquared();
            maxDecelerationMetersPerSecondSquared = entity->maxDecelerationMetersPerSecondSquared();
            maxTurnRateDegreesPerSecond = entity->maxTurnRateDegreesPerSecond();
            radarCrossSectionSquareMeters = entity->radarCrossSectionSquareMeters();
            if (const auto* mission = entity->getComponent<fm::core::MissionComponent>(); mission != nullptr) {
                missionObjective = mission->objective();
                missionBehavior = mission->behavior();
                missionTargetEntityId = mission->targetEntityId();
                if (mission->behaviorType() == fm::core::MissionComponent::Behavior::Intercept) {
                    missionTaskParametersSummary = QStringLiteral("拦截完成距离 %1 m")
                                                      .arg(mission->interceptCompletionDistanceMeters(), 0, 'f', 1)
                                                      .toStdString();
                } else if (mission->behaviorType() == fm::core::MissionComponent::Behavior::Escort) {
                    missionTaskParametersSummary = QStringLiteral("尾随 %1 m, 侧向偏移 %2 m, 槽位容差 %3 m, 稳定保持 %4 s")
                                                      .arg(mission->escortTrailDistanceMeters(), 0, 'f', 1)
                                                      .arg(mission->escortRightOffsetMeters(), 0, 'f', 1)
                                                      .arg(mission->escortSlotToleranceMeters(), 0, 'f', 1)
                                                      .arg(mission->escortCompletionHoldSeconds(), 0, 'f', 1)
                                                      .toStdString();
                } else if (mission->behaviorType() == fm::core::MissionComponent::Behavior::Orbit) {
                    const QString radiusText = mission->hasCustomOrbitRadiusMeters()
                                                   ? QStringLiteral("%1 m").arg(mission->orbitRadiusMeters(), 0, 'f', 1)
                                                   : QStringLiteral("自动");
                    missionTaskParametersSummary = QStringLiteral("半径 %1, 方向 %2, 进入容差 %3 m, 完成容差 %4 m, 稳定保持 %5 s")
                                                      .arg(radiusText)
                                                      .arg(mission->orbitClockwise() ? QStringLiteral("顺时针") : QStringLiteral("逆时针"))
                                                      .arg(mission->orbitAcquireToleranceMeters(), 0, 'f', 1)
                                                      .arg(mission->orbitCompletionToleranceMeters(), 0, 'f', 1)
                                                      .arg(mission->orbitCompletionHoldSeconds(), 0, 'f', 1)
                                                      .toStdString();
                } else if (mission->behaviorType() == fm::core::MissionComponent::Behavior::Transit) {
                    missionTaskParametersSummary = QStringLiteral("沿当前航路完成一次转场后结束").toStdString();
                }
                missionExecutionStatus = mission->executionStatusCode();
                missionExecutionPhase = mission->executionPhase();
                missionTerminalReason = mission->terminalReason();
                missionPrerequisiteEntityId = mission->prerequisiteEntityId();
                missionPrerequisiteStatus = mission->prerequisiteStatus();
                missionActivationSatisfied = mission->activationSatisfied();
                missionTimeoutSeconds = mission->timeoutSeconds();
                missionReplanBehavior = mission->replanBehavior();
                missionRemainingReplanSteps = mission->remainingReplanSteps();
                missionLastReplanReason = mission->lastReplanReason();
                missionLastReplanFromBehavior = mission->lastReplanFromBehavior();
                missionLastReplanToBehavior = mission->lastReplanToBehavior();
                missionPhaseElapsedSeconds = mission->phaseElapsedSeconds();
                missionPhaseEnteredTick = mission->phaseEnteredTick();
                missionCompletedWaypointVisits = mission->completedWaypointVisits();
                missionCompletedRouteCycles = mission->completedRouteCycles();
                missionLastCompletedWaypointName = mission->lastCompletedWaypointName();
            }

            if (const auto* sensor = entity->getComponent<fm::core::SensorComponent>(); sensor != nullptr) {
                sensorEnabled = true;
                sensorRangeMeters = sensor->rangeMeters();
                sensorFieldOfViewDegrees = sensor->fieldOfViewDegrees();
            }

            if (const auto* movement = entity->getComponent<fm::core::MovementComponent>(); movement != nullptr) {
                routeWaypoints.reserve(movement->route().size());
                for (const auto& waypoint : movement->route()) {
                    routeWaypoints.push_back({waypoint.name, waypoint.position, waypoint.loiterSeconds});
                }

                switch (movement->guidanceMode()) {
                case fm::core::MovementGuidanceMode::Inertial:
                    movementGuidanceMode = "inertial";
                    break;
                case fm::core::MovementGuidanceMode::Route:
                    movementGuidanceMode = "route";
                    break;
                case fm::core::MovementGuidanceMode::MissionTarget:
                    movementGuidanceMode = "mission-target";
                    break;
                case fm::core::MovementGuidanceMode::MissionOrbit:
                    movementGuidanceMode = "mission-orbit";
                    break;
                }
                movementGuidanceTargetName = movement->guidanceMode() == fm::core::MovementGuidanceMode::MissionOrbit
                                                 ? movement->orbitGuidanceLabel()
                                                 : movement->guidanceTargetLabel();

                activeRouteWaypointIndex = movement->currentWaypointIndex();
                routeLoiterSecondsRemaining = movement->remainingLoiterSeconds();
                routeIsLoitering = movement->isLoitering();
                if (const auto* activeWaypoint = movement->activeWaypoint(); activeWaypoint != nullptr) {
                    activeRouteWaypointName = activeWaypoint->name;
                }
            }
        }

        states.push_back({
            entitySnapshot.id,
            displayName,
            colorHex,
            side,
            category,
            role,
            tags,
            entitySnapshot.position,
            entitySnapshot.velocity,
            entitySnapshot.headingDegrees,
            preferredSpeedMetersPerSecond,
            maxSpeedMetersPerSecond,
            maxAccelerationMetersPerSecondSquared,
            maxDecelerationMetersPerSecondSquared,
            maxTurnRateDegreesPerSecond,
            radarCrossSectionSquareMeters,
            sensorEnabled,
            sensorType,
            sensorRangeMeters,
            sensorFieldOfViewDegrees,
            radarPeakTransmitPowerWatts,
            radarAntennaGainDecibels,
            radarCenterFrequencyHertz,
            radarSignalBandwidthHertz,
            radarNoiseFigureDecibels,
            radarSystemLossDecibels,
            radarRequiredSnrDecibels,
            radarProcessingGainDecibels,
            radarScanRateHertz,
            radarReceiverTemperatureKelvin,
            movementGuidanceMode,
            movementGuidanceTargetName,
            missionObjective,
            missionBehavior,
            missionTargetEntityId,
            missionTaskParametersSummary,
            missionExecutionStatus,
            missionExecutionPhase,
            missionTerminalReason,
            missionPrerequisiteEntityId,
            missionPrerequisiteStatus,
            missionActivationSatisfied,
            missionTimeoutSeconds,
            missionReplanBehavior,
            missionRemainingReplanSteps,
            missionLastReplanReason,
            missionLastReplanFromBehavior,
            missionLastReplanToBehavior,
            missionPhaseElapsedSeconds,
            missionPhaseEnteredTick,
            missionCompletedWaypointVisits,
            missionCompletedRouteCycles,
            missionLastCompletedWaypointName,
            routeWaypoints,
            activeRouteWaypointIndex,
            activeRouteWaypointName,
            routeLoiterSecondsRemaining,
            routeIsLoitering,
            entitySnapshot.detectedTargetIds,
        });
    }

    return states;
}

const fm::core::Entity* SimulationSession::findEntityById(const std::string& entityId) const
{
    for (const auto& entity : simulation_.entities()) {
        if (entity != nullptr && entity->id() == entityId) {
            return entity.get();
        }
    }

    return nullptr;
}

void SimulationSession::logRenderStateTransitions(const std::vector<EntityRenderState>& currentStates)
{
    std::unordered_map<std::string, const EntityRenderState*> previousStatesById;
    previousStatesById.reserve(lastObservedRenderStates_.size());
    for (const auto& state : lastObservedRenderStates_) {
        previousStatesById.emplace(state.id, &state);
    }

    for (const auto& state : currentStates) {
        const auto previousIt = previousStatesById.find(state.id);
        if (previousIt == previousStatesById.end()) {
            continue;
        }

        const auto* previous = previousIt->second;
        const QString entityName = QString::fromStdString(state.displayName.empty() ? state.id : state.displayName);

        if (previous->missionExecutionStatus != state.missionExecutionStatus
            || previous->missionExecutionPhase != state.missionExecutionPhase) {
            appendLog(QStringLiteral("任务阶段切换: %1 -> %2 / %3")
                          .arg(entityName)
                          .arg(QString::fromStdString(state.missionExecutionStatus))
                          .arg(QString::fromStdString(state.missionExecutionPhase)));

            if (state.missionExecutionStatus == "completed" || state.missionExecutionStatus == "aborted") {
                appendLog(QStringLiteral("任务终态: %1 -> %2 (%3)")
                              .arg(entityName)
                              .arg(QString::fromStdString(state.missionExecutionStatus))
                              .arg(state.missionTerminalReason.empty()
                                       ? QStringLiteral("无")
                                       : QString::fromStdString(state.missionTerminalReason)));
            }
        }

        if (((!previous->missionActivationSatisfied && state.missionActivationSatisfied)
             || (previous->missionExecutionPhase == "awaiting-prerequisite"
                 && state.missionExecutionPhase != "awaiting-prerequisite"))
            && !state.missionPrerequisiteEntityId.empty()) {
            appendLog(QStringLiteral("任务激活: %1 <- %2 %3")
                          .arg(entityName)
                          .arg(QString::fromStdString(state.missionPrerequisiteEntityId))
                          .arg(QString::fromStdString(state.missionPrerequisiteStatus)));
        }

        if (previous->missionLastReplanReason != state.missionLastReplanReason
            && !state.missionLastReplanReason.empty()) {
            appendLog(QStringLiteral("任务重规划: %1 <- %2 / %3 -> %4 (剩余 %5)")
                          .arg(entityName)
                          .arg(QString::fromStdString(state.missionLastReplanReason))
                          .arg(state.missionLastReplanFromBehavior.empty()
                                   ? QStringLiteral("无")
                                   : QString::fromStdString(state.missionLastReplanFromBehavior))
                          .arg(state.missionLastReplanToBehavior.empty()
                                   ? QStringLiteral("无")
                                   : QString::fromStdString(state.missionLastReplanToBehavior))
                          .arg(state.missionRemainingReplanSteps));
        }

        if (previous->movementGuidanceMode != state.movementGuidanceMode
            || previous->movementGuidanceTargetName != state.movementGuidanceTargetName) {
            const QString guidanceTarget = state.movementGuidanceTargetName.empty()
                                               ? QStringLiteral("无")
                                               : QString::fromStdString(state.movementGuidanceTargetName);
            appendLog(QStringLiteral("机动引导切换: %1 -> %2 (%3)")
                          .arg(entityName)
                          .arg(QString::fromStdString(state.movementGuidanceMode))
                          .arg(guidanceTarget));
        }

        if (state.missionCompletedWaypointVisits > previous->missionCompletedWaypointVisits) {
            const QString waypointName = state.missionLastCompletedWaypointName.empty()
                                             ? QStringLiteral("未命名航点")
                                             : QString::fromStdString(state.missionLastCompletedWaypointName);
            appendLog(QStringLiteral("航路点到达: %1 -> %2 (累计 %3 次)")
                          .arg(entityName)
                          .arg(waypointName)
                          .arg(state.missionCompletedWaypointVisits));
        }
    }

    lastObservedRenderStates_ = currentStates;
}

void SimulationSession::recordTrajectories()
{
    constexpr std::size_t maxTrailPoints = 120;

    for (const auto& entity : simulation_.entities()) {
        auto& trajectory = trajectoryHistory_[entity->id()];
        trajectory.push_back(entity->position());
        if (trajectory.size() > maxTrailPoints) {
            trajectory.erase(trajectory.begin());
        }
    }
}

void SimulationSession::recordSnapshot()
{
    constexpr std::size_t maxSnapshots = 500;

    SimulationSnapshot snapshot;
    snapshot.tickCount = tickCount();
    snapshot.elapsedSimulationSeconds = elapsedSimulationSeconds();
    snapshot.entities.reserve(simulation_.entities().size());

    for (const auto& entity : simulation_.entities()) {
        std::vector<std::string> detectedTargetIds;
        if (const auto* sensor = entity->getComponent<fm::core::SensorComponent>(); sensor != nullptr) {
            detectedTargetIds = sensor->detectedTargetIds();
        }

        snapshot.entities.push_back({
            entity->id(),
            entity->position(),
            entity->velocity(),
            entity->headingDegrees(),
            std::move(detectedTargetIds),
        });
    }

    recording_.push_back(std::move(snapshot));
    if (recording_.size() > maxSnapshots) {
        recording_.erase(recording_.begin(), recording_.begin() + static_cast<std::ptrdiff_t>(recording_.size() - maxSnapshots));
    }
}

void SimulationSession::rebuildFromScenario()
{
    simulation_.clearEntities();
    trajectoryHistory_.clear();
    recording_.clear();
    lastObservedRenderStates_.clear();

    for (const auto& definition : scenario_.entities) {
        auto entity = std::make_shared<fm::core::Entity>(definition.identity.id, definition.identity.displayName);
        entity->setSide(definition.identity.side);
        entity->setCategory(definition.identity.category);
        entity->setRole(definition.identity.role);
        entity->setColorHex(definition.identity.colorHex);
        entity->setTags(definition.identity.tags);
        entity->setPosition(definition.kinematics.position);
        entity->setVelocity(definition.kinematics.velocity);
        entity->setHeadingDegrees(definition.kinematics.headingDegrees);
        entity->setMaxSpeedMetersPerSecond(definition.kinematics.maxSpeedMetersPerSecond);
        entity->setMaxAccelerationMetersPerSecondSquared(definition.kinematics.maxAccelerationMetersPerSecondSquared);
        entity->setMaxDecelerationMetersPerSecondSquared(definition.kinematics.maxDecelerationMetersPerSecondSquared);
        entity->setMaxTurnRateDegreesPerSecond(definition.kinematics.maxTurnRateDegreesPerSecond);
        entity->setRadarCrossSectionSquareMeters(definition.signature.radarCrossSectionSquareMeters);
        entity->addComponent<fm::core::MissionComponent>(
            definition.mission.objective,
            definition.mission.behavior,
            definition.mission.targetEntityId,
            fm::core::MissionComponent::TaskParameters {
                definition.mission.parameters.interceptCompletionDistanceMeters,
                definition.mission.parameters.orbitRadiusMeters,
                definition.mission.parameters.orbitClockwise,
                definition.mission.parameters.orbitAcquireToleranceMeters,
                definition.mission.parameters.orbitCompletionToleranceMeters,
                definition.mission.parameters.orbitCompletionHoldSeconds,
                definition.mission.parameters.escortTrailDistanceMeters,
                definition.mission.parameters.escortRightOffsetMeters,
                definition.mission.parameters.escortSlotToleranceMeters,
                definition.mission.parameters.escortCompletionHoldSeconds,
            },
            fm::core::MissionComponent::ConstraintConfig {
                definition.mission.timeoutSeconds,
                definition.mission.activation.prerequisiteEntityId,
                definition.mission.activation.prerequisiteStatus,
                definition.mission.replanOnAbort.objective,
                definition.mission.replanOnAbort.behavior,
                definition.mission.replanOnAbort.targetEntityId,
                definition.mission.replanOnAbort.timeoutSeconds,
                [&definition]() {
                    std::vector<fm::core::MissionComponent::ReplanStep> replanSteps;
                    replanSteps.reserve(definition.mission.replanChain.size());
                    for (const auto& replan : definition.mission.replanChain) {
                        replanSteps.push_back({
                            replan.objective,
                            replan.behavior,
                            replan.targetEntityId,
                            replan.timeoutSeconds,
                            {
                                replan.parameters.interceptCompletionDistanceMeters,
                                replan.parameters.orbitRadiusMeters,
                                replan.parameters.orbitClockwise,
                                replan.parameters.orbitAcquireToleranceMeters,
                                replan.parameters.orbitCompletionToleranceMeters,
                                replan.parameters.orbitCompletionHoldSeconds,
                                replan.parameters.escortTrailDistanceMeters,
                                replan.parameters.escortRightOffsetMeters,
                                replan.parameters.escortSlotToleranceMeters,
                                replan.parameters.escortCompletionHoldSeconds,
                            },
                        });
                    }
                    return replanSteps;
                }(),
            });

        if (!definition.kinematics.route.empty()) {
            std::vector<fm::core::RouteWaypoint> route;
            route.reserve(definition.kinematics.route.size());
            for (const auto& waypoint : definition.kinematics.route) {
                route.push_back({waypoint.name, waypoint.position, waypoint.loiterSeconds});
            }

            const auto preferredSpeed = definition.kinematics.preferredSpeedMetersPerSecond > 0.0
                ? definition.kinematics.preferredSpeedMetersPerSecond
                : std::max(std::hypot(definition.kinematics.velocity.x, definition.kinematics.velocity.y),
                           definition.kinematics.maxSpeedMetersPerSecond);
            entity->addComponent<fm::core::MovementComponent>(
                std::move(route),
                preferredSpeed,
                definition.kinematics.maxSpeedMetersPerSecond,
                definition.kinematics.maxTurnRateDegreesPerSecond,
                0.5,
                definition.kinematics.maxAccelerationMetersPerSecondSquared,
                definition.kinematics.maxDecelerationMetersPerSecondSquared);
        } else {
            entity->addComponent<fm::core::MovementComponent>();
        }
        if (definition.sensor.enabled && definition.sensor.rangeMeters > 0.0) {
            entity->addComponent<fm::core::SensorComponent>(
                definition.sensor.rangeMeters,
                definition.sensor.fieldOfViewDegrees,
                fm::core::SensorComponent::RadarParameters {
                    definition.sensor.radar.peakTransmitPowerWatts,
                    definition.sensor.radar.antennaGainDecibels,
                    definition.sensor.radar.centerFrequencyHertz,
                    definition.sensor.radar.signalBandwidthHertz,
                    definition.sensor.radar.noiseFigureDecibels,
                    definition.sensor.radar.systemLossDecibels,
                    definition.sensor.radar.requiredSnrDecibels,
                    definition.sensor.radar.processingGainDecibels,
                    definition.sensor.radar.scanRateHertz,
                    definition.sensor.radar.receiverTemperatureKelvin,
                });
        } else if (definition.sensor.enabled
                   && definition.sensor.radar.peakTransmitPowerWatts > 0.0
                   && definition.sensor.radar.centerFrequencyHertz > 0.0) {
            entity->addComponent<fm::core::SensorComponent>(
                0.0,
                definition.sensor.fieldOfViewDegrees,
                fm::core::SensorComponent::RadarParameters {
                    definition.sensor.radar.peakTransmitPowerWatts,
                    definition.sensor.radar.antennaGainDecibels,
                    definition.sensor.radar.centerFrequencyHertz,
                    definition.sensor.radar.signalBandwidthHertz,
                    definition.sensor.radar.noiseFigureDecibels,
                    definition.sensor.radar.systemLossDecibels,
                    definition.sensor.radar.requiredSnrDecibels,
                    definition.sensor.radar.processingGainDecibels,
                    definition.sensor.radar.scanRateHertz,
                    definition.sensor.radar.receiverTemperatureKelvin,
                });
        }

        trajectoryHistory_.emplace(definition.identity.id, std::vector<fm::core::Vector2> {definition.kinematics.position});
        simulation_.addEntity(std::move(entity));
    }

    simulation_.reset();
    recordSnapshot();
    lastObservedRenderStates_ = renderStates();
}

}  // namespace fm::app