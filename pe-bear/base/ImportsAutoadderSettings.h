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
	
	size_t calcDllNamesSpace() const
	{
		size_t areaSize = 0;
		for (auto itr = dllFunctions.begin(); itr != dllFunctions.end(); ++itr) {
			const QString dllName = itr.key();
			areaSize += (dllName.length() + 1);
		}
		return areaSize;
	}
	
	size_t calcFuncNamesSpace() const
	{
		size_t areaSize = 0;
		for (auto dItr = dllFunctions.begin(); dItr != dllFunctions.end(); ++dItr) {
			const QStringList funcs = dItr.value();
			for (auto fItr = funcs.begin(); fItr != funcs.end(); ++fItr) {
				const QString func = *fItr;
				areaSize += (func.length() + 1);
			}
		}
		return areaSize;
	}
	
	size_t calcThunksCount() const
	{
		size_t thunksNeeded = 0;
		for (auto dItr = dllFunctions.begin(); dItr != dllFunctions.end(); ++dItr) {
			const QStringList funcs = dItr.value();
			thunksNeeded += (funcs.size() + 1); // records for all the functions, plus the terminator
		}
		return thunksNeeded;
	}
	
	bool addNewSec;
	QMap<QString, QStringList> dllFunctions;
};