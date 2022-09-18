#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"


class DebugTreeModel  : public WrapperTableModel
{
	Q_OBJECT

public:
	DebugTreeModel(PeHandler *peHndl, QObject *parent = 0) 
		: WrapperTableModel(peHndl, parent) {}
	
	virtual int columnCount(const QModelIndex &parent) const { return COLS_NUM; }

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;

	virtual bool containsValue(QModelIndex index) const { return (index.column()  == VALUE); }
	QString makeDockerTitle(uint32_t upId);
	
protected:
	virtual int getFID(const QModelIndex &index) const { return index.row();  }
	virtual int getSID(const QModelIndex &index) const { return index.column(); }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->debugDirWrapper; }

	enum cols {
		OFFSET,
		NAME,
		VALUE,
		VALUE2,
		COLS_NUM
	};
};

//-------------------------------------------------------------------------------------------

class DebugRDSIEntryTreeModel : public WrapperTableModel
{
	Q_OBJECT

public:
	DebugRDSIEntryTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent) { }

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
};
