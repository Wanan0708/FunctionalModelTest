#include "ui/widgets/EntityEditorPanel.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QFrame>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace fm::ui {

EntityEditorPanel::EntityEditorPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    auto* formLayout = new QFormLayout();
    auto* routeEditorLayout = new QVBoxLayout();
    auto* routeEditorWidget = new QWidget(contentWidget);
    auto* routeWaypointFormLayout = new QFormLayout();
    auto* routeWaypointButtonLayout = new QHBoxLayout();
    auto* buttonLayout = new QHBoxLayout();

    hintLabel_ = new QLabel(contentWidget);
    idEdit_ = new QLineEdit(contentWidget);
    displayNameEdit_ = new QLineEdit(contentWidget);
    sideEdit_ = new QLineEdit(contentWidget);
    categoryEdit_ = new QLineEdit(contentWidget);
    roleEdit_ = new QLineEdit(contentWidget);
    colorEdit_ = new QLineEdit(contentWidget);
    positionXEdit_ = new QLineEdit(contentWidget);
    positionYEdit_ = new QLineEdit(contentWidget);
    velocityXEdit_ = new QLineEdit(contentWidget);
    velocityYEdit_ = new QLineEdit(contentWidget);
    headingEdit_ = new QLineEdit(contentWidget);
    maxSpeedEdit_ = new QLineEdit(contentWidget);
    maxTurnRateEdit_ = new QLineEdit(contentWidget);
    sensorEnabledCheckBox_ = new QCheckBox(QStringLiteral("启用传感器"), contentWidget);
    sensorTypeEdit_ = new QLineEdit(contentWidget);
    sensorRangeEdit_ = new QLineEdit(contentWidget);
    sensorFieldOfViewEdit_ = new QLineEdit(contentWidget);
    routeWaypointList_ = new QListWidget(contentWidget);
    routeWaypointNameEdit_ = new QLineEdit(contentWidget);
    routeWaypointXEdit_ = new QLineEdit(contentWidget);
    routeWaypointYEdit_ = new QLineEdit(contentWidget);
    routeWaypointLoiterEdit_ = new QLineEdit(contentWidget);
    routeEdit_ = new QPlainTextEdit(contentWidget);
    addRouteWaypointButton_ = new QPushButton(QStringLiteral("新增航点"), contentWidget);
    updateRouteWaypointButton_ = new QPushButton(QStringLiteral("更新航点"), contentWidget);
    removeRouteWaypointButton_ = new QPushButton(QStringLiteral("删除航点"), contentWidget);
    moveRouteWaypointUpButton_ = new QPushButton(QStringLiteral("上移"), contentWidget);
    moveRouteWaypointDownButton_ = new QPushButton(QStringLiteral("下移"), contentWidget);
    clearRouteWaypointsButton_ = new QPushButton(QStringLiteral("清空航路"), contentWidget);
    addButton_ = new QPushButton(QStringLiteral("新增实体"), contentWidget);
    deleteButton_ = new QPushButton(QStringLiteral("删除当前实体"), contentWidget);
    applyButton_ = new QPushButton(QStringLiteral("应用实体属性"), contentWidget);

    hintLabel_->setWordWrap(true);
    hintLabel_->setText(QStringLiteral("编辑实体基础信息、运动学参数、传感器和航路。可直接增删改航点，底部保留只读航路预览。"));
    idEdit_->setReadOnly(true);
    colorEdit_->setPlaceholderText(QStringLiteral("#2E86AB"));
    sensorTypeEdit_->setPlaceholderText(QStringLiteral("radar / eo / wide-area-radar"));
    routeWaypointList_->setMinimumHeight(110);
    routeWaypointNameEdit_->setPlaceholderText(QStringLiteral("wp-1"));
    routeWaypointXEdit_->setPlaceholderText(QStringLiteral("10.0"));
    routeWaypointYEdit_->setPlaceholderText(QStringLiteral("5.0"));
    routeWaypointLoiterEdit_->setPlaceholderText(QStringLiteral("0.0"));
    routeEdit_->setPlaceholderText(QStringLiteral("wp-1, 10, 5, 0\nwp-2, 20, 8, 15"));
    routeEdit_->setMinimumHeight(120);
    routeEdit_->setReadOnly(true);

    formLayout->addRow(QStringLiteral("实体 ID"), idEdit_);
    formLayout->addRow(QStringLiteral("显示名称"), displayNameEdit_);
    formLayout->addRow(QStringLiteral("阵营"), sideEdit_);
    formLayout->addRow(QStringLiteral("类型"), categoryEdit_);
    formLayout->addRow(QStringLiteral("角色"), roleEdit_);
    formLayout->addRow(QStringLiteral("颜色"), colorEdit_);
    formLayout->addRow(QStringLiteral("位置 X"), positionXEdit_);
    formLayout->addRow(QStringLiteral("位置 Y"), positionYEdit_);
    formLayout->addRow(QStringLiteral("速度 X"), velocityXEdit_);
    formLayout->addRow(QStringLiteral("速度 Y"), velocityYEdit_);
    formLayout->addRow(QStringLiteral("航向"), headingEdit_);
    formLayout->addRow(QStringLiteral("最大速度"), maxSpeedEdit_);
    formLayout->addRow(QStringLiteral("最大转率"), maxTurnRateEdit_);
    formLayout->addRow(QString(), sensorEnabledCheckBox_);
    formLayout->addRow(QStringLiteral("传感器类型"), sensorTypeEdit_);
    formLayout->addRow(QStringLiteral("传感器距离"), sensorRangeEdit_);
    formLayout->addRow(QStringLiteral("传感器视场"), sensorFieldOfViewEdit_);

    routeEditorWidget->setLayout(routeEditorLayout);
    routeEditorLayout->setContentsMargins(0, 0, 0, 0);
    routeEditorLayout->addWidget(routeWaypointList_);
    routeWaypointFormLayout->addRow(QStringLiteral("航点名称"), routeWaypointNameEdit_);
    routeWaypointFormLayout->addRow(QStringLiteral("航点 X"), routeWaypointXEdit_);
    routeWaypointFormLayout->addRow(QStringLiteral("航点 Y"), routeWaypointYEdit_);
    routeWaypointFormLayout->addRow(QStringLiteral("停留秒"), routeWaypointLoiterEdit_);
    routeEditorLayout->addLayout(routeWaypointFormLayout);
    routeWaypointButtonLayout->addWidget(addRouteWaypointButton_);
    routeWaypointButtonLayout->addWidget(updateRouteWaypointButton_);
    routeWaypointButtonLayout->addWidget(removeRouteWaypointButton_);
    routeWaypointButtonLayout->addWidget(moveRouteWaypointUpButton_);
    routeWaypointButtonLayout->addWidget(moveRouteWaypointDownButton_);
    routeWaypointButtonLayout->addWidget(clearRouteWaypointsButton_);
    routeEditorLayout->addLayout(routeWaypointButtonLayout);
    routeEditorLayout->addWidget(routeEdit_);

    formLayout->addRow(QStringLiteral("航路编辑"), routeEditorWidget);

    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(hintLabel_);
    contentLayout->addLayout(formLayout);
    buttonLayout->addWidget(addButton_);
    buttonLayout->addWidget(deleteButton_);
    contentLayout->addLayout(buttonLayout);
    contentLayout->addWidget(applyButton_);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(contentWidget);
    scrollArea->setMinimumHeight(220);

    rootLayout->addWidget(new QLabel(QStringLiteral("实体编辑"), this));
    rootLayout->addWidget(scrollArea);
}

