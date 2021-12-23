#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "vertex.hpp"

namespace krypton::mesh {
	struct Primitive final {
		std::vector<krypton::mesh::Vertex> vertices = {};
		std::vector<krypton::mesh::Index> indices = {};
		Index materialIndex = 0;
	};
}
