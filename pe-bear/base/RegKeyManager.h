#include <string>

namespace RegKeyManager {
	bool removeRegPath(const std::string &extension, const std::string &appName);
	bool addRegPath(const std::string &extension, const std::string &appName, const std::string &value);
	bool isKeySet(const std::string &extension, const std::string &appName);
};
