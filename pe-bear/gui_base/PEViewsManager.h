#pragma once

#include <QtGui>
#include "../PEDockedWidget.h"

class PEViewsManager : public QMainWindow
{
	Q_OBJECT
	
signals:
	void signalChangeHexViewSettings(HexViewSettings &);
	void signalChangeDisasmViewSettings(DisasmViewSettings &);
	void globalFontChanged();

public:
	PEViewsManager(QWidget* parent)
		: QMainWindow(parent)
	{
	}
	
	PEDockedWidget* getPeDockWidget(PeHandler* pe);
	bool removePeDockWidget(PeHandler* pe);
	void clear();

	~PEViewsManager();
	
public slots:

	void changeHexViewSettings(HexViewSettings &_settings)
	{
		hexSettings = _settings;
		emit signalChangeHexViewSettings(_settings);
	}
	
	void changeDisasmViewSettings(DisasmViewSettings &_settings)
	{
		disasmSettings = _settings;
		emit signalChangeDisasmViewSettings(_settings);
	}

	void onGlobalFontChanged()
	{
		emit globalFontChanged();
	}

protected:
	std::map<PeHandler*, PEDockedWidget*> PeViews;
	std::vector<PEDockedWidget*> lastDock;
	
	HexViewSettings hexSettings;
	DisasmViewSettings disasmSettings;
};
