#include "SecHdrsTreeModel.h"

#include <iostream>

enum SecFieldId {
	SEC_NAME = 0,
	SEC_RPTR,
	SEC_RSIZE, 
	SEC_VPTR, 
	SEC_VSIZE, 
	SEC_CHARACT, 
	SEC_RELOC_PTR, 
	SEC_RELOC_NUM,
	SEC_LINENUM_NUM,
	SEC_FIELD_COUNTER
};


SecTreeItem::SecTreeItem(PeHandler* peHndl, int secIndx, int level, SecTreeItem *parent)
    : PeTreeItem(peHndl, secIndx, FIELD_NONE, parent)
{
	this->secIndx = secIndx;
	this->level = level;

	if (peHndl != NULL && secIndx != (-1) && level == 0) {
		//details about section:
		SecTreeItem* item = new SecTreeItem(peHndl, secIndx, 1, this);
		this->appendChild(item);
	}
}

void SecTreeItem::updateSectionsList()
{
	size_t oldNum = this->m_childItems.size();
	if (oldNum == this->m_PE->getSectionsCount()) {
		return;
	}
	int diff = 0;
	bool add = true;

	if (oldNum < this->m_PE->getSectionsCount()) {
		add = true;
		diff = this->m_PE->getSectionsCount() - oldNum;
	} else {
		add = false;
		diff = oldNum - this->m_PE->getSectionsCount();
	}

	for (int i = 0; i < diff; i++) {
		if (add) {
			SecTreeItem* sec = new SecTreeItem(this->myPeHndl, oldNum + i, 0, this);
			this->appendChild(sec);
		} else {
			TreeItem *child = this->child((oldNum - 1)- i);
			this->detachChild(child);
			delete child;
		}
	}
}

SectionHdrWrapper* SecTreeItem::getMySection() const
{
	if (!m_PE) return NULL;
	return m_PE->getSecHdr(secIndx);
}

int SecTreeItem::columnCount() const
{
	return SEC_FIELD_COUNTER;
}

QVariant SecTreeItem::background(int column) const
{
	int fieldIndx = column;
	SectionHdrWrapper *sec = getMySection();
	if (!sec) return QVariant();

	QColor warningCol = Qt::yellow;
	QColor changeCol = Qt::cyan;

	warningCol.setAlpha(200);
	changeCol.setAlpha(50);

	if (fieldIndx == SEC_VSIZE || fieldIndx == SEC_VPTR) {
		bufsize_t mappedVSize = sec->getContentSize(Executable::RVA, true);
		bufsize_t secPtr = sec->getContentOffset(Executable::RVA, false);
		bufsize_t secEnd = sec->getContentEndOffset(Executable::RVA, true);

		if (fieldIndx == SEC_VSIZE || level == 1) {
			if (secEnd > m_PE->getImageSize()) return this->errColor;
		}
		if (fieldIndx == SEC_VSIZE) {
			if (sec->getContentSize(Executable::RVA, false) != mappedVSize) return changeCol;
			if (level == 0) return QVariant(); //Valid!
		}

		if (fieldIndx == SEC_VPTR) {
			if (secPtr > m_PE->getImageSize()) return this->errColor;
		}
		if (level == 0) return QVariant(); //Valid!
	}
	//----
	if (fieldIndx == SEC_RSIZE || fieldIndx == SEC_RPTR) {
		bufsize_t rSize = sec->getContentSize(Executable::RAW, false);
		bufsize_t secPtr = sec->getContentOffset(Executable::RAW, false);
		bufsize_t secEnd = sec->getContentEndOffset(Executable::RAW, false);

		if (fieldIndx == SEC_RSIZE || level == 1) {
			if (secEnd > m_PE->getRawSize()) return this->errColor;
		}
		if (fieldIndx == SEC_RSIZE) {
			if (sec->getContentSize(Executable::RAW, true) != rSize) return changeCol;
			if (level == 0) return QVariant(); //Valid!
		}

		if (fieldIndx == SEC_RPTR) {
			if (secPtr > m_PE->getRawSize()) this->errColor;
		}
		if (level == 0) return QVariant(); //Valid!
	}
	//----
	if (level == 1) {
		QColor flagsColorT = addrColors.flagsColor();
		flagsColorT.setAlpha(addrColors.flagsAlpha());
		return flagsColorT;
	}
	return QVariant();
}

