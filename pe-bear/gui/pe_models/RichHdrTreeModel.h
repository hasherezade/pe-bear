#pragma once
#include <QtGui>

#include <map>
#include <set>
#include "../../gui_base/PeTreeView.h"
#include "../../gui_base/WrapperTableModel.h"
#include "../../gui/PeTreeModel.h"

class RichHdrTreeItem : public PeTreeItem
{
	Q_OBJECT

protected:
	RichHdrWrapper::FieldID role;

public:
	RichHdrTreeItem(PeHandler *peHndl, RichHdrWrapper::FieldID role = RichHdrWrapper::NONE, RichHdrTreeItem *parent = NULL);
	int columnCount() const;

	virtual Qt::ItemFlags flags(int column) const;
	virtual QVariant data(int column) const;

	bool setDataValue(int column, const QVariant &value);
	virtual QVariant foreground(int column) const;
	virtual QVariant background(int column) const;

/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;

protected:
	virtual bool isChildOk(TreeItem* child) { RichHdrTreeItem *ptr = dynamic_cast<RichHdrTreeItem*>(child); return (child)? true : false; }

	RichHdrWrapper* getRichHdr() const
	{
		if (!myPeHndl) return NULL;
		return &myPeHndl->richHdrWrapper;
	}

	QColor approvedColor;

friend class RichHdrTreeModel;
};


class RichHdrTreeModel : public WrapperTableModel
{
	Q_OBJECT

protected slots:
	void onNeedReset();

public:
	RichHdrTreeModel(PeHandler *peHndl, QObject *parent = 0);
	virtual ~RichHdrTreeModel() { }
	
	virtual int columnCount(const QModelIndex &parent) const;

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual bool containsValue(QModelIndex index) const;

/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;

protected:
	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->richHdrWrapper; }
};
