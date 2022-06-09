#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>



namespace Rose
{


	struct Vertex
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
	};

	struct Triangle
	{
		std::array<Rose::Vertex, 3> Verticies;

	};


	struct Mesh
	{
		std::vector<Triangle> Triangles;
		std::vector<uint32_t> Indicies;

	};

}