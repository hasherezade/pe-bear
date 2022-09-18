#pragma once

#include <QtGui>
#include <bearparser/bearparser.h>

#include "../../PEFileTreeModel.h"
#include "../../gui/HexDiffModel.h"
#include "../../gui_base/ExtTableView.h"

#include "../../HexCompareView.h"

//----------------------------------------------------

class DiffWindow : public QMainWindow
{
    Q_OBJECT

public:
	DiffWindow(PeHandlersManager &peManager, QWidget *parent);

signals:
	void contentChanged(BYTE* contentPtr, int size, offset_t startOffset,  ContentIndx indx);
	void contentCleared(ContentIndx indx);
	void addrFormatChanged(int fmt);

public slots:
	void refresh();
	void file1Selected(const QString &text);
	void file2Selected(const QString &text);
	void item1Marked(const QModelIndex & current, const QModelIndex & previous);
	void item2Marked(const QModelIndex & current, const QModelIndex & previous);

	void hexSelectedL();
	void hexSelectedR();
	void hexSelected(ContentIndx contentIndx);

	void changeHexViewSettings(HexViewSettings &_settings)
	{
		hexDumpModelL.changeSettings(_settings);
		hexDumpModelR.changeSettings(_settings);
	}

	void onGlobalFontChanged()
	{
		resizeComponents();
	}

private slots:
	void onSliderMoved(int val);
	void onAddrFormatSelected(int val);

protected:
	void resizeComponents();
	bool createFormatChanger(ContentIndx indx, QToolBar* parent);
	bool reselectPrevious(QList<QString> &stringsList, ContentIndx contentIndx);
	void createActions();
	void destroyActions();

	void removeUnusedTreeModels();
	void setTreeModel(QTreeView &treeView, const QString &text);
	void setPEContent(const QString &fileName, int offset, ContentIndx contentIndx);
	void itemMarked(const QModelIndex & current, const QModelIndex &previous, QTreeView &treeView, ContentIndx contentIndx);

	PeHandlersManager &peManger;
	std::map<QString, PEFileTreeModel*> peModels;
	
	QSplitter mainSplitter;
	QSplitter leftSplitter, rightSplitter;
	QTreeView PEsTree1, PEsTree2;

	QComboBox fileCombo[CNTR];
	QTreeView treeView[CNTR];

	HexDiffModel hexDumpModelL, hexDumpModelR;
	HexCompareView fileView[CNTR];
	QScrollBar fileScrollBar[CNTR];

	QTextEdit numEdit[CNTR];

	offset_t contentOffset[CNTR];
	BYTE* contentPtr[CNTR];
	size_t contentSize[CNTR];

	QToolBar toolBars[CNTR];
	QComboBox addrFmtBox[CNTR];
	QStatusBar statusBar;

	QAction *setHexView;
	QAction *nextDiff;

	QString currName[CNTR];
};

