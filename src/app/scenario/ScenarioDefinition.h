#pragma once

#include <optional>
#include <string>
#include <vector>

#include "core/math/Vector2.h"

namespace fm::app {

struct ScenarioEnvironmentDefinition {
    std::string timeOfDay;
    std::string weather;
    double visibilityMeters {0.0};
    fm::core::Vector2 wind;
};

struct ScenarioMapBoundsDefinition {
    fm::core::Vector2 minimum;
    fm::core::Vector2 maximum;
};

struct EntityIdentityDefinition {
    std::string id;
    std::string displayName;
    std::string side;
    std::string category;
    std::string role;
    std::string colorHex;
    std::vector<std::string> tags;
};

struct EntityKinematicsDefinition {
    struct RouteWaypointDefinition {
        std::string name;
        fm::core::Vector2 position;
        double loiterSeconds {0.0};
    };

    fm::core::Vector2 position;
    fm::core::Vector2 velocity;
    double headingDegrees {0.0};
    double maxSpeedMetersPerSecond {0.0};
    double maxTurnRateDegreesPerSecond {180.0};
    std::vector<RouteWaypointDefinition> route;
};

struct EntitySensorDefinition {
    std::string type;
    double rangeMeters {0.0};
    double fieldOfViewDegrees {360.0};
    bool enabled {true};
};

struct EntityMissionDefinition {
    struct TaskParametersDefinition {
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

    struct ActivationConstraintDefinition {
        std::string prerequisiteEntityId;
        std::string prerequisiteStatus {"completed"};
    };

    struct ReplanDefinition {
        std::string objective;
        std::string behavior;
        std::string targetEntityId;
        double timeoutSeconds {0.0};
        TaskParametersDefinition parameters;
    };

    std::string objective;
    std::string behavior;
    std::string targetEntityId;
    TaskParametersDefinition parameters;
    double timeoutSeconds {0.0};
    ActivationConstraintDefinition activation;
    ReplanDefinition replanOnAbort;
    std::vector<ReplanDefinition> replanChain;
};

struct EntityDefinition {
    EntityIdentityDefinition identity;
    EntityKinematicsDefinition kinematics;
    EntitySensorDefinition sensor;
    EntityMissionDefinition mission;
};

struct ScenarioDefinition {
    std::string name;
    std::string description;
    ScenarioEnvironmentDefinition environment;
    ScenarioMapBoundsDefinition mapBounds;
    std::vector<EntityDefinition> entities;
};

}  // namespace fm::app