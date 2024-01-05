#include "MainWindow.h"
#include <QtGlobal>
#include <bearparser/bearparser.h>
#include <QApplication>
#include <QTranslator>
#include "../../HexView.h"
#include "../../base/RegKeyManager.h"


#ifdef _DEBUG
	#include <iostream>
#endif

#define PE_TREE_WIDTH 220
#define MIN_HEIGHT 600
#define SIG_FILE "SIG.txt"

#define CHANGE_CHECK_INTERVAL 1000

using namespace std;
using namespace sig_ma;
//-----------------------------------------------------------

class UniqueNameHelper
{
public:
	QString getUniqueFromName(const QString &name)
	{
		QString myName = name;
		long nameId = 1;
		while (uniqueNames.find(myName) != uniqueNames.end()) {
			myName = name + "_" + QString::number(nameId++, 10);
		}
		uniqueNames.insert(myName);
		return myName;
	}
	
protected:
	std::set<QString> uniqueNames;
};

//-----------------------------------------------------------
MainWindow::MainWindow(MainSettings &_mainSettings, QWidget *parent) 
	: QMainWindow(parent), mainSettings(_mainSettings),
	m_PeHndl(NULL), m_Timer(this),
	diffWindow(this->m_PEHandlers, this),
	secAddWindow(this), userConfigWindow(this), patternSearchWindow(this),
	sectionsTree(this), sectionMenu(mainSettings, this),
	peFileModel(NULL),
	rightPanel(this), 
	signWindow(&vSign, this),
	currentVer(V_MAJOR, V_MINOR, V_PATCH, V_PATCH_SUB, V_DESC),
	guiSettings()
{
	// load the saved settings:
	MainSettingsHolder::setMainSettings(&this->mainSettings);
	userConfigWindow.setMainSettings(&this->mainSettings);

	winDesc = QString(TITLE) + " v" + currentVer.toString();
	setWindowTitle(winDesc);

	setAcceptDrops(true);
	setMinimumHeight(MIN_HEIGHT);

	setStatusBar(&statusBar);
	statusBar.addPermanentWidget(&urlLabel);

	setupWidgets();
	setupLayout();

	createActions();
	createMenus();
	connectSignals();

	guiSettings.readPersistent();
	selectCurrentStyle();
	startTimer();

	// try to load from alternative files:
	const QString sigFile1 = this->mainSettings.userDataDir() + QDir::separator() + SIG_FILE;
	vSign.loadSignaturesFromFile(sigFile1.toStdString());

	const QString sigFile2 = QDir::currentPath() + QDir::separator() + SIG_FILE;
	vSign.loadSignaturesFromFile(sigFile2.toStdString());

	signWindow.onSigListUpdated();
}

void MainWindow::connectSignals()
{
	connect(&m_PEHandlers, SIGNAL(exeHandlerAdded(PeHandler *)), 
		this, SLOT(onExeHandlerAdded(PeHandler *)), Qt::UniqueConnection);

	connect(&sectionsTree, SIGNAL(handlerSelected(PeHandler *)),
		this, SLOT(onHandlerSelected(PeHandler*)), Qt::UniqueConnection);

	connect(&sectionsTree, SIGNAL(handlerSelected(PeHandler *)),
		&diffWindow, SLOT(refresh()), Qt::UniqueConnection);

	connect(this, SIGNAL(addSectionRequested(PeHandler*)),
		&secAddWindow, SLOT(onAddSectionToPe(PeHandler*)) );

	connect(&signWindow, SIGNAL(signaturesUpdated()),
		&m_PEHandlers, SLOT(checkAllSignatures()));
		
	connect(&m_PEHandlers, SIGNAL(matchedSignatures()),
		&sectionsTree, SLOT(onNeedReset()));

	connect(&guiSettings, SIGNAL(globalFontChanged()),
		&rightPanel, SLOT(onGlobalFontChanged()) );

	connect(&guiSettings, SIGNAL(globalFontChanged()),
		&diffWindow, SLOT(onGlobalFontChanged()) );

	connect(&guiSettings, SIGNAL(hexViewSettingsChanged(HexViewSettings&)),
		&rightPanel, SLOT(changeHexViewSettings(HexViewSettings&)) );
		
	connect(&guiSettings, SIGNAL(hexViewSettingsChanged(HexViewSettings&)),
		&diffWindow, SLOT(changeHexViewSettings(HexViewSettings&)) );

	connect(&guiSettings, SIGNAL(disasmViewSettingsChanged(DisasmViewSettings&)),
		&rightPanel, SLOT(changeDisasmViewSettings(DisasmViewSettings&)) );
}

void MainWindow::reloadFile(const QString &path, const bool isDeleted)
{
	PeHandler* hndl = this->m_PEHandlers.getByName(path);
	if (!hndl) {
		return;
	}
	_reload(hndl, isDeleted);
	if (!isDeleted) {
		this->statusBar.showMessage(tr("File reloaded: ")+ path);
	}
}

