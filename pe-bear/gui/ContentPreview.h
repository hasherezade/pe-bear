#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../gui_base/PeGuiItem.h"
#include "../HexView.h"

class ContentPreview : public QSplitter, public PeViewItem
{
	Q_OBJECT
public:
	ContentPreview(PeHandler *peHndl, QWidget *parent);
	~ContentPreview();

signals:
	void signalChangeHexViewSettings(HexViewSettings &settings);

public slots:
	// forward the settings change to the individual HexTableView-s:
	void changeHexViewSettings(HexViewSettings &_settings)
	{
		emit signalChangeHexViewSettings(_settings);
	}

protected slots:
	void onGoToRVA();
	void onSliderMoved(int val);

protected:
	void applyViewsSettings();
	void connectViewsSignals();

	void createModels();
	void deleteModels();

	QScrollBar *hexScroll, *textScroll;
	HexTableView hexView,  textView;
	HexDumpModel *hexModel, *textModel;

	QMenu defaultMenu;
};
