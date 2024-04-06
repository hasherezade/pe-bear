#pragma once

#include <QtGlobal>

#include <map>
#include <set>

#include "QtCompat.h"
#include "gui_base/PeTreeView.h"
#include "ExeDependentAction.h"
#include "gui/PeTreeModel.h"

enum PEFileFieldId {
	PEFILE_NONE = -1,
	PEFILE_ROOT,
	PEFILE_MAIN,
	//PEFILE_SIGNATURE,
	PEFILE_IMG_DOS_HDR,
	PEFILE_DOS_STUB,
	PEFILE_IMG_NT_HDRS,
	PEFILE_SEC_HDRS,
	PEFILE_SECTIONS,
	PEFILE_OVERLAY,
	PEFILE_FIELD_COUNTER
};

enum PEFileNTHdrFieldId {
	PEFILE_NTHDR_NONE = -1,
	PEFILE_NTHDR_SIGN,
	PEFILE_NTHDR_FILEHDR,
	PEFILE_NTHDR_OPTHDR,
	PEFILE_NTHDR_COUNTER
};

class PEFileEntryPointItem;

class PEFileTreeItem : public PeTreeItem
{
	Q_OBJECT

public:
	enum ViewLevel { DESC = 0, DETAILS = 1 };
	typedef enum ViewLevel level_t;

	PEFileTreeItem(PeHandler *peHndl, level_t level = DESC, PEFileFieldId role = PEFILE_NONE, PEFileTreeItem *parent = NULL);

	bool attachIfBelongs(PEFileTreeItem *child);
	int columnCount() const;

	virtual QVariant data(int column) const;
	virtual QVariant foreground(int column) const;
	virtual QVariant background(int column) const;
	virtual QVariant toolTip(int column) const;
	virtual QVariant whatsThis(int column) const { return this->role; } //WhatsThisRole
	virtual QVariant decoration(int column) const;
	virtual QVariant font(int column) const;

	virtual BYTE* getContent() const;
	ViewLevel getViewLevel() { return level; }

/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;

signals:
	void needReset();

protected slots:
	void onModified();

protected:
	void createTree();
	void deleteOverlayNode();
	void appendOverlayNode();

	PEFileTreeItem* findNodeByRole(int role);

	bufsize_t getOverlaySize() const;

	virtual bool isChildOk(TreeItem* child)
	{
		return (dynamic_cast<PEFileTreeItem*>(child) != 0);
	}

	void setEntryPointItem(PEFileEntryPointItem *EpItem)
	{
		entryPointItem = EpItem;
	}

	PEFileEntryPointItem *entryPointItem;
	ViewLevel level;

	PEFileFieldId role;
	QString fileName;

friend class PEFileTreeModel;
friend class PEFileEntryPointItem;
};

//----------

class PEFileSectionsTreeItem : public PEFileTreeItem
{
	Q_OBJECT

public:
	PEFileSectionsTreeItem(PeHandler *peHndl, level_t level = DESC, int secIndx = (-1), PEFileTreeItem *parent = NULL);
	inline SectionHdrWrapper* getMySection() const;

	virtual QVariant data(int column) const;
	virtual QVariant background(int column) const;
	virtual QVariant decoration(int column) const; // DecorationRole

	virtual BYTE* getContent() const;

public slots:
	void onSectionNumChanged();

/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;
protected:
	
	int secIndx;
};

//----------

class PEFileNTHdrTreeItem : public PEFileTreeItem
{
public:
	PEFileNTHdrTreeItem(PeHandler *peHndl, level_t level = DESC, enum PEFileNTHdrFieldId subrole = PEFILE_NTHDR_NONE, PEFileTreeItem *parent = NULL);

	virtual QVariant data(int column) const;
	virtual BYTE* getContent() const;

/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;
protected:
	enum PEFileNTHdrFieldId subrole;
};

//----------

class PEFileEntryPointItem : public PEFileTreeItem
{
public:
	PEFileEntryPointItem(PeHandler *peHndl, PEFileTreeItem *mainItem, PEFileTreeItem *parent);
	virtual ~PEFileEntryPointItem();

	virtual QVariant decoration(int column) const;
	virtual QVariant data(int column) const;

	virtual BYTE* getContent() const;

/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;

protected:
	offset_t savedEP;
	PEFileTreeItem *mainItem;

friend class PEFileTreeItem;
};

//----------

class PEFileTreeModel : public QAbstractItemModel
{
	Q_OBJECT

signals:
	void modelUpdated();

public slots:
	void refreshView();
	
	QModelIndex addHandler(PeHandler *peHndl);
	void deleteHandler(PeHandler *peHndl);

public:
	PEFileTreeModel(QObject *parent = 0);
	~PEFileTreeModel();

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int) { return false; } //external modifications not allowed
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;

protected:
	void reset()
	{
		//>
		this->beginResetModel();
		this->endResetModel();
		//<
	}

	std::map<PEFile*, PEFileTreeItem*> loadedPeFiles;
	PEFileTreeItem* root;
};

class PEStructureView : public TreeCpView
{
	Q_OBJECT

signals:
	void handlerSelected(PeHandler *);

public:
	PEStructureView(QWidget *parent);
	void setModel(PEFileTreeModel *model);
	void selectionChanged(const QItemSelection &newSel, const QItemSelection &prevSel);
	bool selectHandler(PeHandler *hndl);

protected slots:
	virtual void onNeedReset() { reset(); expandAll(); }
	virtual void onModelUpdated() { emit handlerSelected(NULL); onNeedReset(); }

protected:
	PEFileTreeModel* peTreeModel;
};
