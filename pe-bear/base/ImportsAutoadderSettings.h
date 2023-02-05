#pragma once

#include <QtCore>
//---

struct ImportsAutoadderSettings
{
	ImportsAutoadderSettings() : addNewSec(false) {}
	
	bool addImport(const QString &dll, const QString &func)
	{
		if (dll.length() == 0 || func.length() == 0) return false;
		if (dllFunctions.contains(dll)) {
			if (dllFunctions[dll].contains(func)) return false; // already exists
		}
		dllFunctions[dll].append(func);
		return true;
	}
	
	bool removeImport(const QString &dll, const QString &func)
	{
		if (!dllFunctions.contains(dll)) {
			return false;
		}
		if (!dllFunctions[dll].contains(func)) {
			return false;
		}
		dllFunctions[dll].removeAll(func);
		if (dllFunctions[dll].size() == 0) {
			dllFunctions.remove(dll);
		}
		return true;
	}

	bool addNewSec;
	QMap<QString, QStringList> dllFunctions;
};