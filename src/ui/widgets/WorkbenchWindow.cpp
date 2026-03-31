#include "ui/widgets/WorkbenchWindow.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "ui/widgets/MainWindow.h"

namespace fm::ui {

WorkbenchWindow::WorkbenchWindow()
{
    auto* centralWidget = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(centralWidget);
    auto* actionLayout = new QHBoxLayout();

    auto* titleLabel = new QLabel(QStringLiteral("FunctionalModelTest 工作台"), centralWidget);
    auto* descriptionLabel = new QLabel(
        QStringLiteral("将想定编辑和仿真推演拆成两个入口。编辑界面聚焦场景与任务配置，推演界面聚焦运行、回放和观察。"),
        centralWidget);
    auto* scenarioButton = new QPushButton(QStringLiteral("打开想定编辑界面"), centralWidget);
    auto* simulationButton = new QPushButton(QStringLiteral("打开仿真推演界面"), centralWidget);

    titleLabel->setStyleSheet(QStringLiteral("QLabel { font-size: 26px; font-weight: 600; color: #203126; }"));
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet(QStringLiteral("QLabel { color: #47584E; font-size: 14px; }"));
    scenarioButton->setMinimumHeight(56);
    simulationButton->setMinimumHeight(56);
    scenarioButton->setStyleSheet(QStringLiteral("QPushButton { background: #E0F0E6; border: 1px solid #8AA399; padding: 12px 20px; font-size: 16px; }"));
    simulationButton->setStyleSheet(QStringLiteral("QPushButton { background: #E6EEF7; border: 1px solid #7E9AB8; padding: 12px 20px; font-size: 16px; }"));

    auto* panel = new QFrame(centralWidget);
    panel->setStyleSheet(QStringLiteral("QFrame { background: #F7FAF8; border: 1px solid #D5DDD8; border-radius: 12px; }"));
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->addWidget(titleLabel);
    panelLayout->addSpacing(6);
    panelLayout->addWidget(descriptionLabel);
    panelLayout->addSpacing(18);
    actionLayout->addWidget(scenarioButton);
    actionLayout->addWidget(simulationButton);
    panelLayout->addLayout(actionLayout);

    rootLayout->addStretch();
    rootLayout->addWidget(panel);
    rootLayout->addStretch();
    rootLayout->setContentsMargins(80, 70, 80, 70);

    setCentralWidget(centralWidget);
    setWindowTitle(QStringLiteral("FunctionalModelTest - 工作台"));
    resize(920, 420);

    connect(scenarioButton, &QPushButton::clicked, this, [this]() {
        openScenarioEditor();
    });
    connect(simulationButton, &QPushButton::clicked, this, [this]() {
        openSimulationWindow();
    });
}

WorkbenchWindow::~WorkbenchWindow() = default;

void WorkbenchWindow::openScenarioEditor()
{
    if (scenarioEditorWindow_ == nullptr) {
        scenarioEditorWindow_ = new MainWindow(MainWindow::Mode::ScenarioEditor);
        scenarioEditorWindow_->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(scenarioEditorWindow_, &QObject::destroyed, this, [this]() {
            scenarioEditorWindow_ = nullptr;
        });
    }

    scenarioEditorWindow_->showMaximized();
    scenarioEditorWindow_->raise();
    scenarioEditorWindow_->activateWindow();
}

void WorkbenchWindow::openSimulationWindow()
{
    if (simulationWindow_ == nullptr) {
        simulationWindow_ = new MainWindow(MainWindow::Mode::Simulation);
        simulationWindow_->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(simulationWindow_, &QObject::destroyed, this, [this]() {
            simulationWindow_ = nullptr;
        });
    }

    simulationWindow_->showMaximized();
    simulationWindow_->raise();
    simulationWindow_->activateWindow();
}

}  // namespace fm::ui