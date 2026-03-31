#include <cmath>

#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "app/scenario/ScenarioLoader.h"
#include "app/services/SimulationSession.h"
#include "core/component/MissionComponent.h"
#include "core/component/MovementComponent.h"
#include "core/component/SensorComponent.h"
#include "core/entity/Entity.h"
#include "core/simulation/SimulationManager.h"

class SimulationManagerTests final : public QObject {
    Q_OBJECT

private slots:
    void advanceOneTickIncrementsClock();
    void movementComponentUpdatesPosition();
    void entityStoresIntrinsicMetadata();
    void missionComponentStoresTaskState();
    void missionComponentTracksLoiterAndRouteProgress();
    void missionComponentDrivesInterceptGuidance();
    void missionComponentCompletesInterceptTask();
    void missionComponentAbortsWhenTargetMissing();
    void missionComponentDrivesEscortGuidance();
    void missionComponentCompletesEscortTask();
    void missionComponentDrivesOrbitGuidance();
    void missionComponentCompletesOrbitTask();
    void missionComponentUsesConfiguredTaskParameters();
    void missionComponentCompletesTransitTask();
    void missionComponentWaitsForPrerequisiteActivation();
    void missionComponentTimesOutAndReplans();
    void missionComponentSupportsMultiStageReplanChain();
    void simulationSessionRenderStateIncludesRuntimeMissionState();
    void scenarioLoaderParsesJson();
    void scenarioLoaderSavesAndReloadsJson();
    void scenarioLoaderSupportsLegacyFlatEntityFields();
    void scenarioLoaderRejectsDuplicateEntityIds();
    void simulationSessionTracksTrajectoryDuringStep();
    void simulationSessionUpdatesEntityMissionDefinition();
    void simulationSessionFollowsRouteWaypoints();
    void simulationSessionUpdatesFixedTimeStep();
    void simulationSessionRecordsEventLog();
    void simulationSessionLogsMissionTransitions();
    void simulationSessionLogsMissionConstraintEvents();
    void simulationSessionLogsGuidanceTransitions();
    void simulationSessionLogsMissionTerminalTransitions();
    void simulationSessionRenderStateIncludesTerminalStatus();
    void sensorComponentDetectsNearbyTargets();
    void sensorComponentRespectsFieldOfView();
    void simulationSessionRecordsSnapshots();
    void simulationSessionProvidesSnapshotViews();
};

void SimulationManagerTests::advanceOneTickIncrementsClock()
{
    fm::core::SimulationManager manager(0.1);

    manager.advanceOneTick();

    QCOMPARE(manager.tickCount(), std::uint64_t {1});
    QVERIFY(std::abs(manager.elapsedSimulationSeconds() - 0.1) < 1e-9);
}

void SimulationManagerTests::movementComponentUpdatesPosition()
{
    auto entity = std::make_shared<fm::core::Entity>("alpha", "Alpha");
    entity->setVelocity({10.0, -4.0});
    entity->addComponent<fm::core::MovementComponent>();

    fm::core::SimulationManager manager(0.5);
    manager.addEntity(entity);

    manager.advanceOneTick();

    QVERIFY(std::abs(entity->position().x - 5.0) < 1e-9);
    QVERIFY(std::abs(entity->position().y + 2.0) < 1e-9);
}

void SimulationManagerTests::entityStoresIntrinsicMetadata()
{
    fm::core::Entity entity("blue-1", "Blue One");
    entity.setSide("blue");
    entity.setCategory("aircraft");
    entity.setRole("patrol");
    entity.setColorHex("#2E86AB");
    entity.setTags({"screen", "lead"});
    entity.setMaxSpeedMetersPerSecond(16.0);
    entity.setMaxTurnRateDegreesPerSecond(180.0);

    QCOMPARE(entity.side(), std::string("blue"));
    QCOMPARE(entity.category(), std::string("aircraft"));
    QCOMPARE(entity.role(), std::string("patrol"));
    QCOMPARE(entity.colorHex(), std::string("#2E86AB"));
    QCOMPARE(entity.tags().size(), std::size_t {2});
    QVERIFY(std::abs(entity.maxSpeedMetersPerSecond() - 16.0) < 1e-9);
    QVERIFY(std::abs(entity.maxTurnRateDegreesPerSecond() - 180.0) < 1e-9);
}

void SimulationManagerTests::missionComponentStoresTaskState()
{
    auto entity = std::make_shared<fm::core::Entity>("alpha", "Alpha");
    entity->setPosition({0.0, 0.0});
    entity->setVelocity({8.0, 0.0});
    auto& mission = entity->addComponent<fm::core::MissionComponent>("Screen corridor", "patrol", "target-1");
    entity->addComponent<fm::core::MovementComponent>(
        std::vector<fm::core::RouteWaypoint> {
            {"gate-1", {100.0, 0.0}, 0.0},
            {"gate-2", {120.0, 20.0}, 0.0},
        },
        8.0,
        12.0,
        180.0);

    fm::core::SimulationManager manager(0.1);
    manager.addEntity(entity);

    QCOMPARE(mission.objective(), std::string("Screen corridor"));
    QCOMPARE(mission.behavior(), std::string("patrol"));
    QCOMPARE(mission.targetEntityId(), std::string("target-1"));
    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Unassigned);

    manager.advanceOneTick();

    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Navigating);
    QCOMPARE(mission.executionPhase(), std::string("patrol-transit"));
    QCOMPARE(mission.phaseEnteredTick(), std::uint64_t {1});

    mission.setBehavior("escort");
    QCOMPARE(mission.behavior(), std::string("escort"));
}

void SimulationManagerTests::missionComponentTracksLoiterAndRouteProgress()
{
    auto entity = std::make_shared<fm::core::Entity>("patrol-1", "Patrol One");
    entity->setPosition({0.0, 0.0});
    entity->setVelocity({1.0, 0.0});
    auto& mission = entity->addComponent<fm::core::MissionComponent>("Hold gate", "patrol", "");
    entity->addComponent<fm::core::MovementComponent>(
        std::vector<fm::core::RouteWaypoint> {
            {"gate-1", {1.0, 0.0}, 1.0},
            {"gate-2", {2.0, 0.0}, 0.0},
        },
        1.0,
        1.0,
        180.0);

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(entity);

    manager.advanceOneTick();

    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Loitering);
    QCOMPARE(mission.executionPhase(), std::string("patrol-hold"));
    QCOMPARE(mission.completedWaypointVisits(), std::size_t {1});
    QCOMPARE(mission.completedRouteCycles(), std::size_t {0});
    QCOMPARE(mission.lastCompletedWaypointName(), std::string("gate-1"));
    QCOMPARE(mission.phaseEnteredTick(), std::uint64_t {1});
    QVERIFY(std::abs(mission.phaseElapsedSeconds() - 0.0) < 1e-9);

    manager.advanceOneTick();

    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Navigating);
    QCOMPARE(mission.executionPhase(), std::string("patrol-transit"));
    QCOMPARE(mission.completedWaypointVisits(), std::size_t {1});
}

void SimulationManagerTests::missionComponentDrivesInterceptGuidance()
{
    auto interceptor = std::make_shared<fm::core::Entity>("blue-1", "Blue One");
    interceptor->setPosition({0.0, 0.0});
    interceptor->setVelocity({6.0, 0.0});
    interceptor->setHeadingDegrees(0.0);
    interceptor->setMaxSpeedMetersPerSecond(6.0);
    interceptor->setMaxTurnRateDegreesPerSecond(180.0);
    interceptor->addComponent<fm::core::MovementComponent>(std::vector<fm::core::RouteWaypoint> {}, 6.0, 6.0, 180.0);
    auto& mission = interceptor->addComponent<fm::core::MissionComponent>("Intercept intruder", "intercept", "green-1");

    auto intruder = std::make_shared<fm::core::Entity>("green-1", "Green Intruder");
    intruder->setPosition({0.0, 20.0});
    intruder->setVelocity({0.0, 0.0});

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(interceptor);
    manager.addEntity(intruder);

    manager.advanceOneTick();
    manager.advanceOneTick();

    QVERIFY(interceptor->position().y > 1.0);
    QVERIFY(interceptor->headingDegrees() > 0.0);
    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Navigating);
    QCOMPARE(mission.executionPhase(), std::string("intercept-transit"));
    const auto* movement = interceptor->getComponent<fm::core::MovementComponent>();
    QVERIFY(movement != nullptr);
    QCOMPARE(movement->guidanceMode(), fm::core::MovementGuidanceMode::MissionTarget);
    QCOMPARE(movement->guidanceTargetLabel(), std::string("Green Intruder"));
}

