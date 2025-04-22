#pragma once

#include <QtGui>
#include <map>
#include <set>
#include <iostream>

#include "../../gui_base/WrapperTableModel.h"

class LdConfigTreeModel : public WrapperTableModel
{
	Q_OBJECT

public:
	LdConfigTreeModel(PeHandler *peHndl, QObject *parent = 0) 
		: WrapperTableModel(peHndl, parent) {}
	
	virtual int columnCount(const QModelIndex &parent) const { return COLS_NUM; }

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;

	virtual QVariant toolTip(QModelIndex index) const;
	virtual bool containsValue(QModelIndex index) const { return (index.column()  == VALUE); }
	virtual QString makeDockerTitle(size_t upId);
	
protected:
	virtual int getFID(const QModelIndex &index) const { return index.row();  }
	virtual int getSID(const QModelIndex &index) const { return index.column(); }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->ldConfDirWrapper; }

	enum cols {
		OFFSET,
		NAME,
		VALUE,
		MEANING,
		COLS_NUM
	};
friend class LdConfigTreeView;
};

//-------------------------------------------------------------------------------------------

class LdEntryTreeModel : public WrapperTableModel
{
	Q_OBJECT

public slots:
	void setParentId(size_t parentId)
	{
		this->parentId = parentId;
		reset();
	}
	
public:
	LdEntryTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent), parentId(0){ }

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;// { return MAX_COL; }
	
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	virtual Executable::addr_type addrTypeAt(QModelIndex index) const
	{
		switch(index.column()) {
			case OFFSET: return Executable::RAW;
			case ADDRESS: return Executable::RVA;
		}
		return Executable::NOT_ADDR;
	}
	
	virtual offset_t getFieldOffset(QModelIndex index) const
	{ 
		if (!index.isValid()) return 0;
		LdConfigEntryWrapper *elW = dynamic_cast<LdConfigEntryWrapper*>(wrapperAt(index));
		if (elW == NULL) {
			return 0;
		}
		return elW->getOffset();
	}
	
	virtual bufsize_t getFieldSize(QModelIndex index) const
	{ 
		if (!index.isValid()) return 0;
		LdConfigEntryWrapper *elW = dynamic_cast<LdConfigEntryWrapper*>(wrapperAt(index));
		if (elW == NULL) {
			return 0;
		}
		return elW->getSize();
	}

	virtual bool containsValue(QModelIndex index) const { return index.column() == ADDRESS; }
	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->ldConfDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;
	
protected:
	LdConfigEntryWrapper *getEntryWrapperAtID(int id) const
	{
		LdConfigDirWrapper *w = dynamic_cast<LdConfigDirWrapper*>(wrapper());
		if (w == NULL) return NULL;
		
		LdConfigEntryWrapper *elW = dynamic_cast<LdConfigEntryWrapper*>(w->getSubfieldWrapper(parentId, id));
		if (elW == NULL) return 0;
		return elW;
	}
	
	virtual int getFID(const QModelIndex &index) const { return index.row(); }
	virtual int getSID(const QModelIndex &index) const
	{
		if (!index.isValid()) return 0;
		return index.column() - 1; 
	}

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET = 0,
		//VALUE,
		ADDRESS,
		MAX_COL
	};

	size_t parentId;
};
