#include "app/scenario/ScenarioLoader.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <unordered_set>

namespace {

bool isSupportedMissionBehavior(const QString& value)
{
    return value == QStringLiteral("patrol")
        || value == QStringLiteral("intercept")
        || value == QStringLiteral("escort")
        || value == QStringLiteral("orbit")
        || value == QStringLiteral("transit");
}

bool ensureObjectField(const QJsonObject& object, const QString& fieldName, QString& errorMessage)
{
    if (object.contains(fieldName) && !object.value(fieldName).isObject()) {
        errorMessage = QString("Field '%1' must be an object.").arg(fieldName);
        return false;
    }

    return true;
}

fm::core::Vector2 parseVector2(const QJsonObject& object, const QString& fieldName, QString& errorMessage)
{
    const auto value = object.value(fieldName);
    if (!value.isObject()) {
        errorMessage = QString("Field '%1' must be an object with x/y values.").arg(fieldName);
        return {};
    }

    const auto vectorObject = value.toObject();
    if (!vectorObject.value("x").isDouble() || !vectorObject.value("y").isDouble()) {
        errorMessage = QString("Field '%1' must contain numeric x/y values.").arg(fieldName);
        return {};
    }

    return {vectorObject.value("x").toDouble(), vectorObject.value("y").toDouble()};
}

bool parseOptionalStringArray(const QJsonObject& object, const QString& fieldName, std::vector<std::string>& values, QString& errorMessage)
{
    if (!object.contains(fieldName)) {
        return true;
    }

    if (!object.value(fieldName).isArray()) {
        errorMessage = QString("Field '%1' must be an array of strings.").arg(fieldName);
        return false;
    }

    const auto array = object.value(fieldName).toArray();
    values.clear();
    values.reserve(static_cast<std::size_t>(array.size()));
    for (qsizetype index = 0; index < array.size(); ++index) {
        if (!array.at(index).isString()) {
            errorMessage = QString("Field '%1' must contain only string values.").arg(fieldName);
            return false;
        }

        values.push_back(array.at(index).toString().toStdString());
    }

    return true;
}

bool parseOptionalNonNegativeDouble(const QJsonObject& object,
                                    const QString& fieldName,
                                    double defaultValue,
                                    double& target,
                                    QString& errorMessage)
{
    if (!object.contains(fieldName)) {
        target = defaultValue;
        return true;
    }

    if (!object.value(fieldName).isDouble()) {
        errorMessage = QString("Field '%1' must be numeric.").arg(fieldName);
        return false;
    }

    target = object.value(fieldName).toDouble(defaultValue);
    if (target < 0.0) {
        errorMessage = QString("Field '%1' must be non-negative.").arg(fieldName);
        return false;
    }

    return true;
}

bool parseOptionalDouble(const QJsonObject& object,
                         const QString& fieldName,
                         double defaultValue,
                         double& target,
                         QString& errorMessage)
{
    if (!object.contains(fieldName)) {
        target = defaultValue;
        return true;
    }

    if (!object.value(fieldName).isDouble()) {
        errorMessage = QString("Field '%1' must be numeric.").arg(fieldName);
        return false;
    }

    target = object.value(fieldName).toDouble(defaultValue);
    return true;
}

bool parseOptionalSignedDouble(const QJsonObject& object,
                               const QString& fieldName,
                               std::optional<double>& target,
                               QString& errorMessage)
{
    if (!object.contains(fieldName)) {
        target.reset();
        return true;
    }

    if (!object.value(fieldName).isDouble()) {
        errorMessage = QString("Field '%1' must be numeric.").arg(fieldName);
        return false;
    }

    target = object.value(fieldName).toDouble();
    return true;
}

bool parseOptionalNonNegativeOptionalDouble(const QJsonObject& object,
                                            const QString& fieldName,
                                            std::optional<double>& target,
                                            QString& errorMessage)
{
    if (!parseOptionalSignedDouble(object, fieldName, target, errorMessage)) {
        return false;
    }

    if (target.has_value() && *target < 0.0) {
        errorMessage = QString("Field '%1' must be non-negative.").arg(fieldName);
        return false;
    }

    return true;
}

bool parseOptionalOptionalBool(const QJsonObject& object,
                               const QString& fieldName,
                               std::optional<bool>& target,
                               QString& errorMessage)
{
    if (!object.contains(fieldName)) {
        target.reset();
        return true;
    }

    if (!object.value(fieldName).isBool()) {
        errorMessage = QString("Field '%1' must be boolean.").arg(fieldName);
        return false;
    }

    target = object.value(fieldName).toBool();
    return true;
}

bool parseTaskParameters(const QJsonObject& parametersObject,
                         const QString& fieldPath,
                         fm::app::EntityMissionDefinition::TaskParametersDefinition& target,
                         QString& errorMessage)
{
    if (!parseOptionalNonNegativeOptionalDouble(parametersObject,
                                                "interceptCompletionDistanceMeters",
                                                target.interceptCompletionDistanceMeters,
                                                errorMessage)) {
        errorMessage = QString("field '%1.interceptCompletionDistanceMeters': %2").arg(fieldPath, errorMessage);
        return false;
    }

    if (!parseOptionalNonNegativeOptionalDouble(parametersObject,
                                                "orbitRadiusMeters",
                                                target.orbitRadiusMeters,
                                                errorMessage)) {
        errorMessage = QString("field '%1.orbitRadiusMeters': %2").arg(fieldPath, errorMessage);
        return false;
    }

    if (!parseOptionalOptionalBool(parametersObject, "orbitClockwise", target.orbitClockwise, errorMessage)) {
        errorMessage = QString("field '%1.orbitClockwise': %2").arg(fieldPath, errorMessage);
        return false;
    }

    if (!parseOptionalNonNegativeOptionalDouble(parametersObject,
                                                "orbitAcquireToleranceMeters",
                                                target.orbitAcquireToleranceMeters,
                                                errorMessage)) {
        errorMessage = QString("field '%1.orbitAcquireToleranceMeters': %2").arg(fieldPath, errorMessage);
        return false;
    }

    if (!parseOptionalNonNegativeOptionalDouble(parametersObject,
                                                "orbitCompletionToleranceMeters",
                                                target.orbitCompletionToleranceMeters,
                                                errorMessage)) {
        errorMessage = QString("field '%1.orbitCompletionToleranceMeters': %2").arg(fieldPath, errorMessage);
        return false;
    }

    if (!parseOptionalNonNegativeOptionalDouble(parametersObject,
                                                "orbitCompletionHoldSeconds",
                                                target.orbitCompletionHoldSeconds,
                                                errorMessage)) {
        errorMessage = QString("field '%1.orbitCompletionHoldSeconds': %2").arg(fieldPath, errorMessage);
        return false;
    }

    if (!parseOptionalNonNegativeOptionalDouble(parametersObject,
                                                "escortTrailDistanceMeters",
                                                target.escortTrailDistanceMeters,
                                                errorMessage)) {
        errorMessage = QString("field '%1.escortTrailDistanceMeters': %2").arg(fieldPath, errorMessage);
        return false;
    }

    if (!parseOptionalSignedDouble(parametersObject,
                                   "escortRightOffsetMeters",
                                   target.escortRightOffsetMeters,
                                   errorMessage)) {
        errorMessage = QString("field '%1.escortRightOffsetMeters': %2").arg(fieldPath, errorMessage);
        return false;
    }

    if (!parseOptionalNonNegativeOptionalDouble(parametersObject,
                                                "escortSlotToleranceMeters",
                                                target.escortSlotToleranceMeters,
                                                errorMessage)) {
        errorMessage = QString("field '%1.escortSlotToleranceMeters': %2").arg(fieldPath, errorMessage);
        return false;
    }

    if (!parseOptionalNonNegativeOptionalDouble(parametersObject,
                                                "escortCompletionHoldSeconds",
                                                target.escortCompletionHoldSeconds,
                                                errorMessage)) {
        errorMessage = QString("field '%1.escortCompletionHoldSeconds': %2").arg(fieldPath, errorMessage);
        return false;
    }

    return true;
}

bool parseOptionalBool(const QJsonObject& object, const QString& fieldName, bool defaultValue, bool& target, QString& errorMessage)
{
    if (!object.contains(fieldName)) {
        target = defaultValue;
        return true;
    }

    if (!object.value(fieldName).isBool()) {
        errorMessage = QString("Field '%1' must be boolean.").arg(fieldName);
        return false;
    }

    target = object.value(fieldName).toBool(defaultValue);
    return true;
}

bool parseScenarioEnvironment(const QJsonObject& root, fm::app::ScenarioDefinition& definition, QString& errorMessage)
{
    if (!ensureObjectField(root, "environment", errorMessage)) {
        return false;
    }

    if (!root.contains("environment")) {
        return true;
    }

    const auto environmentObject = root.value("environment").toObject();
    if (environmentObject.contains("timeOfDay") && !environmentObject.value("timeOfDay").isString()) {
        errorMessage = QString("Field 'environment.timeOfDay' must be a string.");
        return false;
    }

    if (environmentObject.contains("weather") && !environmentObject.value("weather").isString()) {
        errorMessage = QString("Field 'environment.weather' must be a string.");
        return false;
    }

    definition.environment.timeOfDay = environmentObject.value("timeOfDay").toString().toStdString();
    definition.environment.weather = environmentObject.value("weather").toString().toStdString();
    if (environmentObject.contains("visibilityMeters")) {
        if (!environmentObject.value("visibilityMeters").isDouble()) {
            errorMessage = QString("Field 'environment.visibilityMeters' must be numeric.");
            return false;
        }

        definition.environment.visibilityMeters = environmentObject.value("visibilityMeters").toDouble();
        if (definition.environment.visibilityMeters < 0.0) {
            errorMessage = QString("Field 'environment.visibilityMeters' must be non-negative.");
            return false;
        }
    }

    if (environmentObject.contains("wind")) {
        definition.environment.wind = parseVector2(environmentObject, "wind", errorMessage);
        if (!errorMessage.isEmpty()) {
            errorMessage = QString("Field 'environment.wind': %1").arg(errorMessage);
            return false;
        }
    }

    return true;
}

bool parseScenarioMapBounds(const QJsonObject& root, fm::app::ScenarioDefinition& definition, QString& errorMessage)
{
    if (!ensureObjectField(root, "mapBounds", errorMessage)) {
        return false;
    }

    if (!root.contains("mapBounds")) {
        return true;
    }

    const auto mapBoundsObject = root.value("mapBounds").toObject();
    definition.mapBounds.minimum = parseVector2(mapBoundsObject, "min", errorMessage);
    if (!errorMessage.isEmpty()) {
        errorMessage = QString("Field 'mapBounds.min': %1").arg(errorMessage);
        return false;
    }

    definition.mapBounds.maximum = parseVector2(mapBoundsObject, "max", errorMessage);
    if (!errorMessage.isEmpty()) {
        errorMessage = QString("Field 'mapBounds.max': %1").arg(errorMessage);
        return false;
    }

    return true;
}

bool parseIdentity(const QJsonObject& entityObject, fm::app::EntityDefinition& entity, QString& errorMessage)
{
    if (!ensureObjectField(entityObject, "identity", errorMessage)) {
        return false;
    }

    const auto identityObject = entityObject.contains("identity") ? entityObject.value("identity").toObject() : entityObject;
    if (!identityObject.value("id").isString()) {
        errorMessage = QString("field 'identity.id' must be a string.");
        return false;
    }

    entity.identity.id = identityObject.value("id").toString().toStdString();
    entity.identity.displayName = identityObject.value("displayName").toString(identityObject.value("id").toString()).toStdString();
    entity.identity.side = identityObject.value("side").toString().toStdString();
    entity.identity.category = identityObject.value("category").toString().toStdString();
    entity.identity.role = identityObject.value("role").toString().toStdString();
    entity.identity.colorHex = identityObject.value("color").toString("#4C956C").toStdString();
    if (!parseOptionalStringArray(identityObject, "tags", entity.identity.tags, errorMessage)) {
        return false;
    }

    return true;
}

bool parseSignature(const QJsonObject& entityObject, fm::app::EntityDefinition& entity, QString& errorMessage)
{
    if (!ensureObjectField(entityObject, "signature", errorMessage)) {
        return false;
    }

    const auto signatureObject = entityObject.contains("signature") ? entityObject.value("signature").toObject() : entityObject;
    if (signatureObject.contains("radarCrossSectionSquareMeters")) {
        if (!parseOptionalNonNegativeDouble(signatureObject,
                                            "radarCrossSectionSquareMeters",
                                            1.0,
                                            entity.signature.radarCrossSectionSquareMeters,
                                            errorMessage)) {
            return false;
        }
    } else if (entityObject.contains("radarCrossSectionSquareMeters")) {
        if (!parseOptionalNonNegativeDouble(entityObject,
                                            "radarCrossSectionSquareMeters",
                                            1.0,
                                            entity.signature.radarCrossSectionSquareMeters,
                                            errorMessage)) {
            return false;
        }
    }

    return true;
}

bool parseKinematics(const QJsonObject& entityObject, fm::app::EntityDefinition& entity, QString& errorMessage)
{
    if (!ensureObjectField(entityObject, "kinematics", errorMessage)) {
        return false;
    }

    const auto kinematicsObject = entityObject.contains("kinematics") ? entityObject.value("kinematics").toObject() : entityObject;
    entity.kinematics.position = parseVector2(kinematicsObject, "position", errorMessage);
    if (!errorMessage.isEmpty()) {
        return false;
    }

    entity.kinematics.velocity = parseVector2(kinematicsObject, "velocity", errorMessage);
    if (!errorMessage.isEmpty()) {
        return false;
    }

    if (!parseOptionalDouble(kinematicsObject, "headingDegrees", 0.0, entity.kinematics.headingDegrees, errorMessage)) {
        return false;
    }

    if (!parseOptionalNonNegativeDouble(kinematicsObject,
                                        "preferredSpeedMetersPerSecond",
                                        0.0,
                                        entity.kinematics.preferredSpeedMetersPerSecond,
                                        errorMessage)) {
        return false;
    }

    if (!parseOptionalNonNegativeDouble(kinematicsObject,
                                        "maxSpeedMetersPerSecond",
                                        0.0,
                                        entity.kinematics.maxSpeedMetersPerSecond,
                                        errorMessage)) {
        return false;
    }

    if (!parseOptionalNonNegativeDouble(kinematicsObject,
                                        "maxAccelerationMetersPerSecondSquared",
                                        0.0,
                                        entity.kinematics.maxAccelerationMetersPerSecondSquared,
                                        errorMessage)) {
        return false;
    }

    if (!parseOptionalNonNegativeDouble(kinematicsObject,
                                        "maxDecelerationMetersPerSecondSquared",
                                        0.0,
                                        entity.kinematics.maxDecelerationMetersPerSecondSquared,
                                        errorMessage)) {
        return false;
    }

    if (!parseOptionalNonNegativeDouble(kinematicsObject,
                                        "maxTurnRateDegreesPerSecond",
                                        180.0,
                                        entity.kinematics.maxTurnRateDegreesPerSecond,
                                        errorMessage)) {
        return false;
    }

    const auto parseRouteArray = [&](const QJsonArray& routeArray, const QString& fieldPath) -> bool {
        entity.kinematics.route.clear();
        entity.kinematics.route.reserve(static_cast<std::size_t>(routeArray.size()));
        for (qsizetype waypointIndex = 0; waypointIndex < routeArray.size(); ++waypointIndex) {
            if (!routeArray.at(waypointIndex).isObject()) {
                errorMessage = QString("field '%1[%2]' must be an object.").arg(fieldPath).arg(waypointIndex);
                return false;
            }

            const auto waypointObject = routeArray.at(waypointIndex).toObject();
            fm::app::EntityKinematicsDefinition::RouteWaypointDefinition waypoint;
            if (waypointObject.contains("name") && !waypointObject.value("name").isString()) {
                errorMessage = QString("field '%1[%2].name' must be a string.").arg(fieldPath).arg(waypointIndex);
                return false;
            }

            waypoint.name = waypointObject.value("name").toString(QString("wp-%1").arg(waypointIndex + 1)).toStdString();
            waypoint.position = parseVector2(waypointObject, "position", errorMessage);
            if (!errorMessage.isEmpty()) {
                errorMessage = QString("field '%1[%2].position': %3").arg(fieldPath).arg(waypointIndex).arg(errorMessage);
                return false;
            }

            if (!parseOptionalNonNegativeDouble(waypointObject, "loiterSeconds", 0.0, waypoint.loiterSeconds, errorMessage)) {
                errorMessage = QString("field '%1[%2].loiterSeconds': %3").arg(fieldPath).arg(waypointIndex).arg(errorMessage);
                return false;
            }

            entity.kinematics.route.push_back(std::move(waypoint));
        }

        return true;
    };

    if (kinematicsObject.contains("route")) {
        if (!kinematicsObject.value("route").isArray()) {
            errorMessage = QString("field 'kinematics.route' must be an array.");
            return false;
        }

        if (!parseRouteArray(kinematicsObject.value("route").toArray(), "kinematics.route")) {
            return false;
        }
    } else if (entityObject.contains("mission") && entityObject.value("mission").isObject()) {
        const auto missionObject = entityObject.value("mission").toObject();
        if (missionObject.contains("patrolRoute")) {
            if (!missionObject.value("patrolRoute").isArray()) {
                errorMessage = QString("field 'mission.patrolRoute' must be an array.");
                return false;
            }

            if (!parseRouteArray(missionObject.value("patrolRoute").toArray(), "mission.patrolRoute")) {
                return false;
            }
        }
    }

    return true;
}

bool parseSensor(const QJsonObject& entityObject, fm::app::EntityDefinition& entity, QString& errorMessage)
{
    if (!ensureObjectField(entityObject, "sensor", errorMessage)) {
        return false;
    }

    const auto sensorObject = entityObject.contains("sensor") ? entityObject.value("sensor").toObject() : QJsonObject {};
    if (sensorObject.contains("type") && !sensorObject.value("type").isString()) {
        errorMessage = QString("field 'sensor.type' must be a string.");
        return false;
    }

    entity.sensor.type = sensorObject.value("type").toString().toStdString();
    if (!parseOptionalBool(sensorObject, "enabled", true, entity.sensor.enabled, errorMessage)) {
        return false;
    }

    if (sensorObject.contains("rangeMeters")) {
        if (!parseOptionalNonNegativeDouble(sensorObject, "rangeMeters", 0.0, entity.sensor.rangeMeters, errorMessage)) {
            return false;
        }
    } else if (entityObject.contains("sensorRangeMeters")) {
        if (!parseOptionalNonNegativeDouble(entityObject, "sensorRangeMeters", 0.0, entity.sensor.rangeMeters, errorMessage)) {
            return false;
        }
    }

    if (!parseOptionalNonNegativeDouble(sensorObject,
                                        "fieldOfViewDegrees",
                                        360.0,
                                        entity.sensor.fieldOfViewDegrees,
                                        errorMessage)) {
        return false;
    }

    if (sensorObject.contains("radar")) {
        if (!sensorObject.value("radar").isObject()) {
            errorMessage = QString("field 'sensor.radar' must be an object.");
            return false;
        }

        const auto radarObject = sensorObject.value("radar").toObject();
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "peakTransmitPowerWatts",
                                            0.0,
                                            entity.sensor.radar.peakTransmitPowerWatts,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.peakTransmitPowerWatts': %1").arg(errorMessage);
            return false;
        }
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "antennaGainDecibels",
                                            0.0,
                                            entity.sensor.radar.antennaGainDecibels,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.antennaGainDecibels': %1").arg(errorMessage);
            return false;
        }
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "centerFrequencyHertz",
                                            0.0,
                                            entity.sensor.radar.centerFrequencyHertz,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.centerFrequencyHertz': %1").arg(errorMessage);
            return false;
        }
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "signalBandwidthHertz",
                                            0.0,
                                            entity.sensor.radar.signalBandwidthHertz,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.signalBandwidthHertz': %1").arg(errorMessage);
            return false;
        }
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "noiseFigureDecibels",
                                            0.0,
                                            entity.sensor.radar.noiseFigureDecibels,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.noiseFigureDecibels': %1").arg(errorMessage);
            return false;
        }
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "systemLossDecibels",
                                            0.0,
                                            entity.sensor.radar.systemLossDecibels,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.systemLossDecibels': %1").arg(errorMessage);
            return false;
        }
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "requiredSnrDecibels",
                                            13.0,
                                            entity.sensor.radar.requiredSnrDecibels,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.requiredSnrDecibels': %1").arg(errorMessage);
            return false;
        }
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "processingGainDecibels",
                                            0.0,
                                            entity.sensor.radar.processingGainDecibels,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.processingGainDecibels': %1").arg(errorMessage);
            return false;
        }
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "scanRateHertz",
                                            0.0,
                                            entity.sensor.radar.scanRateHertz,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.scanRateHertz': %1").arg(errorMessage);
            return false;
        }
        if (!parseOptionalNonNegativeDouble(radarObject,
                                            "receiverTemperatureKelvin",
                                            290.0,
                                            entity.sensor.radar.receiverTemperatureKelvin,
                                            errorMessage)) {
            errorMessage = QString("field 'sensor.radar.receiverTemperatureKelvin': %1").arg(errorMessage);
            return false;
        }
    }

    return true;
}

