#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;

namespace fm::ui {

class SimulationControlPanel final : public QWidget {
public:
    explicit SimulationControlPanel(QWidget* parent = nullptr);

    void setSimulationActionsVisible(bool visible);
    void setPlaybackActionsVisible(bool visible);
    void setTimeStepControlsVisible(bool visible);

    QPushButton* loadButton() const;
    QPushButton* saveButton() const;
    QPushButton* startButton() const;
    QPushButton* pauseButton() const;
    QPushButton* stepButton() const;
    QPushButton* resetButton() const;
    QPushButton* previousSnapshotButton() const;
    QPushButton* nextSnapshotButton() const;
    QPushButton* liveViewButton() const;
    QPushButton* zoomInButton() const;
    QPushButton* zoomOutButton() const;
    QPushButton* resetViewButton() const;
    QCheckBox* showTrailsCheckBox() const;
    QCheckBox* autoFitViewCheckBox() const;
    QCheckBox* lockSelectedEntityViewCheckBox() const;
    QComboBox* entityFilterComboBox() const;
    QDoubleSpinBox* timeStepSpinBox() const;

private:
    QLabel* entityFilterLabel_ {nullptr};
    QLabel* timeStepLabel_ {nullptr};
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
    QCheckBox* showTrailsCheckBox_ {nullptr};
    QCheckBox* autoFitViewCheckBox_ {nullptr};
    QCheckBox* lockSelectedEntityViewCheckBox_ {nullptr};
    QComboBox* entityFilterComboBox_ {nullptr};
    QDoubleSpinBox* timeStepSpinBox_ {nullptr};
};

}  // namespace fm::ui