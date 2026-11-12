#include "ShadersManager.hpp"

namespace Anubis {
	ShadersManager::ShadersManager() {
		terrain2D_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex2D.vert", L"assets\\shr\\water.frag");
		terrain3D_shader = std::make_shared<sf::Shader>(L"assets\\shr\\terrain3D.vert", L"assets\\shr\\terrain3D.frag");
		ghost_chunk_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex3D.vert", L"assets\\shr\\ghost_chunk.frag");
		grid_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex3D.vert", L"assets\\shr\\grid.frag");
		grid2_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex2D.vert", L"assets\\shr\\grid2.frag");
		checkerboard_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex2D.vert", L"assets\\shr\\checkerboard.frag");
		tileOverlay3D_shader = std::make_shared<sf::Shader>(L"assets\\shr\\terrain3D.vert", L"assets\\shr\\tileOverlay3D.frag");
	}

	ShadersManager::~ShadersManager() {

	}

	std::shared_ptr<ShadersManager> shaders_manager = nullptr;
}