void SimulationManagerTests::missionComponentCompletesInterceptTask()
{
    auto interceptor = std::make_shared<fm::core::Entity>("blue-1", "Blue One");
    interceptor->setPosition({0.0, 0.0});
    interceptor->setVelocity({4.0, 0.0});
    interceptor->setHeadingDegrees(0.0);
    interceptor->addComponent<fm::core::MovementComponent>(std::vector<fm::core::RouteWaypoint> {}, 4.0, 4.0, 180.0);
    interceptor->addComponent<fm::core::SensorComponent>(10.0);
    auto& mission = interceptor->addComponent<fm::core::MissionComponent>("Intercept intruder", "intercept", "green-1");

    auto intruder = std::make_shared<fm::core::Entity>("green-1", "Green Intruder");
    intruder->setPosition({1.0, 0.0});
    intruder->setVelocity({0.0, 0.0});

    fm::core::SimulationManager manager(0.5);
    manager.addEntity(interceptor);
    manager.addEntity(intruder);

    manager.advanceOneTick();
    manager.advanceOneTick();

    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Completed);
    QCOMPARE(mission.executionPhase(), std::string("intercept-complete"));
    QCOMPARE(mission.terminalReason(), std::string("target-contained"));
}

void SimulationManagerTests::missionComponentAbortsWhenTargetMissing()
{
    auto interceptor = std::make_shared<fm::core::Entity>("blue-1", "Blue One");
    interceptor->setPosition({0.0, 0.0});
    interceptor->setVelocity({4.0, 0.0});
    interceptor->setHeadingDegrees(0.0);
    interceptor->addComponent<fm::core::MovementComponent>(std::vector<fm::core::RouteWaypoint> {}, 4.0, 4.0, 180.0);
    auto& mission = interceptor->addComponent<fm::core::MissionComponent>("Intercept intruder", "intercept", "green-1");

    fm::core::SimulationManager manager(0.5);
    manager.addEntity(interceptor);

    manager.advanceOneTick();

    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Aborted);
    QCOMPARE(mission.executionPhase(), std::string("target-unavailable"));
    QCOMPARE(mission.terminalReason(), std::string("target-unavailable"));
}

void SimulationManagerTests::missionComponentDrivesEscortGuidance()
{
    auto lead = std::make_shared<fm::core::Entity>("blue-lead", "Blue Lead");
    lead->setPosition({10.0, 10.0});
    lead->setVelocity({4.0, 0.0});
    lead->setHeadingDegrees(0.0);

    auto wing = std::make_shared<fm::core::Entity>("blue-wing", "Blue Wing");
    wing->setPosition({0.0, 0.0});
    wing->setVelocity({5.0, 0.0});
    wing->setHeadingDegrees(0.0);
    wing->setMaxSpeedMetersPerSecond(5.0);
    wing->setMaxTurnRateDegreesPerSecond(180.0);
    wing->addComponent<fm::core::MovementComponent>(std::vector<fm::core::RouteWaypoint> {}, 5.0, 5.0, 180.0);
    auto& mission = wing->addComponent<fm::core::MissionComponent>("Escort lead", "escort", "blue-lead");

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(wing);
    manager.addEntity(lead);

    manager.advanceOneTick();
    manager.advanceOneTick();

    QVERIFY(wing->position().x > 1.0);
    QVERIFY(wing->position().y > 1.0);
    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Navigating);
    QCOMPARE(mission.executionPhase(), std::string("escort-rejoin"));
    const auto* movement = wing->getComponent<fm::core::MovementComponent>();
    QVERIFY(movement != nullptr);
    QCOMPARE(movement->guidanceMode(), fm::core::MovementGuidanceMode::MissionTarget);
    QCOMPARE(movement->guidanceTargetLabel(), std::string("Blue Lead escort slot"));
}

void SimulationManagerTests::missionComponentCompletesEscortTask()
{
    auto lead = std::make_shared<fm::core::Entity>("blue-lead", "Blue Lead");
    lead->setPosition({0.0, 0.0});
    lead->setVelocity({0.0, 0.0});
    lead->setHeadingDegrees(0.0);

    auto wing = std::make_shared<fm::core::Entity>("blue-wing", "Blue Wing");
    wing->setPosition({-6.0, -4.0});
    wing->setVelocity({0.0, 0.0});
    wing->setHeadingDegrees(0.0);
    wing->addComponent<fm::core::MovementComponent>(std::vector<fm::core::RouteWaypoint> {}, 2.0, 2.0, 180.0);
    auto& mission = wing->addComponent<fm::core::MissionComponent>("Escort lead", "escort", "blue-lead");

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(wing);
    manager.addEntity(lead);

    manager.advanceOneTick();
    manager.advanceOneTick();
    manager.advanceOneTick();

    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Completed);
    QCOMPARE(mission.executionPhase(), std::string("escort-complete"));
    QCOMPARE(mission.terminalReason(), std::string("formation-stable"));
}

void SimulationManagerTests::missionComponentDrivesOrbitGuidance()
{
    auto relay = std::make_shared<fm::core::Entity>("gold-relay", "Gold Relay");
    relay->setPosition({10.0, 0.0});
    relay->setVelocity({0.0, 4.0});
    relay->setHeadingDegrees(90.0);
    relay->setMaxSpeedMetersPerSecond(4.0);
    relay->setMaxTurnRateDegreesPerSecond(180.0);
    relay->addComponent<fm::core::MovementComponent>(
        std::vector<fm::core::RouteWaypoint> {
            {"orbit-east", {10.0, 0.0}, 0.0},
            {"orbit-west", {-10.0, 0.0}, 0.0},
        },
        4.0,
        4.0,
        180.0);
    auto& mission = relay->addComponent<fm::core::MissionComponent>("Relay orbit", "orbit", "");

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(relay);

    manager.advanceOneTick();
    manager.advanceOneTick();

    QVERIFY(std::abs(relay->position().x - 10.0) > 0.2 || std::abs(relay->position().y) > 0.2);
    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Navigating);
    const auto* movement = relay->getComponent<fm::core::MovementComponent>();
    QVERIFY(movement != nullptr);
    QCOMPARE(movement->guidanceMode(), fm::core::MovementGuidanceMode::MissionOrbit);
    QCOMPARE(movement->orbitGuidanceLabel(), std::string("orbit-anchor"));
    QVERIFY(mission.executionPhase() == std::string("orbit-acquire") || mission.executionPhase() == std::string("orbit-station"));
}

void SimulationManagerTests::missionComponentCompletesOrbitTask()
{
    auto relay = std::make_shared<fm::core::Entity>("gold-relay", "Gold Relay");
    relay->setPosition({10.0, 0.0});
    relay->setVelocity({0.0, -2.0});
    relay->setHeadingDegrees(-90.0);
    relay->addComponent<fm::core::MovementComponent>(
        std::vector<fm::core::RouteWaypoint> {
            {"orbit-east", {10.0, 0.0}, 0.0},
            {"orbit-west", {-10.0, 0.0}, 0.0},
        },
        2.0,
        2.0,
        180.0);
    auto& mission = relay->addComponent<fm::core::MissionComponent>("Relay orbit", "orbit", "");

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(relay);

    manager.advanceOneTick();
    manager.advanceOneTick();
    manager.advanceOneTick();
    manager.advanceOneTick();
    manager.advanceOneTick();
    manager.advanceOneTick();
    manager.advanceOneTick();
    manager.advanceOneTick();

    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Completed);
    QCOMPARE(mission.executionPhase(), std::string("orbit-complete"));
    QCOMPARE(mission.terminalReason(), std::string("orbit-established"));
}

