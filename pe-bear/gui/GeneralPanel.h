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

class StringsTableModel : public QAbstractTableModel
{
	Q_OBJECT

signals:
	void modelUpdated();

protected slots:
	virtual void onNeedReset() { reset(); emit modelUpdated(); }

public:
	enum COLS {
		COL_OFFSET = 0,
		COL_STRING,
		MAX_COL
	};

	StringsTableModel(PeHandler *peHndl, QObject *parent = 0);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	int rowCount(const QModelIndex &parent) const;//{ return INFO_COUNTER; }

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int) { return false; }

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const { return createIndex(row, column); } //no index item pointer
	QModelIndex parent(const QModelIndex &index) const { return QModelIndex(); } // no parent

	void reset()
	{
		//>
		this->beginResetModel();
		this->endResetModel();
		//<
	}

protected:
	PeHandler *m_PE;
};
//----

class GeneralPanel : public QSplitter, public PeViewItem
{
	Q_OBJECT
public:
	GeneralPanel(PeHandler *peHndl, QWidget *parent);

protected slots:
	void refreshView();

protected:
	void init();
	void connectSignals();
	
	QDockWidget *packersDock, *stringsDock;
	QTextEdit md5Text;
	QTextEdit pathText;

	ExtTableView generalInfo;
	InfoTableModel generalInfoModel;
	FollowablePeTreeView packersTree;
	QTableView stringsTable;
	PackersTableModel packersModel;
	StringsTableModel stringsModel;
};