bool MainWindow::checkFileChanges(const QString &path)
{
	PeHandler* hndl = this->m_PEHandlers.getByName(path);
	if (!hndl) return false;
	
	if (!hndl->updateFileModifTime()) {
		return false;
	}
	QFileInfo fileInfo(path);
	const bool isFileDeleted = fileInfo.exists() ? false : true;

	if (isFileDeleted) {
		this->statusBar.showMessage(tr("File deleted: ")+ path);
	} else {
		this->statusBar.showMessage(tr("File changed: ")+ path);
	}
	
	const t_reload_mode rMode = this->mainSettings.isReloadOnFileChange();
	if (rMode == RELOAD_IGNORE) {
		// ignore the changes...
		return false;
	}
	bool shouldReload = (rMode == RELOAD_AUTO);
	if (!shouldReload || isFileDeleted) {
		const QString wndTitle = (!isFileDeleted) ? tr("File changed!") : tr("File deleted!");
		const QString wndInfo = (!isFileDeleted) 
			? tr("The file:")+"\n" + path + "\n"+tr("- has changed.") + "\n" + tr("Do you want to reload ? " )
			: tr("The file:") +"\n" + path + "\n"+tr("- has been deleted.")+"\n"+tr("Do you want to unload?");

		QMessageBox msgbox(this);

		msgbox.setWindowTitle(wndTitle);
		msgbox.setText(wndInfo);
		msgbox.setIcon(QMessageBox::Question);
		msgbox.addButton(QMessageBox::Yes);
		msgbox.addButton(QMessageBox::No);
		msgbox.setDefaultButton(QMessageBox::Yes);		
#if QT_VERSION >= 0x050000
		QCheckBox cb(tr("Remember the answer"), &msgbox);
		cb.setToolTip(tr("Can be changed in: [Settings] -> [Configure...]"));
		cb.setChecked(false);
		msgbox.setCheckBox(&cb);
#endif
		const int reply = msgbox.exec();
		if (reply == QMessageBox::Yes) {
			shouldReload = true;
		} else {
			shouldReload = false;
		}
		
		t_reload_mode rMode = RELOAD_ASK;
#if QT_VERSION >= 0x050000
		const bool setAuto = cb.isChecked();
#else
		const bool setAuto = false;
#endif
		if (setAuto) {
			if (shouldReload) {
				rMode = RELOAD_AUTO;
			}
			else {
				rMode = RELOAD_IGNORE;
			}
		}
		this->mainSettings.setReloadOnFileChange(rMode);
	}
	if (shouldReload) {
		reloadFile(path, isFileDeleted);
	}
	return true;
}

bool MainWindow::readPersistent()
{
	QSettings settings(COMPANY_NAME, APP_NAME);
	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());

	if (settings.status() != QSettings::NoError ) {
		return false;
	}
	return true;
}

bool MainWindow::writePersistent()
{
	QSettings settings(COMPANY_NAME, APP_NAME);
	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());

	if (settings.status() != QSettings::NoError ) {
		return false;
	}
	return true;
}

void MainWindow::setupLayout()
{
	setCentralWidget(&mainSplitter);
	mainSplitter.setAutoFillBackground(true);
	mainSplitter.insertWidget(0, &this->sectionsTree);
	mainSplitter.insertWidget(1, &this->rightPanel);
	
	mainSplitter.setStretchFactor(0, 0);
	mainSplitter.setStretchFactor(1, 1);
}

void MainWindow::setupWidgets()
{
	peFileModel = new PEFileTreeModel(&sectionsTree);
	connect(&m_PEHandlers, SIGNAL(exeHandlerAdded(PeHandler*)), peFileModel, SLOT(addHandler(PeHandler *)) );
	connect(&m_PEHandlers, SIGNAL(exeHandlerRemoved(PeHandler*)), peFileModel, SLOT(deleteHandler(PeHandler *)) );

	sectionsTree.setModel(peFileModel);

	sectionsTree.setSelectionMode(QAbstractItemView::SingleSelection);
	sectionsTree.setSelectionBehavior( QAbstractItemView::SelectRows);
	urlLabel.setProperty("hasUrl",true);
	urlLabel.setText("<a href=\"" + QString(PEBEAR_LINK) + "\">" + tr("Check for updates") + "</a>");
	urlLabel.setTextFormat(Qt::RichText);
	urlLabel.setTextInteractionFlags(Qt::TextBrowserInteraction);
	urlLabel.setOpenExternalLinks(true);
	urlLabel.setToolTip(tr("Check if new version is available or")+ "\n"+tr("write a comment about your user experience!"));
}

void MainWindow::createTreeActions()
{
	dumpAllSecAction =  new ExeDependentAction(QIcon(":/icons/dump.ico"), tr("Dump all sections to..."), this);
	connect(this->dumpAllSecAction, SIGNAL(triggered(PeHandler*)), this, SLOT(dumpAllSections(PeHandler*)));
	
	QIcon addIco(":/icons/add_entry.ico");
	addSecAction =  new ExeDependentAction(addIco, tr("Add a new section"), this);
	connect(this->addSecAction, SIGNAL(triggered(PeHandler*)), this, SLOT(addSection(PeHandler*)));

	QIcon saveIco(":/icons/Save.ico");
	this->saveAction = new ExeDependentAction(saveIco, tr("&Save the executable as..."), this);
	connect(this->saveAction, SIGNAL(triggered(PeHandler*)), this, SLOT(savePE(PeHandler*)));
	
	this->searchSignature = new ExeDependentAction(QIcon(":/icons/Preview.ico"), tr("Find signature"), this);
	connect(this->searchSignature, SIGNAL(triggered(PeHandler*)), this, SLOT(searchPattern(PeHandler*)));

	reloadAction = new ExeDependentAction(QIcon(":/icons/reload.ico"), tr("&Reload"), this);
	connect(reloadAction, SIGNAL(triggered(PeHandler*)), this, SLOT(reload(PeHandler*)) );

	unloadAction = new ExeDependentAction(QIcon(":/icons/Delete.ico"), tr("&Unload"), this);
	connect(unloadAction, SIGNAL(triggered(PeHandler*)), this, SLOT(closePE(PeHandler*)));

	sectionsTree.setMenu(&sectionsTreeMenu);
	connect(&sectionsTree, SIGNAL(handlerSelected(PeHandler*)), &sectionsTreeMenu, SLOT(onExeChanged(PeHandler*)) );

	sectionsTree.enableMenu(true);

	sectionMenu.setTitle(tr("Edit the section")+"\n");
	sectionsTreeMenu.addMenu(&sectionMenu);
	sectionsTreeMenu.addSeparator();

	sectionsTreeMenu.addAction(this->addSecAction);
	sectionsTreeMenu.addAction(this->dumpAllSecAction);
	sectionsTreeMenu.addAction(this->saveAction);
	sectionsTreeMenu.addSeparator();
	sectionsTreeMenu.addAction(this->searchSignature);
	sectionsTreeMenu.addSeparator();
	sectionsTreeMenu.addAction(this->reloadAction);
	sectionsTreeMenu.addAction(this->unloadAction);
}