bool parseMission(const QJsonObject& entityObject, fm::app::EntityDefinition& entity, QString& errorMessage)
{
    if (!ensureObjectField(entityObject, "mission", errorMessage)) {
        return false;
    }

    if (!entityObject.contains("mission")) {
        return true;
    }

    const auto missionObject = entityObject.value("mission").toObject();
    if (missionObject.contains("objective") && !missionObject.value("objective").isString()) {
        errorMessage = QString("field 'mission.objective' must be a string.");
        return false;
    }

    if (missionObject.contains("behavior") && !missionObject.value("behavior").isString()) {
        errorMessage = QString("field 'mission.behavior' must be a string.");
        return false;
    }

    if (missionObject.contains("targetEntityId") && !missionObject.value("targetEntityId").isString()) {
        errorMessage = QString("field 'mission.targetEntityId' must be a string.");
        return false;
    }

    entity.mission.objective = missionObject.value("objective").toString().toStdString();
    entity.mission.behavior = missionObject.value("behavior").toString().toStdString();
    entity.mission.targetEntityId = missionObject.value("targetEntityId").toString().toStdString();

    if (!entity.mission.behavior.empty() && !isSupportedMissionBehavior(QString::fromStdString(entity.mission.behavior))) {
        errorMessage = QString("field 'mission.behavior' uses unsupported value '%1'. Supported values: patrol, intercept, escort, orbit, transit.")
                           .arg(QString::fromStdString(entity.mission.behavior));
        return false;
    }

    if (missionObject.contains("parameters")) {
        if (!missionObject.value("parameters").isObject()) {
            errorMessage = QString("field 'mission.parameters' must be an object.");
            return false;
        }

        if (!parseTaskParameters(missionObject.value("parameters").toObject(),
                                 QStringLiteral("mission.parameters"),
                                 entity.mission.parameters,
                                 errorMessage)) {
            return false;
        }
    }

    if (missionObject.contains("constraints")) {
        if (!missionObject.value("constraints").isObject()) {
            errorMessage = QString("field 'mission.constraints' must be an object.");
            return false;
        }

        const auto constraintsObject = missionObject.value("constraints").toObject();
        if (!parseOptionalNonNegativeDouble(constraintsObject,
                                            "timeoutSeconds",
                                            0.0,
                                            entity.mission.timeoutSeconds,
                                            errorMessage)) {
            errorMessage = QString("field 'mission.constraints.timeoutSeconds': %1").arg(errorMessage);
            return false;
        }

        if (constraintsObject.contains("activateAfter")) {
            if (!constraintsObject.value("activateAfter").isObject()) {
                errorMessage = QString("field 'mission.constraints.activateAfter' must be an object.");
                return false;
            }

            const auto activateAfterObject = constraintsObject.value("activateAfter").toObject();
            if (activateAfterObject.contains("entityId") && !activateAfterObject.value("entityId").isString()) {
                errorMessage = QString("field 'mission.constraints.activateAfter.entityId' must be a string.");
                return false;
            }

            if (activateAfterObject.contains("status") && !activateAfterObject.value("status").isString()) {
                errorMessage = QString("field 'mission.constraints.activateAfter.status' must be a string.");
                return false;
            }

            entity.mission.activation.prerequisiteEntityId = activateAfterObject.value("entityId").toString().toStdString();
            entity.mission.activation.prerequisiteStatus = activateAfterObject.value("status").toString("completed").toStdString();
        }

        const auto parseReplanDefinition = [&](const QJsonObject& replanObject,
                                               const QString& fieldPath,
                                               fm::app::EntityMissionDefinition::ReplanDefinition& target) -> bool {
            if (replanObject.contains("objective") && !replanObject.value("objective").isString()) {
                errorMessage = QString("field '%1.objective' must be a string.").arg(fieldPath);
                return false;
            }

            if (replanObject.contains("behavior") && !replanObject.value("behavior").isString()) {
                errorMessage = QString("field '%1.behavior' must be a string.").arg(fieldPath);
                return false;
            }

            if (replanObject.contains("targetEntityId") && !replanObject.value("targetEntityId").isString()) {
                errorMessage = QString("field '%1.targetEntityId' must be a string.").arg(fieldPath);
                return false;
            }

            if (!parseOptionalNonNegativeDouble(replanObject,
                                                "timeoutSeconds",
                                                0.0,
                                                target.timeoutSeconds,
                                                errorMessage)) {
                errorMessage = QString("field '%1.timeoutSeconds': %2").arg(fieldPath, errorMessage);
                return false;
            }

            target.objective = replanObject.value("objective").toString().toStdString();
            target.behavior = replanObject.value("behavior").toString().toStdString();
            target.targetEntityId = replanObject.value("targetEntityId").toString().toStdString();

            if (!target.behavior.empty() && !isSupportedMissionBehavior(QString::fromStdString(target.behavior))) {
                errorMessage = QString("field '%1.behavior' uses unsupported value '%2'. Supported values: patrol, intercept, escort, orbit, transit.")
                                   .arg(fieldPath, QString::fromStdString(target.behavior));
                return false;
            }

            if (replanObject.contains("parameters")) {
                if (!replanObject.value("parameters").isObject()) {
                    errorMessage = QString("field '%1.parameters' must be an object.").arg(fieldPath);
                    return false;
                }

                if (!parseTaskParameters(replanObject.value("parameters").toObject(),
                                         QStringLiteral("%1.parameters").arg(fieldPath),
                                         target.parameters,
                                         errorMessage)) {
                    return false;
                }
            }
            return true;
        };

        if (constraintsObject.contains("replanOnAbort")) {
            if (!constraintsObject.value("replanOnAbort").isObject()) {
                errorMessage = QString("field 'mission.constraints.replanOnAbort' must be an object.");
                return false;
            }

            if (!parseReplanDefinition(constraintsObject.value("replanOnAbort").toObject(),
                                       QStringLiteral("mission.constraints.replanOnAbort"),
                                       entity.mission.replanOnAbort)) {
                return false;
            }
        }

        if (constraintsObject.contains("replanChain")) {
            if (!constraintsObject.value("replanChain").isArray()) {
                errorMessage = QString("field 'mission.constraints.replanChain' must be an array.");
                return false;
            }

            const auto replanArray = constraintsObject.value("replanChain").toArray();
            entity.mission.replanChain.clear();
            entity.mission.replanChain.reserve(static_cast<std::size_t>(replanArray.size()));
            for (qsizetype replanIndex = 0; replanIndex < replanArray.size(); ++replanIndex) {
                if (!replanArray.at(replanIndex).isObject()) {
                    errorMessage = QString("field 'mission.constraints.replanChain[%1]' must be an object.").arg(replanIndex);
                    return false;
                }

                fm::app::EntityMissionDefinition::ReplanDefinition replanDefinition;
                if (!parseReplanDefinition(replanArray.at(replanIndex).toObject(),
                                           QStringLiteral("mission.constraints.replanChain[%1]").arg(replanIndex),
                                           replanDefinition)) {
                    return false;
                }

                entity.mission.replanChain.push_back(std::move(replanDefinition));
            }
        }
    }

    return true;
}

