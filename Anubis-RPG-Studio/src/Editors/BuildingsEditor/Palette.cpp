#include "Editors/BuildingsEditor/Palette.hpp"
#include "Editors/BuildingsEditor/Editor.hpp"
#include "DebugLog.hpp"

namespace BuildingsEditor {

	Palette::Palette() : Components::Palette() {

		_categories = std::make_shared<Categories>();

		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\walls.png"),
			ObjectType::None,
			[this]() { loadAll(ObjectType::None); }
		);


		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\floors.png"),
			ObjectType::None,
			[this]() { loadAll(ObjectType::None); }
		);

		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\doors.png"),
			ObjectType::None,
			[this]() { loadAll(ObjectType::None); }
		);

		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\windows.png"),
			ObjectType::None,
			[this]() { loadAll(ObjectType::None); }
		);

		_categories->addCategory(
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_hover.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\category_press.png"),
			textures_manager->getTexture(L"assets\\tex\\palette\\categories\\wall_mounted.png"),
			ObjectType::None,
			[this]() { loadAll(ObjectType::None); }
		);

		_tools = std::make_shared<ToolsTerrain>();
		_slots = std::make_shared<Slots>();


		// set the active group
		loadAll(ObjectType::None);
		//_slots->setCategory(ObjectType::None);
	}

	Palette::~Palette() {

	}

	void Palette::addTools() {
		
		_tools = nullptr;
	}

	void Palette::loadAll(ObjectType type) {
		_categories->setCategory(type);
		addTools();
		

		if(auto tools = std::dynamic_pointer_cast<ToolsTerrain>(_tools); tools) {
			if (tools->_toolType == ToolTerrainType::None) {
				tools->setTool(tools->_tools[0], ToolTerrainType::None);
			}
		}

		_slots->setCategory(type);

		_slots->setFunction(
			[this](std::shared_ptr<Slot> slot, int selectedSlotId) {
				//BuildingsEditor::editor->_building_panel->_cursorOnBuilding->_object = slot->_object;
				_slots->selectSlot(selectedSlotId);
			}
		);

		setPosition(getPosition());
	}

	void Palette::draw() {
		Components::Palette::draw();
	}
}
