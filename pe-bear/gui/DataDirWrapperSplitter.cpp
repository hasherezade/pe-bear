#include "DataDirWrapperSplitter.h"

//--------------------------------

void DataDirWrapperSplitter::onMoveDirTable()
{
	if (this->dataDirId >= pe::DIR_ENTRIES_COUNT) {
		return;
	}
	Executable::addr_type aType = Executable::RVA;
	if (this->dataDirId == pe::DIR_SECURITY) {
		aType = Executable::RAW;
	}
	size_t size = myPeHndl->getDirSize(this->dataDirId);
	bool ok;
	QString typeStr = (aType == Executable::RVA) ? "RVA" : "Raw";

	QString text = QInputDialog::getText(this, 
		"Move Dir. Table", "Target "+ typeStr + ":\n(required free space: 0x" + QString::number(size, 16).toUpper() + ")", 
		QLineEdit::Normal, "",  &ok);

	if (ok == false) return;

	offset_t target = text.toLongLong(&ok, 16);
	if (ok == false) {
		QMessageBox::warning(NULL, "Input error", "Invalid format!"); 
		return;
	}

	offset_t targetRaw = m_PE->toRaw(target, aType);
	if (targetRaw == INVALID_ADDR) {
		QMessageBox::warning(NULL, "Input error", "Offset out of scope!"); 
		return;
	}
	if (myPeHndl->moveDataDirEntry(this->dataDirId, targetRaw)) {
		QMessageBox::information(NULL, "Done!","Directory Table moved!");
	} else {
		QMessageBox::warning(NULL, "Cannot move!", 
			"Not enough free space to fit the table!\n Required size: 0x" + QString::number(size, 16));
	}
}

QString DataDirWrapperSplitter::chooseDumpOutDir()
{
	const QString EMPTY_STR = "";
	if (!myPeHndl) return EMPTY_STR;
	//---
	QFileDialog dialog;
	dialog.setWindowTitle("Choose target directory");
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly, true);

	QString dirPathStr = myPeHndl->getDirPath();

	dialog.setDirectory(dirPathStr);
	int ret = dialog.exec();

	if((QDialog::DialogCode) ret != QDialog::Accepted) return EMPTY_STR;
	//---
	QDir dir = dialog.directory();
	QString fName = dir.absolutePath();
	if (fName.length() > 0) {
		dirPathStr = fName;
	}
	return dirPathStr;
}

void DataDirWrapperSplitter::setScaledIcons()
{
	if (!moveDirTable) return; //not initialized
	
	const int iconDim = ViewSettings::getSmallIconDim(QApplication::font());
	QIcon moveIco = ViewSettings::makeScaledIcon(":/icons/move.ico", iconDim, iconDim);
	moveDirTable->setIcon(moveIco);

	toolBar.setMaximumHeight(iconDim * 2);
}

bool DataDirWrapperSplitter::initToolbar()
{
	this->moveDirTable = new QAction(QString("&Move the Data Directory table"), this);
	toolBar.setProperty("dataDir", true);
	toolBar.addAction(moveDirTable);
	setScaledIcons();
	connect(moveDirTable, SIGNAL(triggered()), this, SLOT(onMoveDirTable()) );
	return true;
}

//----------------------------------

bool SecurityDirSplitter::initToolbar()
{
	this->saveCertAction = new QAction(QString("&Save certificate"), this);
	connect(saveCertAction, SIGNAL(triggered()), this, SLOT(onSaveCert()) );
	setScaledIcons();
	toolBar.addAction(saveCertAction);
	return true;
}

void SecurityDirSplitter::setScaledIcons()
{
	DataDirWrapperSplitter::setScaledIcons();
	if (!saveCertAction) return;
	
	const int iconDim = ViewSettings::getSmallIconDim(QApplication::font());
	QIcon saveIco = ViewSettings::makeScaledIcon(":/icons/save_black.ico", iconDim, iconDim);
	saveCertAction->setIcon(saveIco);
}

void SecurityDirSplitter::onSaveCert()
{
	if (m_PE == NULL || myPeHndl == NULL) return;

	offset_t offset = myPeHndl->securityDirWrapper.getFieldOffset(SecurityDirWrapper::CERT_CONTENT);
	if (offset == INVALID_ADDR) return;

	bufsize_t certLen = myPeHndl->securityDirWrapper.getFieldSize(SecurityDirWrapper::CERT_CONTENT);
	
	QString filter = "All Files (*)";
	QString filename = QFileDialog::getSaveFileName(this, "Dump certificate content as...", myPeHndl->getDirPath(), filter);
	if (filename.size() == 0)
		return;

	if (m_PE->dumpFragment(offset, certLen, filename)) {
		QMessageBox::information(this,"Success","Dumped to: " + filename);
	} else {
		QMessageBox::warning(this,"Failed", "Dumping failed!");
	}
}
