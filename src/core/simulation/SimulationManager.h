#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "core/entity/Entity.h"

namespace fm::core {

enum class SimulationState {
    Stopped,
    Running,
    Paused
};

class SimulationManager {
public:
    explicit SimulationManager(double fixedTimeStepSeconds = 0.05);

    double fixedTimeStepSeconds() const;
    void setFixedTimeStepSeconds(double value);

    double elapsedSimulationSeconds() const;
    std::uint64_t tickCount() const;
    SimulationState state() const;

    void start();
    void pause();
    void stop();
    void reset();
    void advanceOneTick();

    void addEntity(std::shared_ptr<Entity> entity);
    void clearEntities();

    const std::vector<std::shared_ptr<Entity>>& entities() const;

private:
    double fixedTimeStepSeconds_ {0.01};
    double elapsedSimulationSeconds_ {0.0};
    std::uint64_t tickCount_ {0};
    SimulationState state_ {SimulationState::Stopped};
    std::vector<std::shared_ptr<Entity>> entities_;
};

}  // namespace fm::core