#pragma once
#include <QtGlobal>

#include "../PEBear.h"
#include "../gui/pe_models.h"
#include "WrapperTreeView.h"
#include "DataDirWrapperSplitter.h"

//-----------------------------

class ResourcesDirSplitter : public DataDirWrapperSplitter
{
	Q_OBJECT

protected slots:
	void normalizeSelectedId(size_t parentRowId);
	void onUpIdSelected(size_t);
	void childIdSelected(int childId);
	void refreshLeafContent();
	
signals:
	void parentIdSelected(size_t parentId);
	
public:
	ResourcesDirSplitter(PeHandler *peHndl, WrapperTableModel *upModel, WrapperTableModel *downModel, QWidget *parent);
	virtual ~ResourcesDirSplitter();

	virtual bool initToolbar();
	virtual void resizeComponents();

public slots:
	void onSaveEntries();

protected:
	enum res_view_type
	{
		RES_VIEW_RAW = 0,
		RES_VIEW_PIX = 1,
		RES_VIEW_COUNT
	};
	
	void changeView(res_view_type viewType);
	void clearContentDisplay();
	bool displayResVersion(ResourceVersionWrapper *resContent);
	bool displayResStrings(ResourceStringsWrapper *resContent);
	bool displayText(ResourceContentWrapper *resContent);
	bool displayBitmap(ResourceContentWrapper *resContent);
	bool displayIcon(ResourceContentWrapper *resContent, const pe::resource_type &type);
	virtual void init(WrapperTableModel *upModel, WrapperTableModel *downModel);
	void fillList(size_t num);

	QAction* saveAction;
	QComboBox elementsList;
	QTabWidget leafTab;

	QSplitter contentDock;
	QTextEdit contentText;
	QTextEdit contentPixmap;
	QLabel pixmapInfo;
	int contentTab;
};

