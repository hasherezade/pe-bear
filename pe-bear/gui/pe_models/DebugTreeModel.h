#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"

class DebugTreeModel : public WrapperTableModel
{
	Q_OBJECT

public:
	DebugTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent)
	{
		this->connectSignals();
	}

	virtual int columnCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;

	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual bool containsValue(QModelIndex index) const { return true; }

protected:
	virtual int getFID(const QModelIndex &index) const { return index.column() - ADDED_COLS_NUM;  }
	virtual int getSID(const QModelIndex &index) const { return index.row(); }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->debugDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

	inline int32_t columnToFID(int column) const { return column - ADDED_COLS_NUM; }
	inline int32_t FIDtoColumn(int fId) const { return (fId + ADDED_COLS_NUM); }

	virtual bool isComplexValue(const QModelIndex &index) const { return (index.column() >= ADDED_COLS_NUM) ? true : false; }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET,
		NAME,
		ADDED_COLS_NUM
	};
};

//-------------------------------------------------------------------------------------------

class DebugRDSIEntryTreeModel : public WrapperTableModel
{
	Q_OBJECT

public slots:
	void setParentId(size_t _parentId)
	{
		this->parentId = _parentId;
		reset();
	}

public:
	DebugRDSIEntryTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent)
	{
		connect(peHndl, SIGNAL(modified()), this, SLOT(onNeedReset()));
	}

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	
	virtual bool containsValue(QModelIndex index) const { return index.column() == VALUE; }

	virtual ExeElementWrapper* wrapper() const;
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

protected:
	bool setTextData(const QModelIndex &index, const QVariant &value, int role);
	virtual int getFID(const QModelIndex &index) const { return index.row();  }
	virtual int getSID(const QModelIndex &index) const { return index.column();  }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET = 0,
		NAME,
		VALUE,
		MAX_COL
	};
	
	uint32_t parentId;
};
///---
