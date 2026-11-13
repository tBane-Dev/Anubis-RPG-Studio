#include "3D/Model3D.hpp"
#include "3D/Scene.hpp"
#include "Time.hpp"
#include "Cursor.hpp"
#include "BinaryReader.hpp"
#include "DebugLog.hpp"
#include "3D/Skeleton3D.hpp"
#include "JsonReader.hpp"
#include <unordered_set>
#include <set>
#include <functional>

namespace Anubis {

	int Model3D::_nextID = 0;

	Model3D::Model3D() {

		_id = _nextID++;
		_name = L"Model3D";

		_position = sf::Vector3f(0, 0, 0);
		_rotation = Quaternion{ 1.0f, 0, 0, 0 };
		_scale = 1.0f;
		_windStrength = 0.f;
		_isSelectable = false;
		_isAnimating = true;

		_meshes.clear();

		offset = sf::Vector2f(0.0f, 0.0f);
		_lastRotationMouse = sf::Vector2f(0.0f, 0.0f);
	}


	Model3D::Model3D(std::wstring name, sf::Vector3f position, Quaternion rotation, float scale) {

		_id = _nextID++;
		_name = name;

		_position = position;
		_rotation = rotation;
		_scale = scale;
		_windStrength = 0.0f;
		_isSelectable = false;
		_isAnimating = true;

		_meshes.clear();

		offset = sf::Vector2f(0.0f, 0.0f);
		_lastRotationMouse = sf::Vector2f(0.0f, 0.0f);
	}

	Model3D::Model3D(const Model3D& other) {

		_id = _nextID++;
		_name = other._name;

		_position = other._position;
		_rotation = other._rotation;
		_scale = other._scale;
		_windStrength = other._windStrength;
		_isSelectable = other._isSelectable;
		_isAnimating = true;

		_rotationDelta = other._rotationDelta;
		_rotationSpeed = other._rotationSpeed;

		_meshes.reserve(other._meshes.size());
		// Mesh3D copy constructor shares the prefab geometry
		for (auto& src : other._meshes)
			_meshes.push_back(std::make_shared<Mesh3D>(*src));

		_nodes = other._nodes;
		_skins = other._skins;

		_animations = other._animations;
		_player = other._player;

		offset = other.offset;
		_lastRotationMouse = other._lastRotationMouse;
	}

	Model3D::~Model3D() {

	}

	void Model3D::setScene(std::shared_ptr<Scene> scene) {
		_scene = scene;
	}

	void Model3D::addMesh(std::shared_ptr<Mesh3D> mesh) {
		_meshes.push_back(mesh);
	}

	bool Model3D::applyAnimationPose() {
		if (_player._index < 0 || (std::size_t)_player._index >= _animations.size()) {
			DebugError(L"Animation index is out of range");
			return false;
		}

		Animation3D& animation = _animations[_player._index];

		// Restore base transforms for properties without animation channels.
		for (auto& node : _nodes) {
			node._poseTranslation = node._translation;
			node._poseRotation = node._rotation;
			node._poseScale = node._scale;
		}

		for (auto& channel : animation._channels) {
			if (channel._nodeIndex < 0 ||
				(std::size_t)channel._nodeIndex >= _nodes.size() ||
				channel._samplerIndex < 0 ||
				(std::size_t)channel._samplerIndex >=
				animation._samplers.size()) {
				DebugError(L"Animation channel index is out of range");
				return false;
			}

			Node3D& node = _nodes[channel._nodeIndex];
			AnimationSampler3D& sampler = animation._samplers[channel._samplerIndex];

			std::array<float, 4> value{};

			if (!sampleAnimationChannel(sampler, channel._path, _player._time, value))
				return false;

			// glTF stores quaternion components in XYZW order.
			if (channel._path == AnimationPath::Translation) node._poseTranslation = { value[0], value[1], value[2] };
			else if (channel._path == AnimationPath::Rotation) node._poseRotation = Quaternion{ value[3], value[0], value[1], value[2] };
			else if (channel._path == AnimationPath::Scale) node._poseScale = { value[0], value[1], value[2] };
		}

		// Build local matrices using T * R * S.
		for (auto& node : _nodes) {
			if (node._hasMatrix) {
				node._localMatrix = node._matrix;
				continue;
			}

			Matrix4 matrix = node._poseRotation.matrix();

			for (int row = 0; row < 3; row++) {
				matrix[row] *= node._poseScale[0];
				matrix[4 + row] *= node._poseScale[1];
				matrix[8 + row] *= node._poseScale[2];
			}

			matrix[12] = node._poseTranslation[0];
			matrix[13] = node._poseTranslation[1];
			matrix[14] = node._poseTranslation[2];

			node._localMatrix = matrix;
		}

		// The file has already validated the hierarchy.
		// Process parents before their children.
		std::vector<int> pending;
		pending.reserve(_nodes.size());

		for (std::size_t i = 0; i < _nodes.size(); i++) {
			if (_nodes[i]._parent == -1)
				pending.push_back((int)i);
		}

		for (std::size_t i = 0; i < pending.size(); i++) {
			Node3D& node = _nodes[pending[i]];

			if (node._parent == -1)
				node._globalMatrix = node._localMatrix;
			else {
				Matrix4& parent = _nodes[node._parent]._globalMatrix;
				Matrix4& local = node._localMatrix;

				Matrix4 global{};

				for (int column = 0; column < 4; column++) {
					for (int row = 0; row < 4; row++) {
						for (int k = 0; k < 4; k++) {
							global[column * 4 + row] +=
								parent[k * 4 + row] *
								local[column * 4 + k];
						}
					}
				}

				node._globalMatrix = global;
			}

			for (int child : node._children)
				pending.push_back(child);
		}

		for (auto& mesh : _meshes) {
			if (!mesh)
				continue;

			// Generated meshes may have no glTF node.
			if (mesh->_nodeIndex < 0)
				continue;

			if (!uploadMeshSkinPose(*this, *mesh))
				return false;
		}

		return true;
	}

	bool Model3D::playAnimation(int index, bool loop) {
		if (index < 0 || (std::size_t)index >= _animations.size()) {
			DebugError(L"Animation index is out of range");
			return false;
		}

		_player._index = index;
		_player._time = 0.f;
		_player._loop = loop;

		// Apply the starting pose immediately.
		if (!applyAnimationPose()) {
			_player._index = -1;
			return false;
		}

		return true;
	}

	bool Model3D::updateAnimation(float deltaSeconds) {

		if (_player._index < 0)
			return true;

		if ((std::size_t)_player._index >= _animations.size()) {
			DebugError(L"Animation index is out of range");
			return false;
		}

		if (!std::isfinite(deltaSeconds) || !std::isfinite(_player._speed) || !std::isfinite(_player._time)) {
			DebugError(L"Invalid animation playback time or speed");
			return false;
		}

		float duration = _animations[_player._index]._duration;

		if (!std::isfinite(duration) || duration < 0.f) {
			DebugError(L"Invalid animation duration");
			return false;
		}

		//DebugStat(L"Animation time: " + std::to_wstring(_animationTime) + L", delta: " + std::to_wstring(deltaSeconds) + L", duration: " + std::to_wstring(duration));

		float nextTime = _player._time + deltaSeconds * _player._speed;

		if (!std::isfinite(nextTime)) {
			DebugError(L"Animation playback time overflow");
			return false;
		}

		if (duration == 0.f)
			_player._time = 0.f;
		else if (_player._loop) {
			_player._time = std::fmod(nextTime, duration);

			if (_player._time < 0.f)
				_player._time += duration;
		}
		else
			_player._time = std::clamp(nextTime, 0.f, duration);

		return applyAnimationPose();
	}

	void Model3D::handleEvent(const sf::Event& event) {

		if (!_isSelectable)
			return;

		sf::Vector2u size = Anubis::render_window->getSize();
		sf::Vector2i mouse = Anubis::cursor->_position;

		if (size.x == 0 || size.y == 0)
			return;

		std::shared_ptr<Scene> scene = _scene.lock();
		std::shared_ptr<Camera> camera = scene->_camera;
		
		float sinAngle = std::sin(scene->_cameraYAngle);
		float cosAngle = std::cos(scene->_cameraYAngle);

		float relativeX = _position.x - camera->_position.x;
		float relativeY = _position.y - camera->_position.y;
		float relativeZ = _position.z - camera->_position.z;

		float modelViewX = relativeX * cosAngle - relativeZ * sinAngle;
		float modelViewY = relativeY;

		float distance = -relativeX * sinAngle - relativeZ * cosAngle;
		float aspect = (float)size.x / (float)size.y;

		float halfHeight = distance * std::tan(30.f * 3.14159265f / 180.f);
		float halfWidth = halfHeight * aspect;

		float mouseX = (2.f * mouse.x / size.x - 1.f) * halfWidth;
		float mouseY = (1.f - 2.f * mouse.y / size.y) * halfHeight;

		std::vector<std::shared_ptr<Model3D>>::iterator hovered = std::find(scene->_hovered.begin(), scene->_hovered.end(), this->shared_from_this());
		std::vector<std::shared_ptr<Model3D>>::iterator pressed = std::find(scene->_pressed.begin(), scene->_pressed.end(), this->shared_from_this());

		if (hovered != scene->_hovered.end()) {
			if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp && mbp->button == sf::Mouse::Button::Left) {

				if (std::find(scene->_pressed.begin(), scene->_pressed.end(), this->shared_from_this()) == scene->_pressed.end())
					scene->_pressed.push_back(this->shared_from_this());
			}
		}

		pressed = std::find(scene->_pressed.begin(), scene->_pressed.end(), this->shared_from_this());

