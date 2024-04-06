#pragma once
#include <QtGlobal>

#include <bearparser/bearparser.h>

#include "../../PEBear.h"
#include "../../base/PeHandlersManager.h"
#include "../../gui_base/FollowablePeTreeView.h"
#include "../../gui_base/OffsetDependentAction.h"
#include "../../gui/CommentView.h"

//----------------------------------------------------

class OffsetsBrowseModel : public PeTableModel
{
	Q_OBJECT

public:
	OffsetsBrowseModel(PeHandler *peHndl, QObject *parent = 0)
		: PeTableModel(peHndl, parent) {}

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	int rowCount(const QModelIndex &parent) const;

	virtual Executable::addr_type addrTypeAt(QModelIndex index) const 
	{
		return (index.column() == COL_OFFSET) ? Executable::RVA : Executable::NOT_ADDR;
	}

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int);

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
	{
		return createIndex(row, column); //no index item pointer
	}
	
	QModelIndex parent(const QModelIndex &index) const
	{
		return QModelIndex();
	}

	offset_t getFieldOffset(QModelIndex index) const;
	bufsize_t getFieldSize(QModelIndex index) const;

	enum COLS {
		COL_OFFSET = 0,
		COL_ID,
		COL_NAME,
		MAX_COL
	};

};

//----------------------------------------------------

class OffsetsBrowseWindow : public QMainWindow
{
    Q_OBJECT

protected slots:
	void onSaveComment()
	{
		if (this->commentsView) this->commentsView->onSaveComments();
	}

	void onLoadComment()
	{
		if (this->commentsView) this->commentsView->onLoadComments();
	}

public:
	OffsetsBrowseWindow(PeHandler *peHndlr, QWidget *parent);
	virtual ~OffsetsBrowseWindow();

protected:
	/* events */
	void dragEnterEvent(QDragEnterEvent* ev) { ev->accept(); }
    void dropEvent(QDropEvent* event);

private:
	void createMenu();
	FollowablePeTreeView tagsTree;
	CommentView *commentsView;
	OffsetsBrowseModel *tagModel;

};