QJsonObject toJson(const fm::core::Vector2& value)
{
    return QJsonObject {
        {"x", value.x},
        {"y", value.y},
    };
}

QJsonObject buildTaskParametersObject(const fm::app::EntityMissionDefinition::TaskParametersDefinition& parameters)
{
    QJsonObject object;
    if (parameters.interceptCompletionDistanceMeters.has_value()) {
        object.insert("interceptCompletionDistanceMeters", *parameters.interceptCompletionDistanceMeters);
    }
    if (parameters.orbitRadiusMeters.has_value()) {
        object.insert("orbitRadiusMeters", *parameters.orbitRadiusMeters);
    }
    if (parameters.orbitClockwise.has_value()) {
        object.insert("orbitClockwise", *parameters.orbitClockwise);
    }
    if (parameters.orbitAcquireToleranceMeters.has_value()) {
        object.insert("orbitAcquireToleranceMeters", *parameters.orbitAcquireToleranceMeters);
    }
    if (parameters.orbitCompletionToleranceMeters.has_value()) {
        object.insert("orbitCompletionToleranceMeters", *parameters.orbitCompletionToleranceMeters);
    }
    if (parameters.orbitCompletionHoldSeconds.has_value()) {
        object.insert("orbitCompletionHoldSeconds", *parameters.orbitCompletionHoldSeconds);
    }
    if (parameters.escortTrailDistanceMeters.has_value()) {
        object.insert("escortTrailDistanceMeters", *parameters.escortTrailDistanceMeters);
    }
    if (parameters.escortRightOffsetMeters.has_value()) {
        object.insert("escortRightOffsetMeters", *parameters.escortRightOffsetMeters);
    }
    if (parameters.escortSlotToleranceMeters.has_value()) {
        object.insert("escortSlotToleranceMeters", *parameters.escortSlotToleranceMeters);
    }
    if (parameters.escortCompletionHoldSeconds.has_value()) {
        object.insert("escortCompletionHoldSeconds", *parameters.escortCompletionHoldSeconds);
    }
    return object;
}

