#include "3D/Camera.hpp"

namespace Anubis {
	Camera::Camera() {
		_position = sf::Vector3f(0.0f, 0.0f, 0.0f);
		_rotation = Quaternion{ 1.0f, 0.0f, 0.0f, 0.0f };
	}

	Camera::~Camera() {

	}
}