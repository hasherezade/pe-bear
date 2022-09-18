#include <string>

namespace RegKeyManager {
	bool removeRegPath(std::string extension, std::string appName);
	bool addRegPath(std::string extension, std::string appName, std::string value);
	bool isKeySet(std::string extension, std::string appName);
};