QJsonObject buildReplanObject(const fm::app::EntityMissionDefinition::ReplanDefinition& definition)
{
    QJsonObject object;
    if (!definition.objective.empty()) {
        object.insert("objective", QString::fromStdString(definition.objective));
    }
    if (!definition.behavior.empty()) {
        object.insert("behavior", QString::fromStdString(definition.behavior));
    }
    if (!definition.targetEntityId.empty()) {
        object.insert("targetEntityId", QString::fromStdString(definition.targetEntityId));
    }
    if (definition.timeoutSeconds > 0.0) {
        object.insert("timeoutSeconds", definition.timeoutSeconds);
    }
    const QJsonObject parametersObject = buildTaskParametersObject(definition.parameters);
    if (!parametersObject.isEmpty()) {
        object.insert("parameters", parametersObject);
    }
    return object;
}

QJsonObject buildMissionObject(const fm::app::EntityMissionDefinition& mission)
{
    QJsonObject object;
    if (!mission.objective.empty()) {
        object.insert("objective", QString::fromStdString(mission.objective));
    }
    if (!mission.behavior.empty()) {
        object.insert("behavior", QString::fromStdString(mission.behavior));
    }
    if (!mission.targetEntityId.empty()) {
        object.insert("targetEntityId", QString::fromStdString(mission.targetEntityId));
    }

    const QJsonObject parametersObject = buildTaskParametersObject(mission.parameters);
    if (!parametersObject.isEmpty()) {
        object.insert("parameters", parametersObject);
    }

    QJsonObject constraintsObject;
    if (mission.timeoutSeconds > 0.0) {
        constraintsObject.insert("timeoutSeconds", mission.timeoutSeconds);
    }
    if (!mission.activation.prerequisiteEntityId.empty()) {
        constraintsObject.insert("activateAfter", QJsonObject {
            {"entityId", QString::fromStdString(mission.activation.prerequisiteEntityId)},
            {"status", QString::fromStdString(mission.activation.prerequisiteStatus)},
        });
    }
    const QJsonObject replanOnAbortObject = buildReplanObject(mission.replanOnAbort);
    if (!replanOnAbortObject.isEmpty()) {
        constraintsObject.insert("replanOnAbort", replanOnAbortObject);
    }
    if (!mission.replanChain.empty()) {
        QJsonArray replanChainArray;
        for (const auto& replan : mission.replanChain) {
            replanChainArray.append(buildReplanObject(replan));
        }
        constraintsObject.insert("replanChain", replanChainArray);
    }
    if (!constraintsObject.isEmpty()) {
        object.insert("constraints", constraintsObject);
    }

    return object;
}

