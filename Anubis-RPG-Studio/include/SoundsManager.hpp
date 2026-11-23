#pragma once
#include <SFML/Audio.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Anubis {

	class SoundBuffer {
	public:
		std::wstring _name;
		sf::SoundBuffer _buffer;

		SoundBuffer(std::wstring name, std::filesystem::path path);
		~SoundBuffer();
	};

	class SoundsManager {
	public:
		std::vector<std::shared_ptr<SoundBuffer>> _buffers;
		std::vector<std::shared_ptr<sf::Sound>> _sounds;

		SoundsManager();
		~SoundsManager();

		void addBuffer(std::wstring name, std::filesystem::path path);
		std::shared_ptr<SoundBuffer> getBuffer(std::wstring name);
		std::shared_ptr<sf::Sound> addSound(std::wstring name, sf::Vector3f position);
		void clear();
		void update();
	};

	extern std::shared_ptr<SoundsManager> sounds_manager;
}
