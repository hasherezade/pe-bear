#ifdef _WINDOWS
#include <windows.h>
#include <stdio.h>
#include <strsafe.h>
#endif //_WINDOWS

#include "RegKeyManager.h"

std::string PART = "file\\shell\\Open with ";

bool RegKeyManager::removeRegPath(const std::string &extension, const std::string &appName)
{
#ifdef _WINDOWS
	int res1 = 0, res2 = 0;
	HKEY hKey = 0;

	std::string fullName = extension;
	fullName.append(PART).append(appName);
	LPCSTR lpSubKey = fullName.c_str();

	LONG res = RegCreateKeyA(HKEY_CLASSES_ROOT, lpSubKey, &hKey);
	if (res != ERROR_SUCCESS) {
		return false;
	}

	res1 = RegDeleteKeyA(hKey, "command");
	res2 = RegDeleteKeyA(hKey, "");

	res = RegCloseKey(HKEY_CLASSES_ROOT);
	if (res != ERROR_SUCCESS) {
		return false;
	}
	if (res1 == ERROR_SUCCESS || res2 == ERROR_SUCCESS) {
		return true;
	}
#endif //_WINDOWS
	return false;
}

bool RegKeyManager::addRegPath(const std::string &extension, const std::string &appName, const std::string &path)
{
#ifdef _WINDOWS

	int res1 = 0, res2 = 0;
	HKEY hKey = 0;

	std::string fullName = extension;
	fullName.append(PART).append(appName); // example: "dllfile\\shell\\Open with PE-bear";
	const std::string cmdName = fullName + "\\command";

	// set command:
	const std::string cmdPath = path + " \"%1\"";
	LONG res = RegCreateKeyA(HKEY_CLASSES_ROOT, cmdName.c_str(), &hKey);
	if (res != ERROR_SUCCESS) {
		return false;
	}
	res1 = RegSetValueExA(hKey, NULL, 0, REG_SZ, (BYTE*)cmdPath.c_str(), cmdPath.length());
	res = RegCloseKey(HKEY_CLASSES_ROOT);
	if (res1 != ERROR_SUCCESS || res != ERROR_SUCCESS) {
		return false;
	}
	
	//set icon:
	hKey = 0;
	res = RegCreateKeyA(HKEY_CLASSES_ROOT, fullName.c_str(), &hKey);
	if (res != ERROR_SUCCESS) {
		return false;
	}

	const std::string iconPath = path + ",0";
	res1 = RegSetValueExA(hKey, "Icon", 0, REG_SZ, (BYTE*)iconPath.c_str(), iconPath.length());

	res = RegCloseKey(HKEY_CLASSES_ROOT);
	if (res1 != ERROR_SUCCESS || res != ERROR_SUCCESS) {
		return false;
	}
	if (res1 == ERROR_SUCCESS) {
		return true;
	}
#endif //_WINDOWS
	return false;
}


bool RegKeyManager::isKeySet(const std::string &extension, const std::string &appName)
{
#ifdef _WINDOWS

	HKEY hKey = 0;
	std::string fullName = extension;
	fullName.append(PART).append(appName);
	LPCSTR lpSubKey = fullName.c_str();

	LONG res = RegOpenKeyExA(HKEY_CLASSES_ROOT, lpSubKey, 0, KEY_READ, &hKey);
	if (res != ERROR_SUCCESS) {
		return false;
	}
	res = RegCloseKey(HKEY_CLASSES_ROOT);
	if (res == ERROR_SUCCESS) {
		return true;
	}
#endif //_WINDOWS
	return false;
}

