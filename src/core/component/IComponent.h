#pragma once

#include <cassert>

#include "core/simulation/SimulationUpdateContext.h"

namespace fm::core {

class Entity;

enum class UpdatePhase {
    Movement,
    General,
    Perception
};

class IComponent {
public:
    virtual ~IComponent() = default;

    virtual void onAttach(Entity& owner)
    {
        owner_ = &owner;
    }

    virtual void onDetach()
    {
        owner_ = nullptr;
    }

    virtual UpdatePhase phase() const
    {
        return UpdatePhase::General;
    }

    virtual void update(const SimulationUpdateContext& context) = 0;

protected:
    Entity& owner()
    {
        assert(owner_ != nullptr);
        return *owner_;
    }

    const Entity& owner() const
    {
        assert(owner_ != nullptr);
        return *owner_;
    }

private:
    Entity* owner_ {nullptr};
};

}  // namespace fm::core