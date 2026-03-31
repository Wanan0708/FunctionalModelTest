#pragma once

#include <string>

#include <QMainWindow>

#include "app/services/SimulationSession.h"

class QCloseEvent;
class QGraphicsScene;
class QGraphicsView;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class QToolBox;

namespace fm::ui {

class EntityEditorPanel;
class MissionEditorPanel;
class ScenarioEditorPanel;
class SimulationControlPanel;

class MainWindow final : public QMainWindow {
public:
    enum class Mode {
        ScenarioEditor,
        Simulation,
    };

    explicit MainWindow(Mode mode = Mode::Simulation);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();
    void connectSignals();
    void applyWindowMode();
    void updateWindowTitle();
    void setUnsavedChanges(bool hasUnsavedChanges);
    bool promptToSaveUnsavedChanges(const QString& reason);
    bool saveScenarioInteractively();
    void loadDefaultScenario();
    const fm::app::SimulationSnapshot* currentSnapshot() const;
    std::vector<fm::app::EntityRenderState> currentRenderStates() const;
    std::vector<fm::app::EntityRenderState> filteredEntityListStates() const;
    bool isReplayMode() const;
    void refreshScene();
    void updateEntityList();
    void updateScenarioEditor();
    void updateEntityEditor();
    void updateRouteWaypointEditorState();
    void refreshRouteWaypointPreview();
    bool buildRouteWaypointDefinitionFromInputs(fm::app::EntityKinematicsDefinition::RouteWaypointDefinition& waypoint,
                                               QString& errorMessage) const;
    std::vector<fm::app::EntityKinematicsDefinition::RouteWaypointDefinition> routeWaypointDefinitionsFromEditor() const;
    void updateTaskEditor();
    void updateTaskEditorFieldState();
    bool addEntityFromEditor();
    bool removeSelectedEntity();
    bool applyScenarioEditorChanges();
    bool addRouteWaypoint();
    bool updateSelectedRouteWaypoint();
    bool removeSelectedRouteWaypoint();
    bool moveSelectedRouteWaypointUp();
    bool moveSelectedRouteWaypointDown();
    void clearRouteWaypoints();
    bool applyEntityEditorChanges();
    bool applyTaskEditorChanges();
    void updateEntityDetails();
    void updateLogView();
    void updateRecordingSummary();
    void updateStatus();

