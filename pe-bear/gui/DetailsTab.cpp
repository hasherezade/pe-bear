#include "DetailsTab.h"
#include <QtGlobal>

#include "windows/ImportsAddWindow.h"

#define ACTION_PROP_RAW "raw"

//---------------------------------------------------------------------------
void DetailsTab::createModels()
{
	if (!m_PE) return;
	dosHdrModel = new DosHdrTableModel(myPeHndl);
	fileHdrModel = new FileHdrTreeModel(myPeHndl);
	richHdrModel = new RichHdrTreeModel(myPeHndl);
	optHdrModel = new OptionalHdrTreeModel(myPeHndl);

	secHdrsModel = new SecHdrsTreeModel(myPeHndl);
	secDiagramModel = new SecDiagramModel(myPeHndl);
	disasmModel = new DisasmModel(this->myPeHndl);

	importsModel = new ImportsTreeModel(this->myPeHndl);
	impFuncModel  = new ImportedFuncModel(myPeHndl);

	exportsModel = new ExportsTreeModel(myPeHndl);
	expFuncModel = new ExportedFuncTreeModel(myPeHndl);
	tlsModel = new TLSTreeModel(myPeHndl);
	tlsCallbacksModel = new TLSCallbacksModel(myPeHndl);
	securityModel = new SecurityTreeModel(myPeHndl);
	
	relocsModel = new RelocsTreeModel(myPeHndl);
	relocEntriesModel = new RelocEntriesModel(myPeHndl);

	ldConfigModel = new LdConfigTreeModel(myPeHndl);
	ldEntryModel = new LdEntryTreeModel(myPeHndl);

	boundImpModel = new BoundImpTreeModel(myPeHndl);
	delayImpModel = new DelayImpTreeModel(myPeHndl);
	delayFuncModel = new DelayImpFuncModel(myPeHndl);

	debugModel = new DebugTreeModel(myPeHndl);
	debugEntryModel = new DebugRDSIEntryTreeModel(myPeHndl);
	
	exceptionModel = new ExceptionTreeModel(myPeHndl);
	resourcesModel = new ResourcesTreeModel(myPeHndl);
	resourcesLeafModel = new ResourceLeafModel(myPeHndl);
	clrModel = new ClrTreeModel(myPeHndl);
}

void DetailsTab::deleteModels()
{
	delete dosHdrModel;
	delete fileHdrModel;
	delete richHdrModel;
	delete optHdrModel;
	delete secHdrsModel;
	delete disasmModel;
	delete secDiagramModel;
}

void DetailsTab::deleteSplitters()
{
	for (int i = 0; i < pe::DIR_ENTRIES_COUNT; i++) {
		if (dirSplitters[i]) dirSplitters[i]->setParent(this); // will be deleted with parent
	}
	richHdrTree.setParent(this);
}

void DetailsTab::createViews()
{
	secVirtualDiagram = new SectionsDiagram(secDiagramModel, false, &secDiagramSplitter);
	secRawDiagram = new SectionsDiagram(secDiagramModel,true,  &secDiagramSplitter);

	dockedRDiagram = new QDockWidget(&this->secDiagramSplitter);
	dockedVDiagram = new QDockWidget(&this->secDiagramSplitter);
}

