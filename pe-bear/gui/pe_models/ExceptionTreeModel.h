#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"


class ExceptionTreeModel  : public WrapperTableModel
{
	Q_OBJECT

public:
	ExceptionTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent) {}

	virtual int columnCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

	virtual bool containsValue(QModelIndex index) const { return true; }

protected:
	virtual int getFID(const QModelIndex &index) const { return index.column() - ADDED_COLS_NUM;  }
	virtual int getSID(const QModelIndex &index) const { return index.column(); }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->exceptDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

	inline int32_t columnToFID(int column) const { return column - ADDED_COLS_NUM; }
	inline int32_t FIDtoColumn(int fId) const { return (fId + ADDED_COLS_NUM); }

	virtual bool isComplexValue(const QModelIndex &index) const { if (index.column() >= ADDED_COLS_NUM) return true; return false; }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET,
		ADDED_COLS_NUM
	};
friend class ExceptionTreeView;
};


 class ExceptionTreeView : public FollowablePeTreeView
{
	Q_OBJECT

public:
	ExceptionTreeView(QWidget *parent) : FollowablePeTreeView(parent) {}
};
