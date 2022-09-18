#pragma once
#include <QtGui>

#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"

class TLSTreeModel : public WrapperTableModel
{
	Q_OBJECT

public:
	TLSTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent)
	{
		connectSignals();
	}

	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	virtual bool containsValue(QModelIndex index) const { return index.column() == COL_VALUE; }
	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->tlsDirWrapper; }

	virtual QString makeDockerTitle(uint32_t upId);

protected:
	virtual int getFID(const QModelIndex &index) const { return index.row();  }
	virtual int getSID(const QModelIndex &index) const { return index.column();  }

	enum FieldID {
		NONE = FIELD_NONE,
		COL_OFFSET = 0,
		COL_NAME,
		COL_VALUE,
		MAX_COL
	};
};

//-------------------------------------------------------------------------------------------

class TLSCallbacksModel : public WrapperTableModel
{
	Q_OBJECT

public:
	TLSCallbacksModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent)
	{
		connectSignals();
	}

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	
	virtual bool containsValue(QModelIndex index) const { return index.column() == VALUE; }
	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->tlsDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const { return myPeHndl->tlsDirWrapper.getEntryAt(getFID(index)); }

protected:
	virtual int getFID(const QModelIndex &index) const { return index.row();  }
	virtual int getSID(const QModelIndex &index) const { return index.column();  }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET = 0,
		VALUE,
		MAX_COL
	};
};