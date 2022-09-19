#pragma once

#ifdef WITH_QT5
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include <bearparser/bearparser.h>
#include <bear_disasm.h>
#include "gui/CommentView.h"

#include "PEFileTreeModel.h"
#include "HexView.h"
#include "gui_base/OffsetDependentAction.h"

#include "OffsetHeader.h"
#include "ViewSettings.h"

class DisasmModel;

class ArgDependentAction : public OffsetDependentAction 
{
	Q_OBJECT
public:
	ArgDependentAction(int argNum, const Executable::addr_type addrType, const QString &title, QObject* parent)
		: OffsetDependentAction(addrType, title,  parent), myArgNum(argNum) {}

public slots:
	void onOffsetChanged(int argNum, offset_t offset);
	void onOffsetChanged(int argNum, offset_t offset, Executable::addr_type addrType);

protected:
	int myArgNum;
};

//----
class DisasmItemDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
	DisasmItemDelegate(QObject* parent)
		: QStyledItemDelegate(parent)
	{
		validator.setRegExp(QRegExp("[0-9A-Fa-f]{1,}"));
	}

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
	QRegExpValidator validator;
};

//--------------------------------------------------------------------
namespace DisasmView {
	enum DisasmFieldId {
		TAG_COL = 0, 
		HEX_COL, 
		ICON_COL, 
		PTR_COL, 
		DISASM_COL, 
		HINT_COL, 
		DISASM_COL_NUM
	};
};

class DisasmScrollBar : public QScrollBar
{
	Q_OBJECT

public slots:
	void onReset() { setSliderPosition(0); }

public:
	explicit DisasmScrollBar(QWidget *parent=0);

	explicit DisasmScrollBar(Qt::Orientation orientation, QWidget *parent=0) 
		: QScrollBar(orientation, parent), myModel(NULL) {}

	void setModel(DisasmModel* disasmModel);
	void enableMenu(bool enable);

protected slots:
	void customMenuEvent(QPoint p) { defaultMenu.exec(mapToGlobal(p)); }
	void pgUp();
	void pgDown();

protected:
	virtual void mousePressEvent(QMouseEvent *);
	virtual void initMenu();

	DisasmModel *myModel;
	QMenu defaultMenu;
};

class DisasmTreeView : public ExtTableView
{
	Q_OBJECT

signals:
	void changePage(bool up);
	void currentRvaChanged(offset_t targetRVA);
	void argRvaChanged(int argNum, offset_t targetRVA);

public:
	DisasmTreeView(QWidget *parent);
	void setModel(DisasmModel *model);

public slots:
	void onModelUpdated() { reset(); }
	void setBitMode(QAction* action);
	void changeDisasmViewSettings(DisasmViewSettings &_settings);

protected slots:
	void onSetComment(offset_t offset, Executable::addr_type aT);
	void onSetEpAction(offset_t offset, Executable::addr_type aT);
	void onFollowOffset(offset_t offset, Executable::addr_type aT);

	virtual void copySelected();
	virtual void pasteToSelected();

protected:
	/* input: unique list */
	bool isIndexListContinuous(QModelIndexList &uniqueList);

	/* input: sorted, unique, continuous list */
	int blockSize(QModelIndexList &uniqueList);

	void reset();

	void init();
	void initHeader();
	void initHeaderMenu();
	void initMenu();
	void resetFont(const QFont &f);

	bool markBranching(QModelIndex currIndex);
	
	void followBranching(QModelIndex currIndex);
	void setHovered(QModelIndexList indexList);

	void mouseMoveEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void selectionChanged(const QItemSelection &, const QItemSelection &);

	DisasmScrollBar vScrollbar;
	OffsetHeader vHdr;
	DisasmModel *myModel;
	CommentView *commentsView;

	/* actions */
	QAction *imgBaseA, *undoAction;

private:
	void emitArgsRVA(const QModelIndex &index);

	/* returns List of indexes with unique offsets (removes repetition) */
	QModelIndexList uniqOffsets(QModelIndexList list);
};
//--------------------------------------------------------------------
class DisasmModel : public HexDumpModel
{
	Q_OBJECT
/*
signals:
	void scrollReset(); // inherited from HexDumpModel
	*/
public slots:
	virtual void onNeedReset() { rebuildDisamTab(); }
	void setStartingOffset(offset_t start) { startOff = start; rebuildDisamTab(); }
	void setShownContent(offset_t start, bufsize_t size) { setStartingOffset(start);  emit scrollReset(); }

	void resetDisasmMode(uint8_t bitMode);
	void setShowImageBase(bool flag);
	bool setComment(offset_t rva, const QString &comment)
	{
		if (myPeHndl && rva != INVALID_ADDR) {
			myPeHndl->comments.setComment(rva, comment);
			return true;
		}
		return false;
	}
	
	bool setHexData(offset_t raw, size_t size, const QString &data);
	
	void changeDisasmSettings(DisasmViewSettings &_settings)
	{
		this->settings = _settings;
		const QSize iSize = this->settings.getIconSize();
		makeIcons(iSize);
		reset();
	}
	
public:
	DisasmModel(PeHandler *peHndl, QObject *parent = 0);

	/* wrappers for Disasm */
	offset_t getRawAt(int index) const { return myDisasm.getRawAt(index); }
	offset_t getRvaAt(int index) const { return myDisasm.getRvaAt(index); }
	size_t getChunkSize(int index) const { return myDisasm.getChunkSize(index); }
	
	offset_t getTargetRVA(const QModelIndex &index) const;
	offset_t getArgRVA(const int argNum, const QModelIndex &index) const;

	virtual offset_t contentOffsetAt(const QModelIndex &index) const { return getRawAt(index.row()); }
	QVariant getRawContentAt(const QModelIndex &index) const;

	void setMarkedAddress(offset_t cRva, offset_t tRva);
	int disasmCount() const { int size = myDisasm.chunksCount(); return size > 0 ? size - 1 : 0; }

	int rowCount(const QModelIndex &parent) const { return disasmCount(); }
	int columnCount(const QModelIndex &parent) const;

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	QVariant horizHeader(int section, int role) const;
	QVariant verticHeader(int section, int role) const;

	Qt::ItemFlags flags(const QModelIndex &index) const;

	uint32_t getCurrentChunkSize(const QModelIndex &index) const;

	bool isClickable(const QModelIndex &index) const;

	QString getComment(offset_t rva) const { return (myPeHndl) ?  myPeHndl->comments.getCommentAt(rva) : ""; }

	virtual ViewSettings* getSettings()
	{
		return &settings;
	}

protected:
	void makeIcons(const QSize &vSize);
	void rebuildDisamTab();

	QVariant getHint(const QModelIndex &index) const;
	QString getAsm(int index) const;

private:
	pe_bear::PeDisasm myDisasm;
	uint32_t startOff;

	uint8_t bitMode;
	bool isBitModeAuto;
	bool showImageBase;

	QIcon tracerIcon, tracerUpIcon, tracerDownIcon, tracerSelf;
	QIcon callUpIcon, callDownIcon, callWrongIcon, tagIcon;
	
	DisasmViewSettings settings;

friend class DisasmTreeView;
};
