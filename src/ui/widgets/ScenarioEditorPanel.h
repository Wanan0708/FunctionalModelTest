#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

namespace fm::ui {

class ScenarioEditorPanel final : public QWidget {
public:
    explicit ScenarioEditorPanel(QWidget* parent = nullptr);

    QLabel* hintLabel() const;
    QLineEdit* nameEdit() const;
    QPlainTextEdit* descriptionEdit() const;
    QLineEdit* timeOfDayEdit() const;
    QLineEdit* weatherEdit() const;
    QLineEdit* visibilityEdit() const;
    QLineEdit* windXEdit() const;
    QLineEdit* windYEdit() const;
    QLineEdit* boundsMinXEdit() const;
    QLineEdit* boundsMinYEdit() const;
    QLineEdit* boundsMaxXEdit() const;
    QLineEdit* boundsMaxYEdit() const;
    QPushButton* applyButton() const;

private:
    QLabel* hintLabel_ {nullptr};
    QLineEdit* nameEdit_ {nullptr};
    QPlainTextEdit* descriptionEdit_ {nullptr};
    QLineEdit* timeOfDayEdit_ {nullptr};
    QLineEdit* weatherEdit_ {nullptr};
    QLineEdit* visibilityEdit_ {nullptr};
    QLineEdit* windXEdit_ {nullptr};
    QLineEdit* windYEdit_ {nullptr};
    QLineEdit* boundsMinXEdit_ {nullptr};
    QLineEdit* boundsMinYEdit_ {nullptr};
    QLineEdit* boundsMaxXEdit_ {nullptr};
    QLineEdit* boundsMaxYEdit_ {nullptr};
    QPushButton* applyButton_ {nullptr};
};

}  // namespace fm::ui