#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "TreeCpView.h"
#include "PeTableModel.h"

//--------------------------------------------------------------------------

class PeTreeView : public TreeCpView
{
	Q_OBJECT

protected slots:
	void onModelUpdated()
	{
		if (this->itemsExpandable() && autoExpand) {
			this->expandAll();
		}
	};

public :
	PeTreeView(QWidget *parent);
	
	virtual ~PeTreeView()
	{
	}

	void setModel(PeTreeModel *model);
	bool autoExpand;

public slots:
	void expandAll()
	{
		if (itemsExpandable()) {
			TreeCpView::expandAll();
		}
	}
	
	void collapseAll()
	{
		if (itemsExpandable()) {
			TreeCpView::collapseAll();
		}
	}

protected:
	void selectionChanged(const QItemSelection &newSel, const QItemSelection &prevSel);
	void mousePressEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *key);
	void hoverIndex(QModelIndex index);

	PeTreeModel *myModel;
	
	PeHandler* peHndl()
	{
		return (myModel) ? myModel->getPeHandler() : NULL;
	}
	
	PEFile* pe()
	{
		return (myModel) ? myModel->getPE() : NULL;
	}
};

//-----------------------------------------------------