void SimulationManagerTests::missionComponentUsesConfiguredTaskParameters()
{
    auto lead = std::make_shared<fm::core::Entity>("blue-lead", "Blue Lead");
    lead->setPosition({0.0, 0.0});
    lead->setVelocity({0.0, 0.0});
    lead->setHeadingDegrees(0.0);

    auto wing = std::make_shared<fm::core::Entity>("blue-wing", "Blue Wing");
    wing->setPosition({0.0, 0.0});
    wing->setVelocity({4.0, 0.0});
    wing->setHeadingDegrees(0.0);
    wing->addComponent<fm::core::MovementComponent>(std::vector<fm::core::RouteWaypoint> {}, 4.0, 4.0, 180.0);
    fm::core::MissionComponent::TaskParameters escortParameters;
    escortParameters.escortTrailDistanceMeters = 10.0;
    escortParameters.escortRightOffsetMeters = 2.0;
    escortParameters.escortSlotToleranceMeters = 0.75;
    escortParameters.escortCompletionHoldSeconds = 0.5;
    auto& escortMission = wing->addComponent<fm::core::MissionComponent>("Escort lead", "escort", "blue-lead", escortParameters);

    fm::core::SimulationManager escortManager(1.0);
    escortManager.addEntity(wing);
    escortManager.addEntity(lead);
    escortManager.advanceOneTick();

    const auto* wingMovement = wing->getComponent<fm::core::MovementComponent>();
    QVERIFY(wingMovement != nullptr);
    QVERIFY(wingMovement->hasGuidanceTarget());
    QVERIFY(std::abs(wingMovement->guidanceTargetPosition().x + 10.0) < 1e-6);
    QVERIFY(std::abs(wingMovement->guidanceTargetPosition().y + 2.0) < 1e-6);
    QVERIFY(std::abs(escortMission.escortTrailDistanceMeters() - 10.0) < 1e-9);
    QVERIFY(std::abs(escortMission.escortRightOffsetMeters() - 2.0) < 1e-9);

    auto interceptor = std::make_shared<fm::core::Entity>("blue-1", "Blue One");
    interceptor->setPosition({0.0, 0.0});
    interceptor->setVelocity({4.0, 0.0});
    interceptor->setHeadingDegrees(0.0);
    interceptor->addComponent<fm::core::MovementComponent>(std::vector<fm::core::RouteWaypoint> {}, 4.0, 4.0, 180.0);
    interceptor->addComponent<fm::core::SensorComponent>(10.0);
    fm::core::MissionComponent::TaskParameters interceptParameters;
    interceptParameters.interceptCompletionDistanceMeters = 5.0;
    auto& interceptMission = interceptor->addComponent<fm::core::MissionComponent>("Intercept intruder", "intercept", "green-1", interceptParameters);

    auto intruder = std::make_shared<fm::core::Entity>("green-1", "Green Intruder");
    intruder->setPosition({4.0, 0.0});
    intruder->setVelocity({0.0, 0.0});

    fm::core::SimulationManager interceptManager(0.5);
    interceptManager.addEntity(interceptor);
    interceptManager.addEntity(intruder);
    interceptManager.advanceOneTick();
    interceptManager.advanceOneTick();

    QCOMPARE(interceptMission.executionStatus(), fm::core::MissionExecutionStatus::Completed);
    QVERIFY(std::abs(interceptMission.interceptCompletionDistanceMeters() - 5.0) < 1e-9);
}

void SimulationManagerTests::missionComponentCompletesTransitTask()
{
    auto entity = std::make_shared<fm::core::Entity>("transit-1", "Transit One");
    entity->setPosition({0.0, 0.0});
    entity->setVelocity({2.0, 0.0});
    entity->setHeadingDegrees(0.0);
    entity->addComponent<fm::core::MovementComponent>(
        std::vector<fm::core::RouteWaypoint> {
            {"leg-a", {0.0, 0.0}, 0.0},
            {"leg-b", {2.0, 0.0}, 0.0},
        },
        2.0,
        2.0,
        180.0);
    auto& mission = entity->addComponent<fm::core::MissionComponent>("Transit to gate", "transit", "");

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(entity);

    manager.advanceOneTick();
    manager.advanceOneTick();

    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Completed);
    QCOMPARE(mission.executionPhase(), std::string("transit-complete"));
    QCOMPARE(mission.terminalReason(), std::string("route-finished"));
}

void SimulationManagerTests::missionComponentWaitsForPrerequisiteActivation()
{
    auto lead = std::make_shared<fm::core::Entity>("lead", "Lead");
    lead->setPosition({0.0, 0.0});
    lead->setVelocity({4.0, 0.0});
    lead->setHeadingDegrees(0.0);
    lead->addComponent<fm::core::MovementComponent>(std::vector<fm::core::RouteWaypoint> {}, 4.0, 4.0, 180.0);
    lead->addComponent<fm::core::SensorComponent>(10.0);
    auto& leadMission = lead->addComponent<fm::core::MissionComponent>("Intercept target", "intercept", "intruder");

    auto intruder = std::make_shared<fm::core::Entity>("intruder", "Intruder");
    intruder->setPosition({1.0, 0.0});
    intruder->setVelocity({0.0, 0.0});

    auto support = std::make_shared<fm::core::Entity>("support", "Support");
    support->setPosition({-5.0, 0.0});
    support->setVelocity({0.0, 0.0});
    support->setHeadingDegrees(0.0);
    support->addComponent<fm::core::MovementComponent>(std::vector<fm::core::RouteWaypoint> {}, 2.0, 2.0, 180.0);
    auto& supportMission = support->addComponent<fm::core::MissionComponent>(
        "Join after lead finishes",
        "intercept",
        "lead",
        fm::core::MissionComponent::ConstraintConfig {
            0.0,
            "lead",
            "completed",
            "",
            "",
            "",
        });

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(lead);
    manager.addEntity(intruder);
    manager.addEntity(support);

    manager.advanceOneTick();
    QCOMPARE(leadMission.executionStatus(), fm::core::MissionExecutionStatus::Navigating);
    QCOMPARE(supportMission.executionStatus(), fm::core::MissionExecutionStatus::Idle);
    QCOMPARE(supportMission.executionPhase(), std::string("awaiting-prerequisite"));
    QVERIFY(!supportMission.activationSatisfied());

    manager.advanceOneTick();
    manager.advanceOneTick();
    QCOMPARE(leadMission.executionStatus(), fm::core::MissionExecutionStatus::Completed);

    manager.advanceOneTick();
    QVERIFY(supportMission.activationSatisfied());
    QVERIFY(supportMission.executionPhase() == std::string("intercept-transit")
            || supportMission.executionPhase() == std::string("target-intercept-window")
            || supportMission.executionStatus() == fm::core::MissionExecutionStatus::Completed);
}

void SimulationManagerTests::missionComponentTimesOutAndReplans()
{
    auto entity = std::make_shared<fm::core::Entity>("escort-1", "Escort One");
    entity->setPosition({0.0, 0.0});
    entity->setVelocity({2.0, 0.0});
    entity->setHeadingDegrees(0.0);
    entity->addComponent<fm::core::MovementComponent>(
        std::vector<fm::core::RouteWaypoint> {
            {"fallback-a", {0.0, 0.0}, 0.0},
            {"fallback-b", {3.0, 0.0}, 0.0},
        },
        2.0,
        2.0,
        180.0);
    auto& mission = entity->addComponent<fm::core::MissionComponent>(
        "Timed patrol",
        "patrol",
        "",
        fm::core::MissionComponent::ConstraintConfig {
            1.0,
            "",
            "completed",
            "Fallback orbit",
            "orbit",
            "",
        });

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(entity);

    manager.advanceOneTick();
    QCOMPARE(mission.behavior(), std::string("patrol"));

    manager.advanceOneTick();

    QCOMPARE(mission.behavior(), std::string("orbit"));
    QCOMPARE(mission.executionStatus(), fm::core::MissionExecutionStatus::Idle);
    QCOMPARE(mission.executionPhase(), std::string("replanned-after-abort"));
    QCOMPARE(mission.lastReplanReason(), std::string("timeout-expired"));
}

