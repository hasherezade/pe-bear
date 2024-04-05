#include "ImportsAddWindow.h"

ImportsAddWindow::ImportsAddWindow(ImportsAutoadderSettings& _settings, QWidget *parent)
	: QDialog(parent, Qt::Dialog), settings(_settings),
	funcNameValidator(NULL),
	tableModel(NULL), ui_elementsView(NULL)
{
	setWindowFlags(Qt::Dialog);
	setModal(true);

	addSecLabel.setText(tr("Use a new section:"));
	addSecLabel.setBuddy(&addSecCBox);

	propertyLayout5.addWidget(&addSecLabel);
	propertyLayout5.addWidget(&addSecCBox);
	addSecCBox.setChecked(settings.addNewSec);

	separateOFTLabel.setText(tr("Separate Original First Thunk:"));
	separateOFTLabel.setBuddy(&separateOFTBox);

	propertyLayout6.addWidget(&separateOFTLabel);
	propertyLayout6.addWidget(&separateOFTBox);
	separateOFTBox.setChecked(settings.separateOFT);

	propertyLayout0.addWidget(new QLabel("Insert / remove a record:", this));

	dllNameLabel.setText(tr("DLL:"));
	dllNameLabel.setBuddy(&dllNameEdit);

	funcNameLabel.setText(tr("Function:"));
	funcNameLabel.setBuddy(&funcNameEdit);

	propertyLayout1.addWidget(&dllNameLabel);
	propertyLayout2.addWidget(&dllNameEdit);

	propertyLayout1.addWidget(&funcNameLabel);
	propertyLayout2.addWidget(&funcNameEdit);

	funcNameValidator = new QRegularExpressionValidator(QRegularExpression("[0-9A-Za-z._#@?-]{1,}"));
	funcNameEdit.setValidator(funcNameValidator);
	funcNameEdit.setToolTip(tr("A function name, or ordinal prefixed by '#', i.e. #123"));
	
	topLayout.addLayout(&propertyLayout0);
	topLayout.addLayout(&propertyLayout1);
	topLayout.addLayout(&propertyLayout2);

	addButton.setText(tr("Add"));
	removeButton.setText(tr("Remove"));

	buttonLayout1.addWidget(&addButton);
	buttonLayout1.addWidget(&removeButton);
	topLayout.addLayout(&buttonLayout1);

	ui_elementsView = new QTableView(this);
	tableModel = new ImpAdderSettingsTableModel(ui_elementsView, _settings);
	
	QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
	proxyModel->setSourceModel( tableModel );
	ui_elementsView->setModel( proxyModel ); 
	ui_elementsView->setSortingEnabled(true);
#if QT_VERSION >= 0x050000
	ui_elementsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#endif
	ui_elementsView->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui_elementsView->setSelectionMode(QAbstractItemView::SingleSelection);

	propertyLayout3.addWidget(new QLabel(tr("List of imports to be added:"), this));
	propertyLayout4.addWidget(ui_elementsView);
	topLayout.addLayout(&propertyLayout3);
	topLayout.addLayout(&propertyLayout4);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	topLayout.setMargin(5);
#endif
	topLayout.setSpacing(5);
	setLayout(&topLayout);

	okButton.setText(tr("Save"));
	okButton.setDefault(true);
	cancelButton.setText(tr("Cancel"));

	buttonLayout2.addWidget(&okButton);
	buttonLayout2.addWidget(&cancelButton);
	topLayout.addLayout(&propertyLayout6);
	topLayout.addLayout(&propertyLayout5);
	topLayout.addLayout(&buttonLayout2);

	setWindowTitle(tr("Add imports"));

	connect(&addButton, SIGNAL(clicked()),this, SLOT(onAddClicked() ));
	addButton.setShortcut(QKeySequence(Qt::Key_Insert));
	
	connect(&removeButton, SIGNAL(clicked()),this, SLOT(onRemoveClicked()));
	removeButton.setShortcut(QKeySequence(Qt::Key_Delete));
	
	connect(&cancelButton, SIGNAL(clicked()),this, SLOT(reject()));
	connect(&okButton, SIGNAL(clicked()),this, SLOT(onSaveClicked()));
	okButton.setShortcut(QKeySequence(Qt::Key_Enter));
	
	connect(ui_elementsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), 
		this, SLOT(onTableSelectionChanged(const QItemSelection &)));
}

void ImportsAddWindow::onTableSelectionChanged(const QItemSelection &selected)
{
	QModelIndexList indexes = selected.indexes();
	if (indexes.size() < 1) return;
	
	const QModelIndex &index = indexes.at(0);
	if (!index.isValid() || !tableModel) return;
	
	int elNum = index.data(Qt::UserRole).toInt();
	if (elNum == (-1)) return;
	
	QPair<QString,QString> dllAndFunc = tableModel->getPairAt(elNum);
	this->dllNameEdit.setText(dllAndFunc.first);
	this->funcNameEdit.setText(dllAndFunc.second);
}

void ImportsAddWindow::onSaveClicked()
{
	settings.addNewSec = addSecCBox.isChecked();
	settings.separateOFT = separateOFTBox.isChecked();
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
