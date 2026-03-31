#pragma once

#include <QByteArray>
#include <QString>

#include "app/scenario/ScenarioDefinition.h"

namespace fm::app {

struct ScenarioLoadResult {
    ScenarioDefinition scenario;
    QString errorMessage;

    bool ok() const
    {
        return errorMessage.isEmpty();
    }
};

class ScenarioLoader {
public:
    static ScenarioLoadResult loadFromFile(const QString& filePath);
    static ScenarioLoadResult loadFromJson(const QByteArray& jsonBytes, const QString& sourceName);
    static QByteArray saveToJson(const ScenarioDefinition& definition);
    static bool saveToFile(const ScenarioDefinition& definition, const QString& filePath, QString& errorMessage);
};

}  // namespace fm::app