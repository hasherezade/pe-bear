#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"


class ResourcesTreeModel  : public WrapperTableModel
{
	Q_OBJECT

public:
	ResourcesTreeModel(PeHandler *peHndl, QObject *parent = 0) 
		: WrapperTableModel(peHndl, parent)
	{
	}
	
	virtual Executable::addr_type addrTypeAt(QModelIndex index) const;
	virtual int columnCount(const QModelIndex &parent) const { return COLS_NUM; }
	virtual int rowCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;

	virtual bool containsValue(QModelIndex index) const { return (index.column()  == VALUE || index.column()  == VALUE2); }

protected:
	QVariant subdirsData(const QModelIndex &index, int role) const;

	size_t getWrapperFieldsCount() const
	{
		ExeElementWrapper *exeW = wrapper();
		if (!exeW) return 0;

		const size_t fieldsCount = exeW->getFieldsCount();
		return fieldsCount;
	}
	
	virtual int getFID(const QModelIndex &index) const;
	virtual int getSID(const QModelIndex &index) const { return index.column() - VALUE; }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->resourcesDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

	enum cols {
		OFFSET,
		NAME,
		VALUE,
		VALUE2,
		MEANING,
		MEANING2,
		RES_NAME,
		ENTRIES_NUM,
		MARKERS,
		COLS_NUM
	};
};

//------------------------------------

class ResourceLeafModel : public WrapperTableModel
{
	Q_OBJECT

public slots:
	void setParentId(size_t parentId) { this->parentId = parentId; reset(); emit modelUpdated(); }
	void setLeafId(size_t leafId) { this->leafId = leafId; reset(); emit modelUpdated(); }

public:
	ResourceLeafModel(PeHandler *peHndl, QObject *parent = 0) 
		: WrapperTableModel(peHndl, parent), parentId(0), leafId(0)
	{
	}

	virtual int columnCount(const QModelIndex &parent) const { return COLS_NUM; }
	virtual int rowCount(const QModelIndex &parent) const { return ResourceLeafWrapper::FIELD_COUNTER; }

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	
	size_t getParentId() { return parentId; }
	size_t getLeafId() { return leafId; }

	Executable::addr_type addrTypeAt(QModelIndex index) const;
	offset_t getFieldOffset(QModelIndex index) const;
	bufsize_t getFieldSize(QModelIndex index) const;
	
protected:

	virtual int getFID(const QModelIndex &index) const { return leafId; }
	virtual int getSID(const QModelIndex &index) const { return index.row(); }

	virtual ExeElementWrapper* wrapper() const;
	ExeElementWrapper* wrapperAt(QModelIndex index) const;
	virtual bool containsValue(QModelIndex index) const { return (index.column()  == VALUE); }
	
	enum cols {
		OFFSET,
		NAME,
		VALUE,
		COLS_NUM
	};

	size_t parentId;
	size_t leafId;
};
