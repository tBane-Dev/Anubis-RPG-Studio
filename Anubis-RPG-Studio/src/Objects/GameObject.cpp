#pragma once
#include "Objects/GameObject.hpp"
#include "RenderWindow.hpp"
#include "Objects/Collider.hpp"
#include "Objects/Mesh.hpp"

GameObject::GameObject(std::wstring name, sf::Vector2i origin, std::shared_ptr<Collider> collider, std::shared_ptr<Mesh> mesh, std::weak_ptr<Anubis::Model3D> model) : Object()
{
	_name = name;
	_origin = origin;
	_collider = collider;
	_mesh = mesh;
	_model = model;
}

GameObject::GameObject(std::wstring name) : Object()
{
	_name = name;
	_origin = sf::Vector2i(0, 0);
	_collider = nullptr;
	_mesh = nullptr;
	_model = std::weak_ptr<Anubis::Model3D>();
}

GameObject::~GameObject() {

}

std::wstring GameObject::getName() {
	return _name;
}

sf::Vector2i GameObject::getOrigin() {
	return _origin;
}

std::shared_ptr<Collider> GameObject::getCollider() {
	return _collider;
}

std::shared_ptr<Mesh> GameObject::getMesh() {
	return _mesh;
}