#include "Editors/ModelsEditor/PlayerPanel.hpp"
#include "Editors/ModelsEditor/Editor.hpp"
#include "Theme.hpp"
#include "RenderWindow.hpp"
#include "DebugLog.hpp"
#include "Objects/Monster.hpp"
#include "PrefabsManager.hpp"
#include "ShadersManager.hpp"

namespace ModelsEditor {

	PlayerPanel::PlayerPanel(sf::Vector2i margin) : Panel(sf::Vector2i(520, 256), sf::Vector2i(1332 + margin.x, ModelsEditor::editor->_main_menu->getSize().y + margin.y + 192)) {

		_title = std::make_unique<sf::Text>(basicFont, L"Player", 20);
		_title->setFillColor(basic_text_color);
		_title->setPosition(sf::Vector2f(_rect.position.x + 16, _rect.position.y + 16));
	
		// buttons
		_first = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\first.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\first_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\first_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\first_inactive.png"));

		_prev = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\prev.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\prev_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\prev_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\prev_inactive.png"));

		_play = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\play.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\play_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\play_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\play_inactive.png"));

		_pause = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\pause.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\pause_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\pause_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\pause_inactive.png"));

		_next = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\next.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\next_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\next_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\next_inactive.png"));

		_last = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\last.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\last_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\last_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\last_inactive.png"));

		_anim_prev = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_prev.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_prev_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_prev_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_prev_inactive.png"));

		_anim_next = std::make_shared<ButtonWithSprite>(
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_next.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_next_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_next_press.png"),
			textures_manager->getTexture(L"assets\\tex\\editors_ui\\anim_next_inactive.png"));

		int padding = 10;
		int total_width = _rect.size.x - padding * 2;
		int btn_w = 64;
		int buttons_count = 5;
		int buttons_width = buttons_count * btn_w;
		int margin_between_buttons = (total_width - buttons_width) / buttons_count / 4;
		int all_width = buttons_width + margin_between_buttons * (buttons_count - 1);

		sf::Vector2f startPosition;
		startPosition.x = _rect.position.x + padding + (total_width - all_width) / 2;
		startPosition.y = _rect.position.y + _title->getFont().getLineSpacing(20) + 24 + 16 + 16;

		_first->setPosition(sf::Vector2i(startPosition.x, startPosition.y));
		_prev->setPosition(sf::Vector2i(_first->getPosition().x + btn_w + margin_between_buttons, _first->getPosition().y));
		_play->setPosition(sf::Vector2i(_prev->getPosition().x + btn_w + margin_between_buttons, _prev->getPosition().y));
		_pause->setPosition(sf::Vector2i(_play->getPosition().x, _play->getPosition().y));
		_next->setPosition(sf::Vector2i(_pause->getPosition().x + btn_w + margin_between_buttons, _pause->getPosition().y));
		_last->setPosition(sf::Vector2i(_next->getPosition().x + btn_w + margin_between_buttons, _next->getPosition().y));

		_anim_prev->setPosition(sf::Vector2i(_first->getPosition().x, _first->getPosition().y + btn_w + margin_between_buttons));
		_anim_next->setPosition(sf::Vector2i(_last->getPosition().x + btn_w - 80, _anim_prev->getPosition().y));

		_first->_onclick_func = [this]() {

		};

		_prev->_onclick_func = [this]() {
			
		};


		_play->_onclick_func = [this]() {
			ModelsEditor::editor->_preview_panel->_model->_isAnimating = true;
		};

		_pause->_onclick_func = [this]() {
			ModelsEditor::editor->_preview_panel->_model->_isAnimating = false;
		};

		_next->_onclick_func = [this]() {
			
		};

		_last->_onclick_func = [this]() {
		
		};

		_anim_prev->_onclick_func = [this]() {
			if (!ModelsEditor::editor->_preview_panel->_model->_animations.empty()) {
				ModelsEditor::editor->_preview_panel->_model->_player._index -= 1;
				if (ModelsEditor::editor->_preview_panel->_model->_player._index < 0)
					ModelsEditor::editor->_preview_panel->_model->_player._index = (int)ModelsEditor::editor->_preview_panel->_model->_animations.size() - 1;
			}
		};

		_anim_next->_onclick_func = [this]() {
			if (!ModelsEditor::editor->_preview_panel->_model->_animations.empty()) {
				ModelsEditor::editor->_preview_panel->_model->_player._index += 1;
				if (ModelsEditor::editor->_preview_panel->_model->_player._index >= (int)ModelsEditor::editor->_preview_panel->_model->_animations.size())
					ModelsEditor::editor->_preview_panel->_model->_player._index = 0;
			}

		};

		// stats rect
		int m = 8;
		_statsRect = sf::IntRect(
			sf::Vector2i(
				_anim_prev->getPosition().x - 16,
				_anim_prev->getPosition().y + _anim_prev->getSize().y + 16
			),
			sf::Vector2i(
				_anim_next->getPosition().x + _anim_next->getSize().x - _anim_prev->getPosition().x + 32,
				8 * (basicFont.getLineSpacing(basic_text_size) + m) + m)
		);

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

		int s = _statsRect.position.x + _statsRect.size.x;
		_animations->setPosition(sf::Vector2f(s - _animations->getGlobalBounds().size.x - m, _statsRect.position.y + m));
		_triangles->setPosition(sf::Vector2f(s - _triangles->getGlobalBounds().size.x - m, _statsRect.position.y + m + 1 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_meshes->setPosition(sf::Vector2f(s - _meshes->getGlobalBounds().size.x - m, _statsRect.position.y + m + 2 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_nodes->setPosition(sf::Vector2f(s - _nodes->getGlobalBounds().size.x - m, _statsRect.position.y + m + 3 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_skins->setPosition(sf::Vector2f(s - _skins->getGlobalBounds().size.x - m, _statsRect.position.y + m + 4 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_joints->setPosition(sf::Vector2f(s - _joints->getGlobalBounds().size.x - m, _statsRect.position.y + m + 5 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_textures->setPosition(sf::Vector2f(s - _textures->getGlobalBounds().size.x - m, _statsRect.position.y + m + 6 * (basicFont.getLineSpacing(basic_text_size) + m)));
		_size->setPosition(sf::Vector2f(s - _size->getGlobalBounds().size.x - m, _statsRect.position.y + m + 7 * (basicFont.getLineSpacing(basic_text_size) + m)));

		loadModel();
		setButtonsActivity();
		setTooltips();
	}

	PlayerPanel::~PlayerPanel() {

	}

	void PlayerPanel::loadModel() {

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

		if (ModelsEditor::editor && ModelsEditor::editor->_preview_panel && ModelsEditor::editor->_preview_panel->_model) {

			std::shared_ptr<Anubis::Model3D> model = ModelsEditor::editor->_preview_panel->_model;

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

	void PlayerPanel::setButtonsActivity() {

		_first->setActive(true);
		_prev->setActive(true);
		_play->setActive(true);
		_pause->setActive(true);
		_next->setActive(true);
		_last->setActive(true);
		_anim_prev->setActive(true);
		_anim_next->setActive(true);
	}

	void PlayerPanel::setTooltips() {

		_first->setTooltip(256, L"First Frame", L"");
		_prev->setTooltip(256, L"Previous Frame", L"");
		_play->setTooltip(256, L"Play Animation", L"");
		_pause->setTooltip(256, L"Pause Animation", L"");
		_next->setTooltip(256, L"Next Frame", L"");
		_last->setTooltip(256, L"Last Frame", L"");
		_anim_prev->setTooltip(256, L"Previous Animation", L"");
		_anim_next->setTooltip(256, L"Next Animation", L"");
	}

	void PlayerPanel::cursorHover() {
		Components::Panel::cursorHover();

		_first->cursorHover();
		_prev->cursorHover();
		
		bool previewExists = ModelsEditor::editor->_preview_panel != nullptr;
		bool modelExists = ModelsEditor::editor->_preview_panel->_model != nullptr;
		bool hasAnimations = !ModelsEditor::editor->_preview_panel->_model->_animations.empty();
		bool activeAnimation = ModelsEditor::editor->_preview_panel->_model->_player._index != -1;
		bool isStoped = ModelsEditor::editor->_preview_panel->_model->_isAnimating == false;

		(previewExists && modelExists && hasAnimations && activeAnimation && isStoped) ? _play->cursorHover() : _pause->cursorHover();
		
		_next->cursorHover();
		_last->cursorHover();

		_anim_prev->cursorHover();
		_anim_next->cursorHover();

	}

	void PlayerPanel::handleEvent(const sf::Event& event) {
		Components::Panel::handleEvent(event);

		_first->handleEvent(event);
		_prev->handleEvent(event);

		bool previewExists = ModelsEditor::editor->_preview_panel != nullptr;
		bool modelExists = ModelsEditor::editor->_preview_panel->_model != nullptr;
		bool hasAnimations = !ModelsEditor::editor->_preview_panel->_model->_animations.empty();
		bool activeAnimation = ModelsEditor::editor->_preview_panel->_model->_player._index != -1;
		bool isStoped = ModelsEditor::editor->_preview_panel->_model->_isAnimating == false;

		(previewExists && modelExists && hasAnimations && activeAnimation && isStoped) ? _play->handleEvent(event) : _pause->handleEvent(event);

		_next->handleEvent(event);
		_last->handleEvent(event);

		_anim_prev->handleEvent(event);
		_anim_next->handleEvent(event);
	}

	void PlayerPanel::update() {

		if (!editor)
			return;

		Components::Panel::update();

		_first->update();
		_prev->update();
		_play->update();
		_pause->update();
		_next->update();
		_last->update();

		_anim_prev->update();
		_anim_next->update();
	}

	void PlayerPanel::draw() {
		Components::Panel::draw();

		Anubis::render_window->draw(*_title);
		
		_first->draw();
		_prev->draw();

		bool previewExists = ModelsEditor::editor->_preview_panel != nullptr;
		bool modelExists = ModelsEditor::editor->_preview_panel->_model != nullptr;
		bool hasAnimations = !ModelsEditor::editor->_preview_panel->_model->_animations.empty();
		bool activeAnimation = ModelsEditor::editor->_preview_panel->_model->_player._index != -1;
		bool isStoped = ModelsEditor::editor->_preview_panel->_model->_isAnimating == false;
		
		(previewExists && modelExists && hasAnimations && activeAnimation && isStoped) ? _play->draw() : _pause->draw();

		_next->draw();
		_last->draw();

		_anim_prev->draw();
		_anim_next->draw();

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