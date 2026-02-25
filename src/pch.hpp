#pragma once

#ifndef STBI_NO_SIMD
#define STBI_NO_SIMD
#endif

// External dependencies that change rarely
#include <flecs.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float2.hpp>
#include <box2d/box2d.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
