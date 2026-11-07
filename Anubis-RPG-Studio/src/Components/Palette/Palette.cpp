#include "Components/Palette/Palette.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "Objects/Monster.hpp"
#include "PrefabsManager.hpp"
#include "RenderWindow.hpp"
#include "Cursor.hpp"
#include "Editors/MapEditor/Map/CursorOnMap.hpp"
#include "DebugLog.hpp"
#include "Editors/BuildingsEditor/Editor.hpp"
#include "Wallset.hpp"
#include <queue>



namespace Components {

	std::vector<std::shared_ptr<Terrain>> Palette::terrains;

	Palette::Palette() : Element() {


		if (Palette::terrains.empty()) {
			for (int i = 0; i < tileset->groups.size(); i++){
				Palette::terrains.emplace_back(std::make_shared<Terrain>(i, sf::IntRect(sf::Vector2i(15 * 64, i * 64), sf::Vector2i(16, 16))));
			}
		}

		sf::Vector2i size;
		size.x = 600;
		size.y = Anubis::render_window->getSize().y - MapEditor::editor->_main_menu->getSize().y;
		_rect = sf::IntRect(sf::Vector2i(Anubis::render_window->getSize().x - size.x, MapEditor::editor->_main_menu->getSize().y), size);

		_categories = std::make_shared<Categories>();
		_tools = std::make_shared<ToolsTerrain>();
		_slots = std::make_shared<Slots>();

		_minBrushSize = 0;
		_maxBrushSize = 5;
		_brushSize = 2;

		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			nullptr,
			ObjectType::None,
			[this]() {
				loadAll(ObjectType::None);
			}
		);

		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			nullptr, 
			ObjectType::None,
			[this]() {
				loadAll(ObjectType::None);
			}
		);

		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			nullptr, 
			ObjectType::None,
			[this]() {
				loadAll(ObjectType::None);
			}
		);

		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			nullptr, 
			ObjectType::None,
			[this]() {
				loadAll(ObjectType::None);
			}
		);

		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			nullptr,
			ObjectType::None,
			[this]() {
				loadAll(ObjectType::None);
			}
		);

		// set the active group
		loadAll(ObjectType::None);

		if (auto tools = std::dynamic_pointer_cast<ToolsTerrain>(_tools)) {
			tools->setTool(nullptr, ToolTerrainType::None);
		}
	}

	Palette::~Palette() {

	}	

	sf::Vector2i Palette::getPosition() {
		return _rect.position;
	}

	sf::Vector2i Palette::getSize() {
		return _rect.size;
	}

	void Palette::addTools() {
		_tools = nullptr;
	}

	void Palette::loadAll(ObjectType type) {
		_categories->setCategory(type);
		addTools();
		

		_slots->setCategory(type);
		
		setPosition(getPosition());
	}

	void Palette::setPosition(sf::Vector2i position) {

		_rect.position = position;

		int margin = 16;

		// Categories
		_categories->setPosition(position);

		// Tools + Slots
		if (_tools) {
			_tools->setPosition(position + sf::Vector2i(0, _categories->getSize().y + margin));
			_slots->setPosition(position + sf::Vector2i(0, _categories->getSize().y + margin + _tools->getSize().y + margin));
		}
		else {
			_slots->setPosition(position + sf::Vector2i(0, _categories->getSize().y + margin));
		}
	}

	void Palette::cursorHover() {
		
		if (_rect.contains(Cursors::cursor->_position)) {
			GUI_manager->Element_hovered = shared_from_this();
		}

		_categories->cursorHover();

		if (_tools != nullptr)
			_tools->cursorHover();

		_slots->cursorHover();

	}

	void Palette::handleEvent(const sf::Event& event) {

		if (GUI_manager->Element_hovered.get() == this) {
			if (const auto* mbp = event.getIf<sf::Event::MouseButtonPressed>(); mbp && mbp->button == sf::Mouse::Button::Left) {
				GUI_manager->Element_pressed = shared_from_this();
			}
		}

		if (GUI_manager->Element_pressed.get() == this) {
			if (const auto* mbr = event.getIf<sf::Event::MouseButtonReleased>(); mbr && mbr->button == sf::Mouse::Button::Left) {
				GUI_manager->Element_pressed = nullptr;
			}
		}

		_categories->handleEvent(event);

		if (_tools != nullptr)
			_tools->handleEvent(event);

		_slots->handleEvent(event);

	}

	void Palette::update() {

		_categories->update();

		if (_tools != nullptr)
			_tools->update();

		_slots->update();

	}

	void Palette::draw() {

		GUI_manager->setView();

		sf::RectangleShape rect(sf::Vector2f(_rect.size));
		rect.setFillColor(sf::Color(31, 31, 31));
		rect.setPosition(sf::Vector2f(_rect.position));
		Anubis::render_window->draw(rect);

		_categories->draw();
		if (_tools != nullptr)
			_tools->draw();
		_slots->draw();

		GUI_manager->setView();

	}

}