DetailsTab::DetailsTab(PeHandler *peHndl, QWidget *parent)
	: PeViewItem(peHndl), QTabWidget(parent),
	winAddSec(this),
	generalPanel(peHndl, this), stringsBrowseWindow(peHndl, this),
	dosHdrTree(this), richHdrTree(NULL),
	fileHdrTree(this), optionalHdrTree(this), disasmView(this),
	hdrsSplitter(this), secHdrTreeView(&hdrsSplitter), secDiagramSplitter(&hdrsSplitter),
	fileHdrModel(NULL), optHdrModel(NULL), secHdrsModel(NULL), importsModel(NULL), exportsModel(NULL), expFuncModel(NULL), 
	secDiagramModel(NULL), disasmModel(NULL), resourcesModel(NULL), clrModel(NULL),
	cRichHdrTab(-1)
{
	for (int i = 0; i < pe::DIR_ENTRIES_COUNT; i++)  {
		dirSplitters[i] = NULL;
		dirUpModels[i] = NULL;
		dirDownModels[i] = NULL;
	}
	if (!myPeHndl || !m_PE) return;

	setFocusPolicy(Qt::StrongFocus);
	memset(dirTabIds, (-1), pe::DIR_ENTRIES_COUNT * sizeof(dirTabIds[0]));

	createModels();
	createViews();

	this->setWindowTitle(myPeHndl->getShortName());
	this->disasmView.setModel(disasmModel);

	// dosHdrTree
	this->dosHdrTree.setModel(this->dosHdrModel);
	this->dosHdrTree.resizeColsToContent();
	
	//RichHdr
	this->richHdrTree.setModel(this->richHdrModel);
	this->richHdrTree.resizeColsToContent();

	// FileHeader tree
	this->fileHdrTree.setModel(fileHdrModel);
	this->fileHdrTree.resizeColumnToContents(1);
	this->fileHdrTree.expandAll();

	// Optional Header tree
	this->optionalHdrTree.setModel(optHdrModel);
	this->optionalHdrTree.expandAll();
	this->optionalHdrTree.resizeColumnToContents(1);
	this->optionalHdrTree.resizeColumnToContents(3);
	
	this->hdrsSplitter.setOrientation(Qt::Vertical);
	setupSectionsToolbar(&hdrsSplitter);
	this->hdrsSplitter.addWidget(&this->secHdrTreeView);
	this->hdrsSplitter.addWidget(&this->secDiagramSplitter);
	this->hdrsSplitter.setAutoFillBackground(true);

	this->secHdrTreeView.autoExpand = false;
	this->secHdrTreeView.setModel(secHdrsModel);
	
#if QT_VERSION >= 0x050000
	this->secHdrTreeView.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
	this->secHdrTreeView.header()->setResizeMode(QHeaderView::ResizeToContents);
#endif
	
	// sections graph
	dockedRDiagram->setWindowTitle(tr("Raw"));
	dockedVDiagram->setWindowTitle(tr("Virtual"));

	dockedRDiagram->setWidget(this->secRawDiagram);
	this->dockedRDiagram->setParent(&this->secDiagramSplitter);

	this->dockedVDiagram->setWidget(this->secVirtualDiagram);
	this->dockedVDiagram->setParent(&this->secDiagramSplitter);
	this->secDiagramSplitter.setAutoFillBackground(true);

	// IMPORTS
	dirSplitters[pe::DIR_IMPORT] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_IMPORT, importsModel, impFuncModel, NULL);
	dirSplitters[pe::DIR_IMPORT]->title = tr("Imports");
	setupImportsToolbar();

	// EXPORTS
	dirSplitters[pe::DIR_EXPORT] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_EXPORT, exportsModel, expFuncModel, NULL);
	dirSplitters[pe::DIR_EXPORT]->title = tr("Exports");
	//---
	// TLS
	dirSplitters[pe::DIR_TLS] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_TLS, tlsModel, tlsCallbacksModel, NULL);
	dirSplitters[pe::DIR_TLS]->title = tr("TLS");

	// BASE_RELOCS
	dirSplitters[pe::DIR_BASERELOC] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_BASERELOC, relocsModel, relocEntriesModel, NULL);
	dirSplitters[pe::DIR_BASERELOC]->title = tr("BaseReloc");
	
	//SECURITY
	dirSplitters[pe::DIR_SECURITY] = new SecurityDirSplitter(this->myPeHndl, securityModel, NULL, NULL);
	dirSplitters[pe::DIR_SECURITY]->title = tr("Security");

	// LD_CONFIG
	dirSplitters[pe::DIR_LOAD_CONFIG] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_LOAD_CONFIG, ldConfigModel, ldEntryModel, NULL);
	dirSplitters[pe::DIR_LOAD_CONFIG]->title = tr("LoadConfig");
	
	// BOUND_IMPORTS
	dirSplitters[pe::DIR_BOUND_IMPORT] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_BOUND_IMPORT, boundImpModel, NULL, NULL);
	dirSplitters[pe::DIR_BOUND_IMPORT]->title = tr("BoundImports");

	// DELAY_IMPORTS
	dirSplitters[pe::DIR_DELAY_IMPORT] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_DELAY_IMPORT, delayImpModel, delayFuncModel, NULL);
	dirSplitters[pe::DIR_DELAY_IMPORT]->title = tr("DelayedImps");
	
	// DEBUG
	dirSplitters[pe::DIR_DEBUG] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_DEBUG, debugModel, debugEntryModel, NULL);
	dirSplitters[pe::DIR_DEBUG]->title = tr("Debug");

	// EXCEPTION
	dirSplitters[pe::DIR_EXCEPTION] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_EXCEPTION, exceptionModel, NULL, NULL);
	dirSplitters[pe::DIR_EXCEPTION]->title = tr("Exception");

	// RESOURCES
	dirSplitters[pe::DIR_RESOURCE] = new ResourcesDirSplitter(this->myPeHndl, resourcesModel, resourcesLeafModel, NULL);
	dirSplitters[pe::DIR_RESOURCE]->title = tr("Resources");
	
	//CLR
	dirSplitters[pe::DIR_COM_DESCRIPTOR] = new DataDirWrapperSplitter(this->myPeHndl, pe::DIR_COM_DESCRIPTOR, clrModel, NULL, NULL);
	dirSplitters[pe::DIR_COM_DESCRIPTOR]->title = tr(".NET Hdr");

	cDisasmTab = addTab(&disasmView, tr("Disasm"));
	cGeneralTab = addTab(&generalPanel, tr("General"));
	cStringsTab = addTab(&stringsBrowseWindow, tr("Strings"));
	
	cDOSHdrTab = addTab(&dosHdrTree, tr("DOS Hdr"));
	cRichHdrTab = addTab(&richHdrTree, tr("Rich Hdr"));
	cFileHdrsTab = addTab(&fileHdrTree, tr("File Hdr"));
	cOptHdrsTab = addTab(&optionalHdrTree, tr("Optional Hdr"));
	cSecHdrsTab = addTab(&hdrsSplitter, tr("Section Hdrs"));

	setScaledIcons();
	reloadTabsView();

	connect(this->myPeHndl, SIGNAL(modified()), this, SLOT(reloadTabsView()) );
	connect(this->myPeHndl, SIGNAL(pageOffsetModified(offset_t, bufsize_t)), this, SLOT(setDisasmTabText(offset_t)) );
	connect(this->myPeHndl, SIGNAL(pageOffsetModified(offset_t, bufsize_t)), disasmModel, SLOT(setShownContent(offset_t, bufsize_t)) );

	for (int i = 0; i < pe::DIR_ENTRIES_COUNT; i++) {
		const pe::dir_entry dirNum = pe::dir_entry(i);
		const DataDirWrapperSplitter *splitter = dirSplitters[dirNum];
		if (splitter) {
			connect(this, SIGNAL(globalFontChanged()), splitter, SLOT(onGlobalFontChanged()) );
		}
	}
}

