#pragma once

#include <bearparser/core.h>
#include "MouseTrackingTableView.h"

class OffsetedView : public MouseTrackingTableView
{
	Q_OBJECT

signals:
	void offsetChanged(const offset_t offset, const Executable::addr_type aType);

public:
	OffsetedView(QWidget *parent, Executable::addr_type targetAddrType = Executable::RVA)
		: MouseTrackingTableView(parent), m_targetAddrType(targetAddrType),
		m_prevOffset(INVALID_ADDR)
	{
		setMouseTracking(true);
	}

	void setModel(QAbstractItemModel *model);

	//events:
	void mousePressEvent(QMouseEvent *ev); //emits target

protected slots:
	void onIndexChanged(const QModelIndex &current, const QModelIndex &previous); //emits target


protected:
	virtual offset_t getOffsetFromUserData(const QModelIndex &current)
	{
		QAbstractItemModel *model = this->model();
		if (!model|| !current.isValid()) return INVALID_ADDR;

		bool isOk;
		qint64 userData = model->data(current, Qt::UserRole).toULongLong(&isOk);
		return isOk ? userData : INVALID_ADDR;
	}

	const Executable::addr_type m_targetAddrType;

private:
	void emitSelectedTarget(const offset_t offset, Executable::addr_type aType, bool verify)
	{
		//protect from emitting the same target more than once (from various events)
		if (verify && m_prevOffset == offset && m_prevType == aType) {
			return;
		}
		m_prevOffset = offset;
		m_prevType = aType;
		//emit target:
		emit offsetChanged(offset, aType);
	}

	offset_t m_prevOffset; // previously selected target
	Executable::addr_type m_prevType;
};
