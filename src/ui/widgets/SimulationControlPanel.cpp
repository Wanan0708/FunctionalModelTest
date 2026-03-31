#include "ui/widgets/SimulationControlPanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace fm::ui {

SimulationControlPanel::SimulationControlPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* buttonLayout = new QHBoxLayout();
    auto* optionsLayout = new QHBoxLayout();

    loadButton_ = new QPushButton(QStringLiteral("加载场景"), this);
    saveButton_ = new QPushButton(QStringLiteral("保存场景"), this);
    startButton_ = new QPushButton(QStringLiteral("开始"), this);
    pauseButton_ = new QPushButton(QStringLiteral("暂停"), this);
    stepButton_ = new QPushButton(QStringLiteral("单步"), this);
    resetButton_ = new QPushButton(QStringLiteral("重置"), this);
    previousSnapshotButton_ = new QPushButton(QStringLiteral("上一帧"), this);
    nextSnapshotButton_ = new QPushButton(QStringLiteral("下一帧"), this);
    liveViewButton_ = new QPushButton(QStringLiteral("实时视图"), this);
    zoomInButton_ = new QPushButton(QStringLiteral("放大"), this);
    zoomOutButton_ = new QPushButton(QStringLiteral("缩小"), this);
    resetViewButton_ = new QPushButton(QStringLiteral("重置视图"), this);
    showTrailsCheckBox_ = new QCheckBox(QStringLiteral("显示轨迹"), this);
    autoFitViewCheckBox_ = new QCheckBox(QStringLiteral("自动适配视图"), this);
    lockSelectedEntityViewCheckBox_ = new QCheckBox(QStringLiteral("锁定选中实体"), this);
    entityFilterComboBox_ = new QComboBox(this);
    timeStepSpinBox_ = new QDoubleSpinBox(this);
    entityFilterLabel_ = new QLabel(QStringLiteral("实体过滤"), this);
    timeStepLabel_ = new QLabel(QStringLiteral("固定时间步"), this);

    buttonLayout->addWidget(loadButton_);
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(pauseButton_);
    buttonLayout->addWidget(stepButton_);
    buttonLayout->addWidget(resetButton_);
    buttonLayout->addWidget(previousSnapshotButton_);
    buttonLayout->addWidget(nextSnapshotButton_);
    buttonLayout->addWidget(liveViewButton_);
    buttonLayout->addWidget(zoomInButton_);
    buttonLayout->addWidget(zoomOutButton_);
    buttonLayout->addWidget(resetViewButton_);
    buttonLayout->addStretch();

    showTrailsCheckBox_->setChecked(true);
    autoFitViewCheckBox_->setChecked(true);
    entityFilterComboBox_->addItem(QStringLiteral("全部实体"), QStringLiteral("all"));
    entityFilterComboBox_->addItem(QStringLiteral("活动任务"), QStringLiteral("active"));
    entityFilterComboBox_->addItem(QStringLiteral("终态任务"), QStringLiteral("terminal"));
    entityFilterComboBox_->addItem(QStringLiteral("已完成"), QStringLiteral("completed"));
    entityFilterComboBox_->addItem(QStringLiteral("已中止"), QStringLiteral("aborted"));
    timeStepSpinBox_->setDecimals(3);
    timeStepSpinBox_->setRange(0.01, 1.0);
    timeStepSpinBox_->setSingleStep(0.01);
    timeStepSpinBox_->setSuffix(QStringLiteral(" s/tick"));

    optionsLayout->addWidget(showTrailsCheckBox_);
    optionsLayout->addWidget(autoFitViewCheckBox_);
    optionsLayout->addWidget(lockSelectedEntityViewCheckBox_);
    optionsLayout->addWidget(entityFilterLabel_);
    optionsLayout->addWidget(entityFilterComboBox_);
    optionsLayout->addWidget(timeStepLabel_);
    optionsLayout->addWidget(timeStepSpinBox_);
    optionsLayout->addStretch();

    rootLayout->addLayout(buttonLayout);
    rootLayout->addLayout(optionsLayout);
}

void SimulationControlPanel::setSimulationActionsVisible(bool visible)
{
    startButton_->setVisible(visible);
    pauseButton_->setVisible(visible);
    stepButton_->setVisible(visible);
    resetButton_->setVisible(visible);
}

void SimulationControlPanel::setPlaybackActionsVisible(bool visible)
{
    previousSnapshotButton_->setVisible(visible);
    nextSnapshotButton_->setVisible(visible);
    liveViewButton_->setVisible(visible);
}

void SimulationControlPanel::setTimeStepControlsVisible(bool visible)
{
    timeStepLabel_->setVisible(visible);
    timeStepSpinBox_->setVisible(visible);
}

QPushButton* SimulationControlPanel::loadButton() const { return loadButton_; }
QPushButton* SimulationControlPanel::saveButton() const { return saveButton_; }
QPushButton* SimulationControlPanel::startButton() const { return startButton_; }
QPushButton* SimulationControlPanel::pauseButton() const { return pauseButton_; }
QPushButton* SimulationControlPanel::stepButton() const { return stepButton_; }
QPushButton* SimulationControlPanel::resetButton() const { return resetButton_; }
QPushButton* SimulationControlPanel::previousSnapshotButton() const { return previousSnapshotButton_; }
QPushButton* SimulationControlPanel::nextSnapshotButton() const { return nextSnapshotButton_; }
QPushButton* SimulationControlPanel::liveViewButton() const { return liveViewButton_; }
QPushButton* SimulationControlPanel::zoomInButton() const { return zoomInButton_; }
QPushButton* SimulationControlPanel::zoomOutButton() const { return zoomOutButton_; }
QPushButton* SimulationControlPanel::resetViewButton() const { return resetViewButton_; }
QCheckBox* SimulationControlPanel::showTrailsCheckBox() const { return showTrailsCheckBox_; }
QCheckBox* SimulationControlPanel::autoFitViewCheckBox() const { return autoFitViewCheckBox_; }
QCheckBox* SimulationControlPanel::lockSelectedEntityViewCheckBox() const { return lockSelectedEntityViewCheckBox_; }
QComboBox* SimulationControlPanel::entityFilterComboBox() const { return entityFilterComboBox_; }
QDoubleSpinBox* SimulationControlPanel::timeStepSpinBox() const { return timeStepSpinBox_; }

}  // namespace fm::ui