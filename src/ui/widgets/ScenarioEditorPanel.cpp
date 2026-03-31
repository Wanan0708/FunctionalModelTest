#include "ui/widgets/ScenarioEditorPanel.h"

#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace fm::ui {

ScenarioEditorPanel::ScenarioEditorPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    auto* formLayout = new QFormLayout();

    hintLabel_ = new QLabel(contentWidget);
    nameEdit_ = new QLineEdit(contentWidget);
    descriptionEdit_ = new QPlainTextEdit(contentWidget);
    timeOfDayEdit_ = new QLineEdit(contentWidget);
    weatherEdit_ = new QLineEdit(contentWidget);
    visibilityEdit_ = new QLineEdit(contentWidget);
    windXEdit_ = new QLineEdit(contentWidget);
    windYEdit_ = new QLineEdit(contentWidget);
    boundsMinXEdit_ = new QLineEdit(contentWidget);
    boundsMinYEdit_ = new QLineEdit(contentWidget);
    boundsMaxXEdit_ = new QLineEdit(contentWidget);
    boundsMaxYEdit_ = new QLineEdit(contentWidget);
    applyButton_ = new QPushButton(QStringLiteral("应用场景属性"), contentWidget);

    hintLabel_->setWordWrap(true);
    hintLabel_->setText(QStringLiteral("编辑想定名称、描述、环境和地图边界。应用后会刷新场景信息与视图边界。"));
    descriptionEdit_->setMinimumHeight(90);
    descriptionEdit_->setPlaceholderText(QStringLiteral("输入场景说明"));
    visibilityEdit_->setPlaceholderText(QStringLiteral("50000"));

    formLayout->addRow(QStringLiteral("场景名称"), nameEdit_);
    formLayout->addRow(QStringLiteral("场景描述"), descriptionEdit_);
    formLayout->addRow(QStringLiteral("时间段"), timeOfDayEdit_);
    formLayout->addRow(QStringLiteral("天气"), weatherEdit_);
    formLayout->addRow(QStringLiteral("可视距离"), visibilityEdit_);
    formLayout->addRow(QStringLiteral("风 X"), windXEdit_);
    formLayout->addRow(QStringLiteral("风 Y"), windYEdit_);
    formLayout->addRow(QStringLiteral("边界最小 X"), boundsMinXEdit_);
    formLayout->addRow(QStringLiteral("边界最小 Y"), boundsMinYEdit_);
    formLayout->addRow(QStringLiteral("边界最大 X"), boundsMaxXEdit_);
    formLayout->addRow(QStringLiteral("边界最大 Y"), boundsMaxYEdit_);

    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(hintLabel_);
    contentLayout->addLayout(formLayout);
    contentLayout->addWidget(applyButton_);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(contentWidget);
    scrollArea->setMinimumHeight(240);

    rootLayout->addWidget(new QLabel(QStringLiteral("场景编辑"), this));
    rootLayout->addWidget(scrollArea);
}

QLabel* ScenarioEditorPanel::hintLabel() const { return hintLabel_; }
QLineEdit* ScenarioEditorPanel::nameEdit() const { return nameEdit_; }
QPlainTextEdit* ScenarioEditorPanel::descriptionEdit() const { return descriptionEdit_; }
QLineEdit* ScenarioEditorPanel::timeOfDayEdit() const { return timeOfDayEdit_; }
QLineEdit* ScenarioEditorPanel::weatherEdit() const { return weatherEdit_; }
QLineEdit* ScenarioEditorPanel::visibilityEdit() const { return visibilityEdit_; }
QLineEdit* ScenarioEditorPanel::windXEdit() const { return windXEdit_; }
QLineEdit* ScenarioEditorPanel::windYEdit() const { return windYEdit_; }
QLineEdit* ScenarioEditorPanel::boundsMinXEdit() const { return boundsMinXEdit_; }
QLineEdit* ScenarioEditorPanel::boundsMinYEdit() const { return boundsMinYEdit_; }
QLineEdit* ScenarioEditorPanel::boundsMaxXEdit() const { return boundsMaxXEdit_; }
QLineEdit* ScenarioEditorPanel::boundsMaxYEdit() const { return boundsMaxYEdit_; }
QPushButton* ScenarioEditorPanel::applyButton() const { return applyButton_; }

}  // namespace fm::ui