DetailsTab::~DetailsTab()
{
	deleteModels();
	deleteSplitters();
}

void DetailsTab::onAddSection()
{
	this->winAddSec.onAddSectionToPe(myPeHndl);
}

void DetailsTab::onCopyVirtualToRaw()
{
	if (this->myPeHndl->isVirtualEqualRaw()) {
		QMessageBox::information(NULL, tr("No changes required"), tr("Virtual section layout is already the same as raw, no changes required!"));
		return;
	}
	if (!this->myPeHndl->isVirtualFormat()) {
		QString warningText = tr("The PE does not seem to be in the mapped (virtual) format.") + "\n"
			+ tr("Changing the layout may corrupt the file.") + "\n"
			+ tr("Do you really want copy virtual sections layout as raw?");
		QMessageBox::StandardButton reply = QMessageBox::question(NULL, tr("Copy virtual to raw"), warningText,  QMessageBox::Yes|QMessageBox::No);                     
		if (reply != QMessageBox::Yes) return;  
	}
	this->myPeHndl->copyVirtualSizesToRaw();
}

void DetailsTab::onFitSections()
{
	const offset_t fileSize = m_PE->getRawSize();
	const offset_t lastRaw = this->m_PE->getLastMapped(Executable::RAW);

	offset_t imageSize = m_PE->getImageSize();
	offset_t lastRva = this->m_PE->getLastMapped(Executable::RVA);
	const bool fileToResize = fileSize != lastRaw;
	const bool imageToResize = imageSize != lastRva;

	QString info = tr("Last mapped raw = ")+ QString::number(lastRaw, 16) + tr(" when File Size = ") + QString::number(fileSize, 16);
	info += "\n" + tr("Last mapped RVA = ") +  QString::number(lastRva, 16) + tr(" when Image Size = ")+ QString::number(imageSize, 16);

	if (!fileToResize && !imageToResize) {
		QMessageBox::information(NULL, tr("No resizing required!"), info);
		return;
	}

	QString whatToResize = fileToResize ? tr("File"): "";
	if (imageToResize) {
		if (whatToResize.length() > 0) whatToResize += tr(" and ");
		whatToResize += tr("Image");
	}
	QString confirmation = tr("Do you want to resize ") + whatToResize  + tr(" to fit?") + "\n";
	QMessageBox::StandardButton reply = QMessageBox::question(NULL, tr("Do you really want to resize?"), confirmation + '\n' + info,  QMessageBox::Yes|QMessageBox::No);                     
	if (reply != QMessageBox::Yes) return;  

	bool fOk = !fileToResize;
	bool iOk = !imageToResize;

	bool modified = false;
	if (imageToResize) {
		iOk = this->myPeHndl->resizeImage(lastRva);
		modified = true;
	}
	if (fileToResize) {
		fOk = this->myPeHndl->resize(lastRaw, modified);
	}

	if (fOk && iOk) {
		QMessageBox::information(NULL, tr("Done!"), tr("Resizing succeeded!"));
		return;
	}

	if (!fOk) QMessageBox::warning(NULL, tr("Failed!"), tr("Failed resizing the File!"));
	if (!iOk) QMessageBox::warning(NULL, tr("Failed!"), tr("Failed resizing the Image!"));
}

