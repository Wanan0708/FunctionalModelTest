#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;

namespace fm::ui {

class EntityEditorPanel final : public QWidget {
public:
    explicit EntityEditorPanel(QWidget* parent = nullptr);

    QLabel* hintLabel() const;
    QLineEdit* idEdit() const;
    QLineEdit* displayNameEdit() const;
    QLineEdit* sideEdit() const;
    QLineEdit* categoryEdit() const;
    QLineEdit* roleEdit() const;
    QLineEdit* colorEdit() const;
    QLineEdit* positionXEdit() const;
    QLineEdit* positionYEdit() const;
    QLineEdit* velocityXEdit() const;
    QLineEdit* velocityYEdit() const;
    QLineEdit* headingEdit() const;
    QLineEdit* maxSpeedEdit() const;
    QLineEdit* maxTurnRateEdit() const;
    QCheckBox* sensorEnabledCheckBox() const;
    QLineEdit* sensorTypeEdit() const;
    QLineEdit* sensorRangeEdit() const;
    QLineEdit* sensorFieldOfViewEdit() const;
    QListWidget* routeWaypointList() const;
    QLineEdit* routeWaypointNameEdit() const;
    QLineEdit* routeWaypointXEdit() const;
    QLineEdit* routeWaypointYEdit() const;
    QLineEdit* routeWaypointLoiterEdit() const;
    QPlainTextEdit* routeEdit() const;
    QPushButton* addRouteWaypointButton() const;
    QPushButton* updateRouteWaypointButton() const;
    QPushButton* removeRouteWaypointButton() const;
    QPushButton* moveRouteWaypointUpButton() const;
    QPushButton* moveRouteWaypointDownButton() const;
    QPushButton* clearRouteWaypointsButton() const;
    QPushButton* addButton() const;
    QPushButton* deleteButton() const;
    QPushButton* applyButton() const;

private:
    QLabel* hintLabel_ {nullptr};
    QLineEdit* idEdit_ {nullptr};
    QLineEdit* displayNameEdit_ {nullptr};
    QLineEdit* sideEdit_ {nullptr};
    QLineEdit* categoryEdit_ {nullptr};
    QLineEdit* roleEdit_ {nullptr};
    QLineEdit* colorEdit_ {nullptr};
    QLineEdit* positionXEdit_ {nullptr};
    QLineEdit* positionYEdit_ {nullptr};
    QLineEdit* velocityXEdit_ {nullptr};
    QLineEdit* velocityYEdit_ {nullptr};
    QLineEdit* headingEdit_ {nullptr};
    QLineEdit* maxSpeedEdit_ {nullptr};
    QLineEdit* maxTurnRateEdit_ {nullptr};
    QCheckBox* sensorEnabledCheckBox_ {nullptr};
    QLineEdit* sensorTypeEdit_ {nullptr};
    QLineEdit* sensorRangeEdit_ {nullptr};
    QLineEdit* sensorFieldOfViewEdit_ {nullptr};
    QListWidget* routeWaypointList_ {nullptr};
    QLineEdit* routeWaypointNameEdit_ {nullptr};
    QLineEdit* routeWaypointXEdit_ {nullptr};
    QLineEdit* routeWaypointYEdit_ {nullptr};
    QLineEdit* routeWaypointLoiterEdit_ {nullptr};
    QPlainTextEdit* routeEdit_ {nullptr};
    QPushButton* addRouteWaypointButton_ {nullptr};
    QPushButton* updateRouteWaypointButton_ {nullptr};
    QPushButton* removeRouteWaypointButton_ {nullptr};
    QPushButton* moveRouteWaypointUpButton_ {nullptr};
    QPushButton* moveRouteWaypointDownButton_ {nullptr};
    QPushButton* clearRouteWaypointsButton_ {nullptr};
    QPushButton* addButton_ {nullptr};
    QPushButton* deleteButton_ {nullptr};
    QPushButton* applyButton_ {nullptr};
};

}  // namespace fm::ui