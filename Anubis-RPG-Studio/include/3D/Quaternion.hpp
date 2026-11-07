#pragma once

#include <array>
#include <cmath>

namespace Anubis {

	struct Quaternion {
		float w = 1.f;
		float x = 0.f;
		float y = 0.f;
		float z = 0.f;

		Quaternion operator*(Quaternion q) {
			return {
				w * q.w - x * q.x - y * q.y - z * q.z,
				w * q.x + x * q.w + y * q.z - z * q.y,
				w * q.y - x * q.z + y * q.w + z * q.x,
				w * q.z + x * q.y - y * q.x + z * q.w
			};
		}

		void normalize() {
			float length = std::sqrt(w * w + x * x + y * y + z * z);

			if (length > 0.f) {
				w /= length;
				x /= length;
				y /= length;
				z /= length;
			}
		}

		std::array<float, 16> matrix() {
			// column-major layout for OpenGL
			return {
				1.f - 2.f * (y * y + z * z), 2.f * (x * y + w * z), 2.f * (x * z - w * y), 0.f,
				2.f * (x * y - w * z), 1.f - 2.f * (x * x + z * z), 2.f * (y * z + w * x), 0.f,
				2.f * (x * z + w * y), 2.f * (y * z - w * x), 1.f - 2.f * (x * x + y * y), 0.f,
				0.f, 0.f, 0.f, 1.f
			};
		}
	};
}