QVariant SecTreeItem::foreground(int column) const
{
	int fieldIndx = column;
	SectionHdrWrapper *sec = getMySection();
	if (!sec) return QVariant();

	size_t roundedSize = sec->getContentSize(Executable::RAW, true);
	if (roundedSize == 0) {
		switch (fieldIndx) {
			case SEC_RPTR : case SEC_RSIZE : case SEC_VPTR : case SEC_VSIZE : 
			{
				return QColor("gray");
			}
		}
	}
	return QVariant();
}

QStringList SecTreeItem::fetchSecHdrCharact(DWORD characteristics)
{
	std::vector<DWORD> secHdrCharact = SectionHdrWrapper::splitCharacteristics(characteristics);
	QStringList nameSet;
	std::vector<DWORD>::iterator iter;
	for (iter = secHdrCharact.begin(); iter != secHdrCharact.end(); ++iter) {
		DWORD currC = *iter;
		if (characteristics & currC) {
			QString info = QString::number(currC, 16).rightJustified(8, '0') + " : " + SectionHdrWrapper::translateCharacteristics(currC);
			nameSet << info;
		}
	}
	return nameSet;
}

QVariant SecTreeItem::toolTip(int column) const
{
	int fieldIndx = column;
	SectionHdrWrapper *sec = getMySection();
	if (!sec) return QVariant();

	if (level == 0) {
		switch (fieldIndx) {

			case SEC_NAME :
			{
				int indx = m_PE->getSecIndex(sec);
				return "index:\t" + QString::number(indx)+"\n"+ sec->mappedName;
			}
			case SEC_CHARACT :
			{
				QStringList names = SecTreeItem::fetchSecHdrCharact(sec->getCharacteristics());
				return names.join("\n");;
			}
			case SEC_RSIZE:
			{
				bufsize_t rounded = sec->getContentSize(Executable::RAW, true);
				if (sec->getContentSize(Executable::RAW, false) != rounded) return "mapped:\n" + QString::number(rounded, 16);
				break;
			}
			case SEC_VSIZE:
			{
				bufsize_t rounded = sec->getContentSize(Executable::RVA, true);
				if (sec->getContentSize(Executable::RVA, false) != rounded) return "mapped:\n" + QString::number(rounded, 16);
				break;
			}
		}
	}
	return QVariant();
}

QVariant SecTreeItem::edit(int column) const
{
	SectionHdrWrapper *sec = getMySection();
	if (!sec) return QVariant();

	return data(column);
}

QVariant SecTreeItem::data(int column) const
{
	int fieldIndx = column;
	SectionHdrWrapper *sec = getMySection();
	if (!sec || !m_PE) return QVariant();

	if (level == 0) {
		bool isOk = false;
		DWORD val = 0;

		switch (fieldIndx) {
			case SEC_NAME :
				return QString(sec->getName());
			case SEC_VSIZE :
				return QString::number(sec->getContentSize(Executable::RVA, false), 16).toUpper();
			case SEC_VPTR :
				val = sec->getNumValue(SectionHdrWrapper::VPTR, &isOk);
				break;
			case SEC_RSIZE :
				return QString::number(sec->getContentSize(Executable::RAW, false), 16).toUpper();
			case SEC_RPTR : 
				val = sec->getNumValue(SectionHdrWrapper::RPTR, &isOk);
				break;
			case SEC_CHARACT : 
				val = sec->getNumValue(SectionHdrWrapper::CHARACT, &isOk);
				break;
			case SEC_RELOC_PTR :
				val = sec->getNumValue(SectionHdrWrapper::RELOC_PTR, &isOk);
				break;
			case SEC_RELOC_NUM :
				val = sec->getNumValue(SectionHdrWrapper::RELOC_NUM, &isOk);
				break;
			case SEC_LINENUM_NUM :
				val = sec->getNumValue(SectionHdrWrapper::LINENUM_NUM, &isOk);
				break;
		}
		return isOk ? QString::number(val, 16).toUpper() : QVariant();
	} 
	if (level == 1) {
		switch (fieldIndx) {
			case SEC_NAME : 
				if (sec->mappedName != sec->getName())
					return sec->mappedName;
				else return ">";
			case SEC_VSIZE :
				if (sec->getContentSize(Executable::RVA, false) != sec->getContentSize(Executable::RVA, true))
					return "mapped: " + QString::number(sec->getContentSize(Executable::RVA, true), 16).toUpper();
				return "^";
				break;
			case SEC_VPTR :
				return QString::number(sec->getVirtualPtr() + sec->getContentSize(Executable::RVA, true), 16).toUpper();
			case SEC_RSIZE :
				if (sec->getContentSize(Executable::RAW, false) != sec->getContentSize(Executable::RAW, true))
					return "mapped: " + QString::number(sec->getContentSize(Executable::RAW, true), 16).toUpper();
				return "^";
				break;
			case SEC_RPTR : 
				return QString::number(sec->getRawPtr() + sec->getContentSize(Executable::RAW, true), 16).toUpper();
			case SEC_CHARACT : 
			{
				const QString rights = SectionHdrWrapper::getSecHdrAccessRightsDesc(sec->getCharacteristics());
				return rights;
			}
		}
	} 
	return QVariant();
}

