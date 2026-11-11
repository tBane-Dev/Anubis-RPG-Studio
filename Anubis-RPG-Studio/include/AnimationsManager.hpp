#pragma once
#include <SFML/Graphics.hpp>
#include "TexturesManager.hpp"
#include "3D/Model3D.hpp"
#include "Loader.hpp"
#include <fstream>

class Animations {
public:
    std::wstring _path;
    std::shared_ptr<Texture> _texture;
    sf::Vector2i _frameSize;
    int _offsetX;
    int _offsetY;
    int _animationsCount;
    int _framesCount;
    float _interval;
    
    Animations(std::wstring name, std::shared_ptr<Texture> texture, sf::Vector2i frameSize, int animationsCount, int framesCount, int offsetX=0, int offsetY=0, float interval = 0.2f);
    Animations(std::wstring path, sf::Vector2i frameSize, int animationsCount, int framesCount, bool& loadingStatus, int offsetX=0, int offsetY=0, float interval = 0.2f);
    ~Animations();
    

    sf::IntRect getFrameRect(int animation, int frame);
    std::shared_ptr<Texture> getTexture();
};

class AnimationsManager {
public:
    std::vector<std::shared_ptr<Animations>> _animations;
    
    AnimationsManager();
    ~AnimationsManager();
    
    int addAnimations(std::wstring name, std::shared_ptr<Texture> texture, sf::Vector2i frameSize, int animationsCount, int framesCount, float interval);
	int addAnimations(std::shared_ptr<Animations> animations);
    std::weak_ptr<Animations> getAnimations(std::wstring path);
    std::weak_ptr<Animations> getAnimations(int index);
    int getAnimationsID(std::wstring path);
    void removeAnimations(int index);
    int getAnimationsCount();

    void saveFromProject(std::ofstream& file);
    void loadFromProject(std::ifstream& file, std::shared_ptr<Loader>& loader);

    void loadAnimations(std::wstring path, sf::Vector2i frameSize, int animationsCount, int framesCount, float interval);
};

extern std::shared_ptr<AnimationsManager> animations_manager;

namespace Anubis {

    class ModelsManager {
    public:
        std::vector<std::shared_ptr<Model3D>> _models;

        ModelsManager();
        ~ModelsManager();

        int addModel(std::wstring name, std::filesystem::path path);
        int addModel(std::wstring name, std::shared_ptr<Model3D> model);
        int addModel(std::shared_ptr<Model3D> model);
        std::weak_ptr<Model3D> getModel(std::wstring name);
        std::weak_ptr<Model3D> getModel(int index);
        int getModelID(std::wstring name);
        void removeModel(int index);
        int getModelsCount();

        void saveFromProject(std::ofstream& file);
        void loadFromProject(std::ifstream& file, std::shared_ptr<Loader>& loader);

        void load();
    };

    extern std::shared_ptr<ModelsManager> models_manager;
}