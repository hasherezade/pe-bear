#pragma once
#include <QtGlobal>

#include "../QtCompat.h"
#include "../gui/pe_models.h"
#include "WrapperTreeView.h"
#include "WrapperSplitter.h"

class DataDirWrapperSplitter : public WrapperSplitter, public PeViewItem
{
	Q_OBJECT

public:
	DataDirWrapperSplitter(PeHandler *peHndl, pe::dir_entry dirId, QWidget *parent)
		: PeViewItem(peHndl), WrapperSplitter(parent), dataDirId(dirId), moveDirTable(NULL)
	{
	}

	DataDirWrapperSplitter(PeHandler *peHndl, pe::dir_entry dirId, PeTreeModel *upModel, PeTreeModel *downModel, QWidget *parent)
		: PeViewItem(peHndl), WrapperSplitter(upModel, downModel, parent), dataDirId(dirId), moveDirTable(NULL)
	{
		initToolbar();
	}

	virtual ~DataDirWrapperSplitter() {}

	virtual bool initToolbar();
	virtual void setScaledIcons();
	QString chooseDumpOutDir();

public slots:
	void onMoveDirTable();
	void onGlobalFontChanged()
	{
		setScaledIcons();
	}

protected:
	QAction* moveDirTable;
	pe::dir_entry dataDirId;
};

//-----------------------------

class SecurityDirSplitter : public DataDirWrapperSplitter
{
	Q_OBJECT
public:
	SecurityDirSplitter(PeHandler *peHndl, WrapperTableModel *upModel, WrapperTableModel *downModel, QWidget *parent)
		: DataDirWrapperSplitter(peHndl, pe::DIR_SECURITY, upModel, downModel, parent), saveCertAction(NULL)
	{
		initToolbar();
	}

	virtual bool initToolbar();
	virtual void setScaledIcons();
	
public slots:
	void onSaveCert();

private:
	QAction* saveCertAction;
};