void DetailsTab::onAddImportLib()
{
	if (!myPeHndl->addImportLib()) {
		QMessageBox::warning(NULL, tr("Failed!"), tr("No space to add a new entry!")+"\n"+tr("You must move the table first!"));
		return;
	}
}

void DetailsTab::onAddImportFunc()
{
	if (dirSplitters[pe::DIR_IMPORT] == NULL) return;

	QModelIndexList list = dirSplitters[pe::DIR_IMPORT]->getUpperViewSelected();
	if (list.size() == 0) {
		QMessageBox::warning(NULL, tr("Failed"), tr("No library selected!"));
		return;
	}
	uint32_t selectedLib = list.at(0).row();
	if (!myPeHndl->addImportFunc(selectedLib)) {
		QMessageBox::warning(NULL, tr("Failed!"), tr("No space to add a function entry!"));
		return;
	}
}

void DetailsTab::onAutoAddImports()
{
	ImportsAutoadderSettings settings;
/*
	settings.addImport("placeholder1.dll", "demo");
	settings.addImport("placeholder1.dll", "thiis_is_another_func");
	settings.addImport("placeholder1.dll", "yet_another_placeholder");
	settings.addImport("placeholder2.dll", "we_are_doing_a_demo");
	settings.addImport("placeholder3.dll", "hello_world");
*/
	ImportsAddWindow impCreator(settings, this);
	impCreator.exec();
	if (impCreator.result() != QDialog::Accepted){
		return;
	}
	impCreator.close();
	
	if (settings.dllFunctions.size() == 0) {
		//no content to be added
		return;
	}
	try {
		if (!myPeHndl->autoAddImports(settings)) {
			QMessageBox::critical(this, tr("Error"), tr("Auto adding imports failed!"));
		}
	} catch (CustomException &e) {
		QMessageBox::critical(this, tr("Error"), e.what());
		return;
	}
}

void DetailsTab::onGlobalFontChanged()
{
	QMutexLocker locker(&fontMutex);
	
	setFont(QApplication::font());
	setScaledIcons();
	reloadDirViewIcons();
	emit globalFontChanged(); // forward the signal
}

