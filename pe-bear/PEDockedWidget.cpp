#include "PEDockedWidget.h"
#include "gui_base/AddressInputDialog.h"

PEDockedWidget::PEDockedWidget(PeHandler *peHndl, QWidget *parent)
	: PeViewItem(peHndl),  QDockWidget(parent),
	mainSplitter(Qt::Horizontal, this), tagBrowser(peHndl, this),
	cntntSplitter(NULL),
	contentPrev(NULL),
	tabWidget(NULL),
	toolBar(NULL)
{
	if (!myPeHndl || !m_PE) return;

	this->setWidget(&dockWindow);
	dockWindow.setCentralWidget(&mainSplitter);
	
	this->setWindowTitle(myPeHndl->getShortName());
	this->setAutoFillBackground(true);

	cntntSplitter = new QSplitter(Qt::Vertical, &mainSplitter);
	contentPrev = new ContentPreview(peHndl, cntntSplitter);
	tabWidget = new DetailsTab(peHndl, cntntSplitter);
	diagramModel = new SecDiagramModel(this->myPeHndl);
	diagram = new SelectableSecDiagram(diagramModel, true, cntntSplitter);

	tabWidget->setFocusPolicy(Qt::StrongFocus);
	tabWidget->setAutoFillBackground(true);
	setupActionsToolbar(cntntSplitter);

	cntntSplitter->addWidget(contentPrev);
	cntntSplitter->addWidget(tabWidget);

	mainSplitter.addWidget(cntntSplitter);
	mainSplitter.addWidget(diagram);
	setupDiagram();
	
	connect(this, SIGNAL(signalChangeHexViewSettings(HexViewSettings &)),
		contentPrev, SLOT(changeHexViewSettings(HexViewSettings &)) );
		
	connect(this, SIGNAL(signalChangeDisasmViewSettings(DisasmViewSettings &)), 
		&tabWidget->disasmView, SLOT(changeDisasmViewSettings(DisasmViewSettings &)) );
}

void PEDockedWidget::setScaledIcons()
{
	if (!toolBar) return;

	const int iconDim = ViewSettings::getSmallIconDim(QApplication::font());
	toolBar->setIconSize(QSize(iconDim, iconDim));
	toolBar->layout()->setSpacing(iconDim);

	//tool bar icons:
	QIcon previewIco = ViewSettings::makeScaledIcon(":/icons/arrow-right.ico", iconDim, iconDim);
	this->goToEntryPointA->setIcon(previewIco);
	
	QIcon goToRvaIco = ViewSettings::makeScaledIcon(":/icons/go_to_rva.ico", iconDim, iconDim);
	this->goToRvaAction->setIcon(goToRvaIco);
	
	QIcon goToRawIco = ViewSettings::makeScaledIcon(":/icons/go_to_raw.ico", iconDim, iconDim);
	this->goToRawAction->setIcon(goToRawIco);
	
	QIcon backIco = ViewSettings::makeScaledIcon(":/icons/undo.ico", iconDim, iconDim);
	backAction->setIcon(backIco);
	
	QIcon pinIco = ViewSettings::makeScaledIcon(":/icons/red_pin.ico", iconDim, iconDim);
	this->goToModifAction->setIcon(pinIco);
	
	QIcon umodIco = ViewSettings::makeScaledIcon(":/icons/unmodify.ico", iconDim, iconDim);
	this->unModifyAction->setIcon(umodIco);
	
	QIcon tagIco = ViewSettings::makeScaledIcon(":/icons/star.ico", iconDim, iconDim);
	this->tagsAction->setIcon(tagIco);
}

