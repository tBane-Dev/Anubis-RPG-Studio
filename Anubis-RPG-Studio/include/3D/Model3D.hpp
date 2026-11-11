#pragma once
#include <vector>
#include "3D/Quaternion.hpp"
#include "3D/Vertex3D.hpp"
#include "3D/Mesh3D.hpp"
#include "3D/Skeleton3D.hpp"
#include "3D/Animation3D.hpp"
#include "TexturesManager.hpp"
#include "JsonReader.hpp"

namespace Anubis {

	struct AnimationPlayer {
		int _index = -1;
		float _time = 0.f;
		float _speed = 1.f;
		bool _loop = true;
	};

	class Scene;

	class Model3D : public std::enable_shared_from_this<Model3D> {
	public:
		static int _nextID;
		int _id;
		std::wstring _name;
		sf::Vector3f _position;
		Quaternion _rotation;
		float _scale;
		float _windStrength;
		bool _isSelectable;

		Quaternion _rotationDelta;
		float _rotationSpeed;

		std::shared_ptr<Scene> _scene;
		std::vector<std::shared_ptr<Mesh3D>> _meshes;

		std::vector<Node3D> _nodes;
		std::vector<Skin3D> _skins;

		std::vector<Animation3D> _animations;
		AnimationPlayer _player;
		
		sf::Vector2f offset;
		sf::Vector2f _lastRotationMouse;

		Model3D();
		Model3D(std::wstring name, sf::Vector3f position = sf::Vector3f(0.0f, 0.0f, 0.0f), Quaternion rotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f), float scale = 1.0f);
		Model3D(const Model3D& other);
		~Model3D();

		void setScene(std::shared_ptr<Scene> scene);
		void addMesh(std::shared_ptr<Anubis::Mesh3D> mesh);

		bool playAnimation(int index, bool loop = true);
		bool updateAnimation(float deltaSeconds);
		bool applyAnimationPose();

		void handleEvent(const sf::Event& event);
		void update();
	};

	bool loadModelNodes(JsonReader& reader, Model3D& model);
	bool loadInverseBindMatrices(JsonReader& reader, std::vector<std::uint8_t>& binData, int accessorIndex, Skin3D& skin);
	bool loadModelSkins(JsonReader& reader, std::vector<std::uint8_t>& binData, Model3D& model);
	bool loadSkinAttribute(JsonReader& reader, std::vector<std::uint8_t>& binData, int accessorIndex, std::size_t vertexCount, bool joints, std::vector<std::array<float, 4>>& values);
	bool loadMeshSkinData(JsonReader& reader, std::vector<std::uint8_t>& binData, Model3D& model, Mesh3D& mesh, std::vector<Vertex3D>& vertices);
	bool uploadMeshSkinPose(Model3D& model, Mesh3D& mesh);
	bool sampleAnimationChannel(AnimationSampler3D& sampler, AnimationPath path, float time, std::array<float, 4>& result);

	struct AnimationJsonIndex {
		std::vector<int> _accessors;
		std::vector<int> _bufferViews;
	};
	bool indexJsonObjectArray(JsonReader& reader, std::string field, std::vector<int>& positions);
	bool enterIndexedJsonObject(JsonReader& reader, std::vector<int>& positions, int index);
	bool loadAnimationFloatAccessor(JsonReader& reader, std::vector<std::uint8_t>& binData, int accessorIndex, int expectedComponents, std::vector<std::array<float, 4>>& values, AnimationJsonIndex& jsonIndex);
	bool loadModelAnimations(JsonReader& reader, std::vector<std::uint8_t>& binData, Model3D& model);

	std::shared_ptr<Model3D> openModel3DFromFile(std::filesystem::path path);
}