#include "ImportsAddWindow.h"

ImportsAddWindow::ImportsAddWindow(ImportsAutoadderSettings& _settings, QWidget *parent)
	: QDialog(parent, Qt::Dialog), settings(_settings),
	tableModel(NULL)
{
	setWindowFlags(Qt::Dialog);
	setModal(true);

	addSecLabel.setText(tr("Use a new section:"));
	addSecLabel.setBuddy(&addSecCBox);

	propertyLayout5.addWidget(&addSecLabel);
	propertyLayout5.addWidget(&addSecCBox);
	addSecCBox.setChecked(settings.addNewSec);

	propertyLayout0.addWidget(new QLabel("Insert / remove a record:", this));

	dllNameLabel.setText(tr("DLL:"));
	dllNameLabel.setBuddy(&dllNameEdit);

	funcNameLabel.setText(tr("Func:"));
	funcNameLabel.setBuddy(&funcNameEdit);

	propertyLayout1.addWidget(&dllNameLabel);
	propertyLayout2.addWidget(&dllNameEdit);

	propertyLayout1.addWidget(&funcNameLabel);
	propertyLayout2.addWidget(&funcNameEdit);
	
	topLayout.addLayout(&propertyLayout0);
	topLayout.addLayout(&propertyLayout1);
	topLayout.addLayout(&propertyLayout2);

	addButton.setText(tr("Add"));
	addButton.setDefault(true);
	removeButton.setText(tr("Remove"));

	buttonLayout1.addWidget(&addButton);
	buttonLayout1.addWidget(&removeButton);
	topLayout.addLayout(&buttonLayout1);

	ui_elementsView = new QTableView(this);
	tableModel = new ImpAdderSettingsTableModel(ui_elementsView, _settings);
	ui_elementsView->setModel(tableModel);
	ui_elementsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	propertyLayout3.addWidget(new QLabel("List of imports to be added:", this));
	propertyLayout4.addWidget(ui_elementsView);
	topLayout.addLayout(&propertyLayout3);
	topLayout.addLayout(&propertyLayout4);

	topLayout.setMargin(5);
	topLayout.setSpacing(5);
	setLayout(&topLayout);

	okButton.setText(tr("Save"));
	cancelButton.setText(tr("Cancel"));
	buttonLayout2.addWidget(&okButton);
	buttonLayout2.addWidget(&cancelButton);
	topLayout.addLayout(&propertyLayout5);
	topLayout.addLayout(&buttonLayout2);

	setWindowTitle(tr("Add imports"));

	connect(&addButton, SIGNAL(clicked()),this, SLOT(onAddClicked() ));
	connect(&removeButton, SIGNAL(clicked()),this, SLOT(onRemoveClicked()));
	connect(&cancelButton, SIGNAL(clicked()),this, SLOT(reject()));
	connect(&okButton, SIGNAL(clicked()),this, SLOT(onSaveClicked()));
}

void ImportsAddWindow::onSaveClicked()
{
	settings.addNewSec = addSecCBox.isChecked();
	accept();
}

void ImportsAddWindow::onAddClicked()
{
	QString dllName = this->dllNameEdit.text();
	QString funcName = this->funcNameEdit.text();
	
	if (settings.addImport(dllName, funcName)) {
		this->tableModel->reloadSettings();
	}
}

void ImportsAddWindow::onRemoveClicked()
{
	QString dllName = this->dllNameEdit.text();
	QString funcName = this->funcNameEdit.text();
	if (dllName.length() == 0 || funcName.length() == 0) {
		return;
	}
	if (settings.removeImport(dllName, funcName)) {
		this->tableModel->reloadSettings();
	} else {
		QMessageBox::warning(this, "Error", "Entry not found!");
	}
}