void MainWindow::createActions()
{
	createTreeActions();

	//Actions
	newInstance = new QAction(QIcon(":/icons/add_entry.ico"), tr("&New Instance"), this);
	newInstance->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_N));
	newInstance->setShortcutContext(Qt::ApplicationShortcut);
	connect(this->newInstance, SIGNAL(triggered()), this, SLOT(runNewInstance()));

	openAction = new QAction(QIcon(":/icons/Add.ico"), tr("&Load PEs"), this);
	openAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
	openAction->setShortcutContext(Qt::ApplicationShortcut);
	connect(this->openAction, SIGNAL(triggered()), this, SLOT(open()));
	
	unloadAllAction = new QAction(QIcon(":/icons/DeleteAll.ico"), tr("&Unload All"), this);
	connect(this->unloadAllAction, SIGNAL(triggered()), this, SLOT(unloadAllPEs()));

	dumpAllPEsSecAction = new QAction(QIcon(":/icons/dump.ico"), tr("Dump all sections to..."), this);
	connect(this->dumpAllPEsSecAction, SIGNAL(triggered()), this, SLOT(dumpSectionsFromAllPEs()));
	
	exportAllPEsDisasmAction = new QAction(QIcon(":/icons/disasm.ico"), tr("Export disassembly to..."), this);
	connect(this->exportAllPEsDisasmAction, SIGNAL(triggered()), this, SLOT(exportDisasmFromAllPEs()));
	
	exportAllPEsStrings = new QAction(tr("Export strings to..."), this);
	connect(this->exportAllPEsStrings, SIGNAL(triggered()), this, SLOT(exportStringsFromAllPEs()));
	
	setRegKeyAction = new QAction(tr("Add to Explorer"), this);
	this->setRegKeyAction->setCheckable(true);
	this->setRegKeyAction->setChecked(isRegKey());
	connect(this->setRegKeyAction, SIGNAL(triggered(bool)), this, SLOT(setRegistryKey(bool)));

	viewSignAction = new QAction(QIcon(":/icons/List.ico"), tr("&List"), this);
	connect(this->viewSignAction, SIGNAL(triggered()), this, SLOT(viewSignatures()));
	openSignAction = new QAction(tr("&Load"), this);
	connect(this->openSignAction, SIGNAL(triggered()), this, SLOT(openSignatures()));

	infoAction = new QAction(tr("&Info"), this);
	connect(this->infoAction, SIGNAL(triggered()), this, SLOT(info()));

	openDiffWindowAction = new QAction(tr("Compare"), this);
	connect(this->openDiffWindowAction, SIGNAL(triggered()), this, SLOT(openDiffWindow()) );
	
	hexViewFontAction = new QAction(tr("HexView Font"), this);
	connect(this->hexViewFontAction, SIGNAL(triggered()), this, SLOT(changeHexFont()) );
	
	disasmViewFontAction = new QAction(tr("DisasmView Font"), this);
	connect(this->disasmViewFontAction, SIGNAL(triggered()), this, SLOT(changeDisasmFont()) );
	
	globalFontAction = new QAction(tr("Global Font"), this);
	connect(this->globalFontAction, SIGNAL(triggered()), this, SLOT(changeGlobalFont()) );

	defaultFontsAction = new QAction(tr("Reset to defaults"), this);
	connect(this->defaultFontsAction, SIGNAL(triggered()), this, SLOT(setDefaultFonts()));

	zoomInAction = new QAction(tr("Zoom In"), this);
	zoomInAction->setShortcut(QKeySequence::ZoomIn);
	zoomInAction->setShortcutContext(Qt::ApplicationShortcut);
	connect(this->zoomInAction, SIGNAL(triggered()), this, SLOT(zoomInFonts()));
	
	zoomOutAction = new QAction(tr("Zoom Out"), this);
	zoomOutAction->setShortcut(QKeySequence::ZoomOut);
	zoomOutAction->setShortcutContext(Qt::ApplicationShortcut);
	connect(this->zoomOutAction, SIGNAL(triggered()), this, SLOT(zoomOutFonts()));
	
	zoomDefault = new QAction(tr("Default size"), this);
	zoomDefault->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
	zoomDefault->setShortcutContext(Qt::ApplicationShortcut);
	connect(this->zoomDefault, SIGNAL(triggered()), this, SLOT(setDefaultZoom()));
}

void MainWindow::createMenus()
{
	//Create toolbar menus:
	this->fileMenu = menuBar()->addMenu(tr("&File"));
	this->settingsMenu = menuBar()->addMenu(tr("&Settings"));
	this->viewMenu = menuBar()->addMenu(tr("&View"));

	QMenu *zoomMenu = this->viewMenu->addMenu(tr("Zoom"));
	zoomMenu->addAction(zoomInAction);
	zoomMenu->addAction(zoomOutAction);
	zoomMenu->addAction(zoomDefault);
	zoomMenu->insertSeparator(zoomDefault);
	
	QMenu *fontMenu = this->viewMenu->addMenu(tr("Fon&t"));
	fontMenu->addAction(globalFontAction);
	fontMenu->addAction(hexViewFontAction);
	fontMenu->addAction(disasmViewFontAction);
	fontMenu->addAction(defaultFontsAction);
	fontMenu->insertSeparator(defaultFontsAction);

	this->stylesGroup = new QActionGroup(this);
	stylesGroup->setExclusive(true);
	QMenu *styleMenu = this->viewMenu->addMenu(tr("Style"));
	QList<QString> myStyles = this->guiSettings.getStyles();
	QList<QString>::iterator itr;
	for (itr = myStyles.begin(); itr != myStyles.end(); ++itr) {
		const QString style = *itr;
		QAction *action = new QAction(styleMenu);
		if (!action) continue; // should never happen
		action->setText(style);
		action->setCheckable(true);
		styleMenu->addAction(action);
		stylesGroup->addAction(action);
	}
	connect(styleMenu, SIGNAL(triggered(QAction*)), this, SLOT(setSelectedStyle(QAction*)));

	// Signatures menu
	this->signaturesMenu = this->settingsMenu->addMenu(tr("Si&gnatures"));

	menuBar()->addAction(this->openDiffWindowAction);
	menuBar()->addAction(this->infoAction);

	//asociate with actions:
	this->fileMenu->addAction(this->newInstance);
	this->fileMenu->addSeparator();
	this->fileMenu->addAction(this->openAction);
	this->fileMenu->addAction(this->unloadAllAction);

#ifdef _WINDOWS
	this->settingsMenu->addAction(this->setRegKeyAction);
#endif
	
	this->settingsMenu->addSeparator();
	QAction* appearanceAction = new QAction(tr("Configure..."), this);
	this->settingsMenu->addAction(appearanceAction);
	connect(appearanceAction, SIGNAL(triggered()), this, SLOT(editAppearance()));
	//---
	this->signaturesMenu->addAction(this->viewSignAction);
	this->signaturesMenu->addAction(this->openSignAction);
	
	// Process all loaded PEs:
	this->fileMenu->addSeparator();
	this->fromLoadedPEsMenu = this->fileMenu->addMenu(tr("From all loaded..."));
	this->fromLoadedPEsMenu->addAction(this->dumpAllPEsSecAction);
	this->fromLoadedPEsMenu->addAction(this->exportAllPEsDisasmAction);
	this->fromLoadedPEsMenu->addAction(this->exportAllPEsStrings);
}

