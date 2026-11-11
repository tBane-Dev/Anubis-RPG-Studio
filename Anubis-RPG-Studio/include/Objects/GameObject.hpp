#pragma once
#include "Objects/Object.hpp"
#include "ModelsManager.hpp"
#include "Objects/Collider.hpp"
#include "Objects/Mesh.hpp"
#include "3D/Model3D.hpp"

class GameObject : public Object {
public:
	std::wstring _name;
    sf::Vector2i _origin;
	std::shared_ptr<Collider> _collider;
	std::shared_ptr<Mesh> _mesh;
	std::weak_ptr<Anubis::Model3D> _model;

	GameObject(std::wstring name, sf::Vector2i origin, std::shared_ptr<Collider> collider, std::shared_ptr<Mesh> mesh, std::weak_ptr<Anubis::Model3D> modelr);
	GameObject(std::wstring name);
	virtual ~GameObject();

	std::wstring getName();
	sf::Vector2i getOrigin();
	std::shared_ptr<Collider> getCollider();
	std::shared_ptr<Mesh> getMesh();
	std::shared_ptr<Anubis::Model3D> getModel();
};