void DetailsTab::setScaledIcons()
{
	if (!sectionsToolBar) {
		return; //not initialized
	}

	const int iconDim = ViewSettings::getSmallIconDim(QApplication::font());

	QIcon addIco = ViewSettings::makeScaledIcon(":/icons/add_entry.ico", iconDim, iconDim);
	this->addSection->setIcon(addIco);
	this->addImportLib->setIcon(addIco);

	QIcon resizeIco = ViewSettings::makeScaledIcon(":/icons/resize.ico", iconDim, iconDim);
	this->fitSections->setIcon(resizeIco);

	QIcon addSubIco = ViewSettings::makeScaledIcon(":/icons/add_subentry.ico", iconDim, iconDim);
	this->addImportFunc->setIcon(addSubIco);

	QIcon magicAddIco = ViewSettings::makeScaledIcon(":/icons/magic_wand.ico", iconDim, iconDim);
	this->autoAddImports->setIcon(magicAddIco);

	QIcon transformIco = ViewSettings::makeScaledIcon(":/icons/transform.ico", iconDim, iconDim);
	this->copyVirtualToRaw->setIcon(transformIco);

	sectionsToolBar->setFont(QApplication::font());
	sectionsToolBar->setMaximumHeight(iconDim * 2);
	sectionsToolBar->layout()->setSpacing(iconDim);
	
	for (int i = 0; i < pe::DIR_ENTRIES_COUNT; i++) {
		pe::dir_entry dirNum = pe::dir_entry(i);
		if (dirTabIds[dirNum] != (-1)) {
			QToolBar *toolBar = &dirSplitters[dirNum]->toolBar;
			toolBar->setMaximumHeight(iconDim * 2);
			toolBar->layout()->setSpacing(iconDim);
			toolBar->setFont(QApplication::font());
		}
	}
}

void DetailsTab::setupSectionsToolbar(QSplitter *owner)
{
	if (!owner) return;
	
	this->sectionsToolBar = new QToolBar(owner);
	sectionsToolBar->setProperty("dataDir", true);
	owner->addWidget(sectionsToolBar);

	this->addSection = new QAction(QString(tr("&Add a section")), this);
	connect(addSection, SIGNAL(triggered()), this, SLOT(onAddSection()) );

	this->copyVirtualToRaw = new QAction(QString(tr("Copy &virtual offsets/sizes as raw")), this);
	connect(copyVirtualToRaw, SIGNAL(triggered()), this, SLOT(onCopyVirtualToRaw()) );
	
	this->fitSections = new QAction(QString(tr("&Resize to fit sections")), this);
	connect(fitSections, SIGNAL(triggered()), this, SLOT(onFitSections()) );

	sectionsToolBar->addAction(addSection);
	sectionsToolBar->addAction(copyVirtualToRaw);
	sectionsToolBar->addAction(fitSections);
}

void DetailsTab::setupImportsToolbar()
{
	if (dirSplitters[pe::DIR_IMPORT] == NULL) return;

	QToolBar *toolBar = &dirSplitters[pe::DIR_IMPORT]->toolBar;

	this->addImportLib = new QAction(QString(tr("Add a &library")), this);
	connect(addImportLib, SIGNAL(triggered()), this, SLOT(onAddImportLib()) );

	this->addImportFunc = new QAction(QString(tr("Add a &function to the library")), this);
	connect(addImportFunc, SIGNAL(triggered()), this, SLOT(onAddImportFunc()) );
	
	this->autoAddImports = new QAction(QString(tr("Add imports via &wizard")), this);
	connect(autoAddImports, SIGNAL(triggered()), this, SLOT(onAutoAddImports()) );

	toolBar->addAction(addImportLib);
	toolBar->addAction(addImportFunc);
	toolBar->addAction(autoAddImports);
}

void DetailsTab::setDisasmTabText(offset_t raw)
{
	if (!m_PE) {
		setTabText(this->cDisasmTab, tr("Disasm"));
		return;
	}
	int size = PREVIEW_SIZE;
	offset_t endAddr = raw + size;
	//roundup to the file end
	if (m_PE->getRawSize() < size) 
		endAddr = raw + m_PE->getRawSize();
	
	QString secName = "";
	SectionHdrWrapper *sec = m_PE->getSecHdrAtOffset(raw, Executable::RAW);
	SectionHdrWrapper *sec2 = m_PE->getSecHdrAtOffset(endAddr, Executable::RAW);

	//display section names
	if (sec && sec == sec2) {
		secName = ": " + sec->mappedName;
	} else if (sec || sec2) {
		QString sec1Name = sec ? "[" + sec->mappedName + "]" : tr("Headers");
		QString sec2Name = sec2 ? "[" + sec2->mappedName + "]" : "";

		secName = ": " + sec1Name;
		if (sec2Name.size()) 
			secName += " to " + sec2Name;
	}
	setTabText(this->cDisasmTab, tr("Disasm") + secName);
}

