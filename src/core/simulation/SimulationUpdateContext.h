#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace fm::core {

class Entity;

struct SimulationUpdateContext {
    double deltaSeconds {0.0};
    double elapsedSimulationSeconds {0.0};
    std::uint64_t tickCount {0};
    const std::vector<std::shared_ptr<Entity>>& entities;
};

}  // namespace fm::core