QJsonObject buildEntityObject(const fm::app::EntityDefinition& entity)
{
    QJsonObject identityObject {
        {"id", QString::fromStdString(entity.identity.id)},
    };
    if (!entity.identity.displayName.empty()) {
        identityObject.insert("displayName", QString::fromStdString(entity.identity.displayName));
    }
    if (!entity.identity.side.empty()) {
        identityObject.insert("side", QString::fromStdString(entity.identity.side));
    }
    if (!entity.identity.category.empty()) {
        identityObject.insert("category", QString::fromStdString(entity.identity.category));
    }
    if (!entity.identity.role.empty()) {
        identityObject.insert("role", QString::fromStdString(entity.identity.role));
    }
    if (!entity.identity.colorHex.empty()) {
        identityObject.insert("color", QString::fromStdString(entity.identity.colorHex));
    }
    if (!entity.identity.tags.empty()) {
        QJsonArray tagsArray;
        for (const auto& tag : entity.identity.tags) {
            tagsArray.append(QString::fromStdString(tag));
        }
        identityObject.insert("tags", tagsArray);
    }

    QJsonObject signatureObject;
    if (std::abs(entity.signature.radarCrossSectionSquareMeters - 1.0) > 1e-9) {
        signatureObject.insert("radarCrossSectionSquareMeters", entity.signature.radarCrossSectionSquareMeters);
    }

    QJsonObject kinematicsObject {
        {"position", toJson(entity.kinematics.position)},
        {"velocity", toJson(entity.kinematics.velocity)},
        {"headingDegrees", entity.kinematics.headingDegrees},
    };
    if (entity.kinematics.preferredSpeedMetersPerSecond > 0.0) {
        kinematicsObject.insert("preferredSpeedMetersPerSecond", entity.kinematics.preferredSpeedMetersPerSecond);
    }
    if (entity.kinematics.maxSpeedMetersPerSecond > 0.0) {
        kinematicsObject.insert("maxSpeedMetersPerSecond", entity.kinematics.maxSpeedMetersPerSecond);
    }
    if (entity.kinematics.maxAccelerationMetersPerSecondSquared > 0.0) {
        kinematicsObject.insert("maxAccelerationMetersPerSecondSquared", entity.kinematics.maxAccelerationMetersPerSecondSquared);
    }
    if (entity.kinematics.maxDecelerationMetersPerSecondSquared > 0.0) {
        kinematicsObject.insert("maxDecelerationMetersPerSecondSquared", entity.kinematics.maxDecelerationMetersPerSecondSquared);
    }
    if (entity.kinematics.maxTurnRateDegreesPerSecond != 180.0) {
        kinematicsObject.insert("maxTurnRateDegreesPerSecond", entity.kinematics.maxTurnRateDegreesPerSecond);
    }
    if (!entity.kinematics.route.empty()) {
        QJsonArray routeArray;
        for (const auto& waypoint : entity.kinematics.route) {
            routeArray.append(QJsonObject {
                {"name", QString::fromStdString(waypoint.name)},
                {"position", toJson(waypoint.position)},
                {"loiterSeconds", waypoint.loiterSeconds},
            });
        }
        kinematicsObject.insert("route", routeArray);
    }

    QJsonObject sensorObject;
    if (!entity.sensor.type.empty()) {
        sensorObject.insert("type", QString::fromStdString(entity.sensor.type));
    }
    if (entity.sensor.rangeMeters > 0.0) {
        sensorObject.insert("rangeMeters", entity.sensor.rangeMeters);
    }
    if (entity.sensor.fieldOfViewDegrees != 360.0) {
        sensorObject.insert("fieldOfViewDegrees", entity.sensor.fieldOfViewDegrees);
    }
    if (!entity.sensor.enabled || !sensorObject.isEmpty()) {
        sensorObject.insert("enabled", entity.sensor.enabled);
    }
    QJsonObject radarObject;
    if (entity.sensor.radar.peakTransmitPowerWatts > 0.0) {
        radarObject.insert("peakTransmitPowerWatts", entity.sensor.radar.peakTransmitPowerWatts);
    }
    if (entity.sensor.radar.antennaGainDecibels > 0.0) {
        radarObject.insert("antennaGainDecibels", entity.sensor.radar.antennaGainDecibels);
    }
    if (entity.sensor.radar.centerFrequencyHertz > 0.0) {
        radarObject.insert("centerFrequencyHertz", entity.sensor.radar.centerFrequencyHertz);
    }
    if (entity.sensor.radar.signalBandwidthHertz > 0.0) {
        radarObject.insert("signalBandwidthHertz", entity.sensor.radar.signalBandwidthHertz);
    }
    if (entity.sensor.radar.noiseFigureDecibels > 0.0) {
        radarObject.insert("noiseFigureDecibels", entity.sensor.radar.noiseFigureDecibels);
    }
    if (entity.sensor.radar.systemLossDecibels > 0.0) {
        radarObject.insert("systemLossDecibels", entity.sensor.radar.systemLossDecibels);
    }
    if (entity.sensor.radar.requiredSnrDecibels != 13.0) {
        radarObject.insert("requiredSnrDecibels", entity.sensor.radar.requiredSnrDecibels);
    }
    if (entity.sensor.radar.processingGainDecibels > 0.0) {
        radarObject.insert("processingGainDecibels", entity.sensor.radar.processingGainDecibels);
    }
    if (entity.sensor.radar.scanRateHertz > 0.0) {
        radarObject.insert("scanRateHertz", entity.sensor.radar.scanRateHertz);
    }
    if (std::abs(entity.sensor.radar.receiverTemperatureKelvin - 290.0) > 1e-9) {
        radarObject.insert("receiverTemperatureKelvin", entity.sensor.radar.receiverTemperatureKelvin);
    }
    if (!radarObject.isEmpty()) {
        sensorObject.insert("radar", radarObject);
    }

    QJsonObject entityObject {
        {"identity", identityObject},
        {"kinematics", kinematicsObject},
    };
    if (!signatureObject.isEmpty()) {
        entityObject.insert("signature", signatureObject);
    }
    if (!sensorObject.isEmpty()) {
        entityObject.insert("sensor", sensorObject);
    }
    const QJsonObject missionObject = buildMissionObject(entity.mission);
    if (!missionObject.isEmpty()) {
        entityObject.insert("mission", missionObject);
    }
    return entityObject;
}

}  // namespace

