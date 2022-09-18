#include "WrapperTreeView.h"

//---------------------------------------------------------------------------
void  WrapperTreeView::selectionChanged(const QItemSelection &newSel, const QItemSelection &prevSel)
{
	FollowablePeTreeView::selectionChanged(newSel, prevSel);

	WrapperInterface *topModel = dynamic_cast<WrapperInterface*> (this->model());
	if (!topModel) return;

	ExeElementWrapper *mainWrapper = topModel->wrapper();
	if (mainWrapper == NULL) return;
	if (newSel.indexes().size() == 0) return;

	QModelIndex index = newSel.indexes().at(0);
	size_t parentId = (index.isValid()) ? index.row() : 0;
	emit parentIdSelected(parentId);
}