bool SecTreeItem::setDataValue(int column, const QVariant &value)
{
	SectionHdrWrapper *sec = getMySection();
	if (!sec) return false;

	QString text = value.toString();

	bool isConv = false;
	ulong num = text.toULong(&isConv, 16);
	if (column != SEC_NAME && !isConv) {
		return false;
	}
	switch (column) {
		case SEC_NAME:
			{
				char* name = (char*)sec->getFieldPtr(SectionHdrWrapper::NAME);
				if (!name) return false;
				if (m_PE->setTextValue(name, text.toStdString().c_str(), SectionHdrWrapper::SECNAME_LEN)) {
					sec->reloadName();
					return true;
				}
				break;
			}
		case SEC_VPTR:
			if (sec->setNumValue(SectionHdrWrapper::VPTR, num)) return true;
			break;
		case SEC_RPTR:
			if (sec->setNumValue(SectionHdrWrapper::RPTR, num)) return true;
			break;
		case SEC_VSIZE :
			if (sec->setNumValue(SectionHdrWrapper::VSIZE, num)) return true;
			break;
		case SEC_RSIZE :
			if (sec->setNumValue(SectionHdrWrapper::RSIZE, num)) return true;
			break;
		case SEC_CHARACT:
			if (sec->setNumValue(SectionHdrWrapper::CHARACT, num)) return true;
			break;
		case SEC_RELOC_PTR :
			if (sec->setNumValue(SectionHdrWrapper::RELOC_PTR, num)) return true;
			break;
		case SEC_RELOC_NUM :
			if (sec->setNumValue(SectionHdrWrapper::RELOC_NUM, num)) return true;
			break;
		case SEC_LINENUM_NUM :
			if (sec->setNumValue(SectionHdrWrapper::LINENUM_NUM, num)) return true;
			break;
	}
	return false;
}