namespace fm::app {

ScenarioLoadResult ScenarioLoader::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {{}, QString("Unable to open scenario file: %1").arg(filePath)};
    }

    return loadFromJson(file.readAll(), filePath);
}

ScenarioLoadResult ScenarioLoader::loadFromJson(const QByteArray& jsonBytes, const QString& sourceName)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return {{}, QString("Invalid JSON in %1: %2").arg(sourceName, parseError.errorString())};
    }

    if (!document.isObject()) {
        return {{}, QString("Scenario root in %1 must be a JSON object.").arg(sourceName)};
    }

    const auto root = document.object();
    if (!root.value("name").isString()) {
        return {{}, QString("Scenario in %1 must define a string field 'name'.").arg(sourceName)};
    }

    if (!root.value("entities").isArray()) {
        return {{}, QString("Scenario in %1 must define an array field 'entities'.").arg(sourceName)};
    }

    ScenarioDefinition definition;
    definition.name = root.value("name").toString().toStdString();
    definition.description = root.value("description").toString().toStdString();

    QString errorMessage;
    if (!parseScenarioEnvironment(root, definition, errorMessage)) {
        return {{}, QString("Scenario in %1: %2").arg(sourceName, errorMessage)};
    }

    errorMessage.clear();
    if (!parseScenarioMapBounds(root, definition, errorMessage)) {
        return {{}, QString("Scenario in %1: %2").arg(sourceName, errorMessage)};
    }

    const auto entities = root.value("entities").toArray();
    if (entities.isEmpty()) {
        return {{}, QString("Scenario in %1 must contain at least one entity.").arg(sourceName)};
    }

    std::unordered_set<std::string> entityIds;
    for (qsizetype index = 0; index < entities.size(); ++index) {
        if (!entities.at(index).isObject()) {
            return {{}, QString("Entity at index %1 in %2 must be an object.").arg(index).arg(sourceName)};
        }

        const auto entityObject = entities.at(index).toObject();
        EntityDefinition entity;
        errorMessage.clear();
        if (!parseIdentity(entityObject, entity, errorMessage)) {
            return {{}, QString("Entity at index %1 in %2: %3").arg(index).arg(sourceName, errorMessage)};
        }

        if (!entityIds.insert(entity.identity.id).second) {
            return {{}, QString("Scenario in %1 contains duplicate entity id '%2'.")
                            .arg(sourceName, QString::fromStdString(entity.identity.id))};
        }

        errorMessage.clear();
        if (!parseSignature(entityObject, entity, errorMessage)) {
            return {{}, QString("Entity '%1' in %2: %3")
                            .arg(QString::fromStdString(entity.identity.id), sourceName, errorMessage)};
        }

        errorMessage.clear();
        if (!parseKinematics(entityObject, entity, errorMessage)) {
            return {{}, QString("Entity '%1' in %2: %3")
                            .arg(QString::fromStdString(entity.identity.id), sourceName, errorMessage)};
        }

        errorMessage.clear();
        if (!parseSensor(entityObject, entity, errorMessage)) {
            return {{}, QString("Entity '%1' in %2: %3")
                            .arg(QString::fromStdString(entity.identity.id), sourceName, errorMessage)};
        }

        errorMessage.clear();
        if (!parseMission(entityObject, entity, errorMessage)) {
            return {{}, QString("Entity '%1' in %2: %3")
                            .arg(QString::fromStdString(entity.identity.id), sourceName, errorMessage)};
        }

        definition.entities.push_back(std::move(entity));
    }

    return {definition, {}};
}

