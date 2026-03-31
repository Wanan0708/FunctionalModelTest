#include "core/simulation/SimulationManager.h"

#include <stdexcept>

namespace fm::core {

SimulationManager::SimulationManager(double fixedTimeStepSeconds)
    : fixedTimeStepSeconds_(fixedTimeStepSeconds)
{
    if (fixedTimeStepSeconds_ <= 0.0) {
        throw std::invalid_argument("Fixed time step must be greater than zero.");
    }
}

double SimulationManager::fixedTimeStepSeconds() const
{
    return fixedTimeStepSeconds_;
}

void SimulationManager::setFixedTimeStepSeconds(double value)
{
    if (value <= 0.0) {
        throw std::invalid_argument("Fixed time step must be greater than zero.");
    }

    fixedTimeStepSeconds_ = value;
}

double SimulationManager::elapsedSimulationSeconds() const
{
    return elapsedSimulationSeconds_;
}

std::uint64_t SimulationManager::tickCount() const
{
    return tickCount_;
}

SimulationState SimulationManager::state() const
{
    return state_;
}

void SimulationManager::start()
{
    state_ = SimulationState::Running;
}

void SimulationManager::pause()
{
    if (state_ == SimulationState::Running) {
        state_ = SimulationState::Paused;
    }
}

void SimulationManager::stop()
{
    state_ = SimulationState::Stopped;
}

void SimulationManager::reset()
{
    elapsedSimulationSeconds_ = 0.0;
    tickCount_ = 0;
    state_ = SimulationState::Paused;
}

void SimulationManager::advanceOneTick()
{
    const SimulationUpdateContext context {
        fixedTimeStepSeconds_,
        elapsedSimulationSeconds_ + fixedTimeStepSeconds_,
        tickCount_ + 1,
        entities_,
    };

    constexpr UpdatePhase phases[] {
        UpdatePhase::Movement,
        UpdatePhase::General,
        UpdatePhase::Perception,
    };

    for (const auto phase : phases) {
        for (const auto& entity : entities_) {
            entity->update(phase, context);
        }
    }

    elapsedSimulationSeconds_ += fixedTimeStepSeconds_;
    ++tickCount_;
}

void SimulationManager::addEntity(std::shared_ptr<Entity> entity)
{
    entities_.push_back(std::move(entity));
}

void SimulationManager::clearEntities()
{
    entities_.clear();
}

const std::vector<std::shared_ptr<Entity>>& SimulationManager::entities() const
{
    return entities_;
}

}  // namespace fm::core