void SimulationManagerTests::missionComponentSupportsMultiStageReplanChain()
{
    auto lead = std::make_shared<fm::core::Entity>("lead", "Lead");
    lead->setPosition({0.0, 0.0});
    lead->setVelocity({0.0, 0.0});
    lead->setHeadingDegrees(0.0);

    auto entity = std::make_shared<fm::core::Entity>("reserve-1", "Reserve One");
    entity->setPosition({0.0, 0.0});
    entity->setVelocity({2.0, 0.0});
    entity->setHeadingDegrees(0.0);
    entity->addComponent<fm::core::MovementComponent>(
        std::vector<fm::core::RouteWaypoint> {
            {"fallback-a", {0.0, 0.0}, 0.0},
            {"fallback-b", {3.0, 0.0}, 0.0},
        },
        2.0,
        2.0,
        180.0);
    auto& mission = entity->addComponent<fm::core::MissionComponent>(
        "Timed patrol",
        "patrol",
        "",
        fm::core::MissionComponent::ConstraintConfig {
            1.0,
            "",
            "completed",
            "Fallback orbit",
            "orbit",
            "lead",
            1.0,
            {
                {"Fallback escort", "escort", "lead", 0.0},
            },
        });

    fm::core::SimulationManager manager(1.0);
    manager.addEntity(lead);
    manager.addEntity(entity);

    manager.advanceOneTick();
    QCOMPARE(mission.behavior(), std::string("patrol"));
    QCOMPARE(mission.remainingReplanSteps(), std::size_t {2});

    manager.advanceOneTick();
    QCOMPARE(mission.behavior(), std::string("orbit"));
    QCOMPARE(mission.lastReplanFromBehavior(), std::string("patrol"));
    QCOMPARE(mission.lastReplanToBehavior(), std::string("orbit"));
    QCOMPARE(mission.remainingReplanSteps(), std::size_t {1});

    manager.advanceOneTick();
    QCOMPARE(mission.behavior(), std::string("escort"));
    QCOMPARE(mission.lastReplanFromBehavior(), std::string("orbit"));
    QCOMPARE(mission.lastReplanToBehavior(), std::string("escort"));
    QCOMPARE(mission.remainingReplanSteps(), std::size_t {0});
}

void SimulationManagerTests::simulationSessionRenderStateIncludesRuntimeMissionState()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QString scenarioPath = temporaryDir.filePath("runtime_state_scenario.json");
    QFile file(scenarioPath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({
        "name": "Runtime State Scenario",
        "entities": [
            {
                "identity": {
                    "id": "blue-1",
                    "displayName": "Blue One",
                    "side": "blue",
                    "category": "aircraft",
                    "role": "screen",
                    "color": "#2E86AB",
                    "tags": ["lead", "screen"]
                },
                "kinematics": {
                    "position": {"x": 0.0, "y": 0.0},
                    "velocity": {"x": 10.0, "y": 0.0},
                    "headingDegrees": 0.0,
                    "maxSpeedMetersPerSecond": 18.0,
                    "maxTurnRateDegreesPerSecond": 120.0,
                    "route": [
                        {
                            "name": "gate-west",
                            "position": {"x": 50.0, "y": 0.0},
                            "loiterSeconds": 0.0
                        },
                        {
                            "name": "gate-east",
                            "position": {"x": 80.0, "y": 20.0},
                            "loiterSeconds": 0.0
                        }
                    ]
                },
                "sensor": {
                    "type": "radar",
                    "rangeMeters": 25.0,
                    "enabled": true
                },
                "mission": {
                    "objective": "Hold corridor",
                    "behavior": "patrol",
                    "targetEntityId": "green-1"
                }
            }
        ]
    })");
    file.close();

    fm::app::SimulationSession session(0.1);
    QVERIFY(session.loadScenario(scenarioPath));

    const auto initialStates = session.renderStates();
    QCOMPARE(initialStates.size(), std::size_t {1});
    QCOMPARE(initialStates.front().tags.size(), std::size_t {2});
    QVERIFY(std::abs(initialStates.front().maxSpeedMetersPerSecond - 18.0) < 1e-9);
    QVERIFY(std::abs(initialStates.front().maxTurnRateDegreesPerSecond - 120.0) < 1e-9);

    session.step();

    const auto states = session.renderStates();
    QCOMPARE(states.size(), std::size_t {1});
    QCOMPARE(states.front().movementGuidanceMode, std::string("route"));
    QVERIFY(states.front().movementGuidanceTargetName.empty());
    QCOMPARE(states.front().missionExecutionStatus, std::string("navigating"));
    QCOMPARE(states.front().missionExecutionPhase, std::string("patrol-transit"));
    QCOMPARE(states.front().missionPhaseEnteredTick, std::uint64_t {1});
    QCOMPARE(states.front().missionCompletedWaypointVisits, std::size_t {0});
    QCOMPARE(states.front().missionCompletedRouteCycles, std::size_t {0});
    QVERIFY(states.front().missionLastCompletedWaypointName.empty());
    QCOMPARE(states.front().activeRouteWaypointIndex, std::size_t {0});
    QCOMPARE(states.front().activeRouteWaypointName, std::string("gate-west"));
    QVERIFY(!states.front().routeIsLoitering);
    QCOMPARE(states.front().routeWaypoints.size(), std::size_t {2});
}

void SimulationManagerTests::scenarioLoaderParsesJson()
{
    const QByteArray json = R"({
        "name": "Test Scenario",
        "description": "Blue patrol screens the eastern corridor.",
        "environment": {
            "timeOfDay": "dawn",
            "weather": "light-rain",
            "visibilityMeters": 18000.0,
            "wind": {"x": 1.5, "y": -0.5}
        },
        "mapBounds": {
            "min": {"x": -100.0, "y": -50.0},
            "max": {"x": 120.0, "y": 80.0}
        },
        "entities": [
            {
                "identity": {
                    "id": "blue-1",
                    "displayName": "Blue One",
                    "side": "blue",
                    "category": "aircraft",
                    "role": "patrol",
                    "color": "#2E86AB",
                    "tags": ["screen", "lead"]
                },
                "kinematics": {
                    "position": {"x": 10.0, "y": 20.0},
                    "velocity": {"x": 3.0, "y": 4.0},
                    "headingDegrees": 45.0,
                    "maxSpeedMetersPerSecond": 15.5,
                    "route": [
                        {
                            "name": "gate-west",
                            "position": {"x": 8.0, "y": 18.0},
                            "loiterSeconds": 30.0
                        },
                        {
                            "name": "gate-east",
                            "position": {"x": 18.0, "y": 22.0},
                            "loiterSeconds": 45.0
                        }
                    ]
                },
                "sensor": {
                    "type": "radar",
                    "rangeMeters": 15.0,
                    "fieldOfViewDegrees": 120.0,
                    "enabled": true
                },
                "mission": {
                    "objective": "Hold eastern screen line",
                    "behavior": "patrol",
                    "parameters": {
                        "orbitRadiusMeters": 16.0,
                        "orbitClockwise": false,
                        "orbitAcquireToleranceMeters": 0.8,
                        "orbitCompletionToleranceMeters": 1.2,
                        "orbitCompletionHoldSeconds": 4.0,
                        "escortTrailDistanceMeters": 7.0,
                        "escortRightOffsetMeters": -3.0,
                        "escortSlotToleranceMeters": 0.6,
                        "escortCompletionHoldSeconds": 1.5,
                        "interceptCompletionDistanceMeters": 3.4
                    },
                    "constraints": {
                        "timeoutSeconds": 180.0,
                        "activateAfter": {
                            "entityId": "blue-0",
                            "status": "completed"
                        },
                        "replanOnAbort": {
                            "objective": "Fallback orbit",
                            "behavior": "orbit",
                            "targetEntityId": "gold-1",
                            "timeoutSeconds": 45.0,
                            "parameters": {
                                "orbitRadiusMeters": 12.0,
                                "orbitClockwise": true
                            }
                        },
                        "replanChain": [
                            {
                                "objective": "Collapse to escort",
                                "behavior": "escort",
                                "targetEntityId": "blue-0",
                                "timeoutSeconds": 60.0,
                                "parameters": {
                                    "escortTrailDistanceMeters": 9.0,
                                    "escortRightOffsetMeters": 1.5
                                }
                            }
                        ]
                    }
                }
            }
        ]
    })";

    const auto result = fm::app::ScenarioLoader::loadFromJson(json, "inline.json");

    QVERIFY(result.ok());
    QCOMPARE(result.scenario.name, std::string("Test Scenario"));
    QCOMPARE(result.scenario.description, std::string("Blue patrol screens the eastern corridor."));
    QCOMPARE(result.scenario.environment.timeOfDay, std::string("dawn"));
    QCOMPARE(result.scenario.environment.weather, std::string("light-rain"));
    QVERIFY(std::abs(result.scenario.environment.visibilityMeters - 18000.0) < 1e-9);
    QVERIFY(std::abs(result.scenario.mapBounds.minimum.x + 100.0) < 1e-9);
    QCOMPARE(result.scenario.entities.size(), std::size_t {1});
    QCOMPARE(result.scenario.entities.front().identity.id, std::string("blue-1"));
    QCOMPARE(result.scenario.entities.front().identity.side, std::string("blue"));
    QCOMPARE(result.scenario.entities.front().identity.role, std::string("patrol"));
    QVERIFY(std::abs(result.scenario.entities.front().kinematics.position.x - 10.0) < 1e-9);
    QVERIFY(std::abs(result.scenario.entities.front().kinematics.maxSpeedMetersPerSecond - 15.5) < 1e-9);
    QCOMPARE(result.scenario.entities.front().kinematics.route.size(), std::size_t {2});
    QVERIFY(std::abs(result.scenario.entities.front().sensor.rangeMeters - 15.0) < 1e-9);
    QCOMPARE(result.scenario.entities.front().mission.behavior, std::string("patrol"));
    QVERIFY(result.scenario.entities.front().mission.parameters.orbitRadiusMeters.has_value());
    QVERIFY(std::abs(*result.scenario.entities.front().mission.parameters.orbitRadiusMeters - 16.0) < 1e-9);
    QVERIFY(result.scenario.entities.front().mission.parameters.orbitClockwise.has_value());
    QVERIFY(!*result.scenario.entities.front().mission.parameters.orbitClockwise);
    QVERIFY(result.scenario.entities.front().mission.parameters.escortRightOffsetMeters.has_value());
    QVERIFY(std::abs(*result.scenario.entities.front().mission.parameters.escortRightOffsetMeters + 3.0) < 1e-9);
    QVERIFY(result.scenario.entities.front().mission.parameters.interceptCompletionDistanceMeters.has_value());
    QVERIFY(std::abs(*result.scenario.entities.front().mission.parameters.interceptCompletionDistanceMeters - 3.4) < 1e-9);
    QVERIFY(std::abs(result.scenario.entities.front().mission.timeoutSeconds - 180.0) < 1e-9);
    QCOMPARE(result.scenario.entities.front().mission.activation.prerequisiteEntityId, std::string("blue-0"));
    QCOMPARE(result.scenario.entities.front().mission.activation.prerequisiteStatus, std::string("completed"));
    QCOMPARE(result.scenario.entities.front().mission.replanOnAbort.behavior, std::string("orbit"));
    QCOMPARE(result.scenario.entities.front().mission.replanOnAbort.targetEntityId, std::string("gold-1"));
    QVERIFY(std::abs(result.scenario.entities.front().mission.replanOnAbort.timeoutSeconds - 45.0) < 1e-9);
    QVERIFY(result.scenario.entities.front().mission.replanOnAbort.parameters.orbitRadiusMeters.has_value());
    QVERIFY(std::abs(*result.scenario.entities.front().mission.replanOnAbort.parameters.orbitRadiusMeters - 12.0) < 1e-9);
    QCOMPARE(result.scenario.entities.front().mission.replanChain.size(), std::size_t {1});
    QCOMPARE(result.scenario.entities.front().mission.replanChain.front().behavior, std::string("escort"));
    QCOMPARE(result.scenario.entities.front().mission.replanChain.front().targetEntityId, std::string("blue-0"));
    QVERIFY(std::abs(result.scenario.entities.front().mission.replanChain.front().timeoutSeconds - 60.0) < 1e-9);
    QVERIFY(result.scenario.entities.front().mission.replanChain.front().parameters.escortTrailDistanceMeters.has_value());
    QVERIFY(std::abs(*result.scenario.entities.front().mission.replanChain.front().parameters.escortTrailDistanceMeters - 9.0) < 1e-9);
    QCOMPARE(result.scenario.entities.front().kinematics.route.front().name, std::string("gate-west"));
}