		if (pressed != scene->_pressed.end()) {
			if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp) {
				if (mbp->button == sf::Mouse::Button::Left) {
					offset.x = mouseX - modelViewX;
					offset.y = mouseY - modelViewY;
				}

				if (mbp->button == sf::Mouse::Button::Right)
					_lastRotationMouse = sf::Vector2f(mbp->position);
			}
		}

		if (pressed != scene->_pressed.end()) {
			if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && cursor->_isDragging) {
				float viewX = mouseX - offset.x;
				float viewY = mouseY - offset.y;

				_position.x = camera->_position.x + viewX * cosAngle - distance * sinAngle;
				_position.y = camera->_position.y + viewY;
				_position.z = camera->_position.z - viewX * sinAngle - distance * cosAngle;
			}
		}
	}

	void Model3D::update() {

		if (_isAnimating && !updateAnimation(Anubis::deltaTime.asSeconds())) {
			_player._index = -1;
			return;
		}

		// Play the next animation after the current one finishes.
		if (_player._index >= 0 && _player._time >= _animations[_player._index]._duration) {

			if (_player._index + 1 < _animations.size()) {
				if (!playAnimation(_player._index + 1, false)) {
					_player._index = -1;
					return;
				}
			}
			else
				_player._index = -1;
		}

		if (!_isSelectable)
			return;

		std::shared_ptr<Scene> scene = _scene.lock();

		std::vector<std::shared_ptr<Model3D>>::iterator pressed = std::find(scene->_pressed.begin(), scene->_pressed.end(), this->shared_from_this());

		if (pressed != scene->_pressed.end() && sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
			sf::Vector2f delta = sf::Vector2f(Anubis::cursor->_position) - _lastRotationMouse;
			_lastRotationMouse = sf::Vector2f(Anubis::cursor->_position);

			float dx = delta.x;
			float dy = delta.y;
			float length = std::hypot(dx, dy);
			if (length > 0.f) {
				float sensitivity = 0.5f; // degrees per pixel
				float toRadians = 3.14159265f / 180.f;

				float halfAngle = length * sensitivity * toRadians * 0.5f;
				float s = std::sin(halfAngle) / length;

				float sinAngle = std::sin(scene->_cameraYAngle);
				float cosAngle = std::cos(scene->_cameraYAngle);

				Quaternion deltaRotation{
					std::cos(halfAngle),
					dy * s * cosAngle,
					dx * s,
					-dy * s * sinAngle
				};

				_rotation = deltaRotation * _rotation;
				_rotation.normalize();
			}

			return;
		}

		// rotation
		Quaternion q = _rotationDelta;
		float length = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);

		if (length > 0.f) {
			float halfAngle = std::atan2(length, q.w) * _rotationSpeed * Anubis::deltaTime.asSeconds();
			float s = std::sin(halfAngle) / length;

			Quaternion delta{
				std::cos(halfAngle),
				q.x * s,
				q.y * s,
				q.z * s
			};

			_rotation = delta * _rotation;
			_rotation.normalize();
		}
	}

	bool loadModelNodes(JsonReader& reader, Model3D& model) {
		if (!reader.findRootField("nodes"))
			return false;

		if (!reader.expect('['))
			return false;

		std::vector<Node3D> nodes;

		// auto: one lambda for std::array<float, 3>, <float, 4> and Matrix4
		auto readFloatArray = [&](auto& values) -> bool {
			if (!reader.expect('['))
				return false;

			for (std::size_t i = 0; i < values.size(); i++) {
				if (i > 0 && !reader.expect(','))
					return false;

				if (!reader.readFloat(values[i]))
					return false;
			}

			return reader.expect(']');
		};

		if (reader.peek() == ']') {
			DebugError(L"Model node array is empty");
			return false;
		}

		while (true) {
			if (!reader.expect('{'))
				return false;

			Node3D node;
			bool hasTRS = false;

			if (reader.peek() != '}') {
				while (true) {
					if (reader.peek() != '"') {
						DebugError(L"Expected a node property name");
						return false;
					}

					std::string field = reader.readString();

					if (!reader.expect(':'))
						return false;

					if (field == "name") {
						if (reader.peek() != '"') {
							DebugError(L"Expected a node name string");
							return false;
						}

						node._name = reader.readString();
					}
					else if (field == "mesh") {
						if (!reader.readInt(node._mesh))
							return false;
					}
					else if (field == "skin") {
						if (!reader.readInt(node._skin))
							return false;
					}
					else if (field == "translation") {
						if (!readFloatArray(node._translation))
							return false;

						hasTRS = true;
					}
					else if (field == "scale") {
						if (!readFloatArray(node._scale))
							return false;

						hasTRS = true;
					}
					else if (field == "rotation") {
						std::array<float, 4> rotation{};

						if (!readFloatArray(rotation))
							return false;

						// Convert glTF XYZW order to Quaternion WXYZ.
						node._rotation = Quaternion{ rotation[3], rotation[0], rotation[1], rotation[2] };

						Quaternion q = node._rotation;
						float lengthSquared = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;

						if (std::abs(lengthSquared - 1.f) > 0.001f) {
							DebugError(L"Node rotation is not a unit quaternion");
							return false;
						}

						node._rotation.normalize();
						hasTRS = true;
					}
					else if (field == "matrix") {
						if (!readFloatArray(node._matrix))
							return false;

						node._hasMatrix = true;
					}
					else if (field == "children") {
						if (!reader.expect('['))
							return false;

						if (reader.peek() == ']') {
							DebugError(L"Node children array is empty");
							return false;
						}

						while (true) {
							int child = -1;

							if (!reader.readInt(child))
								return false;

							node._children.push_back(child);

							if (reader.peek() == ']')
								break;

							if (!reader.expect(','))
								return false;
						}

						if (!reader.expect(']'))
							return false;
					}
					else {
						// Skip properties not used by this file.
						if (!reader.skipValue())
							return false;
					}

					if (reader.peek() == '}')
						break;

					if (!reader.expect(','))
						return false;
				}
			}

			if (!reader.expect('}'))
				return false;

			if (node._hasMatrix && hasTRS) {
				DebugError(L"Node contains both matrix and TRS properties");
				return false;
			}

			if (node._skin >= 0 && node._mesh < 0) {
				DebugError(L"Node references a skin without a mesh");
				return false;
			}

			if (node._hasMatrix)
				node._localMatrix = node._matrix;
			else {
				// Build the local matrix as T * R * S.
				node._localMatrix = node._rotation.matrix();

				for (int column = 0; column < 3; column++)
					for (int row = 0; row < 3; row++)
						node._localMatrix[column * 4 + row] *= node._scale[column];

				node._localMatrix[12] = node._translation[0];
				node._localMatrix[13] = node._translation[1];
				node._localMatrix[14] = node._translation[2];
			}

			// Initialize the current pose from the default transform.
			node._poseTranslation = node._translation;
			node._poseRotation = node._rotation;
			node._poseScale = node._scale;

			nodes.push_back(std::move(node));

			if (reader.peek() == ']')
				break;

			if (!reader.expect(','))
				return false;
		}

		if (!reader.expect(']'))
			return false;

		// Build parent links and validate child indices.
		for (std::size_t parent = 0; parent < nodes.size(); parent++) {
			for (int child : nodes[parent]._children) {
				if (child < 0 || (std::size_t)child >= nodes.size()) {
					DebugError(L"Node child index is out of range");
					return false;
				}

				if ((std::size_t)child == parent) {
					DebugError(L"Node references itself as a child");
					return false;
				}

				if (nodes[child]._parent != -1) {
					DebugError(L"Node has multiple parents or duplicate child references");
					return false;
				}

				nodes[child]._parent = (int)parent;
			}
		}

		// Traverse roots first to detect cycles and compute global matrices.
		std::vector<std::size_t> pending;

		for (std::size_t i = 0; i < nodes.size(); i++) {
			if (nodes[i]._parent == -1)
				pending.push_back(i);
		}

		std::size_t visited = 0;

		while (!pending.empty()) {
			std::size_t index = pending.back();
			pending.pop_back();

			Node3D& node = nodes[index];

			if (node._parent == -1)
				node._globalMatrix = node._localMatrix;
			else {
				Matrix4& parentMatrix = nodes[node._parent]._globalMatrix;

				Matrix4 global{};

				// Multiply column-major matrices: parent * local.
				for (int column = 0; column < 4; column++) {
					for (int row = 0; row < 4; row++) {
						for (int k = 0; k < 4; k++) {
							global[column * 4 + row] +=
								parentMatrix[k * 4 + row] *
								node._localMatrix[column * 4 + k];
						}
					}
				}

				node._globalMatrix = global;
			}

			visited++;

			for (int child : node._children)
				pending.push_back((std::size_t)child);
		}

		if (visited != nodes.size()) {
			DebugError(L"Cycle detected in model node hierarchy");
			return false;
		}

		// Commit only after successful parsing and validation.
		model._nodes = std::move(nodes);

		DebugStat(L"Loaded model nodes: " + std::to_wstring(model._nodes.size()));

		return true;
	}

	bool loadInverseBindMatrices(JsonReader& reader, std::vector<std::uint8_t>& binData, int accessorIndex, Skin3D& skin) {
		if (!reader.findRootField("accessors") || !reader.enterArrayElement(accessorIndex) || !reader.expect('{'))
			return false;

		int accessorStart = reader._position;

		// Read a property from the beginning of an object.
		std::function<bool(int, std::string, int&, bool)> readIntField = [&](int objectStart, std::string name, int& value, bool required) -> bool {
			reader._position = objectStart;

			if (!reader.findField(name, required))
				return !required;

			return reader.readInt(value);
		};

		reader._position = accessorStart;

		if (reader.findField("sparse", false)) {
			DebugError(L"Sparse inverse bind matrices are not supported yet");
			return false;
		}

		int componentType = 0;
		int count = 0;
		int bufferView = -1;
		int accessorOffset = 0;

		if (!readIntField(
			accessorStart, "componentType", componentType, true) ||
			!readIntField(
				accessorStart, "count", count, true) ||
			!readIntField(
				accessorStart, "bufferView", bufferView, true) ||
			!readIntField(
				accessorStart, "byteOffset", accessorOffset, false)) {
			return false;
		}

		reader._position = accessorStart;

		if (!reader.findField("type"))
			return false;

		if (componentType != 5126 || reader.readString() != "MAT4") {
			DebugError(L"Inverse bind matrices must use FLOAT MAT4");
			return false;
		}

		if (count <= 0 || (std::size_t)count < skin._joints.size()) {
			DebugError(L"Not enough inverse bind matrices");
			return false;
		}

		if (!reader.findRootField("bufferViews") ||
			!reader.enterArrayElement(bufferView) ||
			!reader.expect('{')) {
			return false;
		}

		int viewStart = reader._position;

		int buffer = -1;
		int viewOffset = 0;
		int viewLength = 0;

		if (!readIntField(viewStart, "buffer", buffer, true) ||
			!readIntField(viewStart, "byteOffset", viewOffset, false) ||
			!readIntField(viewStart, "byteLength", viewLength, true)) {
			return false;
		}

		if (buffer != 0) {
			DebugError(L"Only the embedded GLB buffer is supported");
			return false;
		}

		reader._position = viewStart;

		if (reader.findField("byteStride", false)) {
			DebugError(L"Inverse bind matrix buffer view must not use byteStride");
			return false;
		}

		std::uint64_t matrixBytes = 16 * sizeof(float);

		std::uint64_t viewBegin = (std::uint64_t)viewOffset;
		std::uint64_t viewSize = (std::uint64_t)viewLength;
		std::uint64_t localOffset = (std::uint64_t)accessorOffset;
		std::uint64_t dataSize = (std::uint64_t)count * matrixBytes;

		if (viewBegin > binData.size() ||
			viewSize > binData.size() - viewBegin) {
			DebugError(L"Inverse bind matrix buffer view exceeds BIN bounds");
			return false;
		}

		if (localOffset > viewSize ||
			dataSize > viewSize - localOffset) {
			DebugError(L"Inverse bind matrices exceed buffer view bounds");
			return false;
		}

		if (localOffset % sizeof(float) != 0 ||
			(viewBegin + localOffset) % sizeof(float) != 0) {
			DebugError(L"Inverse bind matrices are not properly aligned");
			return false;
		}

		std::vector<Matrix4> matrices(skin._joints.size());

		for (std::size_t i = 0; i < matrices.size(); i++) {
			std::uint64_t offset = viewBegin + localOffset + i * matrixBytes;

			std::memcpy(
				matrices[i].data(),
				binData.data() + (std::size_t)offset,
				(std::size_t)matrixBytes
			);

			for (float value : matrices[i]) {
				if (!std::isfinite(value)) {
					DebugError(L"Inverse bind matrix contains a non-finite value");
					return false;
				}
			}

			Matrix4& matrix = matrices[i];

			// The last row of an affine matrix is [0, 0, 0, 1].
			if (std::abs(matrix[3]) > 0.00001f ||
				std::abs(matrix[7]) > 0.00001f ||
				std::abs(matrix[11]) > 0.00001f ||
				std::abs(matrix[15] - 1.f) > 0.00001f) {
				DebugError(L"Invalid inverse bind matrix last row");
				return false;
			}
		}

		skin._inverseBindMatrices = std::move(matrices);
		return true;
	}

	bool loadModelSkins(JsonReader& reader, std::vector<std::uint8_t>& binData, Model3D& model) {
		reader._position = 0;

		if (!reader.expect('{'))
			return false;

		// Static models may have no skins.
		if (!reader.findField("skins", false)) {
			for (auto& node : model._nodes) {
				if (node._skin >= 0) {
					DebugError(L"Node references a skin but the skins array is missing");
					return false;
				}
			}

			model._skins.clear();
			DebugStat(L"Model has no skins");
			return true;
		}

		if (!reader.expect('['))
			return false;

		if (reader.peek() == ']') {
			DebugError(L"Model skin array is empty");
			return false;
		}

		std::vector<Skin3D> skins;

		while (true) {
			if (!reader.expect('{'))
				return false;

			Skin3D skin;
			int inverseBindAccessor = -1;
			bool hasJoints = false;

			if (reader.peek() != '}') {
				while (true) {
					if (reader.peek() != '"') {
						DebugError(L"Expected a skin property name");
						return false;
					}

					std::string field = reader.readString();

					if (!reader.expect(':'))
						return false;

					if (field == "name") {
						if (reader.peek() != '"') {
							DebugError(L"Expected a skin name string");
							return false;
						}

						skin._name = reader.readString();
					}
					else if (field == "skeleton") {
						if (!reader.readInt(skin._skeletonRoot))
							return false;
					}
					else if (field == "inverseBindMatrices") {
						if (!reader.readInt(inverseBindAccessor))
							return false;
					}
					else if (field == "joints") {
						if (hasJoints) {
							DebugError(L"Duplicate skin joints property");
							return false;
						}

						hasJoints = true;

						if (!reader.expect('['))
							return false;

						if (reader.peek() == ']') {
							DebugError(L"Skin joint array is empty");
							return false;
						}

						std::unordered_set<int> uniqueJoints;

						while (true) {
							int joint = -1;

							if (!reader.readInt(joint))
								return false;

							if ((std::size_t)joint >=
								model._nodes.size()) {
								DebugError(L"Skin joint node index is out of range");
								return false;
							}

							if (!uniqueJoints.insert(joint).second) {
								DebugError(L"Skin contains a duplicate joint");
								return false;
							}

							skin._joints.push_back(joint);

							if (reader.peek() == ']')
								break;

							if (!reader.expect(','))
								return false;
						}

						if (!reader.expect(']'))
							return false;
					}
					else {
						if (!reader.skipValue())
							return false;
					}

					if (reader.peek() == '}')
						break;

					if (!reader.expect(','))
						return false;
				}
			}

			if (!reader.expect('}'))
				return false;

			if (!hasJoints) {
				DebugError(L"Skin joints property is missing");
				return false;
			}

			if (skin._skeletonRoot >= 0) {
				if ((std::size_t)skin._skeletonRoot >=
					model._nodes.size()) {
					DebugError(L"Skeleton root node index is out of range");
					return false;
				}

				// The specified skeleton root must contain every joint.
				for (int joint : skin._joints) {
					int ancestor = joint;

					while (ancestor != -1 &&
						ancestor != skin._skeletonRoot) {
						ancestor = model._nodes[ancestor]._parent;
					}

					if (ancestor == -1) {
						DebugError(L"Skeleton root is not an ancestor of every joint");
						return false;
					}
				}
			}

			// Accessor reading moves the JSON cursor to another root field.
			int nextSkinPosition = reader._position;

			if (inverseBindAccessor >= 0) {
				if (!loadInverseBindMatrices(
					reader, binData, inverseBindAccessor, skin
				)) {
					return false;
				}
			}
			else {
				skin._inverseBindMatrices.assign(
					skin._joints.size(),
					identityMatrix4()
				);
			}

			reader._position = nextSkinPosition;

			skins.push_back(std::move(skin));

			if (reader.peek() == ']')
				break;

			if (!reader.expect(','))
				return false;
		}

		if (!reader.expect(']'))
			return false;

		for (auto& node : model._nodes) {
			if (node._skin >= 0 &&
				(std::size_t)node._skin >= skins.size()) {
				DebugError(L"Node skin index is out of range");
				return false;
			}
		}

		model._skins = std::move(skins);

		DebugStat(
			L"Loaded model skins: " +
			std::to_wstring(model._skins.size())
		);

		for (std::size_t i = 0; i < model._skins.size(); i++) {
			DebugStat(
				L"Skin " + std::to_wstring(i) +
				L": joints = " +
				std::to_wstring(model._skins[i]._joints.size())
			);
		}

		return true;
	}

	bool loadSkinAttribute(JsonReader& reader, std::vector<std::uint8_t>& binData, int accessorIndex, std::size_t vertexCount, bool joints, std::vector<std::array<float, 4>>& values) {

		if (!reader.findRootField("accessors") ||
			!reader.enterArrayElement(accessorIndex) ||
			!reader.expect('{')) {
			return false;
		}

		int accessorStart = reader._position;

		std::function<bool(int, std::string, int&, bool)> readIntField = [&](int start, std::string name, int& value, bool required) -> bool {
			reader._position = start;

			if (!reader.findField(name, required))
				return !required;

			return reader.readInt(value);
		};

		reader._position = accessorStart;

		if (reader.findField("sparse", false)) {
			DebugError(L"Sparse skin attributes are not supported yet");
			return false;
		}

		int count = 0;
		int componentType = 0;
		int bufferView = -1;
		int accessorOffset = 0;

		if (!readIntField(accessorStart, "count", count, true) ||
			!readIntField(accessorStart, "componentType", componentType, true) ||
			!readIntField(accessorStart, "bufferView", bufferView, true) ||
			!readIntField(accessorStart, "byteOffset", accessorOffset, false)) {
			return false;
		}

		if (count <= 0 || (std::size_t)count != vertexCount) {
			DebugError(L"Skin attribute count does not match vertex count");
			return false;
		}

		reader._position = accessorStart;

		if (!reader.findField("type"))
			return false;

		if (reader.readString() != "VEC4") {
			DebugError(L"Skin attribute must use VEC4");
			return false;
		}

		bool normalized = false;
		reader._position = accessorStart;

		if (reader.findField("normalized", false)) {
			reader.skipWhitespace();
			int start = reader._position;

			if (!reader.skipValue())
				return false;

			std::string token = reader._text.substr(start, reader._position - start);

			if (token == "true")
				normalized = true;
			else if (token != "false") {
				DebugError(L"Invalid normalized property");
				return false;
			}
		}

		int componentBytes = 0;

		if (componentType == 5121) componentBytes = 1;
		else if (componentType == 5123) componentBytes = 2;
		else if (componentType == 5126) componentBytes = 4;
		else {
			DebugError(L"Unsupported skin attribute component type");
			return false;
		}

		if (joints) {
			if (componentType == 5126 || normalized) {
				DebugError(L"Joint indices must use non-normalized unsigned integers");
				return false;
			}
		}
		else {
			if ((componentType == 5126 && normalized) ||
				(componentType != 5126 && !normalized)) {
				DebugError(L"Weights must use floats or normalized unsigned integers");
				return false;
			}
		}

		if (!reader.findRootField("bufferViews") ||
			!reader.enterArrayElement(bufferView) ||
			!reader.expect('{')) {
			return false;
		}

		int viewStart = reader._position;

		int buffer = -1;
		int viewOffset = 0;
		int viewLength = 0;
		int stride = componentBytes * 4;

		if (!readIntField(viewStart, "buffer", buffer, true) ||
			!readIntField(viewStart, "byteOffset", viewOffset, false) ||
			!readIntField(viewStart, "byteLength", viewLength, true) ||
			!readIntField(viewStart, "byteStride", stride, false)) {
			return false;
		}

		if (buffer != 0) {
			DebugError(L"Only the embedded GLB buffer is supported");
			return false;
		}

		int elementBytes = componentBytes * 4;

		if (stride < elementBytes || stride > 252 || stride % 4 != 0) {
			DebugError(L"Invalid skin attribute byte stride");
			return false;
		}

		std::uint64_t viewBegin = (std::uint64_t)viewOffset;
		std::uint64_t viewSize = (std::uint64_t)viewLength;
		std::uint64_t localOffset = (std::uint64_t)accessorOffset;

		std::uint64_t requiredBytes = std::uint64_t(count - 1) * stride + elementBytes;

		if (viewBegin > binData.size() ||
			viewSize > binData.size() - viewBegin) {
			DebugError(L"Skin attribute buffer view exceeds BIN bounds");
			return false;
		}

		if (localOffset > viewSize ||
			requiredBytes > viewSize - localOffset) {
			DebugError(L"Skin attribute exceeds buffer view bounds");
			return false;
		}

		if (localOffset % 4 != 0 ||
			(viewBegin + localOffset) % componentBytes != 0) {
			DebugError(L"Skin attribute is not properly aligned");
			return false;
		}

		values.resize(vertexCount);

		for (std::size_t i = 0; i < vertexCount; i++) {
			std::uint64_t offset = viewBegin + localOffset +
				(std::uint64_t)i * stride;

			for (int component = 0; component < 4; component++) {
				std::uint8_t* source = binData.data() + std::size_t(offset + component * componentBytes);

				float value = 0.f;

				if (componentType == 5121) {
					value = (float)*source;

					if (normalized)
						value /= 255.f;
				}
				else if (componentType == 5123) {
					// Decode the little-endian unsigned short.
					std::uint16_t integer =
						(std::uint16_t)source[0] |
						std::uint16_t((std::uint16_t)source[1] << 8);

					value = (float)integer;

					if (normalized)
						value /= 65535.f;
				}
				else {
					// The current target platform uses little-endian floats.
					std::memcpy(&value, source, sizeof(value));
				}

				if (!std::isfinite(value) || value < 0.f) {
					DebugError(L"Invalid skin attribute value");
					return false;
				}

				values[i][component] = value;
			}
		}

		return true;
	}

	bool loadMeshSkinData(JsonReader& reader, std::vector<std::uint8_t>& binData, Model3D& model, Mesh3D& mesh, std::vector<Vertex3D>& vertices) {
		// The current file reads mesh 0, primitive 0.
		int nodeIndex = -1;

		for (std::size_t i = 0; i < model._nodes.size(); i++) {
			if (model._nodes[i]._mesh != 0)
				continue;

			if (nodeIndex != -1) {
				DebugError(L"Multiple instances of mesh 0 are not supported yet");
				return false;
			}

			nodeIndex = (int)i;
		}

		if (nodeIndex == -1) {
			DebugError(L"No node references mesh 0");
			return false;
		}

		Node3D& node = model._nodes[nodeIndex];

		if (node._skin == -1) {
			mesh._nodeIndex = nodeIndex;
			DebugStat(L"Mesh has no skin");
			return true;
		}

		if (node._skin < 0 ||
			(std::size_t)node._skin >= model._skins.size()) {
			DebugError(L"Mesh skin index is out of range");
			return false;
		}

		Skin3D& skin = model._skins[node._skin];

		if (!reader.findRootField("meshes") ||
			!reader.enterArrayElement(0) ||
			!reader.expect('{') ||
			!reader.findField("primitives") ||
			!reader.enterArrayElement(0) ||
			!reader.expect('{') ||
			!reader.findField("attributes") ||
			!reader.expect('{')) {
			return false;
		}

		int attributesStart = reader._position;

		// Reject additional influence sets instead of silently losing weights.
		for (std::string field : { "JOINTS_1", "WEIGHTS_1" }) {
			reader._position = attributesStart;

			if (reader.findField(field, false)) {
				DebugError(L"More than four joint influences per vertex are not supported yet");
				return false;
			}
		}

		int jointsAccessor = -1;
		int weightsAccessor = -1;

		reader._position = attributesStart;

		if (!reader.findField("JOINTS_0") ||
			!reader.readInt(jointsAccessor)) {
			return false;
		}

		reader._position = attributesStart;

		if (!reader.findField("WEIGHTS_0") ||
			!reader.readInt(weightsAccessor)) {
			return false;
		}

		std::vector<std::array<float, 4>> jointValues;
		std::vector<std::array<float, 4>> weightValues;

		if (!loadSkinAttribute(
			reader, binData, jointsAccessor,
			vertices.size(), true, jointValues
		)) {
			return false;
		}

		if (!loadSkinAttribute(
			reader, binData, weightsAccessor,
			vertices.size(), false, weightValues
		)) {
			return false;
		}

		// Validate all vertices before modifying the mesh.
		for (std::size_t i = 0; i < vertices.size(); i++) {
			float totalWeight = 0.f;

			for (int j = 0; j < 4; j++) {
				std::uint16_t joint = (std::uint16_t)jointValues[i][j];

				if (joint >= skin._joints.size()) {
					DebugError(L"Vertex joint index exceeds skin joint count");
					return false;
				}

				totalWeight += weightValues[i][j];

				if (weightValues[i][j] > 0.f) {
					for (int previous = 0; previous < j; previous++) {
						if (weightValues[i][previous] > 0.f &&
							jointValues[i][previous] == jointValues[i][j]) {
							DebugError(L"Vertex has duplicate active joint influences");
							return false;
						}
					}
				}
			}

			if (totalWeight <= 0.f || std::abs(totalWeight - 1.f) > 0.01f) {
				DebugError(L"Vertex weights do not sum approximately to one");
				return false;
			}

			// Correct small rounding errors.
			for (float& weight : weightValues[i])
				weight = weight / totalWeight;
		}

		for (std::size_t i = 0; i < vertices.size(); i++) {
			Vertex3D& vertex = vertices[i];

			for (int j = 0; j < 4; j++) {
				vertex._joints[j] = (std::uint16_t)jointValues[i][j];

				vertex._weights[j] = weightValues[i][j];
			}
		}

		mesh._nodeIndex = nodeIndex;

		DebugStat(L"Mesh node index: " + std::to_wstring(nodeIndex));
		DebugStat(L"Mesh skin index: " + std::to_wstring(node._skin));
		DebugStat(L"Joints accessor: " + std::to_wstring(jointsAccessor));
		DebugStat(L"Weights accessor: " + std::to_wstring(weightsAccessor));

		DebugStat(L"Loaded skin data for vertices: " + std::to_wstring(vertices.size()));

		return true;
	}

	bool uploadMeshSkinPose(Model3D& model, Mesh3D& mesh) {
		if (mesh._nodeIndex < 0 || (std::size_t)mesh._nodeIndex >= model._nodes.size()) {
			DebugError(L"Mesh node index is out of range");
			return false;
		}

		Node3D& node = model._nodes[mesh._nodeIndex];

		if (node._skin == -1)
			return true;

		if (node._skin < 0 || (std::size_t)node._skin >= model._skins.size()) {
			DebugError(L"Mesh skin index is out of range");
			return false;
		}

		Skin3D& skin = model._skins[node._skin];

		if (skin._inverseBindMatrices.size() != skin._joints.size()) {
			DebugError(L"Inverse bind matrix count does not match joint count");
			return false;
		}

		std::vector<Matrix4> matrices(skin._joints.size());

		for (std::size_t i = 0; i < skin._joints.size(); i++) {
			int jointNode = skin._joints[i];

			if (jointNode < 0 || (std::size_t)jointNode >= model._nodes.size()) {
				DebugError(L"Joint node index is out of range");
				return false;
			}

			Matrix4& global = model._nodes[jointNode]._globalMatrix;
			Matrix4& inverseBind = skin._inverseBindMatrices[i];

			Matrix4 result{};

			// Transform bind-space vertices into model space.
			for (int column = 0; column < 4; column++) {
				for (int row = 0; row < 4; row++) {
					for (int k = 0; k < 4; k++) {
						result[column * 4 + row] +=
							global[k * 4 + row] *
							inverseBind[column * 4 + k];
					}
				}
			}

			matrices[i] = result;
		}

		return mesh.uploadJointMatrices(matrices);
	}

	bool sampleAnimationChannel(AnimationSampler3D& sampler, AnimationPath path, float time, std::array<float, 4>& result) {
		std::vector<float>& times = sampler._times;
		std::vector<std::array<float, 4>>& values = sampler._values;

		bool cubic = sampler._interpolation == AnimationInterpolation::CubicSpline;

		std::size_t multiplier = cubic ? 3 : 1;

		if (times.empty() ||
			values.size() / multiplier != times.size() ||
			values.size() % multiplier != 0) {
			DebugError(L"Invalid animation sampler data");
			return false;
		}

		std::function<std::array<float, 4>(std::size_t)> valueAt = [&](std::size_t key) {
			return values[cubic ? key * 3 + 1 : key];
		};

		// Clamp samples outside the channel's time range.
		if (time <= times.front())
			result = valueAt(0);
		else if (time >= times.back())
			result = valueAt(times.size() - 1);
		else {
			std::vector<float>::iterator next = std::upper_bound(times.begin(), times.end(), time);
			std::size_t right = std::size_t(next - times.begin());
			std::size_t left = right - 1;

			float duration = times[right] - times[left];
			float t = (time - times[left]) / duration;

			std::array<float, 4> a = valueAt(left);
			std::array<float, 4> b = valueAt(right);

			if (sampler._interpolation == AnimationInterpolation::Step)
				result = a;
			else if (cubic) {
				float t2 = t * t;
				float t3 = t2 * t;

				float h00 = 2.f * t3 - 3.f * t2 + 1.f;
				float h10 = t3 - 2.f * t2 + t;
				float h01 = -2.f * t3 + 3.f * t2;
				float h11 = t3 - t2;

				std::array<float, 4> outgoing = values[left * 3 + 2];
				std::array<float, 4> incoming = values[right * 3];

				// Spline tangents are scaled by the segment duration.
				for (std::size_t c = 0; c < 4; c++)
					result[c] = h00 * a[c] + h10 * duration * outgoing[c] + h01 * b[c] + h11 * duration * incoming[c];
			}
			else if (path == AnimationPath::Rotation) {
				// Normalize endpoints before spherical interpolation.
				float lengthA = 0.f;
				float lengthB = 0.f;

				for (std::size_t c = 0; c < 4; c++) {
					lengthA += a[c] * a[c];
					lengthB += b[c] * b[c];
				}

				if (!(lengthA > 0.f) || !(lengthB > 0.f)) {
					DebugError(L"Invalid animation rotation endpoint");
					return false;
				}

				lengthA = std::sqrt(lengthA);
				lengthB = std::sqrt(lengthB);

				float dot = 0.f;

				for (std::size_t c = 0; c < 4; c++) {
					a[c] /= lengthA;
					b[c] /= lengthB;
					dot += a[c] * b[c];
				}

				// Follow the shortest rotation path.
				if (dot < 0.f) {
					dot = -dot;

					for (float& value : b)
						value = -value;
				}

				dot = std::clamp(dot, 0.f, 1.f);

				float factorA = 1.f - t;
				float factorB = t;

				if (dot < 0.9995f) {
					float angle = std::acos(dot);
					float denominator = std::sin(angle);

					factorA = std::sin((1.f - t) * angle) / denominator;
					factorB = std::sin(t * angle) / denominator;
				}

				for (std::size_t c = 0; c < 4; c++)
					result[c] = factorA * a[c] + factorB * b[c];
			}
			else {
				for (std::size_t c = 0; c < 4; c++)
					result[c] = (1.f - t) * a[c] + t * b[c];
			}
		}

		for (float value : result) {
			if (!std::isfinite(value)) {
				DebugError(L"Animation sample contains a non-finite value");
				return false;
			}
		}

		if (path == AnimationPath::Rotation) {
			float lengthSquared = 0.f;

			for (float value : result)
				lengthSquared += value * value;

			if (lengthSquared <= 0.f) {
				DebugError(L"Animation produced a zero quaternion");
				return false;
			}

			float inverseLength = 1.f / std::sqrt(lengthSquared);

			for (float& value : result)
				value = value * inverseLength;
		}

		return true;
	}

	bool indexJsonObjectArray(JsonReader& reader, std::string field, std::vector<int>& positions) {
		if (!reader.findRootField(field) || !reader.expect('['))
			return false;

		positions.clear();

		if (reader.peek() == ']')
			return reader.expect(']');

		while (true) {
			if (reader.peek() != '{') {
				DebugError(L"Expected an object while indexing JSON");
				return false;
			}

			// Store the position of the opening brace.
			positions.push_back(reader._position);

			if (!reader.skipValue())
				return false;

			if (reader.peek() == ']')
				return reader.expect(']');

			if (!reader.expect(','))
				return false;
		}
	}

	bool enterIndexedJsonObject(JsonReader& reader, std::vector<int>& positions, int index) {
		if (index < 0 || (std::size_t)index >= positions.size()) {
			DebugError(L"JSON object index is out of range");
			return false;
		}

		reader._position = positions[index];
		return reader.expect('{');
	}

	bool loadAnimationFloatAccessor(JsonReader& reader, std::vector<std::uint8_t>& binData, int accessorIndex, int expectedComponents, std::vector<std::array<float, 4>>& values, AnimationJsonIndex& jsonIndex) {
		if (expectedComponents != 1 && expectedComponents != 3 && expectedComponents != 4) {
			DebugError(L"Invalid animation component count");
			return false;
		}

		if (!enterIndexedJsonObject(reader, jsonIndex._accessors, accessorIndex))
			return false;

		int accessorStart = reader._position;

		std::function<bool(int, std::string, int&, bool)> readIntField = [&](int start, std::string name, int& value, bool required) -> bool {
			reader._position = start;
			if (!reader.findField(name, required))
				return !required;

			return reader.readInt(value);
		};

		reader._position = accessorStart;

		if (reader.findField("sparse", false)) {
			DebugError(L"Sparse animation accessors are not supported yet");
			return false;
		}

		int componentType = 0;
		int count = 0;
		int bufferView = -1;
		int accessorOffset = 0;

		if (!readIntField(accessorStart, "componentType", componentType, true) || !readIntField(accessorStart, "count", count, true) || !readIntField(accessorStart, "bufferView", bufferView, true) || !readIntField(accessorStart, "byteOffset", accessorOffset, false))
			return false;

		if (componentType != 5126) {
			DebugError(L"Only FLOAT animation accessors are supported here");
			return false;
		}

		if (count <= 0) {
			DebugError(L"Animation accessor is empty");
			return false;
		}

		reader._position = accessorStart;

		if (reader.findField("normalized", false)) {
			reader.skipWhitespace();
			int start = reader._position;

			if (!reader.skipValue())
				return false;

			std::string token = reader._text.substr(start, reader._position - start);

			if (token != "false") {
				DebugError(L"FLOAT animation accessor must not be normalized");
				return false;
			}
		}

		reader._position = accessorStart;

		if (!reader.findField("type"))
			return false;

		std::string type = reader.readString();

		std::string expectedType =
			expectedComponents == 1 ? "SCALAR" :
			expectedComponents == 3 ? "VEC3" : "VEC4";

		if (type != expectedType) {
			DebugError(L"Unexpected animation accessor type");
			return false;
		}

		if (!enterIndexedJsonObject(reader, jsonIndex._bufferViews, bufferView))
			return false;

		int viewStart = reader._position;

		int buffer = -1;
		int viewOffset = 0;
		int viewLength = 0;

		if (!readIntField(viewStart, "buffer", buffer, true) || !readIntField(viewStart, "byteOffset", viewOffset, false) || !readIntField(viewStart, "byteLength", viewLength, true))
			return false;

		if (buffer != 0) {
			DebugError(L"Only the embedded GLB buffer is supported");
			return false;
		}

		reader._position = viewStart;

		if (reader.findField("byteStride", false)) {
			DebugError(L"Animation buffer view must not use byteStride");
			return false;
		}

		std::uint64_t elementBytes = (std::uint64_t)expectedComponents * sizeof(float);

		std::uint64_t dataBytes = (std::uint64_t)count * elementBytes;

		std::uint64_t viewBegin = (std::uint64_t)viewOffset;
		std::uint64_t viewSize = (std::uint64_t)viewLength;
		std::uint64_t localOffset = (std::uint64_t)accessorOffset;

		if (viewBegin > binData.size() || viewSize > binData.size() - viewBegin) {
			DebugError(L"Animation buffer view exceeds BIN bounds");
			return false;
		}

		if (localOffset > viewSize || dataBytes > viewSize - localOffset) {
			DebugError(L"Animation accessor exceeds buffer view bounds");
			return false;
		}

		if (localOffset % sizeof(float) != 0 || (viewBegin + localOffset) % sizeof(float) != 0) {
			DebugError(L"Animation accessor is not properly aligned");
			return false;
		}

		std::vector<std::array<float, 4>> loaded((std::size_t)count, std::array<float, 4>{});

		for (std::size_t i = 0; i < loaded.size(); i++) {
			std::uint64_t offset = viewBegin + localOffset + i * elementBytes;

			// The current target platform uses little-endian floats.
			std::memcpy(loaded[i].data(), binData.data() + (std::size_t)offset, (std::size_t)elementBytes);

			for (int component = 0; component < expectedComponents; component++) {
				if (!std::isfinite(loaded[i][component])) {
					DebugError(L"Animation data contains a non-finite value");
					return false;
				}
			}
		}

		// Commit only after successful validation.
		values = std::move(loaded);
		return true;
	}

	bool loadModelAnimations(JsonReader& reader, std::vector<std::uint8_t>& binData, Model3D& model) {
		// Store object positions to allow independent field lookup.
		std::function<bool(std::vector<int>&)> readObjectArray = [&](std::vector<int>& positions) -> bool {
			if (!reader.expect('['))
				return false;

			positions.clear();

			if (reader.peek() == ']')
				return reader.expect(']');

			while (true) {
				if (reader.peek() != '{') {
					DebugError(L"Expected an object in animation array");
					return false;
				}

				positions.push_back(reader._position);

				if (!reader.skipValue())
					return false;

				if (reader.peek() == ']')
					return reader.expect(']');

				if (!reader.expect(','))
					return false;
			}
		};

		std::function<bool(int, std::string, bool)> findObjectField = [&](int objectPosition, std::string name, bool required) -> bool {
			reader._position = objectPosition;

			if (!reader.expect('{'))
				return false;

			return reader.findField(name, required);
		};

		std::function<bool(int, std::string, int&)> readRequiredInt = [&](int objectPosition, std::string name, int& value) -> bool {
			if (!findObjectField(objectPosition, name, true))
				return false;

			return reader.readInt(value);
		};

		reader._position = 0;

		if (!reader.expect('{'))
			return false;

		if (!reader.findField("animations", false)) {
			model._animations.clear();
			model._player._index = -1;
			model._player._time = 0.f;

			DebugStat(L"Model has no animations");
			return true;
		}

		std::vector<int> animationPositions;

		if (!readObjectArray(animationPositions))
			return false;

		if (animationPositions.empty()) {
			DebugError(L"Animation array is empty");
			return false;
		}

		AnimationJsonIndex jsonIndex;

		if (!indexJsonObjectArray(reader, "accessors", jsonIndex._accessors))
			return false;

		if (!indexJsonObjectArray(reader, "bufferViews", jsonIndex._bufferViews))
			return false;

		DebugStat(L"Indexed animation accessors: " + std::to_wstring(jsonIndex._accessors.size()));
		DebugStat(L"Indexed animation buffer views: " + std::to_wstring(jsonIndex._bufferViews.size()));

		std::vector<Animation3D> animations;
		animations.reserve(animationPositions.size());

		for (int animationPosition : animationPositions) {
			Animation3D animation;

			if (findObjectField(animationPosition, "name", false)) {
				if (reader.peek() != '"') {
					DebugError(L"Expected an animation name string");
					return false;
				}

				animation._name = reader.readString();
			}

			std::vector<int> samplerPositions;
			std::vector<int> channelPositions;

			if (!findObjectField(animationPosition, "samplers", true) || !readObjectArray(samplerPositions))
				return false;

			if (!findObjectField(animationPosition, "channels", true) || !readObjectArray(channelPositions))
				return false;

			if (samplerPositions.empty() || channelPositions.empty()) {
				DebugError(L"Animation must contain samplers and channels");
				return false;
			}

			// Record the output shape required by each sampler's channels.
			std::vector<int> samplerComponents(samplerPositions.size(), 0);
			std::set<std::pair<int, AnimationPath>> targets;

			for (int channelPosition : channelPositions) {
				AnimationChannel3D channel;

				if (!readRequiredInt(
					channelPosition, "sampler", channel._samplerIndex
				)) {
					return false;
				}

				if ((std::size_t)channel._samplerIndex >=
					samplerPositions.size()) {
					DebugError(L"Animation sampler index is out of range");
					return false;
				}

				if (!findObjectField(channelPosition, "target", true))
					return false;

				if (reader.peek() != '{') {
					DebugError(L"Expected an animation target object");
					return false;
				}

				int targetPosition = reader._position;

				if (!readRequiredInt(targetPosition, "node", channel._nodeIndex))
					return false;

				if ((std::size_t)channel._nodeIndex >= model._nodes.size()) {
					DebugError(L"Animation target node index is out of range");
					return false;
				}

				if (model._nodes[channel._nodeIndex]._hasMatrix) {
					DebugError(L"Animated node must use TRS instead of a matrix");
					return false;
				}

				if (!findObjectField(targetPosition, "path", true))
					return false;

				std::string path = reader.readString();
				int components = 0;

				if (path == "translation") {
					channel._path = AnimationPath::Translation;
					components = 3;
				}
				else if (path == "rotation") {
					channel._path = AnimationPath::Rotation;
					components = 4;
				}
				else if (path == "scale") {
					channel._path = AnimationPath::Scale;
					components = 3;
				}
				else {
					DebugError(L"Unsupported animation target path");
					return false;
				}

				if (!targets.emplace(channel._nodeIndex, channel._path).second) {
					DebugError(L"Duplicate animation target");
					return false;
				}

				int& expected = samplerComponents[channel._samplerIndex];

				if (expected != 0 && expected != components) {
					DebugError(L"Animation sampler is used with incompatible paths");
					return false;
				}

				expected = components;
				animation._channels.push_back(channel);
			}

			animation._samplers.reserve(samplerPositions.size());

			for (std::size_t i = 0; i < samplerPositions.size(); i++) {
				int samplerPosition = samplerPositions[i];
				AnimationSampler3D sampler;

				int inputAccessor = -1;
				int outputAccessor = -1;

				if (!readRequiredInt(samplerPosition, "input", inputAccessor) || !readRequiredInt(samplerPosition, "output", outputAccessor))
					return false;

				if (findObjectField(samplerPosition, "interpolation", false)) {
					std::string interpolation = reader.readString();

					if (interpolation == "LINEAR") sampler._interpolation = AnimationInterpolation::Linear;
					else if (interpolation == "STEP") sampler._interpolation = AnimationInterpolation::Step;
					else if (interpolation == "CUBICSPLINE") sampler._interpolation = AnimationInterpolation::CubicSpline;
					else {
						DebugError(L"Unsupported animation interpolation");
						return false;
					}
				}

				std::vector<std::array<float, 4>> times;

				if (!loadAnimationFloatAccessor(reader, binData, inputAccessor, 1, times, jsonIndex))
					return false;

				sampler._times.reserve(times.size());

				for (std::size_t key = 0; key < times.size(); key++) {
					float time = times[key][0];

					if (time < 0.f ||
						(key > 0 && time <= times[key - 1][0])) {
						DebugError(L"Animation times must be non-negative and strictly increasing");
						return false;
					}

					sampler._times.push_back(time);
				}

				int components = samplerComponents[i];

				// Unused samplers still retain their array indices.
				if (components == 0) {
					if (!enterIndexedJsonObject(reader, jsonIndex._accessors, outputAccessor))
						return false;

					if (!reader.findField("type"))
						return false;

					std::string type = reader.readString();

					if (type == "SCALAR")
						components = 1;
					else if (type == "VEC3")
						components = 3;
					else if (type == "VEC4")
						components = 4;
					else {
						DebugError(L"Unsupported unused animation sampler type");
						return false;
					}
				}

				if (!loadAnimationFloatAccessor(reader, binData, outputAccessor, components, sampler._values, jsonIndex))
					return false;

				bool cubic = sampler._interpolation == AnimationInterpolation::CubicSpline;

				if (cubic && sampler._times.size() < 2) {
					DebugError(L"Cubic spline animation requires at least two keys");
					return false;
				}

				std::uint64_t expectedValues = (std::uint64_t)sampler._times.size() * (cubic ? 3u : 1u);

				if (sampler._values.size() != expectedValues) {
					DebugError(L"Animation output count does not match input count");
					return false;
				}

				if (samplerComponents[i] != 0)
					animation._duration = std::max(animation._duration, sampler._times.back());

				animation._samplers.push_back(std::move(sampler));
			}

			// Validate quaternion values without modifying spline tangents.
			for (auto& channel : animation._channels) {
				if (channel._path != AnimationPath::Rotation)
					continue;

				AnimationSampler3D& sampler = animation._samplers[channel._samplerIndex];

				bool cubic = sampler._interpolation == AnimationInterpolation::CubicSpline;

				for (std::size_t key = 0; key < sampler._times.size(); key++) {
					std::array<float, 4> q = sampler._values[cubic ? key * 3 + 1 : key];
					float lengthSquared = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];

					if (std::abs(lengthSquared - 1.f) > 0.001f) {
						DebugError(L"Animation rotation key is not a unit quaternion");
						return false;
					}
				}
			}

			animations.push_back(std::move(animation));
		}

		model._animations = std::move(animations);
		model._player._index = -1;
		model._player._time = 0.f;

		DebugStat(L"Loaded animations: " + std::to_wstring(model._animations.size()));

		for (std::size_t i = 0; i < model._animations.size(); i++) {
			Animation3D& animation = model._animations[i];
			DebugStat(L"Animation " + std::to_wstring(i) + L": duration = " + std::to_wstring(animation._duration) + L" s, channels = " + std::to_wstring(animation._channels.size()) + L", samplers = " + std::to_wstring(animation._samplers.size()));
		}

		return true;
	}

	std::shared_ptr<Model3D> openModel3DFromFile(std::filesystem::path path) {

		DebugLog(L"Load the model \"" + path.wstring() + L"\"");

		std::ifstream file;
		file.open(path, std::ios::in | std::ios::binary);
		if (!file) {
			DebugError(L"Failed to open the model");
			return nullptr;
		}

		// .. loading ..

		BinaryReader reader(file);

		uint32_t magic = reader.read_uint32();
		uint32_t version = reader.read_uint32();
		uint32_t length = reader.read_uint32();

		if (magic != 0x46546C67) {
			DebugError(L"The file is not a GLB file.");
			return nullptr;
		}

		if (version != 2) {
			DebugError(L"Unsupported GLB version");
			return nullptr;
		}

		DebugStat(L"Version: " + std::to_wstring(version));
		DebugStat(L"Size: " + std::to_wstring(length) + L" bytes");

		uint32_t chunkLength = reader.read_uint32();
		uint32_t chunkType = reader.read_uint32();

		if (chunkType != 0x4E4F534A) { // "JSON"
			DebugError(L"The first GLB chunk is not JSON");
			return nullptr;
		}

		if (length < 20 || chunkLength > length - 20) {
			DebugError(L"Invalid JSON chunk size");
			return nullptr;
		}

		std::string jsonText(chunkLength, '\0');

		if (!file.read(jsonText.data(), chunkLength)) {
			DebugError(L"Failed to read the JSON chunk");
			return nullptr;
		}

		DebugStat(L"JSON size: " + std::to_wstring(chunkLength));

		uint32_t binLength = reader.read_uint32();
		uint32_t binType = reader.read_uint32();

		if (binType != 0x004E4942) { // "BIN"
			DebugError(L"Expected a BIN chunk.");
			return nullptr;
		}

		// GLB header + JSON header + JSON + BIN header
		uint64_t binStart = 12ull + 8ull + chunkLength + 8ull;

		if (binStart > length || binLength > length - binStart) {
			DebugError(L"Invalid BIN chunk size");
			return nullptr;
		}

		std::vector<uint8_t> binData(binLength);

		if (!file.read((char*)binData.data(), binLength)) {
			DebugError(L"Failed to read the BIN chunk");
			return nullptr;
		}

		DebugStat(L"BIN size: " + std::to_wstring(binLength));

		// Json reading

		JsonReader jsonReader(jsonText);

		if (!jsonReader.findRootField("meshes"))
			return nullptr;

		if (!jsonReader.expect('['))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("primitives"))
			return nullptr;

		if (!jsonReader.expect('['))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("attributes"))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("POSITION"))
			return nullptr;

		int positionAccessor = 0;

		if (!jsonReader.readInt(positionAccessor))
			return nullptr;

		DebugStat(L"Accessor position: " + std::to_wstring(positionAccessor));

		if (!jsonReader.findRootField("accessors"))
			return nullptr;

		if (!jsonReader.enterArrayElement(positionAccessor))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int positionAccessorStart = jsonReader._position;

		if (!jsonReader.findField("count"))
			return nullptr;

		int vertexCount = 0;

		if (!jsonReader.readInt(vertexCount))
			return nullptr;

		DebugStat(L"Vertices count: " + std::to_wstring(vertexCount));

		jsonReader._position = positionAccessorStart;

		if (!jsonReader.findField("bufferView"))
			return nullptr;

		int positionBufferView = 0;

		if (!jsonReader.readInt(positionBufferView))
			return nullptr;

		DebugStat(L"BufferView position: " + std::to_wstring(positionBufferView));

		if (!jsonReader.findRootField("bufferViews"))
			return nullptr;

		if (!jsonReader.enterArrayElement(positionBufferView))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int positionBufferViewStart = jsonReader._position;

		if (!jsonReader.findField("byteOffset"))
			return nullptr;

		int positionBufferOffset = 0;

		if (!jsonReader.readInt(positionBufferOffset))
			return nullptr;

		DebugStat(L"Position buffer offset: " + std::to_wstring(positionBufferOffset));

		jsonReader._position = positionAccessorStart;

		if (!jsonReader.findField("componentType"))
			return nullptr;

		int positionComponentType = 0;

		if (!jsonReader.readInt(positionComponentType))
			return nullptr;

		if (positionComponentType != 5126) { // float
			DebugError(L"Positions are not stored as floats");
			return nullptr;
		}

		jsonReader._position = positionAccessorStart;

		if (!jsonReader.findField("type"))
			return nullptr;

		std::string positionType = jsonReader.readString();

		if (positionType != "VEC3") {
			DebugError(L"Positions are not stored as VEC3");
			return nullptr;
		}

		DebugStat(L"Position format: float, VEC3");

		jsonReader._position = positionBufferViewStart;

		int positionStride = 12;

		if (jsonReader.findField("byteStride", false)) {
			if (!jsonReader.readInt(positionStride))
				return nullptr;
		}

		if (positionStride < 12 || positionStride > 252 || positionStride % 4 != 0) {
			DebugError(L"Invalid position byte stride");
			return nullptr;
		}

		DebugStat(L"Position byte stride: " + std::to_wstring(positionStride));

		jsonReader._position = positionAccessorStart;

		int positionAccessorOffset = 0;

		if (jsonReader.findField("byteOffset", false)) {
			if (!jsonReader.readInt(positionAccessorOffset))
				return nullptr;
		}

		DebugStat(L"Position accessor offset: " + std::to_wstring(positionAccessorOffset));

		// Load The Mesh
		uint64_t positionsStart = (uint64_t)positionBufferOffset + (uint64_t)positionAccessorOffset;

		if (vertexCount <= 0) {
			DebugError(L"Invalid vertex count");
			return nullptr;
		}

		uint64_t positionsEnd = positionsStart + uint64_t(vertexCount - 1) * positionStride + 12;

		if (positionsEnd > binData.size()) {
			DebugError(L"Position data exceeds BIN bounds");
			return nullptr;
		}

		std::shared_ptr<Mesh3D> mesh = std::make_shared<Mesh3D>();
		std::vector<Vertex3D> vertices;
		std::vector<std::uint32_t> indices;
		vertices.reserve(vertexCount);

		for (int i = 0; i < vertexCount; i++) {
			uint64_t offset = positionsStart + (uint64_t)i * positionStride;

			float position[3];

			std::memcpy(
				position,
				binData.data() + offset,
				sizeof(position)
			);

			vertices.emplace_back(
				position[0], position[1], position[2],
				0.f, 0.f, 0.f, // normals — later
				0.f, 0.f       // UV — later
			);
		}

		DebugStat(L"Loaded vertices: " + std::to_wstring(vertices.size()));

		if (!jsonReader.findRootField("meshes"))
			return nullptr;

		if (!jsonReader.enterArrayElement(0))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("primitives"))
			return nullptr;

		if (!jsonReader.enterArrayElement(0))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("indices"))
			return nullptr;

		int indicesAccessor = 0;

		if (!jsonReader.readInt(indicesAccessor))
			return nullptr;

		DebugStat(L"Indices accessor: " + std::to_wstring(indicesAccessor));

		if (!jsonReader.findRootField("accessors"))
			return nullptr;

		if (!jsonReader.enterArrayElement(indicesAccessor))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int indicesAccessorStart = jsonReader._position;

		if (!jsonReader.findField("count"))
			return nullptr;

		int indicesCount = 0;

		if (!jsonReader.readInt(indicesCount))
			return nullptr;

		DebugStat(L"Indices count: " + std::to_wstring(indicesCount));

		jsonReader._position = indicesAccessorStart;

		if (!jsonReader.findField("componentType"))
			return nullptr;

		int indicesComponentType = 0;

		if (!jsonReader.readInt(indicesComponentType))
			return nullptr;

		int indexSize = 0;

		if (indicesComponentType == 5121) {
			indexSize = 1;
		}
		else if (indicesComponentType == 5123) {
			indexSize = 2;
		}
		else if (indicesComponentType == 5125) {
			indexSize = 4;
		}
		else {
			DebugError(L"Unsupported indices component type");
			return nullptr;
		}

		jsonReader._position = indicesAccessorStart;

		if (!jsonReader.findField("type"))
			return nullptr;

		if (jsonReader.readString() != "SCALAR") {
			DebugError(L"Indices are not stored as SCALAR");
			return nullptr;
		}

		jsonReader._position = indicesAccessorStart;

		if (!jsonReader.findField("bufferView"))
			return nullptr;

		int indicesBufferView = 0;

		if (!jsonReader.readInt(indicesBufferView))
			return nullptr;

		DebugStat(L"Indices format: uint32, SCALAR");
		DebugStat(L"Indices buffer view: " + std::to_wstring(indicesBufferView));

		// Optional offset inside the buffer view.
		jsonReader._position = indicesAccessorStart;

		int indicesAccessorOffset = 0;

		if (jsonReader.findField("byteOffset", false)) {
			if (!jsonReader.readInt(indicesAccessorOffset))
				return nullptr;
		}

		// Find the buffer view.
		if (!jsonReader.findRootField("bufferViews"))
			return nullptr;

		if (!jsonReader.enterArrayElement(indicesBufferView))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int indicesBufferViewStart = jsonReader._position;

		int indicesBufferOffset = 0;

		if (jsonReader.findField("byteOffset", false)) {
			if (!jsonReader.readInt(indicesBufferOffset))
				return nullptr;
		}

		DebugStat(L"Indices accessor offset: " + std::to_wstring(indicesAccessorOffset));
		DebugStat(L"Indices buffer offset: " + std::to_wstring(indicesBufferOffset));

		uint64_t indicesStart = (uint64_t)indicesBufferOffset + (uint64_t)indicesAccessorOffset;

		if (indicesCount <= 0 || indicesCount % 3 != 0) {
			DebugError(L"Invalid index count for triangles");
			return nullptr;
		}

		uint64_t indicesBytes = (uint64_t)indicesCount * indexSize;

		if (indicesStart > binData.size() ||
			indicesBytes > binData.size() - indicesStart) {
			DebugError(L"Index data exceeds BIN bounds");
			return nullptr;
		}

		indices.resize(indicesCount);

		for (int i = 0; i < indicesCount; i++) {
			uint64_t offset = indicesStart + (uint64_t)i * indexSize;

			if (indicesComponentType == 5121) {
				indices[i] = binData[offset];
			}
			else if (indicesComponentType == 5123) {
				uint16_t index = 0;
				std::memcpy(&index, binData.data() + offset, sizeof(index));
				indices[i] = index;
			}
			else {
				uint32_t index = 0;
				std::memcpy(&index, binData.data() + offset, sizeof(index));
				indices[i] = index;
			}
		}

		for (auto index : indices) {
			if (index >= vertices.size()) {
				DebugError(L"Index exceeds vertex count");
				return nullptr;
			}
		}

		DebugStat(L"Loaded indices: " + std::to_wstring(indices.size()));

		if (!jsonReader.findRootField("meshes"))
			return nullptr;

		if (!jsonReader.enterArrayElement(0))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("primitives"))
			return nullptr;

		if (!jsonReader.enterArrayElement(0))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("attributes"))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("NORMAL"))
			return nullptr;

		int normalAccessor = 0;

		if (!jsonReader.readInt(normalAccessor))
			return nullptr;

		DebugStat(L"Normal accessor: " + std::to_wstring(normalAccessor));

		if (!jsonReader.findRootField("accessors"))
			return nullptr;

		if (!jsonReader.enterArrayElement(normalAccessor))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int normalAccessorStart = jsonReader._position;

		if (!jsonReader.findField("count"))
			return nullptr;

		int normalCount = 0;

		if (!jsonReader.readInt(normalCount))
			return nullptr;

		if (normalCount != vertexCount) {
			DebugError(L"Normal count does not match vertex count");
			return nullptr;
		}

		jsonReader._position = normalAccessorStart;

		if (!jsonReader.findField("bufferView"))
			return nullptr;

		int normalBufferView = 0;

		if (!jsonReader.readInt(normalBufferView))
			return nullptr;

		DebugStat(L"Normal count: " + std::to_wstring(normalCount));
		DebugStat(L"Normal buffer view: " + std::to_wstring(normalBufferView));

		jsonReader._position = normalAccessorStart;

		if (!jsonReader.findField("componentType"))
			return nullptr;

		int normalComponentType = 0;

		if (!jsonReader.readInt(normalComponentType))
			return nullptr;

		if (normalComponentType != 5126) {
			DebugError(L"Normals are not stored as floats");
			return nullptr;
		}

		jsonReader._position = normalAccessorStart;

		if (!jsonReader.findField("type"))
			return nullptr;

		if (jsonReader.readString() != "VEC3") {
			DebugError(L"Normals are not stored as VEC3");
			return nullptr;
		}

		jsonReader._position = normalAccessorStart;

		int normalAccessorOffset = 0;

		if (jsonReader.findField("byteOffset", false)) {
			if (!jsonReader.readInt(normalAccessorOffset))
				return nullptr;
		}

		DebugStat(L"Normal format: float, VEC3");
		DebugStat(L"Normal accessor offset: " + std::to_wstring(normalAccessorOffset));

		if (!jsonReader.findRootField("bufferViews"))
			return nullptr;

		if (!jsonReader.enterArrayElement(normalBufferView))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int normalBufferViewStart = jsonReader._position;

		int normalBufferOffset = 0;

		if (jsonReader.findField("byteOffset", false)) {
			if (!jsonReader.readInt(normalBufferOffset))
				return nullptr;
		}

		jsonReader._position = normalBufferViewStart;

		int normalStride = 12; // Default for float, VEC3.

		if (jsonReader.findField("byteStride", false)) {
			if (!jsonReader.readInt(normalStride))
				return nullptr;
		}

		if (normalStride < 12 || normalStride > 252 || normalStride % 4 != 0) {
			DebugError(L"Invalid normal byte stride");
			return nullptr;
		}

		DebugStat(L"Normal buffer offset: " + std::to_wstring(normalBufferOffset));
		DebugStat(L"Normal byte stride: " + std::to_wstring(normalStride));

		uint64_t normalsStart = (uint64_t)normalBufferOffset + (uint64_t)normalAccessorOffset;

		if (normalCount <= 0 || (std::size_t)normalCount != vertices.size()) {
			DebugError(L"Invalid normal count");
			return nullptr;
		}

		uint64_t normalsEnd = normalsStart + uint64_t(normalCount - 1) * normalStride + 12;

		if (normalsEnd > binData.size()) {
			DebugError(L"Normal data exceeds BIN bounds");
			return nullptr;
		}

		for (int i = 0; i < normalCount; i++) {
			uint64_t offset = normalsStart + (uint64_t)i * normalStride;

			float normal[3];

			std::memcpy(
				normal,
				binData.data() + offset,
				sizeof(normal)
			);

			Vertex3D& vertex = vertices[i];

			vertex._nx = normal[0];
			vertex._ny = normal[1];
			vertex._nz = normal[2];
		}

		DebugStat(L"Loaded normals: " + std::to_wstring(normalCount));

		if (!jsonReader.findRootField("meshes"))
			return nullptr;

		if (!jsonReader.enterArrayElement(0))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("primitives"))
			return nullptr;

		if (!jsonReader.enterArrayElement(0))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("attributes"))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("TEXCOORD_0"))
			return nullptr;

		int uvAccessor = 0;

		if (!jsonReader.readInt(uvAccessor))
			return nullptr;

		DebugStat(L"UV accessor: " + std::to_wstring(uvAccessor));

		if (!jsonReader.findRootField("accessors"))
			return nullptr;

		if (!jsonReader.enterArrayElement(uvAccessor))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int uvAccessorStart = jsonReader._position;

		if (!jsonReader.findField("count"))
			return nullptr;

		int uvCount = 0;

		if (!jsonReader.readInt(uvCount))
			return nullptr;

		if (uvCount != vertexCount) {
			DebugError(L"UV count does not match vertex count");
			return nullptr;
		}

		jsonReader._position = uvAccessorStart;

		if (!jsonReader.findField("bufferView"))
			return nullptr;

		int uvBufferView = 0;

		if (!jsonReader.readInt(uvBufferView))
			return nullptr;

		DebugStat(L"UV count: " + std::to_wstring(uvCount));
		DebugStat(L"UV buffer view: " + std::to_wstring(uvBufferView));

		jsonReader._position = uvAccessorStart;

		if (!jsonReader.findField("componentType"))
			return nullptr;

		int uvComponentType = 0;

		if (!jsonReader.readInt(uvComponentType))
			return nullptr;

		if (uvComponentType != 5126) {
			DebugError(L"Only float UV coordinates are currently supported");
			return nullptr;
		}

		jsonReader._position = uvAccessorStart;

		if (!jsonReader.findField("type"))
			return nullptr;

		if (jsonReader.readString() != "VEC2") {
			DebugError(L"UV coordinates are not stored as VEC2");
			return nullptr;
		}

		jsonReader._position = uvAccessorStart;

		int uvAccessorOffset = 0;

		if (jsonReader.findField("byteOffset", false)) {
			if (!jsonReader.readInt(uvAccessorOffset))
				return nullptr;
		}

		DebugStat(L"UV format: float, VEC2");
		DebugStat(L"UV accessor offset: " + std::to_wstring(uvAccessorOffset));

		if (!jsonReader.findRootField("bufferViews"))
			return nullptr;

		if (!jsonReader.enterArrayElement(uvBufferView))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int uvBufferViewStart = jsonReader._position;

		int uvBufferOffset = 0;

		if (jsonReader.findField("byteOffset", false)) {
			if (!jsonReader.readInt(uvBufferOffset))
				return nullptr;
		}

		jsonReader._position = uvBufferViewStart;

		int uvStride = 8; // Two floats: u, v.

		if (jsonReader.findField("byteStride", false)) {
			if (!jsonReader.readInt(uvStride))
				return nullptr;
		}

		if (uvStride < 8 || uvStride > 252 || uvStride % 4 != 0) {
			DebugError(L"Invalid UV byte stride");
			return nullptr;
		}

		DebugStat(L"UV buffer offset: " + std::to_wstring(uvBufferOffset));
		DebugStat(L"UV byte stride: " + std::to_wstring(uvStride));

		uint64_t uvStart = (uint64_t)uvBufferOffset + (uint64_t)uvAccessorOffset;

		if (uvCount <= 0 ||
			(std::size_t)uvCount != vertices.size()) {
			DebugError(L"Invalid UV count");
			return nullptr;
		}

		uint64_t uvEnd = uvStart + uint64_t(uvCount - 1) * uvStride + 8;

		if (uvEnd > binData.size()) {
			DebugError(L"UV data exceeds BIN bounds");
			return nullptr;
		}

		for (int i = 0; i < uvCount; i++) {
			uint64_t offset = uvStart + (uint64_t)i * uvStride;

			float uv[2];

			std::memcpy(
				uv,
				binData.data() + offset,
				sizeof(uv)
			);

			Vertex3D& vertex = vertices[i];

			vertex._u = uv[0];
			vertex._v = uv[1];
		}

		DebugStat(L"Loaded UV coordinates: " + std::to_wstring(uvCount));

		if (!jsonReader.findRootField("meshes"))
			return nullptr;

		if (!jsonReader.enterArrayElement(0))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("primitives"))
			return nullptr;

		if (!jsonReader.enterArrayElement(0))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("material"))
			return nullptr;

		int materialIndex = 0;

		if (!jsonReader.readInt(materialIndex))
			return nullptr;

		DebugStat(L"Material index: " + std::to_wstring(materialIndex));

		if (!jsonReader.findRootField("materials"))
			return nullptr;

		if (!jsonReader.enterArrayElement(materialIndex))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("pbrMetallicRoughness"))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("baseColorTexture"))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("index"))
			return nullptr;

		int baseColorTextureIndex = 0;

		if (!jsonReader.readInt(baseColorTextureIndex))
			return nullptr;

		DebugStat(L"Base color texture index: " + std::to_wstring(baseColorTextureIndex));

		if (!jsonReader.findRootField("textures"))
			return nullptr;

		if (!jsonReader.enterArrayElement(baseColorTextureIndex))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		if (!jsonReader.findField("source"))
			return nullptr;

		int baseColorImageIndex = 0;

		if (!jsonReader.readInt(baseColorImageIndex))
			return nullptr;

		DebugStat(L"Base color image index: " + std::to_wstring(baseColorImageIndex));

		if (!jsonReader.findRootField("images"))
			return nullptr;

		if (!jsonReader.enterArrayElement(baseColorImageIndex))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int baseColorImageStart = jsonReader._position;

		if (!jsonReader.findField("mimeType"))
			return nullptr;

		std::string imageMimeType = jsonReader.readString();

		if (imageMimeType != "image/png" && imageMimeType != "image/jpeg") {
			DebugError(L"Only PNG and JPEG images are currently supported");
			return nullptr;
		}

		jsonReader._position = baseColorImageStart;

		if (!jsonReader.findField("bufferView"))
			return nullptr;

		int baseColorImageBufferView = 0;

		if (!jsonReader.readInt(baseColorImageBufferView))
			return nullptr;

		DebugStat(L"Base color image format: PNG");
		DebugStat(L"Base color image buffer view: " + std::to_wstring(baseColorImageBufferView));

		if (!jsonReader.findRootField("bufferViews"))
			return nullptr;

		if (!jsonReader.enterArrayElement(baseColorImageBufferView))
			return nullptr;

		if (!jsonReader.expect('{'))
			return nullptr;

		int imageBufferViewStart = jsonReader._position;

		int imageBufferOffset = 0;

		if (jsonReader.findField("byteOffset", false)) {
			if (!jsonReader.readInt(imageBufferOffset))
				return nullptr;
		}

		jsonReader._position = imageBufferViewStart;

		if (!jsonReader.findField("byteLength"))
			return nullptr;

		int imageByteLength = 0;

		if (!jsonReader.readInt(imageByteLength))
			return nullptr;

		if (imageByteLength <= 0 ||
			(uint64_t)imageBufferOffset > binData.size() ||
			(uint64_t)imageByteLength >
			binData.size() - (uint64_t)imageBufferOffset) {
			DebugError(L"Image data exceeds BIN bounds");
			return nullptr;
		}

		DebugStat(L"Image buffer offset: " + std::to_wstring(imageBufferOffset));
		DebugStat(L"Image byte length: " + std::to_wstring(imageByteLength));

		std::shared_ptr<sf::Image> image = std::make_shared<sf::Image>();

		if (!image->loadFromMemory(binData.data() + imageBufferOffset, (std::size_t)imageByteLength)) {
			DebugError(L"Failed to decode base color image");
			return nullptr;
		}

		sf::Vector2u size = image->getSize();
		float memory = float(size.x * size.y) * 4.f / (1024.f * 1024.f);
		DebugStat(L"GLB Texture: " + std::to_wstring(memory) + L" MiB");

		std::shared_ptr<Texture> texture = std::make_shared<Texture>(path.wstring() + L"#baseColor", image);

		if (texture->_texture->getSize().x == 0 ||
			texture->_texture->getSize().y == 0) {
			DebugError(L"Failed to create base color texture");
			return nullptr;
		}

		mesh->setTexture(texture);

		DebugStat(L"Base color texture loaded");

		file.close();

		std::shared_ptr<Model3D> model = std::make_shared<Model3D>();

		if (!loadModelNodes(jsonReader, *model))
			return nullptr;

		if (!loadModelSkins(jsonReader, binData, *model))
			return nullptr;

		if (!loadMeshSkinData(jsonReader, binData, *model, *mesh, vertices))
			return nullptr;

		DebugStat(L"Loading model animations...");

		if (!loadModelAnimations(jsonReader, binData, *model)) {
			DebugError(L"Failed to load model animations");
			return nullptr;
		}

		DebugStat(L"Finished loading model animations");

		mesh->uploadToGPU(vertices, indices);

		if (!uploadMeshSkinPose(*model, *mesh))
			return nullptr;

		DebugStat(L"Uploaded joint matrices: " + std::to_wstring(mesh->_gpuJointCount));

		model->addMesh(mesh);

		return model;

	}
}
