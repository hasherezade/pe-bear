#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../gui/pe_models.h"
#include "WrapperTreeView.h"

class WrapperSplitter : public QSplitter
{
	Q_OBJECT

public slots:
	virtual void setParentIdInDocker(size_t);
	virtual void reloadDocketTitle();
	
public:
	WrapperSplitter(QWidget *parent);
	WrapperSplitter(PeTreeModel *upModel, PeTreeModel *downModel, QWidget *parent);

	virtual ~WrapperSplitter()
	{
		delete upModel;
		delete downModel;
	}

	virtual bool initToolbar() { return false; }
	
	QModelIndexList getUpperViewSelected()
	{
		return upTree.selectionModel()->selectedIndexes();
	}

	QToolBar toolBar;
	QDockWidget dock;
	QString title;

protected:
	virtual void init(PeTreeModel *upModel, PeTreeModel *downModel);
	
	PeTreeModel *upModel, *downModel;
	WrapperTreeView upTree;
	FollowablePeTreeView downTree;
	
	size_t recordId;
};