Qt::ItemFlags SecTreeItem::flags(int column) const
{
	if (level == 0)
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

offset_t SecTreeItem::getContentOffset() const
{
	SectionHdrWrapper *sec = getMySection();
	if (!sec) return 0;

	const offset_t hdrsOffset = this->m_PE->secHdrsOffset();
	const offset_t hdrsOffsetEnd = this->m_PE->secHdrsEndOffset();
	if (hdrsOffset == INVALID_ADDR || hdrsOffsetEnd == INVALID_ADDR) {
		return 0;
	}

	const bufsize_t SEC_HDRS_SIZE = sizeof(IMAGE_SECTION_HEADER);
	offset_t secOffset = hdrsOffset + (this->secIndx * SEC_HDRS_SIZE);
	return secOffset;
}

bufsize_t SecTreeItem::getContentSize() const
{
	SectionHdrWrapper *sec = getMySection();
	if (!sec) return 0;
	const size_t SEC_HDRS_SIZE = sizeof(IMAGE_SECTION_HEADER);
	return SEC_HDRS_SIZE;
}

//-----------------------------------------------------------------------------
SecHdrsTreeModel::SecHdrsTreeModel(PeHandler* peHndl, QObject *parent)
	: PeTreeModel(peHndl, parent)
{
	if (!peHndl) {
		std::cerr << "PE Handler is not set!\n";
		return;
	}
	if (!m_PE) {
		std::cerr << "PE is not set!\n";
		return;
	}
	rootItem = new SecTreeItem(peHndl);
	size_t secNum = m_PE->getSectionsCount();
	for (int i = 0; i < secNum; i++) {
		rootItem->appendChild(new SecTreeItem(peHndl, i));
	}
	connect(peHndl, SIGNAL(secHeadersModified()), this, SLOT(onSectionNumChanged()));
}

QVariant SecHdrsTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) return QVariant();

	int row = index.row();
	int column = index.column();
	TreeItem* it = static_cast<TreeItem*>(index.internalPointer());
	SecTreeItem* item = dynamic_cast<SecTreeItem*>(it);
	if (!item) {
		return QVariant();
	}
	if (role == Qt::ForegroundRole) return this->addrColor(index);
	//if (role == Qt::ToolTipRole) return toolTip(index);

	switch (role) {
		case Qt::BackgroundColorRole :
			return item->background(index.column());
		case Qt::ForegroundRole:
			return item->foreground(index.column());
		case Qt::SizeHintRole :
		{
			return QVariant(); //get default
		}
		case Qt::DisplayRole :
			return item->data(index.column());
		case Qt::EditRole :
			return item->edit(index.column());
		case Qt::ToolTipRole : // TODO
		{
			QVariant tip = item->toolTip(index.column());
			if (tip.isNull()) tip = toolTip(index);
			if (tip.isNull()) tip = "";
			return tip;
		}
	}
	return QVariant();
}

bool SecHdrsTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	TreeItem* it = static_cast<TreeItem*>(index.internalPointer());
	SecTreeItem* item = dynamic_cast<SecTreeItem*>(it);
	if (!item) {
		return false;
	}
	bool isMod = item->setData(index.column(), value, role);
	return isMod;
}

QVariant SecHdrsTreeModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();

	if (section >= SEC_FIELD_COUNTER) return QVariant();
	switch (section)
	{
		case SEC_NAME: return "Name"; 
		case SEC_VSIZE: return "Virtual Size";
		case SEC_VPTR: return "Virtual Addr.";
		case SEC_RSIZE: return "Raw size";
		case SEC_RPTR: return "Raw Addr.";
		case SEC_CHARACT: return "Characteristics";
		case SEC_RELOC_PTR: return "Ptr to Reloc.";
		case SEC_RELOC_NUM: return "Num. of Reloc.";
		case SEC_LINENUM_NUM: return "Num. of Linenum.";
	}
	return QVariant();
}

Qt::ItemFlags SecHdrsTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	
	SecTreeItem *item = static_cast<SecTreeItem*>(index.internalPointer());
	if (!item)
		return Qt::NoItemFlags;
	
	return item->flags(index.column());
}

Executable::addr_type SecHdrsTreeModel::addrTypeAt(QModelIndex index) const
{
	if (!index.isValid()) return Executable::NOT_ADDR;

	int col = index.column();
	if (col == SEC_RPTR) return Executable::RAW;
	if (col == SEC_VPTR) return Executable::RVA;

	return Executable::NOT_ADDR;
}

offset_t SecHdrsTreeModel::getContentOffset() const
{
	if (!m_PE) return 0;
	return m_PE->secHdrsOffset();
}

bufsize_t SecHdrsTreeModel::getContentSize() const
{
	if (!m_PE) return 0;
	return DWORD(m_PE->secHdrsEndOffset() - m_PE->secHdrsOffset());
}

void SecHdrsTreeModel::onSectionNumChanged()
{
	SecTreeItem* myRootItem = dynamic_cast<SecTreeItem*>(rootItem);
	if (!myRootItem) {
		return;
	}
	beginResetModel();
	myRootItem->updateSectionsList();
	endResetModel();
}
