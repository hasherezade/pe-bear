#include "GeneralPanel.h"
#include <QtGlobal>
#include <bearparser/bearparser.h>

class stringsModel;
enum InfoFieldId {
	INFO_NONE = -1,
	INFO_NAME = 0,
	INFO_IS_TRUNCATED,
	INFO_FILE_SIZE,
	INFO_LOADED_SIZE,
	INFO_UNITS,
	INFO_IMP_HASH,
	INFO_RICHHDR_HASH,
	INFO_CHECKSUM,
	INFO_MD5,
	INFO_SHA1,
	INFO_SHA256,
	INFO_COUNTER
};

InfoTableModel::InfoTableModel(PeHandler *peHndl, QObject *parent)
	: PeTableModel(peHndl, parent)
{
}

QVariant InfoTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	if (orientation == Qt::Horizontal) return tr("File info");
	if (orientation == Qt::Vertical) {
		switch (section) {
			case INFO_NAME: return tr("Path");
			case INFO_IS_TRUNCATED : return tr("Is Truncated?");
			case INFO_UNITS : return tr("File Alignment Units");
			case INFO_LOADED_SIZE: return tr("Loaded size");
			case INFO_FILE_SIZE: return tr("File size");
			case INFO_MD5:  return "MD5";
			case INFO_SHA1: return "SHA1";
#if QT_VERSION >= 0x050000
			case INFO_SHA256: return "SHA256";
#endif
			case INFO_CHECKSUM: return tr("Checksum");
			case INFO_RICHHDR_HASH: return tr("Rich Header Hash");
			case INFO_IMP_HASH: return "ImpHash";
		}
	}
	return QVariant();
}

Qt::ItemFlags InfoTableModel::flags(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}
	const int row = index.row();
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

int InfoTableModel::rowCount(const QModelIndex &parent) const
{
#if QT_VERSION >= 0x050000
	return INFO_COUNTER;
#else
	return (INFO_COUNTER - 1);
#endif
}


QVariant InfoTableModel::data(const QModelIndex &index, int role) const
{
	const QString msg_notAvailable = tr("not available");
	int row = index.row();
	int column = index.column();
	
	if (row == INFO_UNITS || row == INFO_LOADED_SIZE) {
		if (role == Qt::ToolTipRole) return tr("(decimal)") + "\n" + tr("edit to resize the file");
	}
	if (row != INFO_UNITS && row != INFO_LOADED_SIZE) {
		if (role == Qt::BackgroundRole) return QColor(211, 211, 211, 100); // not editable
	}
	
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) return QVariant();

	switch (row) {
		case INFO_NAME: return this->myPeHndl->getFullName();
		case INFO_IS_TRUNCATED : {
			if (role == Qt::ToolTipRole) {
				static QString info = tr("Some files are too large and they must be loaded truncated.");
				return info;
			}
			return m_PE->isTruncated() ? tr("Yes") : tr("No");
		}
		case INFO_LOADED_SIZE: return static_cast<qlonglong>(m_PE->getRawSize());
		case INFO_FILE_SIZE: return static_cast<qlonglong>(m_PE->getFileSize());
		case INFO_UNITS: {
			const size_t fileAlign = m_PE->getFileAlignment();
			if (!fileAlign) return 0;
			return static_cast<qlonglong>(pe_util::roundup(m_PE->getRawSize(), fileAlign) / fileAlign);
		}
		case INFO_MD5: return myPeHndl->getCurrentMd5();
		case INFO_SHA1: return myPeHndl->getCurrentSHA1();
#if QT_VERSION >= 0x050000
		case INFO_SHA256: return myPeHndl->getCurrentSHA256();
#endif
		case INFO_CHECKSUM: return myPeHndl->getCurrentChecksum();
		case INFO_RICHHDR_HASH: {
			if (this->myPeHndl->richHdrWrapper.getPtr()) {
				return myPeHndl->getRichHdrHash();
			} else {
				return msg_notAvailable;
			}
		}
		case INFO_IMP_HASH: {
			if (this->myPeHndl->importDirWrapper.getEntriesCount() > 0) {
				return myPeHndl->getImpHash();
			} else {
				return msg_notAvailable;
			}
		}
	}
	return QVariant();
}

