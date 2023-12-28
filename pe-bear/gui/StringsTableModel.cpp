#include "StringsTableModel.h"

StringsTableModel::StringsTableModel(PeHandler *peHndl, QObject *parent)
	: QAbstractTableModel(parent), m_PE(peHndl), stringsMap(nullptr)
{
}

QVariant StringsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	if (orientation == Qt::Horizontal) {
		switch (section) {
			case COL_OFFSET: return tr("Offset");
			case COL_TYPE: return tr("Type");
			case COL_STRING : return tr("String");
		}
	}
	return QVariant();
}

Qt::ItemFlags StringsTableModel::flags(const QModelIndex &index) const
{	
	if (!index.isValid()) return Qt::NoItemFlags;
	const Qt::ItemFlags fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	return fl;
}

QVariant StringsTableModel::data(const QModelIndex &index, int role) const
{
	if (!this->stringsMap) {
		return QVariant();
	}
	int column = index.column();
	int row = index.row();
	if ((size_t)row >= stringsOffsets.size()) return QVariant();

	if (column == COL_OFFSET) {
		if (role == Qt::UserRole) return qint64(stringsOffsets[row]);
		if (role == Qt::ToolTipRole) return "Right click to follow [" + util::translateAddrTypeName(Executable::RAW) + "]";
	}
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) {
		return QVariant();
	}

	offset_t strOffset = stringsOffsets[row];
	switch (column) {
		case COL_OFFSET:
			return QString::number(strOffset, 16);
		case COL_TYPE:
			return stringsMap->isWide(strOffset) ? "W" : "A";
		case COL_STRING : 
			return stringsMap->getString(strOffset);
	}
	return QVariant();
}

//----

void StringsBrowseWindow::onFilterChanged(QString str)
{

	if (!this->stringsProxyModel) return;
	QRegExp regExp(str.toLower(), Qt::CaseSensitive, QRegExp::FixedString);
	stringsProxyModel->setFilterRegExp(regExp);
}

void StringsBrowseWindow::offsetClicked(offset_t offset, Executable::addr_type type)
{
	if (!this->myPeHndl) {
		return;
	}
	size_t strSize = this->myPeHndl->stringsMap.getStringSize(offset);
	myPeHndl->setDisplayed(false, offset, strSize);
	myPeHndl->setHilighted(offset, strSize);
}

void StringsBrowseWindow::onSave()
{
	QString defaultFileName = this->myPeHndl->getFullName() + ".strings.txt";
	QString filter = tr("Text Files") + "(*.txt);;" + tr("All Files") + "(*)";
	QString fName = QFileDialog::getSaveFileName(this, tr("Save strings as..."), defaultFileName, filter);
	
	if (fName.length() > 0) {
		if (this->myPeHndl->stringsMap.saveToFile(fName)) {
			QMessageBox::information(this, tr("Strings save"), tr("Saved strings to: ") + fName, QMessageBox::Ok);
		}
	}
}

void StringsBrowseWindow::initLayout()
{
	QWidget *widget = new QWidget(this);
	widget->setLayout(&topLayout);
	setCentralWidget(widget);
	saveButton.setText(tr("Save"));

	propertyLayout0.addWidget(&saveButton);
	filterLabel.setText(tr("Search string"));
	propertyLayout0.addWidget(&filterLabel);
	propertyLayout0.addWidget(&filterEdit);
	topLayout.addLayout(&propertyLayout0);
	topLayout.addWidget(&stringsTable);

	connect(&saveButton, SIGNAL(clicked()), this, SLOT(onSave()) );
	connect(&filterEdit, SIGNAL(textChanged(QString)), this, SLOT(onFilterChanged(QString)) );
}
