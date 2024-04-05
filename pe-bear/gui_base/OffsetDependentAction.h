#pragma once
#include <QtGlobal>

#include <bearparser/bearparser.h>

#include "../PEBear.h"

class OffsetDependentAction : public QAction
{
	Q_OBJECT
public:
	OffsetDependentAction(const Executable::addr_type addrType, const QString &title, QObject* parent);

	void setTitle(const QString &title) { this->title = title; }

	offset_t getOffset() { return offset; }
	Executable::addr_type getAddrType() { return addrType; }

public slots:
	void onOffsetChanged(offset_t offset);
	void onOffsetChanged(offset_t offset, Executable::addr_type addrType);

signals:
	void triggered(offset_t offset, Executable::addr_type addrType);

private slots:
	void onTriggered() { emit triggered(offset, addrType); }

protected:
	Executable::addr_type addrType;
	offset_t offset;
	QString title;

friend class OffsetDependentMenu;
};

class OffsetDependentMenu : public QMenu
{
	Q_OBJECT

public:
	OffsetDependentMenu (const Executable::addr_type addrT, const QString &text, QWidget *parent)
		: QMenu(text, parent), 
		addrType(addrT), offset(INVALID_ADDR), title(text)
	{
		onOffsetChanged(offset);
	}

	void addAction(QAction *action);
	void removeAction(QAction *action);

public slots:
	void onOffsetChanged(offset_t offset) { onOffsetChanged(offset, this->addrType); }
	void onOffsetChanged(offset_t offset, Executable::addr_type addrType);

signals:
	void offsetUpdated(offset_t, Executable::addr_type);

protected:
	Executable::addr_type addrType;
	offset_t offset;
	QString title;
};
