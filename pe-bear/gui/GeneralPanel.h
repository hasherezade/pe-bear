#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../gui_base/PeGuiItem.h"
#include "../gui_base/ExtTableView.h"

#include "PackersTableModel.h"
#include "windows/StringsBrowseWindow.h"


class InfoTableModel : public PeTableModel
{
	Q_OBJECT

public:
	InfoTableModel(PeHandler *peHndl, QObject *parent = 0);
	virtual ~InfoTableModel() { }
	
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	
	int columnCount(const QModelIndex &parent) const { return 1; }
	int rowCount(const QModelIndex &parent) const;
	
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int);

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const 
	{
		return createIndex(row, column); //no index item pointer
	}

	QModelIndex parent(const QModelIndex &index) const { return QModelIndex(); } // no parent
};

//----

class GeneralPanel : public QSplitter, public PeViewItem
{
	Q_OBJECT
public:
	GeneralPanel(PeHandler *peHndl, QWidget *parent);

protected slots:
	void refreshView();
	void showExtractedStrCount();
	
protected:
	void init();
	void connectSignals();

	QDockWidget *packersDock, *stringsDock;
	QTextEdit md5Text;
	QTextEdit pathText;

	ExtTableView generalInfo;
	InfoTableModel generalInfoModel;
	FollowablePeTreeView packersTree;

	PackersTableModel packersModel;
	StringsBrowseWindow stringsBrowseWindow;
};