void SimulationManagerTests::scenarioLoaderSavesAndReloadsJson()
{
    const QByteArray json = R"({
        "name": "Round Trip Scenario",
        "description": "Save and reload test.",
        "environment": {
            "timeOfDay": "day",
            "weather": "clear",
            "visibilityMeters": 12000.0,
            "wind": {"x": 2.0, "y": 1.0}
        },
        "mapBounds": {
            "min": {"x": -10.0, "y": -5.0},
            "max": {"x": 20.0, "y": 15.0}
        },
        "entities": [
            {
                "identity": {
                    "id": "blue-1",
                    "displayName": "Blue One",
                    "side": "blue",
                    "category": "aircraft",
                    "role": "escort",
                    "color": "#2E86AB",
                    "tags": ["lead", "screen"]
                },
                "kinematics": {
                    "position": {"x": 1.0, "y": 2.0},
                    "velocity": {"x": 3.0, "y": 0.0},
                    "headingDegrees": 0.0,
                    "maxSpeedMetersPerSecond": 6.0,
                    "route": [
                        {
                            "name": "wp-1",
                            "position": {"x": 2.0, "y": 2.0},
                            "loiterSeconds": 1.0
                        }
                    ]
                },
                "sensor": {
                    "type": "radar",
                    "rangeMeters": 18.0,
                    "fieldOfViewDegrees": 120.0,
                    "enabled": true
                },
                "mission": {
                    "objective": "Escort lead",
                    "behavior": "escort",
                    "targetEntityId": "blue-2",
                    "parameters": {
                        "escortTrailDistanceMeters": 8.0,
                        "escortRightOffsetMeters": 2.0
                    },
                    "constraints": {
                        "timeoutSeconds": 20.0
                    }
                }
            }
        ]
    })";

    const auto loadResult = fm::app::ScenarioLoader::loadFromJson(json, "round-trip-inline.json");
    QVERIFY(loadResult.ok());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString errorMessage;
    const QString savePath = tempDir.filePath("round-trip.json");
    QVERIFY(fm::app::ScenarioLoader::saveToFile(loadResult.scenario, savePath, errorMessage));
    QVERIFY(errorMessage.isEmpty());

    const auto reloadResult = fm::app::ScenarioLoader::loadFromFile(savePath);
    QVERIFY(reloadResult.ok());
    QCOMPARE(reloadResult.scenario.name, std::string("Round Trip Scenario"));
    QCOMPARE(reloadResult.scenario.entities.size(), std::size_t {1});
    QCOMPARE(reloadResult.scenario.entities.front().mission.behavior, std::string("escort"));
    QCOMPARE(reloadResult.scenario.entities.front().mission.targetEntityId, std::string("blue-2"));
    QVERIFY(reloadResult.scenario.entities.front().mission.parameters.escortTrailDistanceMeters.has_value());
    QVERIFY(std::abs(*reloadResult.scenario.entities.front().mission.parameters.escortTrailDistanceMeters - 8.0) < 1e-9);
    QVERIFY(std::abs(reloadResult.scenario.entities.front().mission.timeoutSeconds - 20.0) < 1e-9);
}

void SimulationManagerTests::scenarioLoaderSupportsLegacyFlatEntityFields()
{
    const QByteArray json = R"({
        "name": "Legacy Scenario",
        "entities": [
            {
                "id": "legacy-1",
                "displayName": "Legacy One",
                "color": "#112233",
                "position": {"x": 4.0, "y": -2.0},
                "velocity": {"x": 1.0, "y": 3.0},
                "headingDegrees": 90.0,
                "sensorRangeMeters": 25.0
            }
        ]
    })";

    const auto result = fm::app::ScenarioLoader::loadFromJson(json, "legacy.json");

    QVERIFY(result.ok());
    QCOMPARE(result.scenario.entities.front().identity.id, std::string("legacy-1"));
    QCOMPARE(result.scenario.entities.front().identity.displayName, std::string("Legacy One"));
    QCOMPARE(result.scenario.entities.front().identity.colorHex, std::string("#112233"));
    QVERIFY(std::abs(result.scenario.entities.front().kinematics.position.y + 2.0) < 1e-9);
    QVERIFY(std::abs(result.scenario.entities.front().sensor.rangeMeters - 25.0) < 1e-9);

    const QByteArray legacyRouteJson = R"({
        "name": "Legacy Route Scenario",
        "entities": [
            {
                "id": "legacy-route",
                "position": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 1.0, "y": 0.0},
                "mission": {
                    "behavior": "patrol",
                    "patrolRoute": [
                        {
                            "name": "legacy-wp",
                            "position": {"x": 2.0, "y": 0.0},
                            "loiterSeconds": 5.0
                        }
                    ]
                }
            }
        ]
    })";

    const auto legacyRouteResult = fm::app::ScenarioLoader::loadFromJson(legacyRouteJson, "legacy-route.json");
    QVERIFY(legacyRouteResult.ok());
    QCOMPARE(legacyRouteResult.scenario.entities.front().kinematics.route.size(), std::size_t {1});
    QCOMPARE(legacyRouteResult.scenario.entities.front().kinematics.route.front().name, std::string("legacy-wp"));

    const QByteArray invalidBehaviorJson = R"({
        "name": "Invalid Behavior Scenario",
        "entities": [
            {
                "id": "bad-1",
                "position": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 1.0, "y": 0.0},
                "mission": {
                    "behavior": "unknown-mission"
                }
            }
        ]
    })";

    const auto invalidBehaviorResult = fm::app::ScenarioLoader::loadFromJson(invalidBehaviorJson, "invalid-behavior.json");
    QVERIFY(!invalidBehaviorResult.ok());
    QVERIFY(invalidBehaviorResult.errorMessage.contains("unsupported value"));
}

