#pragma once
#include <SFML/Graphics.hpp>

namespace Anubis {
	class ShadersManager {
	public:
		std::shared_ptr<sf::Shader> terrain3D_shader;
		std::shared_ptr<sf::Shader> terrain2D_shader;
		std::shared_ptr<sf::Shader> ghost_chunk_shader;
		std::shared_ptr<sf::Shader> grid_shader;
		std::shared_ptr<sf::Shader> grid2_shader;
		std::shared_ptr<sf::Shader> checkerboard_shader;
		std::shared_ptr<sf::Shader> tileOverlay3D_shader;

		ShadersManager();
		~ShadersManager();
	};

	extern std::shared_ptr<ShadersManager> shaders_manager;
}