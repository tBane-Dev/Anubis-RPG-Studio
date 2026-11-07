#pragma once
#include <vector>
#include <SFML/OpenGL.hpp>
#include "3D/Quaternion.hpp"
#include "3D/Vertex3D.hpp"
#include "3D/Skeleton3D.hpp"
#include "TexturesManager.hpp"

namespace Anubis {

	struct MeshGeometry {
		GLuint _vertexArray = 0;
		GLuint _vertexBuffer = 0;
		GLuint _indexBuffer = 0;
		GLsizei _indexCount = 0;

		// Local-space bounding box of the vertices (for culling).
		sf::Vector3f _boundsMin = sf::Vector3f(0.f, 0.f, 0.f);
		sf::Vector3f _boundsMax = sf::Vector3f(0.f, 0.f, 0.f);
		bool _hasBounds = false;

		MeshGeometry() = default;
		MeshGeometry(const MeshGeometry&) = delete;
		MeshGeometry& operator=(const MeshGeometry&) = delete;
		~MeshGeometry();
	};

	class Mesh3D {
	public:

		std::shared_ptr<Texture> _texture;

		std::shared_ptr<MeshGeometry> _geometry;

		int _nodeIndex = -1;

		GLuint _jointBuffer = 0;
		GLuint _jointTexture = 0;
		std::size_t _gpuJointCount = 0;

		Mesh3D();
		Mesh3D(const Mesh3D& other);
		Mesh3D& operator=(const Mesh3D& other);
		~Mesh3D();

		void setTexture(std::shared_ptr<Texture> texture);
		bool uploadJointMatrices(std::vector<Matrix4>& matrices);
		void uploadToGPU(std::vector<Vertex3D>& vertices, std::vector<std::uint32_t>& indices);

		bool isOutsideView(GLfloat* modelView, GLfloat* projection);
	};

	std::shared_ptr<Mesh3D> generateCube(float size);
	std::shared_ptr<Mesh3D> generateCube(sf::Vector3f size);
}