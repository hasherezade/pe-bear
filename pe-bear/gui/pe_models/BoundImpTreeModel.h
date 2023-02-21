#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"


class BoundImpTreeModel : public WrapperTableModel
{
	Q_OBJECT

public:
	BoundImpTreeModel(PeHandler *peHndl, QObject *parent = 0) 
		: WrapperTableModel(peHndl, parent)
	{
		connectSignals();
	}
	
	virtual int columnCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual QVariant toolTip(const QModelIndex &index) const;

	virtual bool containsValue(QModelIndex index) const { return (index.column() >= NAME); }

protected:
	virtual int getFID(const QModelIndex &index) const { return index.column() - ADDED_COLS_NUM;  }
	virtual int getSID(const QModelIndex &index) const { return index.row(); }

	inline int32_t columnToFID(int column) const { return column - ADDED_COLS_NUM; }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->boundImpDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const { return myPeHndl->boundImpDirWrapper.getEntryAt(index.row()); }

	enum cols {
		OFFSET,
		NAME,
		ADDED_COLS_NUM
	};
};
