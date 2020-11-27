#pragma once

#include <basis/ECS/ecs.hpp>

#include <cstddef>
#include <cstdint>

namespace ECS {

// Marks entity that can be re-used from memory pool (memory cache)
CREATE_ECS_TAG(UnusedTag)

// Marks entity that must be destructed
CREATE_ECS_TAG(NeedToDestroyTag)

// Marks not fully created entities.
// Can be used to detect if entity was fully constructed
// (constructed entity is entity with all required components).
/// \note Entity expected to be constructed after 1 tick
/// (constructed entity is entity that has all required components).
/// \note Make sure that not fully created entities are properly freed
/// (usually that means that they must have some relationship component
/// (like `FirstChildComponent`, `ChildSiblings` etc.)
/// that will allow them to be freed upon parent entity destruction).
/// /// \note Do not forget to skip entity updates
/// if it has `ECS::DelayedConstruction` component.
//
// MOTIVATION
//
// Assume we want to add required components using third-party plugins
// (components not known beforehand).
// We can not use entity before construction of all required components.
// One solution to that problem is event system where third-party plugins
// subscribe to custom event "eventEntityInConstruction<entityType>".
// But event-based approach needs too many events for each `entityType`.
// Usually we can process all ECS subsystems from third-party plugins
// within 1 tick, so we can assume that almost any entity can be constructed
// after 1 tick.
// Just make sure that each plugin can iterate
// `view<MyEntityTypeTag, DelayedConstruction>` and that entities that have
// `DelayedConstruction` component will be properly destroyed.
/// \note component expected to be removed after 1 tick
CREATE_ECS_TAG(DelayedConstruction)

/// \note It does NOT ALWAYS mark fully created entities because
/// that component expected to be removed after 1 tick from
/// any fully created entity.
// Can be used to perform some checks after entity construction
// (i.e. marks for 1 tick entity that was filled with required components).
CREATE_ECS_TAG(DelayedConstructionJustDone)

} // namespace ECS

ECS_DECLARE_METATYPE(UnusedTag)

ECS_DECLARE_METATYPE(NeedToDestroyTag)

ECS_DECLARE_METATYPE(DelayedConstruction)

ECS_DECLARE_METATYPE(DelayedConstructionJustDone)
