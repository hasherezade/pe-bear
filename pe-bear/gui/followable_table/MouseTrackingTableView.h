#pragma once
#include <QtWidgets>

class MouseTrackingTableView : public QTableView
{
	Q_OBJECT

signals:
	void mouseClicked(const QModelIndex &index);
	void moudeDblClicked(const QModelIndex &index);
	void mouseHovered(const QModelIndex &index);
	void mouseLeave();

public:
	MouseTrackingTableView(QWidget *parent)
		: QTableView(parent)
	{
		setMouseTracking(true);
	}

	void mousePressEvent(QMouseEvent *ev)
	{
		QModelIndex index = this->indexAt(ev->pos());
		ev->accept();
		emit mouseClicked(index);

		return QTableView::mousePressEvent(ev);
	}

	void mouseMoveEvent(QMouseEvent *ev)
	{
		QModelIndex index = this->indexAt(ev->pos());
		ev->accept();
		emit mouseHovered(index);

		return QTableView::mouseMoveEvent(ev);
	}

	void leaveEvent(QEvent *ev)
	{
		ev->accept();
		emit mouseLeave();

		setCursor(Qt::ArrowCursor);
		return QTableView::leaveEvent(ev);
	}

};
