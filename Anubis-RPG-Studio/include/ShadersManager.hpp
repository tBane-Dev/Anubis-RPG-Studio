#pragma once
#include <SFML/Graphics.hpp>

extern std::shared_ptr<sf::Shader> terrain3D_shader;
extern std::shared_ptr<sf::Shader> terrain2D_shader;
extern std::shared_ptr<sf::Shader> ghost_chunk_shader;
extern std::shared_ptr<sf::Shader> grid_shader;
extern std::shared_ptr<sf::Shader> grid2_shader;
extern std::shared_ptr<sf::Shader> checkerboard_shader;
extern std::shared_ptr<sf::Shader> sprite25D_shader;

void loadShaders();