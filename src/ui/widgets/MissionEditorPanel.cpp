#include "ui/widgets/MissionEditorPanel.h"

#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace fm::ui {

MissionEditorPanel::MissionEditorPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    auto* formLayout = new QFormLayout();

    hintLabel_ = new QLabel(contentWidget);
    objectiveEdit_ = new QLineEdit(contentWidget);
    behaviorComboBox_ = new QComboBox(contentWidget);
    targetComboBox_ = new QComboBox(contentWidget);
    orbitDirectionComboBox_ = new QComboBox(contentWidget);
    interceptDistanceEdit_ = new QLineEdit(contentWidget);
    orbitRadiusEdit_ = new QLineEdit(contentWidget);
    orbitAcquireToleranceEdit_ = new QLineEdit(contentWidget);
    orbitCompletionToleranceEdit_ = new QLineEdit(contentWidget);
    orbitHoldEdit_ = new QLineEdit(contentWidget);
    escortTrailEdit_ = new QLineEdit(contentWidget);
    escortOffsetEdit_ = new QLineEdit(contentWidget);
    escortSlotToleranceEdit_ = new QLineEdit(contentWidget);
    escortHoldEdit_ = new QLineEdit(contentWidget);
    applyButton_ = new QPushButton(QStringLiteral("应用任务"), contentWidget);

    hintLabel_->setWordWrap(true);
    hintLabel_->setText(QStringLiteral("选择实体后可修改任务；数值留空表示使用默认参数。应用后会重置仿真到初始态。"));
    objectiveEdit_->setPlaceholderText(QStringLiteral("任务说明"));
    interceptDistanceEdit_->setPlaceholderText(QStringLiteral("默认"));
    orbitRadiusEdit_->setPlaceholderText(QStringLiteral("默认/自动"));
    orbitAcquireToleranceEdit_->setPlaceholderText(QStringLiteral("默认"));
    orbitCompletionToleranceEdit_->setPlaceholderText(QStringLiteral("默认"));
    orbitHoldEdit_->setPlaceholderText(QStringLiteral("默认"));
    escortTrailEdit_->setPlaceholderText(QStringLiteral("默认"));
    escortOffsetEdit_->setPlaceholderText(QStringLiteral("默认"));
    escortSlotToleranceEdit_->setPlaceholderText(QStringLiteral("默认"));
    escortHoldEdit_->setPlaceholderText(QStringLiteral("默认"));

    behaviorComboBox_->addItem(QStringLiteral("巡逻"), QStringLiteral("patrol"));
    behaviorComboBox_->addItem(QStringLiteral("拦截"), QStringLiteral("intercept"));
    behaviorComboBox_->addItem(QStringLiteral("护航"), QStringLiteral("escort"));
    behaviorComboBox_->addItem(QStringLiteral("盘旋"), QStringLiteral("orbit"));
    behaviorComboBox_->addItem(QStringLiteral("转场"), QStringLiteral("transit"));
    targetComboBox_->addItem(QStringLiteral("无"), QString());
    orbitDirectionComboBox_->addItem(QStringLiteral("默认"), QString());
    orbitDirectionComboBox_->addItem(QStringLiteral("顺时针"), QStringLiteral("clockwise"));
    orbitDirectionComboBox_->addItem(QStringLiteral("逆时针"), QStringLiteral("counterclockwise"));

    formLayout->addRow(QStringLiteral("任务说明"), objectiveEdit_);
    formLayout->addRow(QStringLiteral("任务行为"), behaviorComboBox_);
    formLayout->addRow(QStringLiteral("任务目标"), targetComboBox_);
    formLayout->addRow(QStringLiteral("拦截完成距离"), interceptDistanceEdit_);
    formLayout->addRow(QStringLiteral("盘旋半径"), orbitRadiusEdit_);
    formLayout->addRow(QStringLiteral("盘旋方向"), orbitDirectionComboBox_);
    formLayout->addRow(QStringLiteral("盘旋进入容差"), orbitAcquireToleranceEdit_);
    formLayout->addRow(QStringLiteral("盘旋完成容差"), orbitCompletionToleranceEdit_);
    formLayout->addRow(QStringLiteral("盘旋保持时间"), orbitHoldEdit_);
    formLayout->addRow(QStringLiteral("护航尾随距离"), escortTrailEdit_);
    formLayout->addRow(QStringLiteral("护航侧向偏移"), escortOffsetEdit_);
    formLayout->addRow(QStringLiteral("护航槽位容差"), escortSlotToleranceEdit_);
    formLayout->addRow(QStringLiteral("护航稳定保持"), escortHoldEdit_);

    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(hintLabel_);
    contentLayout->addLayout(formLayout);
    contentLayout->addWidget(applyButton_);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(contentWidget);
    scrollArea->setMinimumHeight(160);

    rootLayout->addWidget(new QLabel(QStringLiteral("任务编辑"), this));
    rootLayout->addWidget(scrollArea);
}

QLabel* MissionEditorPanel::hintLabel() const { return hintLabel_; }
QLineEdit* MissionEditorPanel::objectiveEdit() const { return objectiveEdit_; }
QComboBox* MissionEditorPanel::behaviorComboBox() const { return behaviorComboBox_; }
QComboBox* MissionEditorPanel::targetComboBox() const { return targetComboBox_; }
QComboBox* MissionEditorPanel::orbitDirectionComboBox() const { return orbitDirectionComboBox_; }
QLineEdit* MissionEditorPanel::interceptDistanceEdit() const { return interceptDistanceEdit_; }
QLineEdit* MissionEditorPanel::orbitRadiusEdit() const { return orbitRadiusEdit_; }
QLineEdit* MissionEditorPanel::orbitAcquireToleranceEdit() const { return orbitAcquireToleranceEdit_; }
QLineEdit* MissionEditorPanel::orbitCompletionToleranceEdit() const { return orbitCompletionToleranceEdit_; }
QLineEdit* MissionEditorPanel::orbitHoldEdit() const { return orbitHoldEdit_; }
QLineEdit* MissionEditorPanel::escortTrailEdit() const { return escortTrailEdit_; }
QLineEdit* MissionEditorPanel::escortOffsetEdit() const { return escortOffsetEdit_; }
QLineEdit* MissionEditorPanel::escortSlotToleranceEdit() const { return escortSlotToleranceEdit_; }
QLineEdit* MissionEditorPanel::escortHoldEdit() const { return escortHoldEdit_; }
QPushButton* MissionEditorPanel::applyButton() const { return applyButton_; }

}  // namespace fm::ui