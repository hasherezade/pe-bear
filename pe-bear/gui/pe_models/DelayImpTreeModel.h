#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"
#include <iostream>

class DelayImpTreeModel  : public WrapperTableModel
{
	Q_OBJECT

public:
	DelayImpTreeModel(PeHandler *peHndl, QObject *parent = 0) 
		: WrapperTableModel(peHndl, parent)
	{
	}
	
	virtual int columnCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

	virtual bool containsValue(QModelIndex index) const { return (index.column() >= NAME); }

protected:
	virtual int getFID(const QModelIndex &index) const { return index.column() - ADDED_COLS_NUM;  }
	virtual int getSID(const QModelIndex &index) const { return index.row(); }

	inline int32_t columnToFID(int column) const { return column - ADDED_COLS_NUM; }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->delayImpDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const { return myPeHndl->delayImpDirWrapper.getEntryAt(index.row()); }

	enum cols {
		OFFSET,
		NAME,
		ADDED_COLS_NUM
	};

friend class DelayImpTreeView;
};

//----------------------------------------------------------------------

class DelayImpFuncModel  : public WrapperTableModel
{
	Q_OBJECT

public slots:
	void setParentId(size_t libraryId) { this->libraryId = libraryId; reset(); }

public:
	DelayImpFuncModel(PeHandler *peHndl, QObject *parent = 0);

	virtual int columnCount(const QModelIndex &parent) const;
	virtual int rowCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;

	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	//virtual bool containsOffset(QModelIndex index) const { return false; } // Do not show offset!
	virtual Executable::addr_type offsetAddrType() const;
	virtual bool containsValue(QModelIndex index) const { return true; }

protected:
	virtual ExeNodeWrapper* wrapper() const;
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

	virtual bool isComplexValue(const QModelIndex &index) const { return false; }

	virtual int getFID(const QModelIndex &index) const { return index.column() - ADDED_COLS_NUM; }
	virtual int getSID(const QModelIndex &index) const { return index.row(); }

	inline int32_t columnToFID(int column) const { return (column - ADDED_COLS_NUM); }
	inline int32_t FIDtoColumn(int fId) const { return (fId + ADDED_COLS_NUM); }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET,
		NAME,
		ORDINAL,
		HINT,
		ADDED_COLS_NUM
	};
	size_t libraryId;
};