void SimulationManagerTests::scenarioLoaderRejectsDuplicateEntityIds()
{
    const QByteArray json = R"({
        "name": "Duplicate Test",
        "entities": [
            {
                "id": "dup-1",
                "position": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 1.0, "y": 0.0}
            },
            {
                "id": "dup-1",
                "position": {"x": 5.0, "y": 0.0},
                "velocity": {"x": 1.0, "y": 0.0}
            }
        ]
    })";

    const auto result = fm::app::ScenarioLoader::loadFromJson(json, "duplicate.json");

    QVERIFY(!result.ok());
    QVERIFY(result.errorMessage.contains("duplicate entity id"));
}

void SimulationManagerTests::simulationSessionTracksTrajectoryDuringStep()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile scenarioFile(tempDir.path() + "/scenario.json");
    QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
    scenarioFile.write(R"({
        "name": "Session Test",
        "entities": [
            {
                "id": "track-1",
                "displayName": "Track One",
                "position": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 2.0, "y": 0.0},
                "headingDegrees": 0.0
            }
        ]
    })");
    scenarioFile.close();

    fm::app::SimulationSession session(0.5);
    QVERIFY(session.loadScenario(scenarioFile.fileName()));

    const auto beforeStep = session.trajectoryForEntity("track-1");
    QCOMPARE(beforeStep.size(), std::size_t {1});

    session.step();

    const auto afterStep = session.trajectoryForEntity("track-1");
    QCOMPARE(afterStep.size(), std::size_t {2});
    QVERIFY(std::abs(afterStep.back().x - 1.0) < 1e-9);
}

void SimulationManagerTests::simulationSessionUpdatesEntityMissionDefinition()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile scenarioFile(tempDir.path() + "/mission-edit.json");
    QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
    scenarioFile.write(R"({
        "name": "Mission Edit Test",
        "entities": [
            {
                "identity": {
                    "id": "blue-1",
                    "displayName": "Blue One"
                },
                "kinematics": {
                    "position": {"x": 0.0, "y": 0.0},
                    "velocity": {"x": 2.0, "y": 0.0},
                    "headingDegrees": 0.0,
                    "route": [
                        {
                            "name": "wp-1",
                            "position": {"x": 2.0, "y": 0.0},
                            "loiterSeconds": 0.0
                        },
                        {
                            "name": "wp-2",
                            "position": {"x": 4.0, "y": 0.0},
                            "loiterSeconds": 0.0
                        }
                    ]
                },
                "mission": {
                    "objective": "Hold route",
                    "behavior": "patrol",
                    "constraints": {
                        "timeoutSeconds": 12.0
                    }
                }
            }
        ]
    })");
    scenarioFile.close();

    fm::app::SimulationSession session(1.0);
    QVERIFY(session.loadScenario(scenarioFile.fileName()));

    fm::app::EntityMissionDefinition updatedMission = session.scenarioDefinition()->entities.front().mission;
    updatedMission.objective = "Transit to gate";
    updatedMission.behavior = "transit";
    updatedMission.parameters.orbitRadiusMeters = 15.0;

    QVERIFY(session.updateEntityMissionDefinition("blue-1", updatedMission));
    QVERIFY(session.scenarioDefinition() != nullptr);
    QCOMPARE(session.scenarioDefinition()->entities.front().mission.behavior, std::string("transit"));
    QCOMPARE(session.scenarioDefinition()->entities.front().mission.objective, std::string("Transit to gate"));
    QVERIFY(std::abs(session.scenarioDefinition()->entities.front().mission.timeoutSeconds - 12.0) < 1e-9);

    const auto states = session.renderStates();
    QCOMPARE(states.size(), std::size_t {1});
    QCOMPARE(states.front().missionBehavior, std::string("transit"));
}

void SimulationManagerTests::simulationSessionFollowsRouteWaypoints()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile scenarioFile(tempDir.path() + "/patrol.json");
    QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
    scenarioFile.write(R"({
        "name": "Patrol Route Test",
        "entities": [
            {
                "identity": {
                    "id": "patrol-1",
                    "displayName": "Patrol One"
                },
                "kinematics": {
                    "position": {"x": 0.0, "y": 0.0},
                    "velocity": {"x": 1.0, "y": 0.0},
                    "headingDegrees": 0.0,
                    "maxSpeedMetersPerSecond": 1.0,
                    "maxTurnRateDegreesPerSecond": 90.0,
                    "route": [
                        {
                            "name": "wp-1",
                            "position": {"x": 1.0, "y": 0.0},
                            "loiterSeconds": 0.0
                        },
                        {
                            "name": "wp-2",
                            "position": {"x": 1.0, "y": 1.0},
                            "loiterSeconds": 0.0
                        }
                    ]
                },
                "mission": {
                    "behavior": "patrol"
                }
            }
        ]
    })");
    scenarioFile.close();

    fm::app::SimulationSession session(1.0);
    QVERIFY(session.loadScenario(scenarioFile.fileName()));

    session.step();
    auto states = session.renderStates();
    QCOMPARE(states.size(), std::size_t {1});
    QVERIFY(states.front().position.x > 0.5);
    QVERIFY(states.front().position.y >= 0.0);
    const auto firstStepY = states.front().position.y;

    session.step();
    states = session.renderStates();
    QVERIFY(states.front().position.y > firstStepY);
}

void SimulationManagerTests::simulationSessionUpdatesFixedTimeStep()
{
    fm::app::SimulationSession session(0.01);

    QVERIFY(session.setFixedTimeStepSeconds(0.2));
    QVERIFY(std::abs(session.fixedTimeStepSeconds() - 0.2) < 1e-9);
    QVERIFY(!session.setFixedTimeStepSeconds(0.0));
    QVERIFY(std::abs(session.fixedTimeStepSeconds() - 0.2) < 1e-9);
}

void SimulationManagerTests::simulationSessionRecordsEventLog()
{
    fm::app::SimulationSession session(0.01);

    session.start();
    QVERIFY(!session.eventLog().empty());
    QVERIFY(session.eventLog().back().contains(QStringLiteral("尚未加载场景")));

    session.setFixedTimeStepSeconds(0.1);
    QVERIFY(session.eventLog().back().contains(QStringLiteral("固定时间步已更新")));
}

void SimulationManagerTests::simulationSessionLogsMissionTransitions()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile scenarioFile(tempDir.path() + "/mission-log.json");
    QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
    scenarioFile.write(R"({
        "name": "Mission Log Test",
        "entities": [
            {
                "identity": {
                    "id": "patrol-1",
                    "displayName": "Patrol One"
                },
                "kinematics": {
                    "position": {"x": 0.0, "y": 0.0},
                    "velocity": {"x": 1.0, "y": 0.0},
                    "headingDegrees": 0.0,
                    "maxSpeedMetersPerSecond": 1.0,
                    "maxTurnRateDegreesPerSecond": 180.0,
                    "route": [
                        {
                            "name": "gate-1",
                            "position": {"x": 1.0, "y": 0.0},
                            "loiterSeconds": 1.0
                        }
                    ]
                },
                "mission": {
                    "objective": "Hold gate",
                    "behavior": "patrol"
                }
            }
        ]
    })");
    scenarioFile.close();

    fm::app::SimulationSession session(1.0);
    QVERIFY(session.loadScenario(scenarioFile.fileName()));

    session.step();

    QStringList logLines;
    for (const auto& entry : session.eventLog()) {
        logLines.push_back(entry);
    }

    const QString joinedLog = logLines.join(QStringLiteral("\n"));
    QVERIFY(joinedLog.contains(QStringLiteral("任务阶段切换: Patrol One -> loitering / patrol-hold")));
    QVERIFY(joinedLog.contains(QStringLiteral("航路点到达: Patrol One -> gate-1 (累计 1 次)")));
}

