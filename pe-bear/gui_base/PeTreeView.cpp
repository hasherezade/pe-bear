#include "PeTreeView.h"

//---------------------------------------

PeTreeView::PeTreeView(QWidget *parent) 
	: TreeCpView(parent), 
	myModel(NULL), autoExpand(true)
{
	setMouseTracking(true);

	setSelectionBehavior(QAbstractItemView::SelectItems);
	setSelectionMode(SingleSelection);
	setAutoFillBackground(true);
	setAlternatingRowColors(true);

	if (header()) {
#if QT_VERSION >= 0x050000
		header()->setSectionResizeMode(QHeaderView::Interactive);
#else
		header()->setResizeMode(QHeaderView::Interactive);
//		header()->setResizeMode(0, QHeaderView::Fixed);  
		header()->setMovable(false);
#endif
	}
}

void PeTreeView::setModel(PeTreeModel *model)
{
	if (this->myModel) {
		disconnect(this->myModel, SIGNAL(modelUpdated()), this, SLOT(onModelUpdated()) );
	}

	this->myModel = model;
	TreeCpView::setModel(model);
	if (!model) return;

	bool expandable = model->isExpandable();
	setItemsExpandable(expandable);
	setRootIsDecorated(expandable);
	this->setColumnWidth(0, 80);
	this->setContentsMargins(5, 5, 2, 2);
	connect(model, SIGNAL(modelUpdated()), this, SLOT(onModelUpdated()) );
}

void PeTreeView::selectionChanged(const QItemSelection &newSel, const QItemSelection &prevSel)
{
	if (!myModel || !(myModel->getPeHandler())) return;

	if (newSel.indexes().size() == 0) return;

	QModelIndex index = newSel.indexes().at(0);
	hoverIndex(index);
}

void PeTreeView::mousePressEvent(QMouseEvent *event)
{
	if (!myModel || !(myModel->getPeHandler())) return;
	QModelIndex index = this->indexAt(event->pos());
	hoverIndex(index);
	TreeCpView::mousePressEvent(event);
}

void PeTreeView::keyPressEvent(QKeyEvent *keyEv)
{
	switch (keyEv->key()){
		case Qt::Key_Plus :
			this->expandAll();
			return;
		case Qt::Key_Minus :
			this->collapseAll();
			return;
	}
	TreeCpView::keyPressEvent(keyEv);
}

void PeTreeView::hoverIndex(QModelIndex index)
{
	if (!index.isValid()) return;
	if (!myModel || !myModel->getPeHandler()) return;
	
	offset_t offset = this->myModel->getFieldOffset(index);
	offset_t size = this->myModel->getFieldSize(index);
	
	if (!size) {
		offset = this->myModel->getContentOffset();
		size = this->myModel->getContentSize();
	}
	myModel->getPeHandler()->setHovered(false, offset, size);
}

//---------------------------------------