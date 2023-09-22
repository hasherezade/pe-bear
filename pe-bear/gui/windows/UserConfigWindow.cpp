#include "UserConfigWindow.h"
#include <bearparser/Util.h>
#include <QtGlobal>

DirEdit::DirEdit(QWidget* parent)
	: QLineEdit(parent)
{
	connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(validateDir(const QString &)) );
}

void DirEdit::validateDir(const QString &input)
{
	QDir dir(input);
	if (dir.exists() == false) {
		this->setStyleSheet("border: 2px solid red;");
	} else {
		this->setStyleSheet("");
	}
}
//-----------------------------------------------------------------------------------

UserConfigWindow::UserConfigWindow(QWidget *parent)
    : QDialog(parent), settings(NULL)
{
	setWindowTitle(tr("Configure..."));
	setMinimumWidth(270);
	setLayout(&topLayout);

	dirButton.setText("...");
	dirButton.setToolTip(tr("Open"));
	connect(&dirButton, SIGNAL(clicked()), this, SLOT(onDirChose()));
	
	QHBoxLayout *fLayout1 = new QHBoxLayout();
	QHBoxLayout *fLayout1A = new QHBoxLayout();
	QHBoxLayout *fLayout2 = new QHBoxLayout();
	QHBoxLayout *fLayout3 = new QHBoxLayout();
	uddDirLabel.setText(tr("User Data Directory: "));
	languageLabel.setText(tr("Language"));

	fLayout1->addWidget(&uddDirLabel);
	fLayout1->addLayout(fLayout1A);
	fLayout1A->addWidget(&uddDirEdit);
	fLayout1A->addWidget(&dirButton);
	fLayout3->addWidget(&languageLabel);
	fLayout3->addWidget(&languageEdit);
	languageEdit.insertItem(0, tr("(default)"));
	
	languageEdit.setToolTip(tr("Changing the language version requires application restart"));

	reloadFileLabel.setText(tr("Reload file on change? "));
	reloadFileStates.addItem(tr("Ignore  "), RELOAD_IGNORE);
	reloadFileStates.addItem(tr("Ask     "), RELOAD_ASK);
	reloadFileStates.addItem(tr("Reload  "), RELOAD_AUTO);
	
	fLayout2->addWidget(&reloadFileLabel);
	fLayout2->addWidget(&reloadFileStates);
	fLayout2->setSizeConstraint(QLayout::SetMaximumSize);
	
	autoSaveTagsCBox.setText(tr("Auto-save tags"));
	QHBoxLayout *buttonLayout = new QHBoxLayout();
	okButton.setText(tr("Save"));
	connect(&okButton, SIGNAL(clicked()), this, SLOT(onOkClicked()));

	cancelButton.setDefault(true);
	cancelButton.setText(tr("Cancel"));
	connect(&cancelButton, SIGNAL(clicked()), this, SLOT(hide()));

	buttonLayout->addWidget(&okButton);
	//buttonLayout->addStretch();
	buttonLayout->addWidget(&cancelButton);
    
	topLayout.addLayout(fLayout1);
	topLayout.addLayout(fLayout3);
	topLayout.addWidget(&autoSaveTagsCBox);
	topLayout.addLayout(fLayout2);

	topLayout.addStretch();
	topLayout.addLayout(buttonLayout);

	uddDirEdit.setAutoFillBackground(true);
	setAutoFillBackground(true);
}

int UserConfigWindow::loadAvailableTranslations()
{
	if (!settings) return 0;
	int cntr = 0;
	QString rootDir = settings->userDataDir() + QDir::separator() + settings->languageDir;
	QDirIterator iter(rootDir, QDir::Dirs | QDir::NoDotAndDotDot);
	while(iter.hasNext()) {
		cntr++;
		QString item = iter.next();
		QFileInfo f(item);
		languageEdit.insertItem(1, f.baseName());
	}
	return cntr;
}

void UserConfigWindow::setMainSettings(MainSettings *settings)
{
	this->settings = settings;
	loadAvailableTranslations();
	if (this->isVisible()) {
		refrehSettingsView();
	}
}

void UserConfigWindow::setReloadMode(const t_reload_mode rMode)
{
	int index = reloadFileStates.findData(rMode, Qt::UserRole);
	if ( index != -1 ) { // -1 for not found
		reloadFileStates.setCurrentIndex(index);
	}
}

t_reload_mode UserConfigWindow::getReloadMode()
{
	t_reload_mode rMode = RELOAD_ASK;
#if QT_VERSION >= 0x050000
	rMode = intToReloadMode(reloadFileStates.currentData(Qt::UserRole).toInt());
	return rMode;
#else
	int index = intToReloadMode(reloadFileStates.currentIndex());
	rMode = intToReloadMode(reloadFileStates.itemData(index,Qt::UserRole).toInt());
#endif
	return rMode;
}

void UserConfigWindow::refrehSettingsView()
{
	if (settings == NULL) return;
	uddDirEdit.setText(settings->userDataDir());
	int index = languageEdit.findText(settings->language);
	if ( index != -1 ) { // -1 for not found
		languageEdit.setCurrentIndex(index);
	} else {
		languageEdit.setCurrentIndex(0); // default
	}
	autoSaveTagsCBox.setChecked(settings->isAutoSaveTags());
	
	setReloadMode(settings->isReloadOnFileChange());
}

void UserConfigWindow::onOkClicked()
{
	QString fName = uddDirEdit.text();
	this->settings->language = languageEdit.currentText();
	this->settings->setUserDataDir(fName);
	this->settings->setAutoSaveTags(autoSaveTagsCBox.isChecked());

	const t_reload_mode rMode = getReloadMode();
	this->settings->setReloadOnFileChange(rMode);
	this->settings->writePersistent();
	this->hide();
}

void UserConfigWindow::onDirChose()
{
	QFileDialog dialog;
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly, true);
	dialog.setDirectory(settings->userDataDir());
	if (dialog.exec()) {
		QDir dir = dialog.directory();
		QString fName = dir.absolutePath();
		if (fName.length() > 0) {
			uddDirEdit.setText(fName);
		}
	}
}
