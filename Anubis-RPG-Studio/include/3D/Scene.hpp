#pragma once
#include "3D/Camera.hpp"
#include "Model3D.hpp"
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>

namespace Anubis {

	sf::Vector3f shadowNormalize(sf::Vector3f v);
	sf::Vector3f shadowCross(sf::Vector3f a, sf::Vector3f b);
	float shadowDot(sf::Vector3f a, sf::Vector3f b);
	std::array<float, 16> shadowMultiply(std::array<float, 16> a, std::array<float, 16> b);

	class Scene : public std::enable_shared_from_this<Scene> {
	public:

		std::shared_ptr<Camera> _camera;
		float _cameraXAngle;
		float _cameraYAngle;
		float _cameraRadius;
		float _cameraZoom = 1.0f;
		sf::Vector2i _lastCameraMouse;
		bool _isPressedMiddle = false;
		float _panPlaneY = 0.f;		// height of the grabbed terrain point while panning with the middle button
		bool _cameraIsMoving = false;

		std::vector<std::shared_ptr<Model3D>> _models;
		sf::Shader _modelShader;

		GLuint _pickingFBO = 0;
		GLuint _pickingColor = 0;
		GLuint _pickingDepth = 0;
		int _pickingWidth = 0;
		int _pickingHeight = 0;

		std::vector<std::shared_ptr<Model3D>> _hovered;
		std::vector<std::shared_ptr<Model3D>> _pressed;
		std::vector<std::shared_ptr<Model3D>> _selected;	// highlight only, set by MapEditor::CursorOnMap

		// Projection parameters used by setProjection() and by unprojection.
		float _fovY = 60.f;
		float _nearPlane = 0.1f;
		float _farPlane = 10000.f;

		float _cameraSpeed = 400.f;	// WASD / arrows, pixels per second

		// Shadow map of the sun (directional light)
		int _shadowMapSize = 512;
		GLenum _shadowDepthComponent = 0x81A6;
		float _maxShadowRadius = 4000.f;
		int _shadowSamples = 1;			// 1 = one sample, 3 = 3x3 (terrain3D.frag)
		GLuint _shadowFBO = 0;
		GLuint _shadowDepth = 0;
		sf::Vector3f _sunDirection = { 0.1f, -1.0f, -0.1f };
		std::array<float, 16> _lightViewProjection{};

		// Fast model drawing: the model shader is bound once per pass, only per-model / per-mesh data changes.
		// Uniform locations are read once in initializeModel3DShaders().
		bool _fastPathReady = false;
		GLint _locModel = -1;
		GLint _locView = -1;
		GLint _locProjection = -1;
		GLint _locSkinned = -1;
		GLint _locPicking = -1;
		GLint _locTextured = -1;
		GLint _locTint = -1;
		GLint _locPickColor = -1;
		GLint _locWind = -1;

		// State of the current pass (beginModelPass .. endModelPass).
		GLfloat _passView[16] = {};
		GLfloat _passProjection[16] = {};
		bool _passPicking = false;
		bool _passCullMeshes = false;
		bool _passSkinned = false;
		bool _passTextured = false;
		GLuint _passTexture = 0;
		GLint _passPreviousProgram = 0;
		GLint _passPreviousVertexArray = 0;
		GLint _passPreviousTexture = 0;
		GLint _passPreviousActiveTexture = 0;

		Scene();
		~Scene();

		bool initializeModel3DShaders();

		void setViewport();
		void setProjection();
		void setCamera();
		void lookAt(sf::Vector3f target);

		void addModel(std::shared_ptr<Anubis::Model3D> model);
		void removeModel(std::shared_ptr<Model3D> model);

		bool createShadowBuffer();
		void renderShadowMap();
		bool beginModelPass(GLfloat* view, GLfloat* projection, bool picking, bool cullMeshes);
		void drawModelFast(Model3D& model, GLfloat* tint, GLfloat* pickColor);
		void endModelPass();

		void drawShadowCasters(std::array<float, 16> lightView, std::array<float, 16> lightProjection, sf::Vector3f center, sf::Vector3f side, sf::Vector3f up, float radius);

		void drawModels();

		bool createPickingBuffer(unsigned int width, unsigned int height);
		int getHoveredModelId();
		int getHoveredModelId(sf::Vector2i screenPixel);
		bool screenToRay(sf::Vector2i pixel, sf::Vector3f& origin, sf::Vector3f& dir);
		bool screenToRay(sf::Vector2f pixel, sf::Vector3f& origin, sf::Vector3f& dir);
		bool worldToScreen(sf::Vector3f point, sf::Vector2f& pixel);
		bool panByPixels(sf::Vector2f anchorPixel, sf::Vector2f pixelDelta);
		bool unprojectDepth(sf::Vector2i pixel, float depth, sf::Vector3f& hit);
		std::vector<std::shared_ptr<Model3D>> getModelsInRect(sf::IntRect rect);

		void cursorHover();
		void handleEvent(const sf::Event& event);
		void update();
		void draw();
	};

}