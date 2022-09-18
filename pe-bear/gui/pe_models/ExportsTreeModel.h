#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"

//----------------------------

class ExportsTreeModel : public WrapperTableModel
{
	Q_OBJECT

public:
	ExportsTreeModel(PeHandler *peHndl, QObject *parent = 0) 
		: WrapperTableModel(peHndl, parent) {}
	
	virtual int columnCount(const QModelIndex &parent) const { return COLS_NUM; }

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;

	virtual bool containsValue(QModelIndex index) const { return (index.column()  == VALUE); }
	QString makeDockerTitle(uint32_t id);
	
protected:
	virtual int getFID(const QModelIndex &index) const { return index.row();  }
	virtual int getSID(const QModelIndex &index) const { return index.column(); }

	virtual ExeElementWrapper* wrapper() const { return myPeHndl ? &myPeHndl->exportDirWrapper : NULL; }

	enum cols {
		OFFSET,
		NAME,
		VALUE,
		VALUE2,
		COLS_NUM
	};
friend class LdConfigTreeView;
};

//----------------------------------------------------------------------

class ExportedFuncTreeModel : public WrapperTableModel
{
	Q_OBJECT

public:
	ExportedFuncTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent) {}

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	
	virtual bool containsValue(QModelIndex index) const { return index.column() == VALUE; }
	virtual ExeElementWrapper* wrapper() const { return myPeHndl ? &myPeHndl->exportDirWrapper : NULL; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const { return myPeHndl->exportDirWrapper.getEntryAt(index.row()); }

	virtual Executable::addr_type addrTypeAt(QModelIndex index) const;// { return  WrapperInterface::addrTypeAt(index); }
protected:
	virtual int getFID(const QModelIndex &index) const { return index.column() - VALUE;  }
	virtual int getSID(const QModelIndex &index) const { return index.column();  }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET = 0,
		ORDINAL,
		VALUE,
		NAME_RVA,
		NAME,
		FORWARDER,
		MAX_COL
	};
};
