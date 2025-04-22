#pragma once
#include <QtGui>

#include <map>
#include <set>
#include "../../gui_base/PeTreeView.h"
#include "../../gui_base/WrapperTableModel.h"
#include "../../gui/PeTreeModel.h"
#include "../PeWrapperModel.h"


class ClrHdrTreeItem : public PeTreeItem
{
	Q_OBJECT

protected:
	enum ViewLevel { DESC = 0, DETAILS = 1 };
	typedef enum ViewLevel level_t;

	QList<ClrHdrTreeItem*> childItems;
	ClrHdrTreeItem *parentItem;
	level_t level;
	ClrDirWrapper::FieldID role;
	ClrDirWrapper &clrDir;

public:
	ClrHdrTreeItem(
		PeHandler *peHndl, 
		level_t level = DESC,
		ClrDirWrapper::FieldID role = ClrDirWrapper::NONE, 
		ClrHdrTreeItem *parent = NULL
	);

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
	virtual bool isChildOk(TreeItem* child)
	{
		const ClrHdrTreeItem *ptr = dynamic_cast<ClrHdrTreeItem*>(child);
		return (ptr)? true : false;
	}

friend class ClrTreeModel;
};

//-------------------

class ClrFlagsTreeItem : public ClrHdrTreeItem
{
	Q_OBJECT

public:
	ClrFlagsTreeItem(PeHandler *peHndl, level_t level = DESC, DWORD flags = 0, ClrHdrTreeItem *parent = NULL);
	virtual QVariant data(int column) const;
	void loadChildren();

protected:

	DWORD getAllFlags() const;
	DWORD flags;
};

//-------------------

class ClrTreeModel : public PeWrapperModel
{
	Q_OBJECT

protected slots:
	void reload();

public:
	ClrTreeModel(PeHandler *peHndl, QObject *parent = 0);

	virtual int columnCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

	Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual bool containsValue(QModelIndex index) const;
	QString makeDockerTitle(size_t upId);

protected:
	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->clrDirWrapper; }

	ClrFlagsTreeItem* flagsItem;
};
