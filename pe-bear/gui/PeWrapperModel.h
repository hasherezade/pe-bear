#pragma once

#include "PeTreeModel.h"
#include "../gui_base/WrapperInterface.h"

class PeWrapperModel : public PeTreeModel, public WrapperInterface
{
	Q_OBJECT

public slots:
	virtual void setParentId(size_t parentId)
	{
		/* only if the down table shows the details of the row from the up table */
	}

public:
	PeWrapperModel(PeHandler *peHndl, QObject *parent = 0) 
		: PeTreeModel(peHndl, parent), WrapperInterface()
	{
	}

	virtual ~PeWrapperModel()
	{
	}
	
	//---
	virtual Executable::addr_type addrTypeAt(QModelIndex index) const { return  WrapperInterface::addrTypeAt(index); }
	virtual offset_t getFieldOffset(QModelIndex index) const { return WrapperInterface::getFieldOffset(index); }
	virtual bufsize_t getFieldSize(QModelIndex index) const { return WrapperInterface::getFieldSize(index); }
	//---
	/* sets data value, do backup and notifies handler */
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual QString makeDockerTitle(uint32_t upId);
	
	virtual int getFID(const QModelIndex &index) const
	{
		if (!index.isValid()) {
			return FIELD_NONE;
		}
		PeTreeItem *item = static_cast<PeTreeItem*>(index.internalPointer());
		if (item) {
			return item->getFID();
		}
		return index.row();
	}
	
	virtual int getSID(const QModelIndex &index) const
	{
		if (!index.isValid()) {
			return FIELD_NONE;
		}
		PeTreeItem *item = static_cast<PeTreeItem*>(index.internalPointer());
		if (item) {
			return item->getSID();
		}
		return index.column();
	}
};
