#include "OffsetedView.h"

void OffsetedView::setModel(QAbstractItemModel *model)
{
    QItemSelectionModel *prev = this->selectionModel();
    QTableView::setModel(model);
    if (prev) {
        disconnect(prev, SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(onIndexChanged(const QModelIndex&, const QModelIndex&)));
    }
    connect(this->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)),
        this, SLOT(onIndexChanged(const QModelIndex&, const QModelIndex&)));
}

void OffsetedView::mousePressEvent(QMouseEvent *ev)
{
    if (!ev) return;
    ev->accept();
    QModelIndex index = this->indexAt(ev->pos());
    if (index.isValid()) {
        const offset_t offset = getOffsetFromUserData(index);
        //mouse click should always emit the target, without verifying
        emitSelectedTarget(offset, m_targetAddrType, false);
    }
    return MouseTrackingTableView::mousePressEvent(ev);
}

void OffsetedView::onIndexChanged(const QModelIndex &current, const QModelIndex &previous)
{
    const offset_t offset = getOffsetFromUserData(current);
    //protect from emitting the same target more than once (from various events)
    emitSelectedTarget(offset, m_targetAddrType, true);
}

