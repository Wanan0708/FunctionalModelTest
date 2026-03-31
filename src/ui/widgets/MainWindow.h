#pragma once

#include <string>

#include <QMainWindow>

#include "app/services/SimulationSession.h"

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

namespace fm::ui {

class MainWindow final : public QMainWindow {
public:
    MainWindow();
    ~MainWindow() override;

private:
    void buildUi();
    void connectSignals();
    void loadDefaultScenario();
    const fm::app::SimulationSnapshot* currentSnapshot() const;
    std::vector<fm::app::EntityRenderState> currentRenderStates() const;
    std::vector<fm::app::EntityRenderState> filteredEntityListStates() const;
    bool isReplayMode() const;
    void refreshScene();
    void updateEntityList();
    void updateTaskEditor();
    void updateTaskEditorFieldState();
    bool applyTaskEditorChanges();
    void updateEntityDetails();
    void updateLogView();
    void updateRecordingSummary();
    void updateStatus();

    fm::app::SimulationSession session_;
    QGraphicsScene* scene_ {nullptr};
    QGraphicsView* view_ {nullptr};
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
    QLabel* taskEditorHintLabel_ {nullptr};
    QLabel* recordingSummaryLabel_ {nullptr};
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
};

}  // namespace fm::ui