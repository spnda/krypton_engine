#pragma once

#include <string>
#include <vector>

#include "material.hpp"
#include "primitive.hpp"

namespace krypton::mesh {
	struct Mesh final {
		std::string name = {};
		std::vector<krypton::mesh::Primitive> primitives;
		glm::mat4x4 transform = {};
	};
}