void MainWindow::startTimer()
{
	m_Timer.setInterval(CHANGE_CHECK_INTERVAL);
	connect(&m_Timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
	m_Timer.start(CHANGE_CHECK_INTERVAL);
}

void MainWindow::stopTimer()
{
	m_Timer.stop();
}

void MainWindow::onTimeout()
{
	if (!this->m_PeHndl) return;

	const QString path = m_PeHndl->getFullName();
	checkFileChanges(path);
}

void MainWindow::onHandlerSelected(PeHandler* peHndl)
{
	if (peHndl && this->checkFileChanges(peHndl->getFullName())) {
		return;
	}
	PEFile* pePtr = (!peHndl) ? NULL : peHndl->getPe();
	const bool isPe = (!pePtr) ? false : true;
	const bool isChanged = (m_PeHndl != peHndl) ? true : false;
	
	this->m_PeHndl = peHndl;
	this->unloadAction->setEnabled(isPe);
	
	const offset_t offset = (peHndl) ? peHndl->getDisplayedOffset() : INVALID_ADDR;
	SectionHdrWrapper* sec = (peHndl) ? pePtr->getSecHdrAtOffset(offset, Executable::RAW) : nullptr;
	this->sectionMenu.sectionSelected(peHndl, sec);
	
	if (!sec) {
		ExeElementWrapper* selectedWrapper = (peHndl) ? pePtr->getWrapperContaining(offset) : nullptr;
		this->sectionMenu.wrapperSelected(peHndl, selectedWrapper);
	}

	if (!isPe) {
		this->setWindowTitle(this->winDesc);
		return;
	}

	if (isChanged) {
		QString fName = " [" + peHndl->getFullName() + "]";
		this->setWindowTitle(this->winDesc + fName);
	}

	PEDockedWidget *p = rightPanel.getPeDockWidget(peHndl);
	if (!p) return;

	p->show();
	p->raise();
}

void MainWindow::onExeHandlerAdded(PeHandler* hndl)
{
	if (hndl == NULL) return;

	const QString loadInfo = tr("Loaded: ") + hndl->getFullName();
	this->statusBar.showMessage(loadInfo);
	this->loadTagsForPe(hndl);
}

int MainWindow::openMultiplePEs(QStringList fNames)
{
	PeHandler* lastHndl = NULL;
	int cntr = 0;
	QStringList::Iterator urlItr;
	const bool showAlert = fNames.size() > 1 ? false : true;
	
	for (urlItr = fNames.begin() ; urlItr != fNames.end(); urlItr++) {
		//QString fName = urlItr->toLocalFile();
		QString fName = *urlItr;
		PeHandler *loaded = loadPE(fName, showAlert);
		if (loaded) {
			cntr++;
			lastHndl = loaded;
		}
	}
	//set the last one as active:
	if (lastHndl) {
		this->sectionsTree.selectHandler(lastHndl);
	}
	return cntr;
}

void MainWindow::dropEvent(QDropEvent* ev) 
{ 
	QList<QUrl> urls = ev->mimeData()->urls();
	QList<QUrl>::Iterator urlItr;
	QCursor cur = this->cursor();
	this->setCursor(Qt::BusyCursor);
	
	QStringList fNames;
	for (urlItr = urls.begin() ; urlItr != urls.end(); urlItr++) {
		QString fName = urlItr->toLocalFile();
		fNames << fName;
	}

	const int cntr = openMultiplePEs(fNames);
	const int failed = fNames.size() - cntr;

	this->setCursor(cur);
	
	if (cntr > 1) {
		this->statusBar.showMessage(tr("Loaded") + ": "  + QString::number(cntr));
		QString loadedStr = tr("Loaded ")+ QString::number(cntr);
		QString failedStr = "\n"+ tr("Failed to load: ") + QString::number(failed);
		if (failed) loadedStr += failedStr;
		QMessageBox::information(this, tr("Done!"), loadedStr);
	}
}

void MainWindow::setRegistryKey(bool enable)
{
	QString path = QApplication::applicationFilePath();
	for (int i = 0; i < path.size(); i++) {
		if (path[i] == '/') path[i] = '\\';
	}
	path = "\"" + path + "\"";
	
	QString accessErrInfo = tr("In order to do it you must run ") + QString(TITLE) + " " + tr("as administrator");

	bool isSet = false;
	const size_t extensionsCount = 4;
	std::string extensions[extensionsCount] = { "exe", "dll", "sys", "scr" }; 

	if (enable) {
		for (size_t i = 0; i < extensionsCount; i++) {
			isSet = RegKeyManager::addRegPath(extensions[i], TITLE, path.toStdString());
			if (!isSet) break;
		}
		if (!isSet) QMessageBox::warning(this, tr("Cannot set!"), accessErrInfo);

	} else {
		for (size_t i = 0; i < extensionsCount; i++) {
			isSet = RegKeyManager::removeRegPath(extensions[i], TITLE);
			if (!isSet) break;
		}
		if (!isSet) QMessageBox::warning(this, tr("Cannot remove!"), accessErrInfo);
	}

	this->setRegKeyAction->setChecked(isRegKey());
	if (isSet) {
		QMessageBox::information(this, tr("OK"), tr("Done!"));
	}
}

bool MainWindow::selectCurrentStyle()
{
	if (!this->stylesGroup) return false;
	
	QList<QAction *>actions = this->stylesGroup->actions();
	QList<QAction *>::iterator itr;
	for (itr = actions.begin(); itr != actions.end(); ++itr) {
		QAction *a = *itr;
		QString style = a->text();
		if (style == this->guiSettings.currentStyleName()) {
			a->setChecked(true);
			return true;
		}
	}
	return false;
}

bool MainWindow::isRegKey()
{
	bool isSet = false;
#ifdef _WINDOWS
	isSet = RegKeyManager::isKeySet("exe", TITLE);
#endif
	return isSet;
}

void MainWindow::autoSaveAllTags()
{
	if (this->mainSettings.isAutoSaveTags() == false) return;

	std::map<PEFile*, PeHandler*> &handlers = m_PEHandlers.getHandlersMap();
	std::map<PEFile*, PeHandler*>::iterator iter;
	for (iter = handlers.begin(); iter != handlers.end(); ++iter) {
		autoSaveTags(iter->second);
	}
}

void MainWindow::unloadAllPEs()
{
	std::map<PEFile*, PeHandler*> &handlers = m_PEHandlers.getHandlersMap();
	std::map<PEFile*, PeHandler*>::iterator iter;
	for (iter = handlers.begin(); iter != handlers.end();) {
		PeHandler *oldHndl = iter->second;
		++iter;
		closePE(oldHndl);
	}
	m_PEHandlers.clear();
}

void MainWindow::dumpSectionsFromAllPEs()
{
	std::map<PEFile*, PeHandler*> &handlers = m_PEHandlers.getHandlersMap();
	const size_t count = handlers.size();
	if (count == 0) return;
 
 	QString dirPath = chooseDumpOutDir(NULL);
	if (!dirPath.length()) return;
	
	UniqueNameHelper uniquePeHelper;
	size_t dumped = 0;
	std::map<PEFile*, PeHandler*>::iterator iter;
	for (iter = handlers.begin(); iter != handlers.end(); ++iter) {
		PeHandler *hndl = iter->second;
		if (!hndl) continue;
		PEFile *pe = hndl->getPe();
		if (!pe) continue;
		
		const QString peName = uniquePeHelper.getUniqueFromName(hndl->getShortName()); //protect against duplicated names
		if (dumpAllPeSections(pe, dirPath, peName)) {
			dumped++;
		}
	}
	if (dumped != count) {
		QMessageBox::warning(this, tr("Error"), tr("Dumping sections from some of the PEs failed!"));
	}
	if (dumped > 0) {
		QMessageBox::information(this, tr("Done!"), tr("Dumped sections from: ") + QString::number(dumped)
			+ tr(" PEs into:")+"\n" + dirPath);
	}
}

void MainWindow::exportDisasmFromAllPEs()
{
	std::map<PEFile*, PeHandler*> &handlers = m_PEHandlers.getHandlersMap();
	const size_t count = handlers.size();
	if (count == 0) return;
 
 	QString dirPath = chooseDumpOutDir(NULL);
	if (!dirPath.length()) return;
	
	size_t dumped = 0;
	UniqueNameHelper uniquePeHelper;
	
	std::map<PEFile*, PeHandler*>::iterator iter;
	for (iter = handlers.begin(); iter != handlers.end(); ++iter) {
		PeHandler *hndl = iter->second;
		if (!hndl) continue;
		
		PEFile *pe = hndl->getPe();
		if (!pe) continue;
		
		DWORD ep = pe->getEntryPoint(Executable::RAW);
		SectionHdrWrapper* sec = pe->getSecHdrAtOffset(ep, Executable::RAW, true);
		if (!sec) continue;
		
		const QString peName = uniquePeHelper.getUniqueFromName(hndl->getShortName()); //protect against duplicated names
		const QString secName = sec->mappedName;
		const QString fileName = dirPath + QDir::separator() + peName + "[" + secName + "].txt";

		const offset_t startOff = sec->getContentOffset(Executable::RAW, true);
		const size_t previewSize = sec->getContentSize(Executable::RAW, true);
		if (hndl->exportDisasm(fileName, startOff, previewSize)) {
			dumped++;
		}
	}
	if (dumped != count) {
		QMessageBox::warning(this, tr("Error"), tr("Exporting disasm from some of the PEs failed!"));
	}
	if (dumped > 0) {
		QMessageBox::information(this, tr("Done!"), tr("Exported disasm from: ") + QString::number(dumped)
			+ tr(" PEs into:") + "\n" + dirPath);
	}
}

void MainWindow::exportStringsFromAllPEs()
{
	std::map<PEFile*, PeHandler*> &handlers = m_PEHandlers.getHandlersMap();
	const size_t count = handlers.size();
	if (count == 0) return;
 
 	QString dirPath = chooseDumpOutDir(NULL);
	if (!dirPath.length()) return;
	
	size_t dumped = 0;
	UniqueNameHelper uniquePeHelper;
	
	std::map<PEFile*, PeHandler*>::iterator iter;
	for (iter = handlers.begin(); iter != handlers.end(); ++iter) {
		PeHandler *hndl = iter->second;
		if (!hndl) continue;
		const QString peName = uniquePeHelper.getUniqueFromName(hndl->getShortName()); //protect against duplicated names
		const QString fileName = dirPath + QDir::separator() + peName + ".strings.txt";
		if (hndl->stringsMap.saveToFile(fileName)) {
			dumped++;
		}
	}
	if (dumped != count) {
		QMessageBox::warning(this, tr("Error"), tr("Exporting strings from some of the PEs failed!"));
	}
	if (dumped > 0) {
		QMessageBox::information(this, tr("Done!"), tr("Exported strings from: ") + QString::number(dumped)
			+ tr(" PEs into:") + "\n" + dirPath);
	}
}

void MainWindow::_reload(PeHandler* hndl, const bool isDeleted)
{
	if (!hndl) return;

	const QString path = hndl->getFullName();
	this->closePE(hndl);
	if (!isDeleted) {
		this->openPE(path);
	}
}

void MainWindow::reload(PeHandler* hndl)
{
	this->_reload(hndl,false);
}

void MainWindow::closePE(PeHandler* hndl)
{
	if (hndl == NULL) return;

	PEFile *oldPE = hndl->getPe();
	if (this->mainSettings.isAutoSaveTags()) {
		autoSaveTags(hndl);
	}
	this->rightPanel.removePeDockWidget(hndl);
	this->m_PEHandlers.removePe(oldPE);
	//---
	this->sectionsTree.expandAll();
}

void MainWindow::showEvent(QShowEvent * event)
{
	this->readPersistent();
	QMainWindow::showEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	this->mainSettings.writePersistent();
	this->guiSettings.writePersistent();
	this->writePersistent();
	this->autoSaveAllTags();

	/* close all windows */
	diffWindow.close();
	secAddWindow.close();
	userConfigWindow.close();

	unloadAllPEs();
}

void MainWindow::info()
{
	QPixmap p(":/main_ico.ico");
	QString msg = "<b>" + QString::fromLatin1(TITLE) + " - " + tr( "Portable Executable reversing tool") + "</b>";
	msg += "<br/>";
	msg +=tr( "version: ") + currentVer.toString() + "\n";
	msg += "<br/>";
	msg += tr("built on: ") + QString(__DATE__) + "\n";
	msg += "<br/>";
#ifdef COMMIT_HASH
	QString hash = QString(COMMIT_HASH);
	if (hash.length() > 0) {
		msg += tr("commit hash: ") + QString(COMMIT_HASH) + "<br/>";
	}
#endif
	msg += tr("author: Hasherezade") + " (<a href='" + QString(MY_SITE_LINK) + "'>" + tr("homepage") +"</a>)<br/>";
	msg += tr("Source code & more info:") + " <a href='" + QString(SOURCE_LINK) + "'>" + tr("here") +"</a><br/>";
	msg += "<br/>";
	msg += "<i>" + tr("using:") + "</i><br/>";
#if QT_VERSION < 0x050000
	msg += "Qt 4";
#else
	msg += "Qt " + QString::number(QT_VERSION_MAJOR, 10) + "." + QString::number(QT_VERSION_MINOR, 10) + "." + QString::number(QT_VERSION_PATCH, 10);
#endif
	msg += "<br/>";
	msg += tr("bearparser");
	msg += " (<a href='" + QString(BEARPARSER_LICENSE) + "'>" + tr("LICENSE")+ "</a>)<br/>";

#ifdef BUILD_WITH_UDIS86
	msg += "Udis86\n";
#else
	msg += tr("Capstone Engine");
	msg += " (<a href='" + QString(CAPSTONE_LICENSE) + "'>" + tr("LICENSE")+ "</a>)";

	msg += "<br/><br/>";
#endif
	msg += "\n"+ tr( "This software is provided by the copyright holders and contributors \"as is\", without any warranty.");
	msg += "<br/>";

#if QT_VERSION < 0x050000
	msg += "\n" + tr("WARNING: this is a legacy build with Qt4. The builds with Qt5 are recommened for the best user experience.");
#endif

	QMessageBox msgBox(this);
	msgBox.setProperty("hasUrl", true);

	msgBox.setWindowTitle(tr("Info"));
	msgBox.setTextFormat(Qt::RichText);

	msgBox.setText(msg);
	msgBox.setAutoFillBackground(true);
	msgBox.setIconPixmap(p);

	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.exec();
}

void MainWindow::zoomFonts(bool zoomIn)
{
	guiSettings.zoomAllFonts(zoomIn);
}

void MainWindow::changeHexFont()
{
	bool ok = false;
	QFont font = QFontDialog::getFont( &ok, guiSettings.getHexViewFont(), this, tr("Pick a HexView font") );
	if (!ok) {
		return;
	}
	guiSettings.setHexViewFont(font);
}

void MainWindow::changeDisasmFont()
{
	bool ok = false;
	QFont font = QFontDialog::getFont( &ok, guiSettings.getDisasmViewFont(), this, tr("Pick a DisasmView font") );
	if (!ok) {
		return;
	}
	guiSettings.setDisasmViewFont(font);
}

void MainWindow::changeGlobalFont()
{
	bool ok = false;
	QFont font = QFontDialog::getFont( &ok, QApplication::font(), this, tr("Pick a Global font") );
	if (!ok) {
		return;
	}
	guiSettings.setGlobalFont(font);
}

void MainWindow::setDefaultFonts()
{
	this->guiSettings.resetFontsToDefaults();
}

void MainWindow::setDefaultZoom()
{
	this->guiSettings.resetSizesToDefaults();
}

void MainWindow::setSelectedStyle(QAction* pAction)
{
	Q_ASSERT(pAction);
	QString actionName = pAction->text();
	this->guiSettings.setStyleByName(actionName);
	if (actionName == this->guiSettings.currentStyleName()) {
		pAction->setChecked(true);
	}
}

void MainWindow::runNewInstance()
{
	const QString currentPath = QCoreApplication::instance()->applicationFilePath();
	const QStringList arguments;
	QProcess::startDetached(currentPath, arguments);
}

void MainWindow::open()
{
	QString filter = tr("All Files ") + "(*);;" + tr("Applications") +" (*.exe);;" 
		+ tr("Libraries") + " (*.dll);;"+ tr("Drivers") + " (*.sys);;" + tr("Screensavers") + " (*.scr)";
	QFileDialog dialog(NULL, tr("Open"), QDir::homePath(), filter);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	QStringList fNames = dialog.getOpenFileNames(NULL, tr("Open"), this->mainSettings.lastExePath(), filter);

	if (fNames.size() == 0) {
		this->statusBar.showMessage(tr("No file chosen"));
		return;
	}
	openMultiplePEs(fNames);
}

bool MainWindow::openPE(QString fName)
{
	PeHandler* hndl = loadPE(fName, true);
	if (hndl) {
		this->sectionsTree.selectHandler(hndl);
		return true;
	}
	return false;
}

void MainWindow::onSigSearchResult(int foundCount, int reqType)
{
	if (reqType == 0) return;
	if (foundCount > 0) {
		QMessageBox::information(this, tr("Done!"), tr("Found: " )+ QString::number(foundCount));
	} else {
		QMessageBox::information(this, tr("Done!"), tr("Not found!"));
	}
}

bool MainWindow::loadTagsForPe(PeHandler *hndl, QString name)
{
	if (!hndl) return false;
	
	if (name.length() == 0) {
		name = hndl->getFullName() + ".tag";
	}
	if (hndl->comments.loadFromFile(name)) {
		//printf("Tags loaded from file directory\n");
		return true;
	}
	name = this->mainSettings.userDataDir() + QDir::separator() + hndl->getShortName() + ".tag";
	if (hndl->comments.loadFromFile(name)) {
		//printf("Tags loaded from UDD\n");
		return true;
	}
	return true;
}

bool MainWindow::autoSaveTags(PeHandler *hndl)
{
	if (hndl == NULL) return false;

	QString name = hndl->getFullName() + ".tag";
	if (this->mainSettings.userDataDir().size() > 0) {
		name = this->mainSettings.userDataDir() + QDir::separator() + hndl->getShortName() + ".tag";
	}
	if (hndl->comments.saveToFile(name)) {
		//printf("Autosaved file");
	}
	return true;
}

ExeFactory::exe_type MainWindow::recognizeFileType(QString &fName, const bool showAlert)
{
	if (!QFile::exists(fName)) {
		if (showAlert) QMessageBox::warning(this, tr("Open error!"), tr("File does not exist:") + "\n" + fName);
		return ExeFactory::NONE;
	}
	FileView* fileView = NULL;
	QString msg;
	try {
		fileView = new FileView(fName);
	}
	catch (BufferException &e1) {
		msg = e1.getInfo();
		fileView = NULL;
	}
	if (!fileView) {
		if (showAlert) QMessageBox::warning(this, tr("Open error!"),  tr("Failed loading the file:") + "\n" + fName + "\n" + msg);
		return ExeFactory::NONE;
	}

	ExeFactory::exe_type type = ExeFactory::findMatching(fileView);
	if (!this->m_PEHandlers.isSupportedType(type)) {
		QString msg = tr("Not supported filetype!");

		if (type != ExeFactory::NONE) {
			//if the type is recognized but not supported, show it:
			msg = tr("Not supported filetype: ")
				+ ExeFactory::getTypeName(type);
		}
		this->statusBar.showMessage(msg + " [" + fName + "]");
		if (showAlert) QMessageBox::warning(this, tr("Cannot load!"), tr( "Cannot load:") + "\n" + fName + "\n" + msg);
		type = ExeFactory::NONE;
	}
	delete fileView; fileView = NULL;
	return type;
}

PeHandler* MainWindow::loadPE(QString fName, const bool showAlert)
{
	QString link = QFile::symLinkTarget(fName);
	if (link.length() > 0) fName = link;

	if (this->m_PEHandlers.getByName(fName)) {
		this->statusBar.showMessage(tr("File:")  + " ["  + fName + "] " + tr("is already loaded!"));
		if (showAlert) QMessageBox::warning(this, tr(TITLE), tr("This file is already loaded!"), QMessageBox::Ok);
		return NULL;
	}

	if (!QFile::exists(fName)) {
		if (showAlert) QMessageBox::warning(this, tr("Open error!"),  tr("Invalid path or access rights:") + "\n" + fName);
		return NULL;
	}
	const ExeFactory::exe_type type = recognizeFileType(fName, showAlert);
	if (type == ExeFactory::NONE) {
		return NULL;
	}
	PeHandler *hndl = NULL;
	if (this->m_PEHandlers.openExe(fName, type, true)) {
		hndl = this->m_PEHandlers.getByName(fName);
		if (hndl) {
			this->statusBar.showMessage(tr("File: ") + fName);
			this->mainSettings.setLastExePath(fName);
			hndl->setPackerSignFinder(&this->vSign);
			connect(hndl, SIGNAL(foundSignatures(int, int)), this, SLOT(onSigSearchResult(int, int)));
		}
	}
	if (!hndl) {
		QString msg = tr("Error occured during loading the file: ") + fName +"\n" + tr("Type: ") + ExeFactory::getTypeName(type);
		this->statusBar.showMessage(msg + " [" + fName + "]");
		if (showAlert) QMessageBox::warning(this, tr( "Cannot load!"), msg);
		return NULL;
	}
	if (hndl->getPe()->isTruncated()) {
		QString alert = tr("The file:") + " \n" + fName + "\n "+ tr( "is too big and was loaded truncated!");
		QMessageBox::StandardButton res = QMessageBox::warning(this, tr("Too big file!"), alert, QMessageBox::Ok);
	}
	// show warnings:
	QStringList peInfo;
	if (hndl->isPeAtypical(&peInfo)) {
		this->statusBar.showMessage(tr("WARNING: ") + hndl->getFullName() + ": " + peInfo.join(";"));
		if (showAlert) QMessageBox::warning(this, tr("Warning"), hndl->getFullName() + ":\n" + peInfo.join("\n"));
	}
	return hndl; 
}

void MainWindow::openSignatures()
{
	QString filter = tr("Text Files")+ " (*.txt);;" + tr("All Files") + " (*)";
	QString fName= QFileDialog::getOpenFileName(NULL, tr("Open file with signatures"), NULL, filter);
	std::string filename = fName.toStdString();

	if (filename.length() > 0) {
		int i = vSign.loadSignaturesFromFile(filename);
		signWindow.onSigListUpdated();
		QMessageBox msgBox;
		msgBox.setText(tr("Added new signatures: ") + QString::number(i));
		msgBox.exec();
	}//
	this->m_PEHandlers.checkAllSignatures();

}
//----
void MainWindow::savePE(PeHandler* selectedPeHndl)
{
	if (!selectedPeHndl) return;

	PEFile *pe = selectedPeHndl->getPe();
	if (!pe) return;

	const QString filter = tr("All Files") + " (*);;" + tr("Applications") + " (*.exe);;" 
		+ tr("Libraries") + " (*.dll);;" + tr("Drivers") +" (*.sys);;"+ tr("Screensavers") +" (*.scr)";
	QString filename = QFileDialog::getSaveFileName(NULL, tr("Save as..."), selectedPeHndl->getDirPath(), filter);
	if (filename.size() == 0) return;

	bufsize_t dSize = FileBuffer::dump(filename, *pe);
	if (dSize) {
		QMessageBox::information(this, tr("Success"),tr("Dumped PE to: ") + filename);
	} else {
		QMessageBox::warning(this, tr("Failed"), tr("Dumping failed!"));
	}
}

QString MainWindow::chooseDumpOutDir(PeHandler *selectedPeHndl)
{
	const QString EMPTY_STR = "";
	//---
	QFileDialog dialog;
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly, true);

	QString dirPathStr = this->mainSettings.dirDump;
	QString currentPeDir = selectedPeHndl ? selectedPeHndl->getDirPath() : QDir::currentPath();

	if (dirPathStr == "") dirPathStr = currentPeDir;
	dialog.setDirectory(dirPathStr);
	int ret = dialog.exec();

	if ((QDialog::DialogCode) ret != QDialog::Accepted) {
		return EMPTY_STR;
	}
	//---
	QDir dir = dialog.directory();
	QString fName = dir.absolutePath();
	if (fName.length() > 0) {
		dirPathStr = fName;
		if (dirPathStr != currentPeDir) this->mainSettings.dirDump = dirPathStr;
	}
	return dirPathStr;
}

