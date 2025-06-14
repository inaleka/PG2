#pragma once

#include <glm/glm.hpp>

//vertex description
struct Vertex {
	glm::vec3 position;  // position vector
	glm::vec3 normal;    // normal vector
	glm::vec2 texcoord;  // texture coordinates
};