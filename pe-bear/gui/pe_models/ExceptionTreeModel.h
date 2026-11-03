#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"


class ExceptionTreeModel  : public WrapperTableModel
{
	Q_OBJECT

public:
	/* A table of this kind can hold hundreds of thousands of records (one per
	   function of a 64-bit image), more than a tree view lays out in
	   reasonable time, so the model shows one page of them at a time -- the
	   same way as the Strings window. Pages are counted from 0. */
	enum PagingLimits {
		DEFAULT_PER_PAGE = 10000,
		MIN_PER_PAGE = 100,
		MAX_PER_PAGE = 100000
	};

	ExceptionTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent), pageNum(0), limitPerPage(DEFAULT_PER_PAGE) {}

	virtual int columnCount(const QModelIndex &parent) const;
	virtual int rowCount(const QModelIndex &parent) const;

	/** Records in the whole directory, not only on the current page. */
	int totalCount() const;
	int pagesCount() const;
	int page() const { return pageNum; }
	int maxPerPage() const { return limitPerPage; }

public slots:
	void setPage(int page);
	void setMaxPerPage(int maxPerPage);

signals:
	/** The page, the page size, or the number of pages may have changed. */
	void pagingChanged();

public:

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

	virtual bool containsValue(QModelIndex index) const { return true; }

protected:
	virtual int getFID(const QModelIndex &index) const { return index.column() - ADDED_COLS_NUM;  }
	virtual int getSID(const QModelIndex &index) const { return index.column(); }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->exceptDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

	/** Index, within the whole directory, of the first record of the page. */
	qint64 pageStartIndex() const { return qint64(pageNum) * limitPerPage; }

	inline int32_t columnToFID(int column) const { return column - ADDED_COLS_NUM; }
	inline int32_t FIDtoColumn(int fId) const { return (fId + ADDED_COLS_NUM); }

	virtual bool isComplexValue(const QModelIndex &index) const { if (index.column() >= ADDED_COLS_NUM) return true; return false; }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET,
		ADDED_COLS_NUM
	};
	int pageNum;
	int limitPerPage;

friend class ExceptionTreeView;
};


 class ExceptionTreeView : public FollowablePeTreeView
{
	Q_OBJECT

public:
	ExceptionTreeView(QWidget *parent) : FollowablePeTreeView(parent) {}
};