size_t MainWindow::dumpAllPeSections(PEFile *pe, const QString &dirPath, const QString &peName)
{
	if (!pe || dirPath.length() == 0) return 0;
	
	size_t dumped = 0;
	size_t count = pe->getSectionsCount();
	for (size_t i = 0; i < count; ++i) {
		SectionHdrWrapper *sec = pe->getSecHdr(i);
		if (!sec) continue;

		QString secName = sec->mappedName;
		QString fileName = dirPath + QDir::separator() + peName + "[" + secName + "]";
		if (pe->dumpSection(sec, fileName)) dumped++;
	}
	return dumped;
}

void MainWindow::dumpAllSections(PeHandler* selectedPeHndl)
{
	if (!selectedPeHndl) return;

	PEFile *pe = selectedPeHndl->getPe();
	QString dirPath = chooseDumpOutDir(selectedPeHndl);
	
	if (!pe || dirPath.length() == 0) return;
	
	const size_t dumped = dumpAllPeSections(pe, dirPath, selectedPeHndl->getShortName());
	if (dumped > 0) {
		QMessageBox::information(this, tr("Done!"), tr("Dumped: ") + QString::number(dumped)
			+ " " + tr("sections into:") + "\n" + dirPath);
	}
	else {
		QMessageBox::warning(this, tr("Error"), tr("Dumping sections failed!"));
	}
}