void SimulationManagerTests::simulationSessionLogsMissionConstraintEvents()
{
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QFile scenarioFile(tempDir.path() + "/constraint-activation-log.json");
        QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
        scenarioFile.write(R"({
            "name": "Constraint Activation Log Test",
            "entities": [
                {
                    "identity": {
                        "id": "lead",
                        "displayName": "Lead"
                    },
                    "kinematics": {
                        "position": {"x": 0.0, "y": 0.0},
                        "velocity": {"x": 4.0, "y": 0.0},
                        "headingDegrees": 0.0,
                        "maxSpeedMetersPerSecond": 4.0,
                        "maxTurnRateDegreesPerSecond": 180.0
                    },
                    "sensor": {
                        "rangeMeters": 10.0,
                        "enabled": true
                    },
                    "mission": {
                        "objective": "Intercept intruder",
                        "behavior": "intercept",
                        "targetEntityId": "intruder"
                    }
                },
                {
                    "identity": {
                        "id": "intruder",
                        "displayName": "Intruder"
                    },
                    "kinematics": {
                        "position": {"x": 1.0, "y": 0.0},
                        "velocity": {"x": 0.0, "y": 0.0},
                        "headingDegrees": 180.0
                    }
                },
                {
                    "identity": {
                        "id": "support",
                        "displayName": "Support"
                    },
                    "kinematics": {
                        "position": {"x": -5.0, "y": 0.0},
                        "velocity": {"x": 0.0, "y": 0.0},
                        "headingDegrees": 0.0,
                        "maxSpeedMetersPerSecond": 2.0,
                        "maxTurnRateDegreesPerSecond": 180.0
                    },
                    "mission": {
                        "objective": "Wait for lead",
                        "behavior": "intercept",
                        "targetEntityId": "lead",
                        "constraints": {
                            "activateAfter": {
                                "entityId": "lead",
                                "status": "completed"
                            }
                        }
                    }
                }
            ]
        })");
        scenarioFile.close();

        fm::app::SimulationSession session(1.0);
        QVERIFY(session.loadScenario(scenarioFile.fileName()));

        session.step();
        session.step();
        session.step();
        session.step();

        QStringList logLines;
        for (const auto& entry : session.eventLog()) {
            logLines.push_back(entry);
        }

        const QString joinedLog = logLines.join(QStringLiteral("\n"));
        QVERIFY(joinedLog.contains(QStringLiteral("任务阶段切换: Support -> idle / awaiting-prerequisite")));
    }

    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QFile scenarioFile(tempDir.path() + "/constraint-replan-log.json");
        QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
        scenarioFile.write(R"({
            "name": "Constraint Replan Log Test",
            "entities": [
                {
                    "identity": {
                        "id": "escort-1",
                        "displayName": "Escort One"
                    },
                    "kinematics": {
                        "position": {"x": 0.0, "y": 0.0},
                        "velocity": {"x": 2.0, "y": 0.0},
                        "headingDegrees": 0.0,
                        "maxSpeedMetersPerSecond": 2.0,
                        "maxTurnRateDegreesPerSecond": 180.0,
                        "route": [
                            {
                                "name": "fallback-a",
                                "position": {"x": 0.0, "y": 0.0},
                                "loiterSeconds": 0.0
                            },
                            {
                                "name": "fallback-b",
                                "position": {"x": 3.0, "y": 0.0},
                                "loiterSeconds": 0.0
                            }
                        ]
                    },
                    "mission": {
                        "objective": "Timed patrol",
                        "behavior": "patrol",
                        "constraints": {
                            "timeoutSeconds": 1.0,
                            "replanOnAbort": {
                                "objective": "Fallback orbit",
                                "behavior": "orbit"
                            }
                        }
                    }
                }
            ]
        })");
        scenarioFile.close();

        fm::app::SimulationSession session(1.0);
        QVERIFY(session.loadScenario(scenarioFile.fileName()));

        session.step();
        session.step();

        QStringList logLines;
        for (const auto& entry : session.eventLog()) {
            logLines.push_back(entry);
        }

        const QString joinedLog = logLines.join(QStringLiteral("\n"));
        QVERIFY(joinedLog.contains(QStringLiteral("任务重规划: Escort One <- timeout-expired / patrol -> orbit")));
    }
}

void SimulationManagerTests::simulationSessionLogsGuidanceTransitions()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile scenarioFile(tempDir.path() + "/guidance-log.json");
    QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
    scenarioFile.write(R"({
        "name": "Guidance Log Test",
        "entities": [
            {
                "identity": {
                    "id": "blue-1",
                    "displayName": "Blue One"
                },
                "kinematics": {
                    "position": {"x": 0.0, "y": 0.0},
                    "velocity": {"x": 6.0, "y": 0.0},
                    "headingDegrees": 0.0,
                    "maxSpeedMetersPerSecond": 6.0,
                    "maxTurnRateDegreesPerSecond": 180.0
                },
                "mission": {
                    "objective": "Intercept intruder",
                    "behavior": "intercept",
                    "targetEntityId": "green-1"
                }
            },
            {
                "identity": {
                    "id": "green-1",
                    "displayName": "Green Intruder"
                },
                "kinematics": {
                    "position": {"x": 0.0, "y": 20.0},
                    "velocity": {"x": 0.0, "y": 0.0},
                    "headingDegrees": 180.0
                }
            }
        ]
    })");
    scenarioFile.close();

    fm::app::SimulationSession session(1.0);
    QVERIFY(session.loadScenario(scenarioFile.fileName()));

    session.step();

    QStringList logLines;
    for (const auto& entry : session.eventLog()) {
        logLines.push_back(entry);
    }

    const QString joinedLog = logLines.join(QStringLiteral("\n"));
    QVERIFY(joinedLog.contains(QStringLiteral("任务阶段切换: Blue One -> navigating / intercept-transit")));
    QVERIFY(joinedLog.contains(QStringLiteral("机动引导切换: Blue One -> mission-target (Green Intruder)")));
}

void SimulationManagerTests::simulationSessionLogsMissionTerminalTransitions()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile scenarioFile(tempDir.path() + "/terminal-log.json");
    QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
    scenarioFile.write(R"({
        "name": "Terminal Log Test",
        "entities": [
            {
                "identity": {
                    "id": "blue-1",
                    "displayName": "Blue One"
                },
                "kinematics": {
                    "position": {"x": 0.0, "y": 0.0},
                    "velocity": {"x": 4.0, "y": 0.0},
                    "headingDegrees": 0.0,
                    "maxSpeedMetersPerSecond": 4.0,
                    "maxTurnRateDegreesPerSecond": 180.0
                },
                "sensor": {
                    "rangeMeters": 10.0,
                    "enabled": true
                },
                "mission": {
                    "objective": "Intercept intruder",
                    "behavior": "intercept",
                    "targetEntityId": "green-1"
                }
            },
            {
                "identity": {
                    "id": "green-1",
                    "displayName": "Green Intruder"
                },
                "kinematics": {
                    "position": {"x": 1.0, "y": 0.0},
                    "velocity": {"x": 0.0, "y": 0.0},
                    "headingDegrees": 180.0
                }
            }
        ]
    })");
    scenarioFile.close();

    fm::app::SimulationSession session(0.5);
    QVERIFY(session.loadScenario(scenarioFile.fileName()));

    session.step();
    session.step();

    QStringList logLines;
    for (const auto& entry : session.eventLog()) {
        logLines.push_back(entry);
    }

    const QString joinedLog = logLines.join(QStringLiteral("\n"));
    QVERIFY(joinedLog.contains(QStringLiteral("任务阶段切换: Blue One -> completed / intercept-complete")));
    QVERIFY(joinedLog.contains(QStringLiteral("任务终态: Blue One -> completed (target-contained)")));
}