QLabel* EntityEditorPanel::hintLabel() const { return hintLabel_; }
QLineEdit* EntityEditorPanel::idEdit() const { return idEdit_; }
QLineEdit* EntityEditorPanel::displayNameEdit() const { return displayNameEdit_; }
QLineEdit* EntityEditorPanel::sideEdit() const { return sideEdit_; }
QLineEdit* EntityEditorPanel::categoryEdit() const { return categoryEdit_; }
QLineEdit* EntityEditorPanel::roleEdit() const { return roleEdit_; }
QLineEdit* EntityEditorPanel::colorEdit() const { return colorEdit_; }
QLineEdit* EntityEditorPanel::positionXEdit() const { return positionXEdit_; }
QLineEdit* EntityEditorPanel::positionYEdit() const { return positionYEdit_; }
QLineEdit* EntityEditorPanel::velocityXEdit() const { return velocityXEdit_; }
QLineEdit* EntityEditorPanel::velocityYEdit() const { return velocityYEdit_; }
QLineEdit* EntityEditorPanel::headingEdit() const { return headingEdit_; }
QLineEdit* EntityEditorPanel::maxSpeedEdit() const { return maxSpeedEdit_; }
QLineEdit* EntityEditorPanel::maxTurnRateEdit() const { return maxTurnRateEdit_; }
QCheckBox* EntityEditorPanel::sensorEnabledCheckBox() const { return sensorEnabledCheckBox_; }
QLineEdit* EntityEditorPanel::sensorTypeEdit() const { return sensorTypeEdit_; }
QLineEdit* EntityEditorPanel::sensorRangeEdit() const { return sensorRangeEdit_; }
QLineEdit* EntityEditorPanel::sensorFieldOfViewEdit() const { return sensorFieldOfViewEdit_; }
QListWidget* EntityEditorPanel::routeWaypointList() const { return routeWaypointList_; }
QLineEdit* EntityEditorPanel::routeWaypointNameEdit() const { return routeWaypointNameEdit_; }
QLineEdit* EntityEditorPanel::routeWaypointXEdit() const { return routeWaypointXEdit_; }
QLineEdit* EntityEditorPanel::routeWaypointYEdit() const { return routeWaypointYEdit_; }
QLineEdit* EntityEditorPanel::routeWaypointLoiterEdit() const { return routeWaypointLoiterEdit_; }
QPlainTextEdit* EntityEditorPanel::routeEdit() const { return routeEdit_; }
QPushButton* EntityEditorPanel::addRouteWaypointButton() const { return addRouteWaypointButton_; }
QPushButton* EntityEditorPanel::updateRouteWaypointButton() const { return updateRouteWaypointButton_; }
QPushButton* EntityEditorPanel::removeRouteWaypointButton() const { return removeRouteWaypointButton_; }
QPushButton* EntityEditorPanel::moveRouteWaypointUpButton() const { return moveRouteWaypointUpButton_; }
QPushButton* EntityEditorPanel::moveRouteWaypointDownButton() const { return moveRouteWaypointDownButton_; }
QPushButton* EntityEditorPanel::clearRouteWaypointsButton() const { return clearRouteWaypointsButton_; }
QPushButton* EntityEditorPanel::addButton() const { return addButton_; }
QPushButton* EntityEditorPanel::deleteButton() const { return deleteButton_; }
QPushButton* EntityEditorPanel::applyButton() const { return applyButton_; }

}  // namespace fm::ui