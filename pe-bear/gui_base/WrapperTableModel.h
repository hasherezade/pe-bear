#pragma once

#include <QtGui>

#include "FollowablePeTreeView.h"
#include "WrapperInterface.h"


//---
class WrapperTableModel : public PeTableModel, public WrapperInterface
{
	Q_OBJECT

public slots:
	virtual void setParentId(size_t parentId)
	{
		/* only if the down table shows the details of the row from the up table */
	}

public:
	WrapperTableModel(PeHandler *peHndl, QObject *parent = 0) 
		: PeTableModel(peHndl, parent), WrapperInterface()
	{
		WrapperTableModel::counter++;
	}

	virtual ~WrapperTableModel() { WrapperTableModel::counter--; }
	//---
	virtual Executable::addr_type addrTypeAt(QModelIndex index) const { return  WrapperInterface::addrTypeAt(index); }
	virtual offset_t getFieldOffset(QModelIndex index) const { return WrapperInterface::getFieldOffset(index); }
	virtual bufsize_t getFieldSize(QModelIndex index) const { return WrapperInterface::getFieldSize(index); }
	//---
	/* sets data value, do backup and notifies handler */
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual int rowCount(const QModelIndex &parent) const { return wrapper() ? wrapper()->getFieldsCount() : 0; }
	
	virtual QString makeDockerTitle(uint32_t upId);
	
protected:
	static int64_t counter;
};
//------------
