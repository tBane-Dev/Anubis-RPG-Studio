#include "ShadersManager.hpp"

std::shared_ptr<sf::Shader> terrain3D_shader;
std::shared_ptr<sf::Shader> terrain2D_shader;
std::shared_ptr<sf::Shader> ghost_chunk_shader;
std::shared_ptr<sf::Shader> grid_shader;
std::shared_ptr<sf::Shader> grid2_shader;
std::shared_ptr<sf::Shader> checkerboard_shader;
std::shared_ptr<sf::Shader> sprite25D_shader;

void loadShaders() {
	terrain2D_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex2D.vert", L"assets\\shr\\water.frag");
	terrain3D_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex3D.vert", L"assets\\shr\\water.frag");
	
	ghost_chunk_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex3D.vert", L"assets\\shr\\ghost_chunk.frag");

	grid_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex3D.vert", L"assets\\shr\\grid.frag");
	grid2_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex2D.vert", L"assets\\shr\\grid2.frag");
	checkerboard_shader = std::make_shared<sf::Shader>(L"assets\\shr\\vertex2D.vert", L"assets\\shr\\checkerboard.frag");

	sprite25D_shader = std::make_shared<sf::Shader>(L"assets\\shr\\sprite2.5D.vert", L"assets\\shr\\sprite2.5D.frag");
}