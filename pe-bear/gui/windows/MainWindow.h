#pragma once

#ifdef WITH_QT5
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include <bearparser/bearparser.h>
#include <sig_finder.h>

#include "../../base/BearVers.h"

#include "DiffWindow.h"
#include "SectionAddWindow.h"
#include "PatternSearchWindow.h"
#include "SignaturesBrowseWindow.h"
#include "UserConfigWindow.h"

#include "../../SectionsDiagram.h"
#include "../../PEFileTreeModel.h"
#include "../../HexView.h"
#include "../../DisasmView.h"
#include "../../PEDockedWidget.h"

#include "../../gui_base/PEViewsManager.h"
#include "../../base/MainSettings.h"

#include "../../ViewSettings.h"

//----------------------------------------------

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(MainSettings &_mainSettings, QWidget *parent = 0);
	~MainWindow() { stopTimer(); }

	bool openPE(QString name);
	int openMultiplePEs(QStringList fNames);
	
signals:
	void addSectionRequested(PeHandler *peHndl);

public slots:
	void onSigSearchResult(int foundCount, int reqType);
	void runNewInstance();
	void unloadAllPEs();

	void dumpSectionsFromAllPEs();
	void exportDisasmFromAllPEs();
	void exportStringsFromAllPEs();
	void open();
	void closePE(PeHandler*);
	void reload(PeHandler*);
	void savePE(PeHandler* selectedPeHndl);
	void dumpAllSections(PeHandler* );
	void addSection(PeHandler* );
	//void sigSearch(PeHandler* );
	void searchPattern(PeHandler*);

	void openSignatures();

	void viewSignatures()
	{
		signWindow.show();
		signWindow.raise();
	}

	void info();

	void openDiffWindow()
	{
		this->diffWindow.show();
		this->diffWindow.raise(); 
	}

	// zooming:
	void zoomFonts(bool zoomIn);
	void zoomInFonts() { zoomFonts(true); }
	void zoomOutFonts() { zoomFonts(false); }

	// font switching:
	void changeHexFont();
	void changeDisasmFont();
	void changeGlobalFont();
	void setDefaultFonts();
	void setDefaultZoom();

	// style switching:
	void setSelectedStyle(QAction* a);

protected slots:
	void onExeHandlerAdded(PeHandler*);
	void onHandlerSelected(PeHandler*);
	
	void onTimeout();

	void setRegistryKey(bool enable);
	void editAppearance()
	{
		userConfigWindow.show();
		userConfigWindow.raise(); 
	}

protected:
	void _reload(PeHandler*, const bool isDeleted);

	/* events */
	
	virtual void wheelEvent(QWheelEvent* event)
	{
		if (QApplication::keyboardModifiers() & Qt::ControlModifier ) {
			bool zoomIn = false;
#if QT_VERSION >= 0x050000
			zoomIn = (event->angleDelta().y() > 0) ? true : false;
#else
			zoomIn = (event->delta() > 0) ? true : false;
#endif
			if (zoomIn) {
				zoomFonts(true);
			} else {
				zoomFonts(false);
			}
			return;
		}
		return QMainWindow::wheelEvent(event);
	}

	void dragEnterEvent(QDragEnterEvent* event) { event->accept(); }
	void dropEvent(QDropEvent* event);
	void showEvent(QShowEvent * event);
	void closeEvent(QCloseEvent *event);

	PeHandler* loadPE(QString name, const bool showAlert);

	bool loadTagsForPe(PeHandler *hndl, QString name = "");
	void autoSaveAllTags();
	bool autoSaveTags(PeHandler *hndl);
	bool isRegKey();
	bool selectCurrentStyle();

	QString chooseDumpOutDir(PeHandler *);
	void reloadFile(const QString &path, const bool isDeleted);
	bool checkFileChanges(const QString &path);

private:
	size_t dumpAllPeSections(PEFile *pe, const QString &dirPath, const QString &peName);
	ExeFactory::exe_type recognizeFileType(QString &name, const bool showAlert);
	bool readPersistent();
	bool writePersistent();

	void connectSignals();
	void setupLayout();
	void setupWidgets();
	
	void createActions();
	void createTreeActions();
	void createMenus();
	
	void startTimer();
	void stopTimer();
	
//---
	pe_bear::BearVers currentVer;

	PeHandler *m_PeHndl;
	QTimer m_Timer;

	std::vector<sig_finder::Signature*> signatures;
	sig_finder::Node sigFinder;
	MainSettings &mainSettings;
	GuiSettings guiSettings;

	QString winDesc;

	QSplitter mainSplitter;
	PEViewsManager rightPanel;
	PeHandlersManager m_PEHandlers;

	DiffWindow diffWindow;
	SectionAddWindow secAddWindow;

	QStatusBar statusBar;
	QGridLayout cntntLayout;

	PEStructureView sectionsTree;
	ExeDependentMenu sectionsTreeMenu;
	SectionMenu sectionMenu;

	PEFileTreeModel* peFileModel;

	//-------------------------
	QLabel urlLabel;

	QMenu *fileMenu, 
		*settingsMenu,
		*viewMenu,
		*signaturesMenu,
		*fromLoadedPEsMenu;

	QActionGroup *stylesGroup;

	QAction *openDiffWindowAction,
		*newInstance,
		*openAction,
		*unloadAllAction,
		*openSignAction,
		*viewSignAction,
		*infoAction,
		*setRegKeyAction,
		*hexViewFontAction,
		*disasmViewFontAction,
		*globalFontAction,
		*defaultFontsAction,
		*zoomInAction, *zoomOutAction, *zoomDefault,
		*darkStyle, *defaultStyle,
		*dumpAllPEsSecAction,
		*searchSignature,
		*exportAllPEsDisasmAction,
		*exportAllPEsStrings;

	ExeDependentAction *dumpAllSecAction,
		*addSecAction,
		*saveAction,
		*unloadAction,
		*reloadAction;

	SignaturesBrowseWindow signWindow;
	UserConfigWindow userConfigWindow;
};