void PEDockedWidget::setupActionsToolbar(QSplitter* owner)
{
	if (!owner) return;
	
	this->toolBar = new QToolBar(&dockWindow);
	dockWindow.addToolBar(Qt::TopToolBarArea, toolBar);;
	
	//const Qt::ShortcutContext context = Qt::WidgetWithChildrenShortcut;
 	this->goToEntryPointA = new QAction(tr("&Preview Entry Point\n[CTRL + E]"), &dockWindow);
	//goToEntryPointA->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
	//goToEntryPointA->setShortcutContext(context);
 	connect(goToEntryPointA, SIGNAL(triggered()), this, SLOT(goToEntryPoint()) );
 
 	this->goToRvaAction = new QAction(tr("&Go to RVA/VA\n[CTRL + R]"), &dockWindow);
	//goToRvaAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
	//goToRvaAction->setShortcutContext(context);
 	connect(goToRvaAction, SIGNAL(triggered()), this, SLOT(goToRVA()) );
 	
 	this->goToRawAction = new QAction(tr("&Go to raw+\n[CTRL + G]"), &dockWindow);
	//goToRawAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_G));
	//goToRawAction->setShortcutContext(context);
 	connect(goToRawAction, SIGNAL(triggered()), this, SLOT(goToOffset()) );
 
	this->backAction = new QAction(tr("&Back to last visited offset\n[CTRL + B]"), &dockWindow);
	//backAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
	//backAction->setShortcutContext(context);
 	connect(backAction, SIGNAL(triggered()), this, SLOT(undoOffset()) );
 
 	this->goToModifAction = new QAction(tr("&Go to last modification"), &dockWindow);
 	connect(goToModifAction, SIGNAL(triggered()), this, SLOT(goToLastModif()) );
 
 	this->unModifyAction = new QAction( tr("&Undo last modifications\n[CTRL + Z]"), &dockWindow);
	//unModifyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Z));
	//unModifyAction->setShortcutContext(context);
 	connect(unModifyAction, SIGNAL(triggered()), this, SLOT(unModify()) );
 
	this->tagsAction = new QAction(tr("&Tag"), &dockWindow);
	//tagsAction->setShortcut(QKeySequence(Qt::CTRL + ';'));
	//tagsAction->setShortcutContext(context);
 	connect(tagsAction, SIGNAL(triggered()), this, SLOT(browseTags()) );

	toolBar->addAction(goToEntryPointA);
	toolBar->addAction(goToRvaAction);
	toolBar->addAction(goToRawAction);
	toolBar->addAction(backAction);
	toolBar->addAction(goToModifAction);
	toolBar->addAction(unModifyAction);
	toolBar->addAction(tagsAction);

	const size_t marginSize = 5;
	toolBar->layout()->setContentsMargins(marginSize, marginSize, marginSize, marginSize);
	toolBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	setScaledIcons();
	updateModifActions();
	updateNavigActions();

	connect(this->myPeHndl, SIGNAL(modified()), this, SLOT(updateModifActions()));
	connect(this->myPeHndl, SIGNAL(pageOffsetModified(offset_t, bufsize_t)), this, SLOT( updateNavigActions() ));
}

void PEDockedWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->matches(QKeySequence::Undo)) {
		return myPeHndl->unModify();
	}
	
	switch (event->key()) {
		case Qt::Key_PageUp:
			myPeHndl->advanceOffset(- PREVIEW_SIZE);
			break;
		case Qt::Key_PageDown:
			myPeHndl->advanceOffset(PREVIEW_SIZE);
			break;
		case Qt::Key_Up:
			myPeHndl->advanceOffset(-1);
			break;
		case Qt::Key_Down:
			myPeHndl->advanceOffset(1);
			break;
		case Qt::Key_G:
			goToOffset();
			break;
		case Qt::Key_E:
			goToEntryPoint();
			break;
		case Qt::Key_R:
			goToRVA();
			break;
		case Qt::Key_B:
			myPeHndl->undoDisplayOffset();
			break;
	}
	QDockWidget::keyPressEvent(event);
}

void PEDockedWidget::updateNavigActions()
{
	bool hasModif = this->myPeHndl->prevOffsets.size() > 0;
	QString num;
	if (hasModif) {
		const offset_t off = this->myPeHndl->prevOffsets.top();
		num = ": " + QString::number(off, 16);
	}
	QString text = tr("&Back to last visited offset") + num + "\n[CTRL + B]";
	backAction->setText(text);
	backAction->setEnabled(hasModif);
}

void PEDockedWidget::updateModifActions()
{
	bool hasModif = this->myPeHndl->modifHndl.countOperations() ? true : false;
	goToModifAction->setEnabled(hasModif);
	unModifyAction->setEnabled(hasModif);
}

void PEDockedWidget::setupDiagram()
{
	this->diagram->setBackgroundColor(Qt::black);
	this->diagram->contourColor = Qt::lightGray;
	this->diagram->setMaximumWidth(this->diagram->minimumSizeHint().width());
	this->diagram->setMinimumWidth(this->diagram->minimumSizeHint().width());
	connect(this->myPeHndl, SIGNAL(pageOffsetModified(offset_t, bufsize_t)), 
		this->diagramModel, SLOT(setSelectedArea(offset_t, bufsize_t)) );
}


void PEDockedWidget::goToLastModif()
{
	offset_t modS = this->myPeHndl->modifHndl.getLastModifiedOffset();
	if (modS == INVALID_ADDR) {
		QMessageBox::warning(0, tr("Cannot go"), tr("No modifications!"));
		return;
	}

	myPeHndl->setDisplayed(false, modS);
}

