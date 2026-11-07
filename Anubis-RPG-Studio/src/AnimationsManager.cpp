#include "AnimationsManager.hpp"
#include "DebugLog.hpp"
#include "BinaryWriter.hpp"
#include "BinaryReader.hpp"

Animations::Animations(std::wstring path, sf::Vector2i frameSize, int animationsCount, int framesCount, bool& loadingStatus, int offsetX, int offsetY, float interval) {
	
	_path = path;

	_frameSize = frameSize;
	_animationsCount = animationsCount;
	_framesCount = framesCount;
	

	_texture = textures_manager->getTexture(path);

	_offsetX = offsetX;
	_offsetY = offsetY;

	loadingStatus = (_texture)? true : false;

	_interval = interval;

	
}

Animations::Animations(std::wstring name, std::shared_ptr<Texture> texture, sf::Vector2i frameSize, int animationsCount, int framesCount, int offsetX, int offsetY, float interval) {
	_path = name;

	_frameSize = frameSize;
	_animationsCount = animationsCount;
	_framesCount = framesCount;

	_texture = texture;

	_offsetX = offsetX;
	_offsetY = offsetY;

	_interval = interval;
}

Animations::~Animations() {
	
}
    
sf::IntRect Animations::getFrameRect(int animation, int frame) {
	
	if (!_texture || _framesCount <= 0 || _animationsCount <= 0)
		return sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(0, 0));

	sf::IntRect frameRect;
	
	frameRect.size = _frameSize;
	frameRect.position.x = frame * frameRect.size.x + _offsetX;
	frameRect.position.y = animation * frameRect.size.y + _offsetY;
	
	return frameRect;

}

std::shared_ptr<Texture> Animations::getTexture() {
	return _texture;
}

AnimationsManager::AnimationsManager() {
	_animations.clear();
}

AnimationsManager::~AnimationsManager() {
	
}

std::weak_ptr<Animations> AnimationsManager::getAnimations(std::wstring path) {
	for (auto& anim : _animations) {
		if (anim && anim->_path == path) {
			return anim;
		}
	}

	return std::weak_ptr<Animations>();
}

std::weak_ptr<Animations> AnimationsManager::getAnimations(int index) {
	if (index >= 0 && index < (int)_animations.size()) {
		return _animations[index];
	}

	return std::weak_ptr<Animations>();
}

int AnimationsManager::getAnimationsID(std::wstring path) {
	for (int i = 0; i < (int)_animations.size(); i++) {
		if (_animations[i] && _animations[i]->_path == path) {
			return i;
		}
	}

	return -1;
}

void AnimationsManager::removeAnimations(int index) {
	if (index >= 0 && index < (int)_animations.size()) {
		_animations.erase(_animations.begin() + index);
	}
}

int AnimationsManager::getAnimationsCount() {
	return (int)_animations.size();
}

int AnimationsManager::addAnimations(std::wstring name, std::shared_ptr<Texture> texture, sf::Vector2i frameSize, int animationsCount, int framesCount, float interval) {
	std::shared_ptr<Animations> animations = std::make_shared<Animations>(name, texture, frameSize, animationsCount, framesCount);
	_animations.push_back(animations);
	return _animations.size() - 1;
}

int AnimationsManager::addAnimations(std::shared_ptr<Animations> animations) {
	_animations.push_back(animations);
	return _animations.size() - 1;
}

void AnimationsManager::saveFromProject(std::ofstream& file) {

	BinaryWriter writer(file);
		
	for (auto& a : _animations) {
		writer.write_wstring(a->_path);
		writer.write_Vector2i(a->_frameSize);
		writer.write_int32(a->_animationsCount);
		writer.write_int32(a->_framesCount);
		writer.write_int32(a->_offsetX);
		writer.write_int32(a->_offsetY);
		writer.write_Image(a->_texture->_texture->copyToImage());
		writer.write_float(a->_interval);

		//DebugLog(L"Saved animations: " + a->_path);
		//DebugLog(L"Frame size: " + std::to_wstring(a->_frameSize.x) + L"x" + std::to_wstring(a->_frameSize.y));
		//DebugLog(L"Animations count: " + std::to_wstring(a->_animationsCount));
		//DebugLog(L"Frames count: " + std::to_wstring(a->_framesCount));
		//DebugLog(L"Offset: " + std::to_wstring(a->_offsetX) + L"," + std::to_wstring(a->_offsetY));
		//DebugLog(L"Texture size: " + std::to_wstring(a->_texture->getSize().x) + L"x" + std::to_wstring(a->_texture->getSize().y));
		//DebugLog(L"Interval: " + std::to_wstring(a->_interval));
		//DebugLog(L"----");
	}
}

void AnimationsManager::loadFromProject(std::ifstream& file, std::shared_ptr<Loader>& loader) {
	
	loader->setTitle(L"Loading animations ...");
	BinaryReader reader(file);

	_animations.clear();
	int animationsCount = loader->counts.animationsCount;

	DebugLog(L"Loading animations:");

	for (int i = 0; i < animationsCount; i++) {
		std::wstring path = reader.read_wstring();
		
		loader->nextStep(path); 

		sf::Vector2i frameSize = reader.read_Vector2i();
		int animationsCount = reader.read_int32();
		int framesCount = reader.read_int32();
		int offsetX = reader.read_int32();
		int offsetY = reader.read_int32();
		std::shared_ptr<sf::Image> image = std::make_shared<sf::Image>(reader.read_Image());
		float interval = reader.read_float();
		std::shared_ptr<Texture> texture = std::make_shared<Texture>(path, image);
		std::shared_ptr<Animations> animations = std::make_shared<Animations>(path, texture, frameSize, animationsCount, framesCount, offsetX, offsetY, interval);
		addAnimations(animations);

		DebugStat(path);

		//DebugLog(L"Loaded animations: " + path);
		//DebugLog(L"Frame size: " + std::to_wstring(frameSize.x) + L"x" + std::to_wstring(frameSize.y));
		//DebugLog(L"Animations count: " + std::to_wstring(animationsCount));
		//DebugLog(L"Frames count: " + std::to_wstring(framesCount));
		//DebugLog(L"Offset: " + std::to_wstring(offsetX) + L"," + std::to_wstring(offsetY));
		//DebugLog(L"Texture size: " + std::to_wstring(texture->getSize().x) + L"x" + std::to_wstring(texture->getSize().y));
		//DebugLog(L"Interval: " + std::to_wstring(animations->_interval));
		//DebugLog(L"----");
	}
}

void AnimationsManager::loadAnimations(std::wstring path, sf::Vector2i frameSize, int animationsCount, int framesCount, float interval) {
	
	bool loadingStatus = true;
    std::shared_ptr<Animations> animations = std::make_shared<Animations>(path, frameSize, animationsCount, framesCount,  loadingStatus, 0, 0, interval);
    
    if(loadingStatus)
        _animations.push_back(animations);
}

std::shared_ptr<AnimationsManager> animations_manager = nullptr;