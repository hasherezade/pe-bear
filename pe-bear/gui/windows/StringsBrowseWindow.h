#pragma once

#include <bearparser/bearparser.h>
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../../base/PeHandler.h"
#include "../../base/MainSettings.h"
#include "../followable_table/FollowableOffsetedView.h"

#define DEFAULT_STR_PER_PAGE 10000

class StringsTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	enum COLS {
		COL_OFFSET = 0,
		COL_TYPE,
		COL_LENGTH,
		COL_STRING,
		MAX_COL
	};

	StringsTableModel(PeHandler *peHndl, ColorSettings &addrColors, int maxPerPage = DEFAULT_STR_PER_PAGE, QObject *parent = 0);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	int columnCount(const QModelIndex &parent) const { return MAX_COL; }

	int rowCount(const QModelIndex &parent) const
	{
		const int startRow = this->getPageStartIndx();
		const int totalCount = stringsOffsets.size();
		if (startRow >= totalCount) return 0;
		const int remCount = totalCount - startRow;
		if (remCount < this->limitPerPage) return remCount;
		return this->limitPerPage;
	}
	
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

	int pagesCount() const 
	{
		const int totalCount = stringsOffsets.size();
		int fullPages = totalCount / this->limitPerPage;
		if ((totalCount % this->limitPerPage) != 0) {
			fullPages++;
		}
		return fullPages;
	}

public slots:
	void setMaxPerPage(int _maxPerPage)
	{
		this->limitPerPage = _maxPerPage;
		reset();
	}
	
	void setPage(int _pageNum)
	{
		this->pageNum = _pageNum;
		reset();
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
	
	int getPageStartIndx() const
	{
		const int pageStart = pageNum * limitPerPage;
		return pageStart;
	}

	StringsCollection *stringsMap;
	QList<offset_t> stringsOffsets;
	PeHandler *m_PE;
	ColorSettings &addrColors;
	
	int pageNum;
	int limitPerPage;
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
		: myPeHndl(peHndl), stringsModel(nullptr), stringsProxyModel(nullptr),
		stringsTable(this, Executable::RAW)
	{
		this->stringsModel = new StringsTableModel(myPeHndl, addrColors, DEFAULT_STR_PER_PAGE, this);
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
			connect( myPeHndl, SIGNAL(stringsUpdated()), this, SLOT(updateStringsView()) );
			connect( myPeHndl, SIGNAL(modified()), this, SLOT(refreshView()) );
		}
		connect( &pageSelectBox, SIGNAL(valueChanged(int)), stringsModel, SLOT(setPage(int)) );
		connect( &maxPerPageSelectBox, SIGNAL(valueChanged(int)), stringsModel, SLOT(setMaxPerPage(int)) );
		connect( &maxPerPageSelectBox, SIGNAL(valueChanged(int)), this, SLOT(resetPageSelection()) );

		connect( &stringsTable, SIGNAL(targetClicked(offset_t, Executable::addr_type)), this, SLOT(offsetClicked(offset_t, Executable::addr_type)) );
	}

private slots:
	void resetPageSelection()
	{
		const int totalPages = this->stringsModel->pagesCount();
		int pagesCount = totalPages;
		if (pagesCount > 0) pagesCount--;
		this->pageSelectBox.setMinimum(0);
		this->pageSelectBox.setMaximum(pagesCount);
		this->pageSelectBox.setToolTip(tr("Total pages") + ": " + QString::number(totalPages, 10));
	}

	void refreshView()
	{
		this->stringsModel->reset();
		this->stringsTable.reset();
	}
	
	void updateStringsView()
	{
		refreshView();
		resetPageSelection();
	}
	
	void onSave();
	void onFilterChanged(QString);
	void offsetClicked(offset_t offset, Executable::addr_type type);

private:
	void initLayout();

	PeHandler *myPeHndl;

	ColorSettings addrColors;
	FollowableOffsetedView stringsTable;
	StringsTableModel *stringsModel;
	StringsSortFilterProxyModel* stringsProxyModel;

	QVBoxLayout topLayout;
	QHBoxLayout propertyLayout0;
	QPushButton saveButton;
	QLabel filterLabel;
	QLineEdit filterEdit;
	QSpinBox pageSelectBox;
	QSpinBox maxPerPageSelectBox;
};
