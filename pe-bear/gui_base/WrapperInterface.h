#pragma once

#include <QtGui>

#include "PeGuiItem.h"


class WrapperInterface
{
public:
	WrapperInterface();
	virtual ~WrapperInterface() {}

	Executable::addr_type addrTypeAt(QModelIndex index) const;

	offset_t getFieldOffset(QModelIndex index) const;
	bufsize_t getFieldSize(QModelIndex index) const;
	
	virtual bool containsValue(QModelIndex index) const = 0;
	virtual bool containsOffset(QModelIndex index) const;
	virtual Executable::addr_type offsetAddrType() const { return Executable::RAW; }

	virtual ExeElementWrapper* wrapper() const = 0;
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const { return wrapper(); }

	virtual QString makeDockerTitle(size_t upId) { return ""; }
	
protected:
	virtual int getFID(const QModelIndex &index) const { return index.row(); }
	virtual int getSID(const QModelIndex &index) const { return index.column(); }

	virtual QVariant dataValue(const QModelIndex &index) const;
	virtual bool isComplexValue(const QModelIndex &index) const { return false; }
	virtual QVariant complexValue(const QModelIndex &index) const;// TODO
	virtual bool setComplexValue(const QModelIndex &index, const QVariant &value, int role)  { return false; }
};
