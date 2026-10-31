#pragma once
#include "Objects/Building/Outside.hpp"
#include "RenderWindow.hpp"
#include "DebugLog.hpp"
#include "Objects/Building/Building.hpp"
#include "Editors/MapEditor/Editor.hpp"
#include "Editors/BuildingsEditor/Editor.hpp"
#include "Editors/MapEditor/Map/CursorOnMap.hpp"
#include "Editors/MapEditor/Map/PlacedGameObjects.hpp"
#include "ShadersManager.hpp"


Outside::Outside(std::weak_ptr<Building> building) : PlacedGameObject(std::weak_ptr<GameObject>()){
	_type = ObjectType::Outside;
	_building = building;
}

Outside::~Outside() {

}

void Outside::setTexture(sf::Texture texture) {
	_texture = texture;
}

sf::Vector2i Outside::getSortPosition() {
	std::shared_ptr<Building> building = _building.lock();
	std::shared_ptr<BuildingPrefab> prefab = std::dynamic_pointer_cast<BuildingPrefab>(building->_prefab.lock());
	std::shared_ptr<Roof> roof = prefab->_roof;

	return _position - roof->_roofOverhangSize;

}

void Outside::cursorHover() {

}

void Outside::draw() {
	
	std::shared_ptr<BuildingPrefab> buildingPrefab = std::dynamic_pointer_cast<BuildingPrefab>(_building.lock()->_prefab.lock());

	if (_building.lock()->_renderOutsideLook) {

		

		if (Main::editor_manager->get_back() == BuildingsEditor::editor) {


			std::shared_ptr<Roof> roof = buildingPrefab->_roof;
			sf::Vector2i roofOverhangSize = roof ? roof->_roofOverhangSize : sf::Vector2i(0, 0);
			float scale = BuildingsEditor::editor->_building_panel->_building->_scale;

			sf::Vector2f buildingPosition(_building.lock()->getPosition());
			float topOffset = roof ? roof->getTopOffset(scale) : 0.f;

			sf::Vector2f position(
				buildingPosition.x - (float)(roofOverhangSize.x) * scale,
				buildingPosition.y -  topOffset - (float)(roofOverhangSize.y) * scale
			);


			sf::Sprite sprite(_texture);
			sprite.setPosition(position);
			sprite.setScale(sf::Vector2f(scale, scale));
			Anubis::render_window->draw(sprite);
		}

		if (Main::editor_manager->get_back() == MapEditor::editor) {
			//std::shared_ptr<Roof> roof = buildingPrefab->_roof;
			//sf::Vector2i roofOverhangSize = roof ? roof->_roofOverhangSize : sf::Vector2i(0, 0);
			//sf::Sprite sprite(_texture);
			//sf::Vector2f position = sf::Vector2f(_position.x - roofOverhangSize.x, _position.y - roofOverhangSize.y - (int)(_texture.getSize().y));
			//sprite.setPosition(position);
			//Anubis::render_window->draw(sprite);
		
			const sf::View& view = Anubis::render_window->getView();
			sf::Vector2f size = view.getSize();
			sf::IntRect viewport = Anubis::render_window->getViewport(view);
			const float aspect = (float)(viewport.size.x) / (float)(viewport.size.y);
			float pi = 3.14159265f;
			float fov = 60.0f;
			const float distance = (size.y * 0.5f) / std::tan(fov * 0.5f * pi / 180.0f);

			std::shared_ptr<Roof> roof = buildingPrefab->_roof;
			sf::Vector2i roofOverhangSize = roof ? roof->_roofOverhangSize : sf::Vector2i(0, 0);
			sf::Sprite sprite(_texture);
			sf::Vector2f position = sf::Vector2f(_position.x - roofOverhangSize.x, _position.y - roofOverhangSize.y - (int)(_texture.getSize().y));
			sf::Vector2f origin(float(roofOverhangSize.x), float(roofOverhangSize.y) + float(_texture.getSize().y));
			sprite.setPosition(sf::Vector2f(0, 0));
			sprite.setOrigin(sf::Vector2f(0, 0));

			sprite25D_shader->setUniform("cameraCenter", view.getCenter());
			sprite25D_shader->setUniform("cameraDistance", distance);
			sprite25D_shader->setUniform("aspect", aspect);
			sprite25D_shader->setUniform("spritePosition", sf::Vector2f(position) + origin);
			sprite25D_shader->setUniform("spriteOrigin", origin);
			sprite25D_shader->setUniform("texture", sf::Shader::CurrentTexture);

			sf::RenderStates states;
			states.shader = &*sprite25D_shader;

			Anubis::render_window->draw(sprite, states);
		}

		
	}
}