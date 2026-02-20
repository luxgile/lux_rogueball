
#pragma once

#include "flecs.h"
#include "glm/ext/vector_float2.hpp"

struct common_module {
  common_module(flecs::world &world);
};

template <typename Event, typename Component>
static auto emit_event(flecs::entity e, Event evnt) -> void {
  e.world().event<Event>().template id<Component>().entity(e).ctx(evnt).emit();
}
