#pragma once
#include "Objects/Nature.hpp"

NaturePrefab::NaturePrefab(std::wstring name, std::weak_ptr<Animations> animations, sf::Vector2i origin, std::shared_ptr<Collider> collider, std::shared_ptr<Mesh> mesh, std::weak_ptr<Anubis::Model3D> model) : GameObject(name, animations, origin, collider, mesh, model) {
	_type = ObjectType::Nature;
}

NaturePrefab::~NaturePrefab() {

}

Nature::Nature(std::weak_ptr<GameObject> prefab) : PlacedGameObject(prefab) {
	_type = ObjectType::Nature;
}

Nature::~Nature() {

}