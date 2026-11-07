#pragma once
#include <SFML/Graphics.hpp>
#include "3D/Quaternion.hpp"

namespace Anubis {
	class Camera {
	public:
		sf::Vector3f _position;
		Quaternion _rotation;

		Camera();
		~Camera();
	};
}