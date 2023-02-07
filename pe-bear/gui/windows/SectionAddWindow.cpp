#include "SectionAddWindow.h"

#define EMPTY_FILE_TXT "Load from file"


SectionAddWindow::SectionAddWindow(QWidget *parent)
	: QDialog(0, Qt::Dialog),
	currPeHndl(NULL)
{
	//setWindowFlags(Qt::Dialog);
	setModal(true);
	
	secVsizeEdit.setMaximum(0);
	secRsizeEdit.setMaximum(0);

	this->fileButton.setText("Choose a file");
	connect(&fileButton, SIGNAL(clicked()), this, SLOT(onFileChose()));
	this->fileCBox.setText(EMPTY_FILE_TXT);
	this->fileCBox.setChecked(false);

	QHBoxLayout *fLayout = new QHBoxLayout();
	fLayout->addWidget(&fileCBox);
	fLayout->addWidget(&fileButton);
	topLayout.addLayout(fLayout);

	secNameLabel.setText(tr("Section name:"));
	secNameLabel.setBuddy(&secNameEdit);
	secPropertyLayout3.addWidget(&secNameLabel);
	secPropertyLayout3.addWidget(&secNameEdit);
	topLayout.addLayout(&secPropertyLayout3);

	secRsizeLabel.setText(tr("Raw size:"));
	secRsizeLabel.setBuddy(&secRsizeEdit);
	secPropertyLayout.addWidget(&secRsizeLabel);
	secPropertyLayout.addWidget(&secRsizeEdit);
	topLayout.addLayout(&secPropertyLayout);

	secVsizeLabel.setText(tr("Virtual size:"));
	secVsizeLabel.setBuddy(&secVsizeEdit);
	secPropertyLayout2.addWidget(&secVsizeLabel);
	secPropertyLayout2.addWidget(&secVsizeEdit);
	topLayout.addLayout(&secPropertyLayout2);
	
	okButton.setText(tr("OK"));
	okButton.setDefault(true);
	cancelButton.setText(tr("Cancel"));

	//READ = 0, WRITE, EXEC
	rightsCBox[READ].setText("read");
	rightsCBox[WRITE].setText("write");
	rightsCBox[EXEC].setText("execute");

	QHBoxLayout *rightsLayout = new QHBoxLayout();
	topLayout.addLayout(rightsLayout);
	for (int i = 0; i < ACCESS_NUM ; i++) {
		rightsLayout->addWidget(&rightsCBox[i]);
	}
	buttonLayout.addWidget(&okButton);
	buttonLayout.addWidget(&cancelButton);
	topLayout.addLayout(&buttonLayout);

	topLayout.addStretch();
	setLayout(&topLayout);
	setWindowTitle(tr("Add a new section"));

	connect(&okButton, SIGNAL(clicked()),this, SLOT(onOkClicked() ));
	connect(&cancelButton, SIGNAL(clicked()),this, SLOT(close()));
}

void SectionAddWindow::onAddSectionToPe(PeHandler *peHndl)
{
	if (!peHndl) return;
	PEFile *pe = peHndl->getPe();
	if (!pe) return;

	currPeHndl = peHndl;
	int max = INT_MAX;
	secRsizeEdit.setMaximum(max);
	secVsizeEdit.setMaximum(max);
	this->show();
}

void SectionAddWindow::onFileChose()
{
	QString path = QFileDialog::getOpenFileName(NULL, "Open section file", NULL, NULL);
	if (path.length() == 0) {
		this->fileCBox.setText(EMPTY_FILE_TXT);
		return;
	}

	QFile file(path);
	if (file.open(QFile::ReadOnly) == false) {
		QMessageBox::warning(this, "Error", "Cannot read this file");
		this->fileCBox.setText(EMPTY_FILE_TXT);
		return;
	}
	long fSize = FileBuffer::getReadableSize(file);
	file.close();

	this->filePath = path;
	if (fSize < 0) fSize = 0; //invalid size!

	this->fileCBox.setText(this->filePath);
	this->fileCBox.setChecked(true);

	this->secRsizeEdit.setValue(fSize);
	this->secVsizeEdit.setValue(fSize);
	
	return;
}

void SectionAddWindow::onOkClicked()
{
	uint32_t rVal = this->secRsizeEdit.value();
	uint32_t vVal = this->secVsizeEdit.value();
	QString name = this->secNameEdit.text();
	if (!currPeHndl) return;

	DWORD accessRights = 0;
	if (rightsCBox[READ].checkState() == Qt::Checked) accessRights |= SCN_MEM_READ;
	if (rightsCBox[WRITE].checkState() == Qt::Checked) accessRights |= SCN_MEM_WRITE;
	if (rightsCBox[EXEC].checkState() == Qt::Checked) accessRights |= SCN_MEM_EXECUTE;

	try {
		SectionHdrWrapper* sec = currPeHndl->addSection(name, rVal, vVal);
		if (!sec) throw CustomException("Cannot add a new section");

		sec->setNumValue(SectionHdrWrapper::CHARACT, accessRights);

		if (this->filePath.length() && this->fileCBox.isChecked()) {

			QFile file(this->filePath);
			if (file.open(QFile::ReadOnly) == false) {
				QMessageBox::warning(this, "Error", "Cannot read this file");
				this->fileCBox.setText(EMPTY_FILE_TXT);
				return;
			}
			currPeHndl->loadSectionContent(sec, file, true);
			file.close();
			sec = NULL;// after reloading PEFile, pointer is no longer valid
		}
	} catch (CustomException e) {
		QMessageBox::critical(this, "Error", e.what());
		return;
	}

	QMessageBox::information(this, "Success!", "Section "+ name +" added!");
	this->hide();
}