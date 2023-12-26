#pragma once

//#include <iostream>
#include <bearparser/bearparser.h>
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../base/PeHandler.h"


class StringsTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	enum COLS {
		COL_OFFSET = 0,
		COL_TYPE,
		COL_STRING,
		MAX_COL
	};

	StringsTableModel(PeHandler *peHndl, QObject *parent = 0);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	int rowCount(const QModelIndex &parent) const { return stringsOffsets.size(); }

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int) { return false; }

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
	{
		return createIndex(row, column); //no index item pointer
	}

	QModelIndex parent(const QModelIndex &index) const { return QModelIndex(); } // no parent

	void reset()
	{
		//>
		this->beginResetModel();
		reloadList();
		this->endResetModel();
		//<
	}

protected:
	bool reloadList()
	{
		if (!m_PE || m_PE->stringsMap.size() == 0) {
			this->stringsMap = nullptr;
			this->stringsOffsets.clear();
			return false;
		}
		this->stringsMap = &m_PE->stringsMap;
		this->stringsOffsets = stringsMap->getOffsets();
		return true;
	}

	StringsCollection *stringsMap;
	QList<offset_t> stringsOffsets;
	PeHandler *m_PE;
	size_t page;
};

//----

class StringsSortFilterProxyModel : public QSortFilterProxyModel
{
public:
	StringsSortFilterProxyModel(QObject *parent)
		: QSortFilterProxyModel(parent)
	{
	}
	
	bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const
	{
		QAbstractItemModel *source = sourceModel();
		if (!source) return false;
		
		QModelIndex index = source->index(sourceRow, StringsTableModel::COL_STRING, sourceParent);
		if (source->data(index).toString().toLower().trimmed().contains(filterRegExp()))
			return true;
		return false;
	}
};

//----------------------------------------------------

class StringsBrowseWindow : public QMainWindow
{
    Q_OBJECT
public:
	StringsBrowseWindow(PeHandler *peHndl, QWidget *parent)
		: myPeHndl(peHndl), stringsModel(nullptr), stringsProxyModel(nullptr)
	{
		this->stringsModel = new StringsTableModel(myPeHndl, this);
		this->stringsProxyModel = new StringsSortFilterProxyModel(this);
		stringsProxyModel->setSourceModel( this->stringsModel );
		stringsTable.setModel( this->stringsProxyModel );
		stringsTable.setSortingEnabled(false);
		stringsTable.setMouseTracking(true);
		stringsTable.setSelectionBehavior(QAbstractItemView::SelectItems);
		stringsTable.setSelectionMode(QAbstractItemView::SingleSelection);
		stringsTable.setAutoFillBackground(true);
		stringsTable.setAlternatingRowColors(false);
		QHeaderView *hdr = stringsTable.horizontalHeader();
		if (hdr) hdr->setStretchLastSection(true);
	
		initLayout();
		refreshView();
		if (myPeHndl) {
			connect(myPeHndl, SIGNAL(stringsUpdated()), this, SLOT(refreshView()));
		}
	}

private slots:
	void refreshView()
	{
		this->stringsModel->reset();
		this->stringsTable.reset();
	}
	
	void onSave();
	void onFilterChanged(QString);

private:
	void initLayout();
	
	PeHandler *myPeHndl;
	
	QTableView stringsTable;
	StringsTableModel *stringsModel;
	StringsSortFilterProxyModel* stringsProxyModel;
	
	QVBoxLayout topLayout;
	QHBoxLayout propertyLayout0;
	QPushButton saveButton;
	QLabel filterLabel;
	QLineEdit filterEdit;
};
