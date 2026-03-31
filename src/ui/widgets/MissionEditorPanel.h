#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;

namespace fm::ui {

class MissionEditorPanel final : public QWidget {
public:
    explicit MissionEditorPanel(QWidget* parent = nullptr);

    QLabel* hintLabel() const;
    QLineEdit* objectiveEdit() const;
    QComboBox* behaviorComboBox() const;
    QComboBox* targetComboBox() const;
    QComboBox* orbitDirectionComboBox() const;
    QLineEdit* interceptDistanceEdit() const;
    QLineEdit* orbitRadiusEdit() const;
    QLineEdit* orbitAcquireToleranceEdit() const;
    QLineEdit* orbitCompletionToleranceEdit() const;
    QLineEdit* orbitHoldEdit() const;
    QLineEdit* escortTrailEdit() const;
    QLineEdit* escortOffsetEdit() const;
    QLineEdit* escortSlotToleranceEdit() const;
    QLineEdit* escortHoldEdit() const;
    QPushButton* applyButton() const;

private:
    QLabel* hintLabel_ {nullptr};
    QLineEdit* objectiveEdit_ {nullptr};
    QComboBox* behaviorComboBox_ {nullptr};
    QComboBox* targetComboBox_ {nullptr};
    QComboBox* orbitDirectionComboBox_ {nullptr};
    QLineEdit* interceptDistanceEdit_ {nullptr};
    QLineEdit* orbitRadiusEdit_ {nullptr};
    QLineEdit* orbitAcquireToleranceEdit_ {nullptr};
    QLineEdit* orbitCompletionToleranceEdit_ {nullptr};
    QLineEdit* orbitHoldEdit_ {nullptr};
    QLineEdit* escortTrailEdit_ {nullptr};
    QLineEdit* escortOffsetEdit_ {nullptr};
    QLineEdit* escortSlotToleranceEdit_ {nullptr};
    QLineEdit* escortHoldEdit_ {nullptr};
    QPushButton* applyButton_ {nullptr};
};

}  // namespace fm::ui