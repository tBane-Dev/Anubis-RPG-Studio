#pragma once
#include <fstream>

namespace Anubis {
	struct ObjectsCounts { int32_t animationsCount; int32_t prefabsCount; int32_t modelsCount; int32_t chunksCount; int32_t gameObjectsCount; };

	class Loader {
	public:
		ObjectsCounts counts;
		int _total;
		int _current;
		std::wstring _titleText;
		std::wstring _currentText;

		Loader(std::ifstream& file);
		~Loader();

		void setTitle(std::wstring title);
		void nextStep(std::wstring stepName);
		void end();
		void draw();
	};
}