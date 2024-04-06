#pragma once

#include <QtGlobal>

#include "../PEBear.h"
#include "../gui/pe_models.h"
#include "../gui_base/PeGuiItem.h"
#include "../gui/windows/SectionAddWindow.h"
#include "../gui/DosHdrTableModel.h"
#include "../HexView.h"
#include "../DisasmView.h"
#include "../gui_base/PeTreeView.h"
#include "../SectionsDiagram.h"
#include "GeneralPanel.h"
#include "WrapperTreeView.h"
#include "DataDirWrapperSplitter.h"
#include "ResourceDirSplitter.h"

class DetailsTab : public QTabWidget, public PeViewItem
{
    Q_OBJECT
public:
	DetailsTab(PeHandler *peHndl, QWidget *parent);
	~DetailsTab();
	
	DisasmTreeView disasmView;
	
signals:
	void globalFontChanged();

public slots:
	void onGlobalFontChanged();

protected slots:
	void onAddSection();
	void onFitSections();
	void onCopyVirtualToRaw();
	void onAddImportLib();
	void onAddImportFunc();
	void onAutoAddImports();

	void setDisasmTabText(offset_t raw);
	void reloadTabsView();

protected:
	void manageDirTab(pe::dir_entry dirNum);
	void manageRichHdrTab();
	void shirtTabsAfter(pe::dir_entry dirNum, bool toTheLeft);
	void createModels();
	void deleteModels();
	void deleteSplitters();
	void createViews();

	void setupSectionsToolbar(QSplitter *owner);
	void setupImportsToolbar();
	void setScaledIcons();
	void reloadDirViewIcons();

	/* models */
	DosHdrTableModel *dosHdrModel;
	RichHdrTreeModel* richHdrModel;
	FileHdrTreeModel* fileHdrModel;
	OptionalHdrTreeModel* optHdrModel;
	SecHdrsTreeModel* secHdrsModel;
	
	ImportsTreeModel* importsModel;
	ImportedFuncModel* impFuncModel;

	ExportsTreeModel* exportsModel;
	ExportedFuncTreeModel* expFuncModel;

	SecDiagramModel *secDiagramModel;

	TLSTreeModel *tlsModel;
	TLSCallbacksModel *tlsCallbacksModel;
	RelocsTreeModel *relocsModel;
	RelocEntriesModel *relocEntriesModel;

	SecurityTreeModel *securityModel;
	LdConfigTreeModel *ldConfigModel;
	LdEntryTreeModel* ldEntryModel;
	BoundImpTreeModel *boundImpModel;

	DelayImpTreeModel *delayImpModel;
	DelayImpFuncModel *delayFuncModel;
	ClrTreeModel *clrModel;

	DebugTreeModel *debugModel;
	DebugRDSIEntryTreeModel *debugEntryModel;
	
	ExceptionTreeModel *exceptionModel;
	ResourcesTreeModel *resourcesModel;
	ResourceLeafModel  *resourcesLeafModel;

	DisasmModel *disasmModel;

	/* splitters */
	QSplitter hdrsSplitter;
	QSplitter secDiagramSplitter;
	DataDirWrapperSplitter* dirSplitters[pe::DIR_ENTRIES_COUNT];
	WrapperTableModel* dirUpModels[pe::DIR_ENTRIES_COUNT];
	WrapperTableModel* dirDownModels[pe::DIR_ENTRIES_COUNT];

	GeneralPanel generalPanel;
	StringsBrowseWindow stringsBrowseWindow;

	/* views */
	FollowablePeTreeView dosHdrTree;
	FollowablePeTreeView richHdrTree;
	FollowablePeTreeView fileHdrTree;
	FollowablePeTreeView optionalHdrTree;
	FollowablePeTreeView secHdrTreeView;

	SectionsDiagram *secRawDiagram, *secVirtualDiagram;
	
	/* widgets */
	QDockWidget* dockedRDiagram, *dockedVDiagram;
	
	QToolBar *sectionsToolBar;
	QAction *addSection,
		*copyVirtualToRaw,
		*fitSections, 
		*addImportLib, 
		*addImportFunc,
		*autoAddImports;

	SectionAddWindow winAddSec;
	QMutex fontMutex;

	/* tabs indexes */
	int cDisasmTab, cDOSHdrTab, cRichHdrTab, cFileHdrsTab, cOptHdrsTab, cSecHdrsTab, cGeneralTab, cStringsTab;
	int dirTabIds[pe::DIR_ENTRIES_COUNT];
};
