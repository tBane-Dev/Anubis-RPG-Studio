#include "Editors/ModelsEditor/StatisticsPanel.hpp"
#include "Editors/ModelsEditor/Editor.hpp"
#include "Theme.hpp"
#include "RenderWindow.hpp"
#include "DebugLog.hpp"

namespace ModelsEditor {

	StatisticsPanel::StatisticsPanel(sf::Vector2i margin) : Panel(sf::Vector2i(520, 900 - 192 - 192), sf::Vector2i(1332 + margin.x, ModelsEditor::editor->_main_menu->getSize().y + margin.y + 192 + 192)) {

		_title = std::make_unique<sf::Text>(basicFont, L"Statistics", 20);
		_title->setFillColor(basic_text_color);
		_title->setPosition(sf::Vector2f(_rect.position.x + 16, _rect.position.y + 16));

		// stats rect
		int m = 8;
		_statsRect.size = sf::Vector2i(384, 8 * (basicFont.getLineSpacing(basic_text_size) + m) + m);
		_statsRect.position = getPosition() + sf::Vector2i((512 - _statsRect.size.x) / 2, 96);

		_animations_label = std::make_unique<sf::Text>(basicFont, L"Animations", basic_text_size);
		_triangles_label = std::make_unique<sf::Text>(basicFont, L"Triangles", basic_text_size);
		_meshes_label = std::make_unique<sf::Text>(basicFont, L"Meshes", basic_text_size);
		_nodes_label = std::make_unique<sf::Text>(basicFont, L"Nodes", basic_text_size);
		_skins_label = std::make_unique<sf::Text>(basicFont, L"Skins", basic_text_size);
		_joints_label = std::make_unique<sf::Text>(basicFont, L"Joints", basic_text_size);
		_textures_label = std::make_unique<sf::Text>(basicFont, L"Textures", basic_text_size);
		_size_label = std::make_unique<sf::Text>(basicFont, L"Size", basic_text_size);

		_animations_label->setFillColor(basic_text_color);
		_triangles_label->setFillColor(basic_text_color);
		_meshes_label->setFillColor(basic_text_color);
		_nodes_label->setFillColor(basic_text_color);
		_skins_label->setFillColor(basic_text_color);
		_joints_label->setFillColor(basic_text_color);
		_textures_label->setFillColor(basic_text_color);
		_size_label->setFillColor(basic_text_color);

		_animations_label->setPosition(sf::Vector2f(_statsRect.position.x + m, _statsRect.position.y + m));

		_triangles_label->setPosition(sf::Vector2f(_statsRect.position.x + m, _statsRect.position.y + m + 1 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_meshes_label->setPosition(sf::Vector2f(_statsRect.position.x + m, _statsRect.position.y + m + 2 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_nodes_label->setPosition(sf::Vector2f(_statsRect.position.x + m, _statsRect.position.y + m + 3 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_skins_label->setPosition(sf::Vector2f(_statsRect.position.x + m, _statsRect.position.y + m + 4 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_joints_label->setPosition(sf::Vector2f(_statsRect.position.x + m, _statsRect.position.y + m + 5 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_textures_label->setPosition(sf::Vector2f(_statsRect.position.x + m, _statsRect.position.y + m + 6 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_size_label->setPosition(sf::Vector2f(_statsRect.position.x + m, _statsRect.position.y + m + 7 * (basicFont.getLineSpacing(basic_text_size) + m)));

		_animations = std::make_unique<sf::Text>(basicFont, L"--", basic_text_size);
		_triangles = std::make_unique<sf::Text>(basicFont, L"--", basic_text_size);
		_meshes = std::make_unique<sf::Text>(basicFont, L"--", basic_text_size);
		_nodes = std::make_unique<sf::Text>(basicFont, L"--", basic_text_size);
		_skins = std::make_unique<sf::Text>(basicFont, L"--", basic_text_size);
		_joints = std::make_unique<sf::Text>(basicFont, L"--", basic_text_size);
		_textures = std::make_unique<sf::Text>(basicFont, L"--", basic_text_size);
		_size = std::make_unique<sf::Text>(basicFont, L"--", basic_text_size);

		_animations->setFillColor(basic_text_color);
		_triangles->setFillColor(basic_text_color);
		_meshes->setFillColor(basic_text_color);
		_nodes->setFillColor(basic_text_color);
		_skins->setFillColor(basic_text_color);
		_joints->setFillColor(basic_text_color);
		_textures->setFillColor(basic_text_color);
		_size->setFillColor(basic_text_color);

		loadModel(nullptr);
	}

	StatisticsPanel::~StatisticsPanel() {

	}

	void StatisticsPanel::loadModel(std::shared_ptr<Anubis::Model3D> model) {

		_animations->setString(L"--");
		_triangles->setString(L"--");
		_meshes->setString(L"--");
		_nodes->setString(L"--");
		_skins->setString(L"--");
		_joints->setString(L"--");
		_textures->setString(L"--");
		_size->setString(L"--");

		int m = 8;
		int s = _statsRect.position.x + _statsRect.size.x;
		_animations->setPosition(sf::Vector2f(s - _animations->getGlobalBounds().size.x - m, _statsRect.position.y + m));
		_triangles->setPosition(sf::Vector2f(s - _triangles->getGlobalBounds().size.x - m, _statsRect.position.y + m + 1 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_meshes->setPosition(sf::Vector2f(s - _meshes->getGlobalBounds().size.x - m, _statsRect.position.y + m + 2 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_nodes->setPosition(sf::Vector2f(s - _nodes->getGlobalBounds().size.x - m, _statsRect.position.y + m + 3 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_skins->setPosition(sf::Vector2f(s - _skins->getGlobalBounds().size.x - m, _statsRect.position.y + m + 4 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_joints->setPosition(sf::Vector2f(s - _joints->getGlobalBounds().size.x - m, _statsRect.position.y + m + 5 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_textures->setPosition(sf::Vector2f(s - _textures->getGlobalBounds().size.x - m, _statsRect.position.y + m + 6 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_size->setPosition(sf::Vector2f(s - _size->getGlobalBounds().size.x - m, _statsRect.position.y + m + 7 * (basicFont.getLineSpacing(basic_text_size) + m)));

		if (ModelsEditor::editor && ModelsEditor::editor->_preview_panel && model) {

			int triangles = 0;
			std::vector<std::shared_ptr<Texture>> textures;

			for (auto& mesh : model->_meshes) {
				if (!mesh)
					continue;

				if (mesh->_geometry) triangles += mesh->_geometry->_indexCount / 3;
				if (mesh->_texture && std::find(textures.begin(), textures.end(), mesh->_texture) == textures.end()) textures.push_back(mesh->_texture);
			}

			int joints = 0;
			for (auto& skin : model->_skins)
				joints += skin._joints.size();

			_animations->setString(std::to_wstring(model->_animations.size()));
			_triangles->setString(std::to_wstring(triangles));
			_meshes->setString(std::to_wstring(model->_meshes.size()));
			_nodes->setString(std::to_wstring(model->_nodes.size()));
			_skins->setString(std::to_wstring(model->_skins.size()));
			_joints->setString(std::to_wstring(joints));
			_textures->setString(std::to_wstring(textures.size()));
			_size->setString(L"--");
		}

		_animations->setPosition(sf::Vector2f(s - _animations->getGlobalBounds().size.x - m, _statsRect.position.y + m));
		_triangles->setPosition(sf::Vector2f(s - _triangles->getGlobalBounds().size.x - m, _statsRect.position.y + m + 1 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_meshes->setPosition(sf::Vector2f(s - _meshes->getGlobalBounds().size.x - m, _statsRect.position.y + m + 2 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_nodes->setPosition(sf::Vector2f(s - _nodes->getGlobalBounds().size.x - m, _statsRect.position.y + m + 3 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_skins->setPosition(sf::Vector2f(s - _skins->getGlobalBounds().size.x - m, _statsRect.position.y + m + 4 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_joints->setPosition(sf::Vector2f(s - _joints->getGlobalBounds().size.x - m, _statsRect.position.y + m + 5 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_textures->setPosition(sf::Vector2f(s - _textures->getGlobalBounds().size.x - m, _statsRect.position.y + m + 6 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_size->setPosition(sf::Vector2f(s - _size->getGlobalBounds().size.x - m, _statsRect.position.y + m + 7 * (basicFont.getLineSpacing(basic_text_size) + m)));
	}
	void StatisticsPanel::cursorHover() {
		Components::Panel::cursorHover();

	}

	void StatisticsPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);
	}

	void StatisticsPanel::update() {

		if (!editor)
			return;

		Components::Panel::update();

	}

	void StatisticsPanel::draw() {
		Components::Panel::draw();

		Anubis::render_window->draw(*_title);

		// draw stats rect
		int m = 2;
		sf::RectangleShape statsRectShape(sf::Vector2f(_statsRect.size + sf::Vector2i(2 * m, 2 * m)));
		statsRectShape.setPosition(sf::Vector2f(_statsRect.position - sf::Vector2i(m, m)));
		statsRectShape.setFillColor(sf::Color(24, 24, 24, 255));
		statsRectShape.setOutlineThickness(m);
		statsRectShape.setOutlineColor(sf::Color(0, 0, 0, 255));
		Anubis::render_window->draw(statsRectShape);

		// texts labels
		Anubis::render_window->draw(*_animations_label);
		Anubis::render_window->draw(*_triangles_label);
		Anubis::render_window->draw(*_meshes_label);
		Anubis::render_window->draw(*_nodes_label);
		Anubis::render_window->draw(*_skins_label);
		Anubis::render_window->draw(*_joints_label);
		Anubis::render_window->draw(*_textures_label);
		Anubis::render_window->draw(*_size_label);

		// text inputs
		Anubis::render_window->draw(*_animations);
		Anubis::render_window->draw(*_triangles);
		Anubis::render_window->draw(*_meshes);
		Anubis::render_window->draw(*_nodes);
		Anubis::render_window->draw(*_skins);
		Anubis::render_window->draw(*_joints);
		Anubis::render_window->draw(*_textures);
		Anubis::render_window->draw(*_size);

		
	}
}