QByteArray ScenarioLoader::saveToJson(const ScenarioDefinition& definition)
{
    QJsonObject root {
        {"name", QString::fromStdString(definition.name)},
    };

    if (!definition.description.empty()) {
        root.insert("description", QString::fromStdString(definition.description));
    }

    const bool hasEnvironment = !definition.environment.timeOfDay.empty()
        || !definition.environment.weather.empty()
        || definition.environment.visibilityMeters > 0.0
        || std::abs(definition.environment.wind.x) > 1e-9
        || std::abs(definition.environment.wind.y) > 1e-9;
    if (hasEnvironment) {
        QJsonObject environmentObject;
        if (!definition.environment.timeOfDay.empty()) {
            environmentObject.insert("timeOfDay", QString::fromStdString(definition.environment.timeOfDay));
        }
        if (!definition.environment.weather.empty()) {
            environmentObject.insert("weather", QString::fromStdString(definition.environment.weather));
        }
        if (definition.environment.visibilityMeters > 0.0) {
            environmentObject.insert("visibilityMeters", definition.environment.visibilityMeters);
        }
        if (std::abs(definition.environment.wind.x) > 1e-9 || std::abs(definition.environment.wind.y) > 1e-9) {
            environmentObject.insert("wind", toJson(definition.environment.wind));
        }
        root.insert("environment", environmentObject);
    }

    const bool hasBounds = definition.mapBounds.maximum.x > definition.mapBounds.minimum.x
        && definition.mapBounds.maximum.y > definition.mapBounds.minimum.y;
    if (hasBounds) {
        root.insert("mapBounds", QJsonObject {
            {"min", toJson(definition.mapBounds.minimum)},
            {"max", toJson(definition.mapBounds.maximum)},
        });
    }

    QJsonArray entitiesArray;
    for (const auto& entity : definition.entities) {
        entitiesArray.append(buildEntityObject(entity));
    }
    root.insert("entities", entitiesArray);

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool ScenarioLoader::saveToFile(const ScenarioDefinition& definition, const QString& filePath, QString& errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        errorMessage = QString("Unable to open scenario file for writing: %1").arg(filePath);
        return false;
    }

    const QByteArray jsonBytes = saveToJson(definition);
    if (file.write(jsonBytes) != jsonBytes.size()) {
        errorMessage = QString("Failed to write scenario file: %1").arg(filePath);
        return false;
    }

    return true;
}

}  // namespace fm::app