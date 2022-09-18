#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/PeTreeView.h"
#include "../PeWrapperModel.h"

//---
class OptionalHdrTreeItem : public PeTreeItem
{
protected:
	enum ViewLevel { DESC = 0, DETAILS = 1 };
	typedef enum ViewLevel level_t;

	level_t level;
	OptHdrWrapper::OptHdrFID role;

public:
	OptionalHdrTreeItem(PeHandler *peHndl, level_t level = DESC, OptHdrWrapper::OptHdrFID role = OptHdrWrapper::NONE, OptionalHdrTreeItem *parent = NULL);
	virtual ~OptionalHdrTreeItem() { }

	int columnCount() const;

	virtual QVariant font(int column) const;
	virtual QVariant data(int column) const;
	virtual QVariant foreground(int column) const;
	virtual QVariant background(int column) const;
	virtual QVariant toolTip(int column) const;
	virtual Qt::ItemFlags flags(int column) const;

/* PeViewItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;

protected:
	virtual bool isChildOk(TreeItem* child) { OptionalHdrTreeItem *ptr = dynamic_cast<OptionalHdrTreeItem*>(child); return (child)? true : false;}
	bool setDataValue(int column, const QVariant &value);

	OptHdrWrapper& optHdr;

	enum COLS {
		COL_OFFSET = 0,
		COL_NAME,
		COL_VALUE,
		COL_VALUE2,
		COL_SPACE,
		MAX_COL
	};

private:
	QVariant dataValMeanings() const;
	
friend class OptionalHdrTreeModel;
};

//---

class DataDirTreeItem : public OptionalHdrTreeItem
{
public:
	DataDirTreeItem(PeHandler *peHndl, level_t level = DESC, int recordNum = -1, OptionalHdrTreeItem *parent = NULL);
	virtual ~DataDirTreeItem() { }

	virtual QVariant data(int column) const;
	virtual bool setDataValue(int column, const QVariant &value);

	virtual QVariant background(int column) const;
	virtual QVariant toolTip(int column) const;

	virtual QVariant edit(int column) const;
	virtual Qt::ItemFlags flags(int column) const;
	virtual bool containsRVA(QModelIndex index) const;

/* PeViewItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;

protected:
	int getFID(int column = 0) const { return recordNum; }
	int getSID(int column) const { return column - COL_VALUE; }

	DataDirWrapper *wrapper() const { return (this->myPeHndl != NULL) ? &this->myPeHndl->dataDirWrapper : NULL; }
	int recordNum;

friend class  OptionalHdrTreeModel;
};

//---

class OptHdrDllCharactTreeItem : public OptionalHdrTreeItem
{
public:
	OptHdrDllCharactTreeItem(PeHandler *peHndl, level_t level = DESC, DWORD characteristics = 0, OptionalHdrTreeItem *parent = NULL);
	virtual ~OptHdrDllCharactTreeItem() { }

	void init();
	virtual QVariant data(int column) const;

/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;

protected:
	DWORD characteristics;
};

//---

class OptionalHdrTreeModel : public PeWrapperModel
{
	Q_OBJECT

protected slots:
	void reload();

public:
	OptionalHdrTreeModel(PeHandler *peHndl, QObject *parent = 0);
	virtual ~OptionalHdrTreeModel() { }

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual bool containsValue(QModelIndex index) const;

	virtual int getFID(const QModelIndex &index) const;
	virtual int getSID(const QModelIndex &index) const;

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->optHdrWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

	enum COLS {
		COL_OFFSET = 0,
		COL_NAME,
		COL_VALUE,
		COL_VALUE2,
		COL_SPACE,
		MAX_COL
	};

private:
	OptHdrDllCharactTreeItem* dllCharact;
};
