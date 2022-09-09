#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>



namespace Rose
{


	struct Vertex
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Normal = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Tangent = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Binormal = { 0.0f, 0.0f, 0.0f };
		glm::vec2 TexCoord = { 0.0f, 0.0f };
	};

	struct Triangle
	{
		std::array<Rose::Vertex, 3> Verticies;

	};


	struct Mesh
	{
		std::vector<Vertex> Verticies;
		std::vector<uint32_t> Indicies;

	};

}