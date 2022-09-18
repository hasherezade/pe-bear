#pragma once
#include <QtGui>

#include <map>
#include <set>
#include "../../gui_base/PeTreeView.h"
#include "../PeWrapperModel.h"

class FileHdrTreeItem : public PeTreeItem
{
	Q_OBJECT

protected:
	enum ViewLevel { DESC = 0, DETAILS = 1 };
	typedef enum ViewLevel level_t;

	QList<FileHdrTreeItem*> childItems;
	FileHdrTreeItem *parentItem;
	level_t level;
	FileHdrWrapper::FieldID role;

public:
	FileHdrTreeItem(PeHandler *peHndl, level_t level = DESC, FileHdrWrapper::FieldID role = FileHdrWrapper::NONE, FileHdrTreeItem *parent = NULL);
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
	virtual bool isChildOk(TreeItem* child) { FileHdrTreeItem *ptr = dynamic_cast<FileHdrTreeItem*>(child); return (child)? true : false; }

	FileHdrWrapper &fileHdr;

friend class FileHdrTreeModel;
};

//-------------------

class FileHdrCharactTreeItem : public FileHdrTreeItem
{
	Q_OBJECT
public:
	FileHdrCharactTreeItem(PeHandler *peHndl, level_t level = DESC, DWORD characteristics = 0, FileHdrTreeItem *parent = NULL);
	virtual QVariant data(int column) const;
	void loadChildren();

protected:
	DWORD characteristics;
};

//-------------------

class FileHdrTreeModel : public PeWrapperModel
{
	Q_OBJECT

protected slots:
	void reload();

public:
	FileHdrTreeModel(PeHandler *peHndl, QObject *parent = 0);
	virtual ~FileHdrTreeModel() { }

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual bool containsValue(QModelIndex index) const;

/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;

protected:
	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->fileHdrWrapper; }

private:
	FileHdrCharactTreeItem *charactItem;
};
