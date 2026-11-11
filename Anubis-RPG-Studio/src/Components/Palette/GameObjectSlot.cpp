#include "Components/Palette/GameObjectSlot.hpp"
#include "RenderWindow.hpp"
#include "PrefabsManager.hpp"

GameObjectSlot::GameObjectSlot(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> hoverTexture, std::shared_ptr<Texture> pressTexture, std::shared_ptr<Texture> inactiveTexture, sf::Vector2i position) : Slot(texture, hoverTexture, pressTexture, inactiveTexture, position) {
	_object = std::weak_ptr<Object>();

	_scene = std::make_shared<Anubis::Scene>();
	_scene->initializeModel3DShaders();

	_model = nullptr;
	_modelRadius = 1.f;
}

GameObjectSlot::~GameObjectSlot() {

}

void GameObjectSlot::setObject(std::shared_ptr<Object> object) {

	if (!object) return;

	_object = object;

	std::shared_ptr<GameObject> prefab = std::dynamic_pointer_cast<GameObject>(_object.lock());

	if (!prefab) return;

	_scene->_models.clear();
	_model = nullptr;

	if (!prefab || !Anubis::prefabs_manager)
		return;

	// TO-DO - rewrite to the new prefabs manager
	std::shared_ptr<Anubis::Prefab> modelPrefab = Anubis::prefabs_manager->getPrefab(prefab->getName());
	if (!modelPrefab || !modelPrefab->_model)
		return;

	std::shared_ptr<Anubis::Model3D> model = std::make_shared<Anubis::Model3D>(*modelPrefab->_model);
	//

	sf::Vector3f boundsMin;
	sf::Vector3f boundsMax;
	bool hasBounds = false;

	for (auto& mesh : model->_meshes) {
		if (!mesh || !mesh->_geometry || !mesh->_geometry->_hasBounds)
			continue;

		sf::Vector3f mn = mesh->_geometry->_boundsMin;
		sf::Vector3f mx = mesh->_geometry->_boundsMax;

		if (!hasBounds) {
			boundsMin = mn;
			boundsMax = mx;
			hasBounds = true;
			continue;
		}

		boundsMin = sf::Vector3f(std::min(boundsMin.x, mn.x), std::min(boundsMin.y, mn.y), std::min(boundsMin.z, mn.z));
		boundsMax = sf::Vector3f(std::max(boundsMax.x, mx.x), std::max(boundsMax.y, mx.y), std::max(boundsMax.z, mx.z));
	}

	if (!hasBounds)
		return;

	sf::Vector3f center = (boundsMin + boundsMax) * 0.5f;
	sf::Vector3f size = boundsMax - boundsMin;

	_modelRadius = 0.5f * std::sqrt(size.x * size.x + size.y * size.y + size.z * size.z);
	if (_modelRadius <= 0.f) _modelRadius = 1.f;

	// the model stands in the middle of its own scene
	model->_scale = 1.f;
	model->_rotation = Anubis::Quaternion(1.f, 0.f, 0.f, 0.f);
	model->_position = -center;

	_model = model;
	_scene->addModel(_model);
}

void GameObjectSlot::drawModel() {
	sf::Vector2u windowSize = Anubis::render_window->getSize();

	int padding = 4;
	sf::Vector2i topLeft = Anubis::render_window->mapCoordsToPixel(sf::Vector2f(_rect.position + sf::Vector2i(padding, padding)));
	sf::Vector2i bottomRight = Anubis::render_window->mapCoordsToPixel(sf::Vector2f(_rect.position + _rect.size - sf::Vector2i(padding, padding)));

	int width = bottomRight.x - topLeft.x;
	int height = bottomRight.y - topLeft.y;

	if (width <= 0 || height <= 0)
		return;

	// OpenGL counts y from the bottom of the window
	int glY = (int)windowSize.y - bottomRight.y;

	glEnable(GL_SCISSOR_TEST);
	glScissor(topLeft.x, glY, width, height);
	glViewport(topLeft.x, glY, width, height);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glClear(GL_DEPTH_BUFFER_BIT);

	float r = _modelRadius * 1.05f;
	float aspect = (float)width / (float)height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-r * aspect, r * aspect, -r, r, -r * 4.f, r * 4.f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	std::array<float, 16> rotation = _scene->_camera->_rotation.matrix();
	glMultMatrixf(rotation.data());

	_scene->draw();

	glDisable(GL_SCISSOR_TEST);
	Anubis::render_window->resetGLStates();
}

void GameObjectSlot::cursorHover() {
	ButtonWithSprite::cursorHover();
}

void GameObjectSlot::handleEvent(const sf::Event& event) {
	ButtonWithSprite::handleEvent(event);
}

void GameObjectSlot::update() {
	ButtonWithSprite::update();

	_scene->update();
}

void GameObjectSlot::draw() {

	Slot::draw();

	if (_object.expired())
		return;

	if (_model) {
		drawModel(); 
		return;
	}

}