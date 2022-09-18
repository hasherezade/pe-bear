#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"


class SecurityTreeModel  : public WrapperTableModel
{
	Q_OBJECT

public:
	SecurityTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent) {}

	virtual int columnCount(const QModelIndex &parent) const { return COLS_NUM; }

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;

	virtual bool containsValue(QModelIndex index) const { return (index.column()  == VALUE); }

protected:
	virtual int getFID(QModelIndex index) const { return index.row();  }
	virtual int getSID(QModelIndex index) const { return index.column(); }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->securityDirWrapper; }

	enum cols {
		OFFSET,
		NAME,
		VALUE,
		VALUE2,
		COLS_NUM
	};
friend class SecurityTreeView;
};