void PEDockedWidget::goToEntryPoint()
{
	if (!m_PE) return; 

	offset_t raw =  0;
	try {
		raw = m_PE->rvaToRaw(m_PE->getEntryPoint());
	} catch (CustomException e) {
		QMessageBox::warning(0, tr("Warning!"), tr("Wrong RVA supplied!")+ QString::fromStdString(e.what()));
		return;
	}
	myPeHndl->setDisplayed(false, raw);
}

void PEDockedWidget::goToOffset()
{
	goToAddress(true);
}

void PEDockedWidget::goToRVA()
{
	goToAddress(false);
}

void PEDockedWidget::goToAddress(bool isRaw)
{
	if (!m_PE) return; 

	long long number = myPeHndl->displayedOffset;
	if (!isRaw) {
		try {
			number = m_PE->rawToRva(number);
		} catch (CustomException e) {
			number = 0;
		}
	}
	AddressInputDialog dialog(m_PE, isRaw, this->addrColors, this);
	dialog.setDefaultValue(number);

	int dialogRet = dialog.exec();
	bool isOk = false;
	number = dialog.getNumValue(NULL);
	
	if (dialogRet != QDialog::Accepted) {
		return;
	}
	//----
	Executable::addr_type aT = dialog.getAddrType();
	try {
		offset_t raw = m_PE->toRaw(number, aT, true);
		if (raw == INVALID_ADDR) {
			QMessageBox::warning(0, tr("Warning!"), tr("Wrong address supplied!"));
			return;
		}
		myPeHndl->setDisplayed(false, raw);

	} catch (CustomException e) {
		QMessageBox::warning(0, tr("Warning!"), tr("Wrong address supplied: ")+ QString::fromStdString(e.what()));
	}
}

//--------------------------------------------------------------------------------------

SectionMenu::SectionMenu(MainSettings &settings, QWidget *parent) 
	: QMenu(parent), 
	mainSettings(settings),
	peHndl(NULL), selectedSection(NULL), selectedWrapper(NULL)
{
	createActions();
}

void SectionMenu::createActions()
{
	this->dumpSelSecAction = new QAction(tr("&Save the content as..."), this);
	connect(this->dumpSelSecAction, SIGNAL(triggered()), this, SLOT(dumpSelectedSection()) );

	QIcon clearContentIco(":/icons/eraser.ico");
	this->clearSelSecAction = new QAction(clearContentIco, tr("&Clear the content"), this);
	connect(this->clearSelSecAction, SIGNAL(triggered()), this, SLOT(clearSelectedSection()) );

	this->loadSelSecAction = new QAction(tr("Substitute the content"), this);
	connect(this->loadSelSecAction, SIGNAL(triggered()), this, SLOT(loadSelectedSection()) );

	QIcon disasmIco(":/icons/disasm.ico");
	this->dumpDisasmAction = new QAction(disasmIco, tr("&Export section disassembly as..."), this);
	connect(dumpDisasmAction, SIGNAL(triggered()), this, SLOT(exportSectionDisasm()) );

	addAction(this->dumpSelSecAction);
	addAction(this->loadSelSecAction);
	addAction(this->clearSelSecAction);
	addSeparator();
	addAction(this->dumpDisasmAction);
}

void SectionMenu::enableSecActions(bool actionsEnabled)
{
	if (this->clearSelSecAction) this->clearSelSecAction->setVisible(actionsEnabled);
	if (this->dumpDisasmAction) this->dumpDisasmAction->setVisible(actionsEnabled);
	if (this->loadSelSecAction) this->loadSelSecAction->setVisible(actionsEnabled);
}

void SectionMenu::wrapperSelected(PeHandler *pe, ExeElementWrapper* wrapper)
{
	this->peHndl = pe;
	this->selectedSection = nullptr;
	this->selectedWrapper = wrapper;
	bool isEnabled = (pe && wrapper) ? true : false;

	this->setEnabled(isEnabled);
	this->setTitle(tr("No wrapper selected"));
	if (!isEnabled) {
		return;
	}
	this->setTitle(wrapper->getName());
	enableSecActions(false);
}

void SectionMenu::sectionSelected(PeHandler *pe, SectionHdrWrapper *sec)
{
	this->peHndl = pe;
	this->selectedSection = sec;
	this->selectedWrapper = nullptr;
	bool isEnabled = (pe && sec) ? true : false;

	this->setEnabled(isEnabled);
	this->setTitle(tr("No section selected"));
	if (!isEnabled) {
		return;
	}
	this->setTitle(tr("Section: [") + sec->mappedName + "]");
	enableSecActions(true);
}