void DetailsTab::shirtTabsAfter(pe::dir_entry dirNum, bool toTheLeft)
{
	if (!m_PE || (dirNum >= pe::DIR_ENTRIES_COUNT)) return;
	// shift tabs:
	for (int nextTab = dirNum + 1; nextTab < pe::DIR_ENTRIES_COUNT; nextTab++) {
		if (dirTabIds[nextTab] == (-1)) {
			continue; // the tab not found, nothing to do
		}
		if (toTheLeft) {
			dirTabIds[nextTab]--; // shift the tab index to the left
		}
		else {
			dirTabIds[nextTab]++; // shift the tab index to the right
		}
	}
}

void DetailsTab::manageDirTab(pe::dir_entry dirNum)
{
	if (!myPeHndl || !m_PE) return;

	const bool hasDataDir = this->myPeHndl->hasDirectory(dirNum); //can the Data Dir be properly parsed?
	const bool hasDirTab = (dirTabIds[dirNum] != (-1)) ? true : false;
	if (hasDataDir == hasDirTab) {
		// no changes required
		return;
	}
	// Data Dir no longer exist, remove the tab:
	if (!hasDataDir && hasDirTab) {
		// the tab for non-existing dir was found:
		removeTab(dirTabIds[dirNum]);
		dirTabIds[dirNum] = (-1);
		// shift tabs:
		shirtTabsAfter(dirNum, true);
		return;
	}
	// Data Dir was created, add a tab for it:
	if (hasDataDir && !hasDirTab) {
		//find index
		int lastBefore = this->cSecHdrsTab;
		for (int i = dirNum - 1; i >= 0; i--) {
			if (dirTabIds[i] != (-1)) {
				lastBefore = dirTabIds[i];
				break;
			}
		}
		int tabIndex = lastBefore + 1;

		if (!dirSplitters[dirNum]) return; // should never happen
		//attach tab
		dirTabIds[dirNum] = this->insertTab(tabIndex, dirSplitters[dirNum], dirSplitters[dirNum]->title);

		const int iconDim = ViewSettings::getSmallIconDim(QApplication::font());
		QIcon dataDirIco = ViewSettings::makeScaledIcon(":/icons/data_dir_gray.ico", iconDim, iconDim);

		this->setTabIcon(dirTabIds[dirNum], dataDirIco);
		this->setTabToolTip(dirTabIds[dirNum], tr("Data Directory[") + QString::number(dirNum) + "]: " + dirSplitters[dirNum]->title);
		// shift tabs:
		shirtTabsAfter(dirNum, false);
	}
}

void DetailsTab::manageRichHdrTab()
{
	if (!myPeHndl || !m_PE) return;
	
	//add/remove Rich Header tab
	if (m_PE->getRichHeaderSign()) {
		if (cRichHdrTab == (-1)) {
			int lastBefore = this->cDOSHdrTab;
			int tabIndex = lastBefore + 1;
			cRichHdrTab = this->insertTab(tabIndex, &richHdrTree, tr("Rich Hdr"));
		}
	}
	else {
		if (cRichHdrTab != (-1)) {
			removeTab(cRichHdrTab);
			cRichHdrTab = (-1);
		}
	}
}

void DetailsTab::reloadTabsView()
{
	if (!myPeHndl || !m_PE) return;

	manageRichHdrTab();
	
	for (int i = 0; i < pe::DIR_ENTRIES_COUNT; i++) {
		manageDirTab(pe::dir_entry(i));
	}
}

void DetailsTab::reloadDirViewIcons()
{
	const int iconDim = ViewSettings::getSmallIconDim(QApplication::font());
	QIcon dataDirIco = ViewSettings::makeScaledIcon(":/icons/data_dir_gray.ico", iconDim, iconDim);
	
	setIconSize(QSize(iconDim, iconDim));
	
	for (int i = 0; i < pe::DIR_ENTRIES_COUNT; i++) {
		pe::dir_entry dirNum = pe::dir_entry(i);
		if (dirTabIds[dirNum] != (-1)) {
			this->setTabIcon(dirTabIds[dirNum], dataDirIco);
		}
	}
}

//----------------------------------