bool InfoTableModel::setData(const QModelIndex &index, const QVariant &data, int role)
{
	const int row = index.row();
	if (row != INFO_UNITS && row != INFO_LOADED_SIZE) return false;

	bufsize_t newSize = 0;
	if (row == INFO_UNITS) {
		bufsize_t newUnits = data.toULongLong();
		const bufsize_t fileAlign = m_PE->getFileAlignment();
		bufsize_t current = fileAlign ? (pe_util::roundup(m_PE->getRawSize(), fileAlign) / fileAlign) : 0;

		if (newUnits == current) return false;
		bufsize_t dif = (newUnits - current) * m_PE->getFileAlignment();
		newSize = m_PE->getRawSize() + dif;
	} else if (row == INFO_LOADED_SIZE) {
		newSize = data.toULongLong();
		if (QString::number(newSize, 10) != data.toString()) {
			return false; // failed value
		}
	}

	static QString alert = tr("Do your really want to resize file?");
	static QPixmap enlarge(":/icons/enlarge.ico");
	static QPixmap shrink(":/icons/shrink.ico");

	QMessageBox msgBox;
	msgBox.setText(alert);
	msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	bufsize_t currentSize = m_PE->getRawSize();

	if ((newSize > BUFSIZE_MAX) || !m_PE->canResize(newSize)) {
		QMessageBox msgBox;
		msgBox.setText(tr("Incorrect new size supplied!"));

		if (newSize < currentSize) {
			msgBox.setInformativeText(tr("Choose the size that will not damage headers!"));
		}
		else {
			msgBox.setInformativeText(tr("Too big!"));
		}
		msgBox.exec();
		return false;
	}
	if (newSize == currentSize) {
		return false;
	}
	QString info;
	bufsize_t dif = 0;
	if (newSize < currentSize) {
		dif = currentSize - newSize;
		msgBox.setIconPixmap(shrink);
		info = tr("bytes are going to be cropped!");
	} else {
		dif = newSize - currentSize;
		msgBox.setIconPixmap(enlarge);
		info = tr("bytes are going to be added!");
	}
	msgBox.setInformativeText( tr("New size") + ": " + QString::number(newSize) + " (0x" + QString::number(newSize, 16) + ")" + "\n"
		+ tr("Current size: ") + QString::number(currentSize) + " (0x" + QString::number(currentSize, 16) + ")" + "\n"
		+ QString::number(dif) + " (0x" + QString::number(dif, 16) + ")" + " " + info);

	int ret = msgBox.exec();
	if (ret != QMessageBox::Ok) {
		QMessageBox::information(NULL, tr("Canceled!"), tr("Resizing canceled!"));
		return false;
	}
	bool isOk = myPeHndl->resize(newSize);
	if (!isOk) {
		QMessageBox msgBox;
		msgBox.setText(tr("Resizing failed!"));
		msgBox.exec();
	}
	return isOk;
}

//--------------------------

GeneralPanel::GeneralPanel(PeHandler *peHndl, QWidget *parent)
	: QSplitter(Qt::Horizontal, parent), PeViewItem(peHndl),
	packersModel(peHndl, this), packersTree(this),
	generalInfoModel(peHndl, this), generalInfo(this),
	packersDock(NULL)
{
	if (!this->myPeHndl) return;
	if (!this->myPeHndl->getPe()) return;
	init();
	connectSignals();
}

void GeneralPanel::init()
{
	this->setOrientation(Qt::Vertical);
	QHeaderView *hdr = generalInfo.horizontalHeader();
	if (hdr) hdr->setStretchLastSection(true);

	generalInfo.setModel(&generalInfoModel);
	this->addWidget(&generalInfo);

	packersTree.setItemsExpandable(false);
	packersTree.setRootIsDecorated(false);
	packersTree.setModel(&this->packersModel);

	packersDock = new QDockWidget(this);
	packersDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
	packersDock->setWidget(&packersTree);
	packersDock->setWindowIcon(QIcon(":/icons/Locked.ico"));
	packersDock->setWindowTitle(tr("Found signatures"));
	this->addWidget(packersDock);
	this->packersDock->setVisible(this->myPeHndl->isPacked());
}


void GeneralPanel::connectSignals()
{
	if (!myPeHndl) return;

	connect(myPeHndl, SIGNAL(modified()), this, SLOT(refreshView()));
	connect(myPeHndl, SIGNAL(foundSignatures(int, int)), this, SLOT(refreshView()));
	connect(myPeHndl, SIGNAL(hashChanged()), &generalInfoModel, SLOT(onNeedReset()));
}

void GeneralPanel::refreshView()
{
	if (!myPeHndl) return;

	this->generalInfo.reset();
	this->packersTree.reset();
	this->packersDock->setVisible(this->myPeHndl->isPacked());
}