void MainWindow::addSection(PeHandler* selectedPeHndl)
{
	if (!selectedPeHndl) return;
	emit addSectionRequested(selectedPeHndl);
	return;
}

void MainWindow::sigSearch(PeHandler* selectedPeHndl)
{
	if (!selectedPeHndl) return;
	
	offset_t offset = selectedPeHndl->getDisplayedOffset();
	SectionHdrWrapper *sec = selectedPeHndl->getPe()->getSecHdrAtOffset(offset, Executable::RAW, false);
	if (sec == NULL) {
		QMessageBox::warning(this, tr("Cannot search!"), tr("No section selected"));
		return;
	}
	offset_t secBgn = sec->getContentOffset(Executable::RAW, true);
	bufsize_t size = sec->getContentSize(Executable::RAW, true);
	if (secBgn == INVALID_ADDR || offset < secBgn) {
		return;
	}
	size = size - (offset - secBgn);
	selectedPeHndl->findPackerInArea(offset, size, sig_ma::FRONT_TO_BACK);
}

void MainWindow::searchPattern(PeHandler* selectedPeHndl)
{
	if (!selectedPeHndl || !selectedPeHndl->getPe()) return;
	
	offset_t maxOffset = selectedPeHndl->getPe()->getContentSize();
	offset_t offset = selectedPeHndl->getDisplayedOffset();
	int ret = patternSearchWindow.exec(offset, maxOffset);
	if ((QDialog::DialogCode) ret != QDialog::Accepted) {
		return;
	}
	QString text = patternSearchWindow.getSignature();
	if (!text.length()) {
		return;
	}
	size_t fullSize = selectedPeHndl->getPe()->getContentSize();
	if (offset >= fullSize) return;

	sig_ma::SigFinder localSignFinder;
	if (!localSignFinder.loadSignature("Searched", text.toStdString())) {
		QMessageBox::information(this, tr("Info"), tr("Could not parse the signature!"), QMessageBox::Ok);
		return;
	}

	while (offset < fullSize){
		std::vector<sig_ma::FoundPacker> signAtOffset;
		size_t areaSize = fullSize - offset;
		selectedPeHndl->findSignatureInArea(offset, areaSize, localSignFinder, signAtOffset, false);
		if (signAtOffset.size()) {
			sig_ma::FoundPacker &pckr = *(signAtOffset.begin());
			selectedPeHndl->setDisplayed(false, pckr.offset, pckr.signaturePtr->length());
			selectedPeHndl->setHilighted(pckr.offset, pckr.signaturePtr->length());
			if (QMessageBox::question(this, tr("Info"), tr("Signature found at:") + " 0x" + QString::number(pckr.offset, 16) + "\n"+ 
					tr("Search next?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			{
				break;
			}
			offset = pckr.offset + 1;
		} else {
			QMessageBox::information(this, tr("Info"), tr("Signature not found!"), QMessageBox::Ok);
			break;
		}
	}
}