void SectionMenu::dumpSelectedSection()
{
	if (!peHndl) return;
	PEFile *pe = peHndl->getPe();
	if (!pe) return;
	if (!selectedSection && !selectedWrapper) return;

	QString outDir = mainSettings.dirDump;
	if (outDir == "") outDir = peHndl->getDirPath();
	
	QString mappedName = (selectedSection) ? selectedSection->mappedName : selectedWrapper->getName();

	QString defaultPath = outDir + QDir::separator() + peHndl->getShortName() + "[" + mappedName + "]";
	QString path = QFileDialog::getSaveFileName(this, tr("Save as..."), defaultPath);
	if (path.size() == 0) return;
	if (selectedSection) {
		if (pe->dumpSection(selectedSection, path)) {
			QMessageBox::information(this, tr("Done!"), tr("Dumped section: ") + mappedName +"\n"+ tr("into: ") + path);
			return;
		} else {
			QMessageBox::warning(this, tr("Error"), tr("Dumping section failed!"));
		}
	}
	if (selectedWrapper) {
		size_t wSize = selectedWrapper->getContentSize();
		BufferView wView(selectedWrapper, 0, wSize);

		bufsize_t dumpedSize = FileBuffer::dump(path, wView, false);
		if (dumpedSize) {
			QMessageBox::information(this, tr("Done!"), tr("Dumped wrapper: ") + mappedName +"\n"+ tr("into: ") + path);
		} else {
			QMessageBox::warning(this, tr("Error"), tr("Dumping wrapper failed!"));
		}
	}
}

void SectionMenu::exportSectionDisasm()
{
	if (!peHndl || !selectedSection) return;

	QString outDir = mainSettings.dirDump;
	if (outDir == "") outDir = peHndl->getDirPath();

	QString defaultPath = outDir + QDir::separator() + peHndl->getShortName() + "[" + selectedSection->mappedName + "].txt";
	QString path = QFileDialog::getSaveFileName(this, tr("Save disasembly as..."), defaultPath);
	if (path.size() == 0) return;

	const offset_t startOff = selectedSection->getContentOffset(Executable::RAW, true);
	const size_t previewSize = selectedSection->getContentSize(Executable::RAW, true);
	
	if (!peHndl->exportDisasm(path, startOff, previewSize)) {
		QMessageBox::warning(this, tr("Error"), tr("Dumping section failed!"));
		return;
	}
	QMessageBox::information(this, tr("Done!"), tr("Dumped section disasembly: ")+ selectedSection->mappedName +"\n"+ tr("into: ") + path);
	return;
}

void SectionMenu::clearSelectedSection()
{
	if (!peHndl) return;
	PEFile *pe = peHndl->getPe();
	if (!pe) return;

	if (!selectedSection) return;

	int answer = QMessageBox::warning(0, tr("Clearing section"), tr("Do you really want to clear content of ") + selectedSection->mappedName + "?",
		QMessageBox::Yes | QMessageBox::No);

	if (answer != QMessageBox::Yes) return;

	const offset_t secOffset = selectedSection->getContentOffset(Executable::RAW, true);
	const bufsize_t secSize = selectedSection->getContentSize(Executable::RAW, true);

	peHndl->backupModification(secOffset, secSize);
	bool isOk = pe->clearContent(selectedSection);
	if (isOk) {
		peHndl->setDisplayed(false, secOffset, secSize);
		peHndl->setBlockModified(secOffset, secSize);
	}
	if (!isOk) {
		QMessageBox::warning(this, tr("Failed"), tr("Cannot clear the section content"));
	}
}

void SectionMenu::loadSelectedSection()
{
	if (!peHndl) return;
	PEFile *pe = peHndl->getPe();
	if (!pe) return;

	if (!selectedSection) return;

	QString path = QFileDialog::getOpenFileName(this, tr("Open"), peHndl->getFullName(), "*");
	if (path.length() == 0) return;

	QFile fIn(path);
	if (fIn.open(QFile::ReadOnly) == false) {
		QMessageBox::warning(this, tr("Failed"), tr("Cannot open file"));
		return;
	}

	size_t loaded = peHndl->loadSectionContent(selectedSection, fIn);
	fIn.close();

	QMessageBox::information(this, tr("Loaded"), tr("Loaded from file: 0x") + QString::number(loaded, 16) + " bytes");
}
