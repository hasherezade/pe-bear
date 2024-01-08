#pragma once
#include <QtGui>
#include <bearparser/core.h>
#include "OffsetedView.h"

namespace util {
	inline QString translateAddrTypeName(const Executable::addr_type addrType)
	{
		switch (addrType) {
			case Executable::RAW : return "Raw";
			case Executable::RVA : return "RVA";
			case Executable::VA : return "VA";
		}
		return "";
	}
};
//---

class FollowableOffsetedView : public OffsetedView
{
	Q_OBJECT

signals:
	void targetClicked(offset_t offset, Executable::addr_type);

public:
	enum ACTIONS
	{
		ACTION_FOLLOW = 0,
		COUNT_ACTIONS
	};

	FollowableOffsetedView(QWidget *parent, Executable::addr_type targetAddrType = Executable::RVA)
		: OffsetedView(parent, targetAddrType),
		m_ContextMenu(this),
		m_isMenuEnabled(true)
	{
		for (int i = 0; i < COUNT_ACTIONS; i++) {
			m_contextActions[i] = nullptr;
		}
		initContextMenu();
		enableMenu(true);
		connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuEvent(QPoint)));
	}

	QMenu& getMenu() { return m_ContextMenu; }

	void enableMenu(bool enable)
	{
		if (enable)
			this->setContextMenuPolicy(Qt::CustomContextMenu);
		else 
			this->setContextMenuPolicy(Qt::DefaultContextMenu);

		m_isMenuEnabled = enable;
	}

	bool enableAction(enum FollowableOffsetedView::ACTIONS id, bool state)
	{
		if (id >= COUNT_ACTIONS || m_contextActions[id] == nullptr) {
			return false;
		}
		m_contextActions[id]->setEnabled(state);
		return true;
	}

protected slots:
	bool updateActionsToOffset()
	{
		offset_t currentOffset = getSelectedOffset();
		if (currentOffset == INVALID_ADDR) {
			m_contextActions[ACTION_FOLLOW]->setText("Cannot follow");
			m_contextActions[ACTION_FOLLOW]->setEnabled(false);
			return false;
		}
		m_contextActions[ACTION_FOLLOW]->setText("Follow "+ util::translateAddrTypeName(m_targetAddrType) +": "+ QString::number(currentOffset, 16));
		m_contextActions[ACTION_FOLLOW]->setEnabled(true);
		return true;
	}
	
	void customMenuEvent(QPoint p)
	{
		if (!updateActionsToOffset()) return;
		m_ContextMenu.exec(mapToGlobal(p));
	}

	void followSelectedOffset()
	{
		offset_t currentOffset = getSelectedOffset();
		if (currentOffset != INVALID_ADDR) {
			emit targetClicked(currentOffset, m_targetAddrType);
		}
	}

protected:

	void initContextMenu()
	{
	
		m_contextActions[ACTION_FOLLOW] = new QAction("Follow", &m_ContextMenu);
		m_ContextMenu.addAction(m_contextActions[ACTION_FOLLOW]);
		connect(m_contextActions[ACTION_FOLLOW], SIGNAL(triggered()), this, SLOT(followSelectedOffset()));
	}

	virtual offset_t getSelectedOffset()
	{
		if (this->selectedIndexes().size() == 0) return INVALID_ADDR;
		QModelIndex current = this->selectedIndexes().back();
		return getOffsetFromUserData(current);
	}

	bool m_isMenuEnabled;
	QMenu m_ContextMenu;
	QAction* m_contextActions[COUNT_ACTIONS]; 
};