    Mode mode_ {Mode::Simulation};
    fm::app::SimulationSession session_;
    SimulationControlPanel* simulationControlPanel_ {nullptr};
    ScenarioEditorPanel* scenarioEditorPanel_ {nullptr};
    EntityEditorPanel* entityEditorPanel_ {nullptr};
    MissionEditorPanel* missionEditorPanel_ {nullptr};
    QGraphicsScene* scene_ {nullptr};
    QGraphicsView* view_ {nullptr};
    QWidget* recordingPanel_ {nullptr};
    QWidget* logPanel_ {nullptr};
    QToolBox* sideToolBox_ {nullptr};
    QListWidget* entityList_ {nullptr};
    QCheckBox* showTrailsCheckBox_ {nullptr};
    QCheckBox* autoFitViewCheckBox_ {nullptr};
    QCheckBox* selectedEntityLogOnlyCheckBox_ {nullptr};
    QCheckBox* lockSelectedEntityViewCheckBox_ {nullptr};
    QComboBox* entityFilterComboBox_ {nullptr};
    QComboBox* logFilterComboBox_ {nullptr};
    QDoubleSpinBox* timeStepSpinBox_ {nullptr};
    QPushButton* loadButton_ {nullptr};
    QPushButton* saveButton_ {nullptr};
    QPushButton* startButton_ {nullptr};
    QPushButton* pauseButton_ {nullptr};
    QPushButton* stepButton_ {nullptr};
    QPushButton* resetButton_ {nullptr};
    QPushButton* previousSnapshotButton_ {nullptr};
    QPushButton* nextSnapshotButton_ {nullptr};
    QPushButton* liveViewButton_ {nullptr};
    QPushButton* zoomInButton_ {nullptr};
    QPushButton* zoomOutButton_ {nullptr};
    QPushButton* resetViewButton_ {nullptr};
    QPlainTextEdit* logView_ {nullptr};
    QLabel* statusLabel_ {nullptr};
    QLabel* scenarioLabel_ {nullptr};
    QLabel* entityDetailsLabel_ {nullptr};
    QLabel* scenarioEditorHintLabel_ {nullptr};
    QLabel* taskEditorHintLabel_ {nullptr};
    QLabel* entityEditorHintLabel_ {nullptr};
    QLabel* recordingSummaryLabel_ {nullptr};
    QLineEdit* scenarioNameEdit_ {nullptr};
    QPlainTextEdit* scenarioDescriptionEdit_ {nullptr};
    QLineEdit* scenarioTimeOfDayEdit_ {nullptr};
    QLineEdit* scenarioWeatherEdit_ {nullptr};
    QLineEdit* scenarioVisibilityEdit_ {nullptr};
    QLineEdit* scenarioWindXEdit_ {nullptr};
    QLineEdit* scenarioWindYEdit_ {nullptr};
    QLineEdit* scenarioBoundsMinXEdit_ {nullptr};
    QLineEdit* scenarioBoundsMinYEdit_ {nullptr};
    QLineEdit* scenarioBoundsMaxXEdit_ {nullptr};
    QLineEdit* scenarioBoundsMaxYEdit_ {nullptr};
    QPushButton* applyScenarioButton_ {nullptr};
    QLineEdit* entityIdEdit_ {nullptr};
    QLineEdit* entityDisplayNameEdit_ {nullptr};
    QLineEdit* entitySideEdit_ {nullptr};
    QLineEdit* entityCategoryEdit_ {nullptr};
    QLineEdit* entityRoleEdit_ {nullptr};
    QLineEdit* entityColorEdit_ {nullptr};
    QLineEdit* entityPositionXEdit_ {nullptr};
    QLineEdit* entityPositionYEdit_ {nullptr};
    QLineEdit* entityVelocityXEdit_ {nullptr};
    QLineEdit* entityVelocityYEdit_ {nullptr};
    QLineEdit* entityHeadingEdit_ {nullptr};
    QLineEdit* entityPreferredSpeedEdit_ {nullptr};
    QLineEdit* entityMaxSpeedEdit_ {nullptr};
    QLineEdit* entityMaxAccelerationEdit_ {nullptr};
    QLineEdit* entityMaxDecelerationEdit_ {nullptr};
    QLineEdit* entityMaxTurnRateEdit_ {nullptr};
    QLineEdit* entityRadarCrossSectionEdit_ {nullptr};
    QCheckBox* entitySensorEnabledCheckBox_ {nullptr};
    QLineEdit* entitySensorTypeEdit_ {nullptr};
    QLineEdit* entitySensorRangeEdit_ {nullptr};
    QLineEdit* entitySensorFieldOfViewEdit_ {nullptr};
    QLineEdit* entityRadarPeakPowerEdit_ {nullptr};
    QLineEdit* entityRadarAntennaGainEdit_ {nullptr};
    QLineEdit* entityRadarCenterFrequencyEdit_ {nullptr};
    QLineEdit* entityRadarBandwidthEdit_ {nullptr};
    QLineEdit* entityRadarNoiseFigureEdit_ {nullptr};
    QLineEdit* entityRadarSystemLossEdit_ {nullptr};
    QLineEdit* entityRadarRequiredSnrEdit_ {nullptr};
    QLineEdit* entityRadarProcessingGainEdit_ {nullptr};
    QLineEdit* entityRadarScanRateEdit_ {nullptr};
    QLineEdit* entityRadarReceiverTemperatureEdit_ {nullptr};
    QListWidget* entityRouteWaypointList_ {nullptr};
    QLineEdit* entityRouteWaypointNameEdit_ {nullptr};
    QLineEdit* entityRouteWaypointXEdit_ {nullptr};
    QLineEdit* entityRouteWaypointYEdit_ {nullptr};
    QLineEdit* entityRouteWaypointLoiterEdit_ {nullptr};
    QPlainTextEdit* entityRouteEdit_ {nullptr};
    QPushButton* addRouteWaypointButton_ {nullptr};
    QPushButton* updateRouteWaypointButton_ {nullptr};
    QPushButton* removeRouteWaypointButton_ {nullptr};
    QPushButton* moveRouteWaypointUpButton_ {nullptr};
    QPushButton* moveRouteWaypointDownButton_ {nullptr};
    QPushButton* clearRouteWaypointsButton_ {nullptr};
    QPushButton* addEntityButton_ {nullptr};
    QPushButton* removeEntityButton_ {nullptr};
    QPushButton* applyEntityButton_ {nullptr};
    QLineEdit* taskObjectiveEdit_ {nullptr};
    QComboBox* taskBehaviorComboBox_ {nullptr};
    QComboBox* taskTargetComboBox_ {nullptr};
    QComboBox* taskOrbitDirectionComboBox_ {nullptr};
    QLineEdit* taskInterceptDistanceEdit_ {nullptr};
    QLineEdit* taskOrbitRadiusEdit_ {nullptr};
    QLineEdit* taskOrbitAcquireToleranceEdit_ {nullptr};
    QLineEdit* taskOrbitCompletionToleranceEdit_ {nullptr};
    QLineEdit* taskOrbitHoldEdit_ {nullptr};
    QLineEdit* taskEscortTrailEdit_ {nullptr};
    QLineEdit* taskEscortOffsetEdit_ {nullptr};
    QLineEdit* taskEscortSlotToleranceEdit_ {nullptr};
    QLineEdit* taskEscortHoldEdit_ {nullptr};
    QPushButton* applyTaskButton_ {nullptr};
    QTimer* timer_ {nullptr};
    std::string selectedEntityId_;
    int replaySnapshotIndex_ {-1};
    bool forceFitViewOnNextRefresh_ {true};
    bool hasUnsavedChanges_ {false};
};

}  // namespace fm::ui