void SimulationManagerTests::simulationSessionRenderStateIncludesTerminalStatus()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile scenarioFile(tempDir.path() + "/terminal-render.json");
    QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
    scenarioFile.write(R"({
        "name": "Terminal Render Test",
        "entities": [
            {
                "identity": {
                    "id": "blue-1",
                    "displayName": "Blue One"
                },
                "kinematics": {
                    "position": {"x": 0.0, "y": 0.0},
                    "velocity": {"x": 4.0, "y": 0.0},
                    "headingDegrees": 0.0,
                    "maxSpeedMetersPerSecond": 4.0,
                    "maxTurnRateDegreesPerSecond": 180.0
                },
                "sensor": {
                    "rangeMeters": 10.0,
                    "enabled": true
                },
                "mission": {
                    "objective": "Intercept intruder",
                    "behavior": "intercept",
                    "targetEntityId": "green-1"
                }
            },
            {
                "identity": {
                    "id": "green-1",
                    "displayName": "Green Intruder"
                },
                "kinematics": {
                    "position": {"x": 1.0, "y": 0.0},
                    "velocity": {"x": 0.0, "y": 0.0},
                    "headingDegrees": 180.0
                }
            }
        ]
    })");
    scenarioFile.close();

    fm::app::SimulationSession session(0.5);
    QVERIFY(session.loadScenario(scenarioFile.fileName()));

    session.step();
    session.step();

    const auto states = session.renderStates();
    QCOMPARE(states.size(), std::size_t {2});
    QCOMPARE(states.front().missionExecutionStatus, std::string("completed"));
    QCOMPARE(states.front().missionExecutionPhase, std::string("intercept-complete"));
    QCOMPARE(states.front().missionTerminalReason, std::string("target-contained"));
}

void SimulationManagerTests::sensorComponentDetectsNearbyTargets()
{
    auto sensorEntity = std::make_shared<fm::core::Entity>("sensor", "Sensor");
    sensorEntity->setPosition({0.0, 0.0});
    sensorEntity->setSide("blue");
    auto& sensor = sensorEntity->addComponent<fm::core::SensorComponent>(10.0);

    auto nearbyEntity = std::make_shared<fm::core::Entity>("near", "Near");
    nearbyEntity->setPosition({6.0, 8.0});
    nearbyEntity->setSide("green");

    auto nearbyFriendlyEntity = std::make_shared<fm::core::Entity>("friend", "Friend");
    nearbyFriendlyEntity->setPosition({3.0, 4.0});
    nearbyFriendlyEntity->setSide("blue");

    auto distantEntity = std::make_shared<fm::core::Entity>("far", "Far");
    distantEntity->setPosition({20.0, 0.0});
    distantEntity->setSide("green");

    fm::core::SimulationManager manager(0.1);
    manager.addEntity(sensorEntity);
    manager.addEntity(nearbyEntity);
    manager.addEntity(nearbyFriendlyEntity);
    manager.addEntity(distantEntity);

    manager.advanceOneTick();

    QCOMPARE(sensor.detectedTargetIds().size(), std::size_t {1});
    QCOMPARE(sensor.detectedTargetIds().front(), std::string("near"));
}

void SimulationManagerTests::sensorComponentRespectsFieldOfView()
{
    auto sensorEntity = std::make_shared<fm::core::Entity>("sensor", "Sensor");
    sensorEntity->setPosition({0.0, 0.0});
    sensorEntity->setHeadingDegrees(0.0);
    sensorEntity->setSide("blue");
    auto& sensor = sensorEntity->addComponent<fm::core::SensorComponent>(10.0, 90.0);

    auto forwardEntity = std::make_shared<fm::core::Entity>("ahead", "Ahead");
    forwardEntity->setPosition({6.0, 0.0});
    forwardEntity->setSide("green");

    auto flankEntity = std::make_shared<fm::core::Entity>("flank", "Flank");
    flankEntity->setPosition({0.0, 6.0});
    flankEntity->setSide("green");

    fm::core::SimulationManager manager(0.1);
    manager.addEntity(sensorEntity);
    manager.addEntity(forwardEntity);
    manager.addEntity(flankEntity);

    manager.advanceOneTick();

    QCOMPARE(sensor.detectedTargetIds().size(), std::size_t {1});
    QCOMPARE(sensor.detectedTargetIds().front(), std::string("ahead"));
}

void SimulationManagerTests::simulationSessionRecordsSnapshots()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile scenarioFile(tempDir.path() + "/recording.json");
    QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
    scenarioFile.write(R"({
        "name": "Recording Test",
        "entities": [
            {
                "id": "rec-1",
                "displayName": "Recorder",
                "position": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 4.0, "y": 0.0},
                "headingDegrees": 0.0,
                "sensorRangeMeters": 10.0
            },
            {
                "id": "rec-2",
                "displayName": "Target",
                "position": {"x": 6.0, "y": 0.0},
                "velocity": {"x": 0.0, "y": 0.0},
                "headingDegrees": 180.0
            }
        ]
    })");
    scenarioFile.close();

    fm::app::SimulationSession session(0.5);
    QVERIFY(session.loadScenario(scenarioFile.fileName()));
    QCOMPARE(session.recording().size(), std::size_t {1});

    session.step();
    QCOMPARE(session.recording().size(), std::size_t {2});

    const auto* latestSnapshot = session.latestSnapshot();
    QVERIFY(latestSnapshot != nullptr);
    QCOMPARE(latestSnapshot->tickCount, std::uint64_t {1});
    QVERIFY(std::abs(latestSnapshot->elapsedSimulationSeconds - 0.5) < 1e-9);
    QCOMPARE(latestSnapshot->entities.size(), std::size_t {2});
    QCOMPARE(latestSnapshot->entities.front().id, std::string("rec-1"));
}

void SimulationManagerTests::simulationSessionProvidesSnapshotViews()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile scenarioFile(tempDir.path() + "/replay.json");
    QVERIFY(scenarioFile.open(QIODevice::WriteOnly));
    scenarioFile.write(R"({
        "name": "Replay Test",
        "entities": [
            {
                "identity": {
                    "id": "viewer",
                    "displayName": "Viewer",
                    "side": "blue",
                    "category": "aircraft",
                    "role": "interceptor",
                    "color": "#2E86AB"
                },
                "kinematics": {
                    "position": {"x": 0.0, "y": 0.0},
                    "velocity": {"x": 2.0, "y": 0.0},
                    "headingDegrees": 0.0,
                    "route": [
                        {
                            "name": "intercept-gate",
                            "position": {"x": 1.0, "y": 2.0},
                            "loiterSeconds": 10.0
                        }
                    ]
                },
                "sensor": {
                    "type": "radar",
                    "rangeMeters": 8.0
                },
                "mission": {
                    "objective": "Track intruder",
                    "behavior": "intercept",
                    "targetEntityId": "target"
                }
            },
            {
                "identity": {
                    "id": "target",
                    "displayName": "Target",
                    "side": "green",
                    "category": "aircraft",
                    "role": "intruder"
                },
                "kinematics": {
                    "position": {"x": 4.0, "y": 0.0},
                    "velocity": {"x": 0.0, "y": 0.0},
                    "headingDegrees": 180.0
                }
            }
        ]
    })");
    scenarioFile.close();

    fm::app::SimulationSession session(0.5);
    QVERIFY(session.loadScenario(scenarioFile.fileName()));
    session.step();

    const auto* snapshot0 = session.snapshotAt(0);
    const auto* snapshot1 = session.snapshotAt(1);
    QVERIFY(snapshot0 != nullptr);
    QVERIFY(snapshot1 != nullptr);
    QCOMPARE(snapshot0->tickCount, std::uint64_t {0});
    QCOMPARE(snapshot1->tickCount, std::uint64_t {1});

    const auto states0 = session.renderStatesAt(0);
    const auto states1 = session.renderStatesAt(1);
    QCOMPARE(states0.size(), std::size_t {2});
    QCOMPARE(states1.size(), std::size_t {2});
    QVERIFY(std::abs(states0.front().position.x - 0.0) < 1e-9);
    QVERIFY(states1.front().position.x > 0.2);
    QVERIFY(states1.front().position.x < 1.0);
    QVERIFY(states1.front().position.y > 0.4);
    QCOMPARE(states0.front().displayName, std::string("Viewer"));
    QCOMPARE(states0.front().side, std::string("blue"));
    QCOMPARE(states0.front().role, std::string("interceptor"));
    QCOMPARE(states0.front().missionObjective, std::string("Track intruder"));
    QCOMPARE(states0.front().missionBehavior, std::string("intercept"));
    QCOMPARE(states0.front().missionTargetEntityId, std::string("target"));
    QCOMPARE(states0.front().routeWaypoints.size(), std::size_t {1});
    QCOMPARE(states0.front().routeWaypoints.front().name, std::string("intercept-gate"));
    QVERIFY(states0.front().detectedTargetIds.empty());
    QVERIFY(!states1.front().detectedTargetIds.empty());
}

QTEST_APPLESS_MAIN(SimulationManagerTests)

#include "SimulationManagerTests.moc"