#pragma once
#include <QtGlobal>

#include "QtCompat.h"
#include "gui_base/PeGuiItem.h"
#include "gui/ContentPreview.h"
#include "gui/DetailsTab.h"
#include "gui/windows/OffsetsBrowseWindow.h"


class PEDockedWidget : public QDockWidget, public PeViewItem
{
    Q_OBJECT
public:
	PEDockedWidget(PeHandler *peHndl, QWidget *parent);
	~PEDockedWidget() { delete diagramModel; }
	
Q_SIGNALS:
	void signalChangeHexViewSettings(HexViewSettings &);
	void signalChangeDisasmViewSettings(DisasmViewSettings &);

public slots:
	void goToEntryPoint();
	void goToRVA();
	void goToOffset();
	void goToLastModif();
	
	void undoOffset() { if (myPeHndl) myPeHndl->undoDisplayOffset(); }
	void unModify() { if (myPeHndl) myPeHndl->unModify(); }
	
	void pageUp() { if (myPeHndl) myPeHndl->advanceOffset(-PREVIEW_SIZE); }
	void pageDown() { if (myPeHndl) myPeHndl->advanceOffset(PREVIEW_SIZE); }
	
	void offsetUp(){ if (myPeHndl) myPeHndl->advanceOffset(-1); }
	void offsetDown(){ if (myPeHndl) myPeHndl->advanceOffset(1); }
	
	void offsetPrev(){ if (myPeHndl) myPeHndl->undoDisplayOffset(); }
	
	void browseTags()
	{
		tagBrowser.show();
		tagBrowser.raise();
	}
	
	void updateModifActions();
	void updateNavigActions();

	void changeHexViewSettings(HexViewSettings &_settings)
	{
		emit signalChangeHexViewSettings(_settings);
	}
	
	void changeDisasmViewSettings(DisasmViewSettings &_settings)
	{
		emit signalChangeDisasmViewSettings(_settings);
	}

	void refreshFonts()
	{
		this->setFont(QApplication::font());
		setScaledIcons();
		
		// adjust side diagram:
		this->diagram->setMaximumWidth(this->diagram->minimumSizeHint().width());
		this->diagram->setMinimumWidth(this->diagram->minimumSizeHint().width());
		this->tabWidget->onGlobalFontChanged();
	}
	
protected:
	void keyPressEvent(QKeyEvent *event);
	void setScaledIcons();
	void goToAddress(bool isRaw);

	void setupDiagram();
	void setupActionsToolbar(QSplitter* owner);
	
	QMainWindow dockWindow;
	QToolBar *toolBar;

	//toolbar actions:
	QAction *goToEntryPointA, *goToRvaAction, *goToRawAction,
		*goToModifAction, *unModifyAction, *tagsAction, *backAction;

	OffsetsBrowseWindow tagBrowser;
	QSplitter mainSplitter;
	ContentPreview *contentPrev;
	DetailsTab *tabWidget;
	QSplitter *cntntSplitter;
	
	SecDiagramModel* diagramModel;
	SelectableSecDiagram* diagram;
};

//--------------------------------------------------------------------------------------

class SectionMenu : public QMenu
{
	Q_OBJECT
public:
	SectionMenu(MainSettings &settings, QWidget *parent = 0);

public slots:
	void sectionSelected(PeHandler *pe, SectionHdrWrapper *sec);

	void dumpSelectedSection();
	void clearSelectedSection();
	void loadSelectedSection();
	void exportSectionDisasm();

protected:
	void createActions();

	MainSettings &mainSettings;
	PeHandler *peHndl;
	SectionHdrWrapper *selectedSection;
	QAction *dumpSecAction, 
		*dumpSelSecAction, 
		*clearSelSecAction, 
		*loadSelSecAction, 
		*searchSigAction, 
		*dumpDisasmAction;
};
