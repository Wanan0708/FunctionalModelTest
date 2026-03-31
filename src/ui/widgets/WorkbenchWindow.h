#pragma once

#include <QMainWindow>

namespace fm::ui {

class MainWindow;

class WorkbenchWindow final : public QMainWindow {
public:
    WorkbenchWindow();
    ~WorkbenchWindow() override;

private:
    void openScenarioEditor();
    void openSimulationWindow();

    MainWindow* scenarioEditorWindow_ {nullptr};
    MainWindow* simulationWindow_ {nullptr};
};

}  // namespace fm::ui