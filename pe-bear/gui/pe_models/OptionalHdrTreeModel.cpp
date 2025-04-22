#include "OptionalHdrTreeModel.h"


DataDirTreeItem::DataDirTreeItem(PeHandler *peHndl, level_t level, int recordNum, OptionalHdrTreeItem *parent)
    : OptionalHdrTreeItem(peHndl, level, OptHdrWrapper::DATA_DIR, parent)
{
	init();
	this->recordNum = recordNum;
	this->sID = recordNum;
}

void DataDirTreeItem::init()
{
	DataDirWrapper *dDir = wrapper();
	const int recordsCount = (dDir) ? dDir->getDirsCount() : 0;

	//int recordsCount = pe::DIR_ENTRIES_COUNT;
	if (recordsCount && this->level == DESC) {
		//create subitems:
		for (size_t childIndx = 0; childIndx < recordsCount; childIndx++) 
			this->appendChild(new DataDirTreeItem(myPeHndl, DETAILS, childIndx, this));

		return;
	}
}

QVariant DataDirTreeItem::background(int column) const
{
	DataDirWrapper *dDir = wrapper();
	if(!dDir) return QVariant();
	const int recordsCount = (dDir) ? dDir->getDirsCount() : 0;

	int fId = this->getFID(column);
	if (fId >= recordsCount) return QVariant();

	if (level == DESC && column > COL_NAME && column <= COL_VALUE2) {
		QColor dataDirNameCol = addrColors.dataDirNameColor();
		dataDirNameCol.setAlpha(addrColors.dataDirNameAlpha());
		return dataDirNameCol;
	}
	if (level != DETAILS) return QVariant();
	if (column == COL_NAME) {
		QColor dataDirNameCol = addrColors.dataDirNameColor();
		dataDirNameCol.setAlpha(addrColors.dataDirNameAlpha());
		return dataDirNameCol;
	}
	if (column == COL_VALUE2) {
		int sId =  this->getSID(column);

		bool isOk = false;
		offset_t addr = dDir->getNumValue(fId, DataDirWrapper::ADDRESS, &isOk);
		if (!isOk) return errColor;

		uint64_t size = dDir->getNumValue(fId, DataDirWrapper::SIZE, &isOk);
		if (!isOk) return errColor;

		Executable::addr_type aType = dDir->containsAddrType(fId, DataDirWrapper::ADDRESS);
		if (addr == INVALID_ADDR || (size && !m_PE->getContentAt(addr, aType, size, false))) {
			return errColor;
		}
	}
	if (column > COL_NAME && column <= COL_VALUE2) {
		//make color
		QColor ddirCol = addrColors.dataDirColor();
		
		if (column % 2) 
			ddirCol.setAlpha(200);
		else 
			ddirCol.setAlpha(100);

		return ddirCol;
	}
	return QVariant();
}

QVariant DataDirTreeItem::edit(int column) const
{
	if (this->level == DESC) return QVariant();
	return data(column);
}

Qt::ItemFlags DataDirTreeItem::flags(int column) const
{
	DataDirWrapper *dDir = wrapper();
	if(!dDir) return Qt::NoItemFlags;
	const int recordsCount = (dDir) ? dDir->getDirsCount() : 0;

	int fId = this->getFID(column);
	if (fId >= recordsCount) return Qt::NoItemFlags;
	
	static Qt::ItemFlags fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if (column == 0 || column > COL_VALUE2) return fl;

	if (level == DETAILS && column > COL_NAME && column <= COL_VALUE2)
		return fl | Qt::ItemIsEditable;
	return fl;
}

bool DataDirTreeItem::containsRVA(QModelIndex index) const
{
	if (index.column() != COL_VALUE) return false;
	if (this->recordNum == pe::DIR_SECURITY) return false;
	return true;
}

offset_t DataDirTreeItem::getContentOffset() const
{
	DataDirWrapper *dDir = wrapper();
	if(!dDir) return INVALID_ADDR;

	int fId = this->getFID();
	return dDir->getFieldOffset(fId);
}

bufsize_t DataDirTreeItem::getContentSize() const
{
	DataDirWrapper *dDir = wrapper();
	if(!dDir) return 0;

	int fId = this->getFID();
	return dDir->getFieldSize(fId, FIELD_NONE);
}

QVariant DataDirTreeItem::data(int column) const
{
	DataDirWrapper *dDir = wrapper();
	if(!dDir) return QVariant();

	int fId = this->getFID(column);
	const int recordsCount = (dDir) ? dDir->getDirsCount() : 0;
	if (fId >= recordsCount) return QVariant();

	if (this->level == DESC) {
		switch (column) {
			case COL_NAME : return dDir->getName();
		    case COL_VALUE : return (tr("Address"));
		    case COL_VALUE2 : return (tr("Size"));
		}
		return QVariant();
	}

	if (column == COL_OFFSET) {
		return QString::number(getContentOffset(), 16).toUpper();
	}

	if (column == COL_NAME) {
		return dDir->getFieldName(fId);
	}

	if (column == COL_VALUE || column == COL_VALUE2) {
		int sId =  this->getSID(column);

		bool isOk = false;
		uint64_t val = dDir->getNumValue(fId, sId, &isOk); //DataDirWrapper::ADDRESS , DataDirWrapper::SIZE
		if (!isOk) return "UNK";

		return QString::number((uint32_t) val, 16).toUpper();
	}
	return QVariant();
}

bool DataDirTreeItem::setDataValue(int column, const QVariant &value)
{
	bool isConv = false;
	DWORD number = value.toString().toInt(&isConv, 16);
	if (!isConv) return isConv;

	DataDirWrapper *dDir = wrapper();
	if(!dDir) return false;
	int fId = this->getFID(column);

	if (column == COL_VALUE || column == COL_VALUE2) {
		int sId =  this->getSID(column);
		return dDir->setNumValue(fId, sId, number);
	}
	return false;
}

QVariant DataDirTreeItem::toolTip(int column) const
{
	if (m_PE == NULL) return QVariant();

	if (this->level == DESC) return QVariant();
	
	DataDirWrapper *dDir = wrapper();
	const size_t fId = this->getFID(column);
	const size_t recordsCount = (dDir) ? dDir->getDirsCount() : 0;
	if (fId >= recordsCount) return QVariant();
	
	if (column == COL_OFFSET) return tr("Right click to follow");
	IMAGE_DATA_DIRECTORY *dataDir = this->m_PE->getDataDirectory();

	DWORD va = dataDir[recordNum].VirtualAddress;
	DWORD size = dataDir[recordNum].Size;
	if (va == 0 && size == 0) return tr("<empty>");

	SectionHdrWrapper *sec = m_PE->getSecHdrAtOffset(va, Executable::RVA);
	if (sec == NULL)
		return "";
	return sec->mappedName;
}
//---

OptHdrDllCharactTreeItem::OptHdrDllCharactTreeItem(PeHandler *peHndl, level_t level, DWORD characteristics, OptionalHdrTreeItem *parent)
	: OptionalHdrTreeItem(peHndl, level, OptHdrWrapper::DLL_CHARACT, parent)
{
	this->characteristics = characteristics;
	if (this->level == DESC)
		init();
}

void OptHdrDllCharactTreeItem::init()
{
	if (!myPeHndl || this->level != DESC) return;

	bool isOk;
	this->characteristics = myPeHndl->optHdrWrapper.getNumValue(role, &isOk);
	if (!isOk) return;

	/* create subrecords */ 
	std::vector<DWORD> charactSet = OptHdrWrapper::splitDllCharact(this->characteristics);
	for (std::vector<DWORD>::const_iterator chIter = charactSet.begin(); chIter != charactSet.end(); ++chIter) {
		DWORD splitedCharacter = *chIter;
		this->appendChild(new OptHdrDllCharactTreeItem(myPeHndl, DETAILS, splitedCharacter, this));
	}
}

QVariant OptHdrDllCharactTreeItem::data(int column) const
{
	if (!m_PE) return QVariant();

	if (this->level == DESC) {
		switch (column) {
			case COL_OFFSET : return QString::number(getContentOffset(), 16).toUpper();
			case COL_NAME : return ("DLL Characteristics");
			case COL_VALUE : return QString::number(this->characteristics, 16).toUpper();
		}
		return QVariant();
	}

	switch (column) {
		case COL_VALUE : return QString::number(this->characteristics, 16).toUpper();
		case COL_VALUE2 : 
			return OptHdrWrapper::translateDllCharacteristics(this->characteristics);
	}
	return QVariant();
}

offset_t OptHdrDllCharactTreeItem::getContentOffset() const
{
	offset_t offset = this->m_PE->peOptHdrOffset();
	if (offset == INVALID_ADDR) return 0;
	
	Executable::exe_bits mode = this->m_PE->getBitMode();
	if (mode != Executable::BITS_32 && mode != Executable::BITS_64) return 0;
	
	static IMAGE_OPTIONAL_HEADER32 h32;
	static IMAGE_OPTIONAL_HEADER64 h64;
	return offset + (mode == Executable::BITS_32 ? ((uint64_t)&h32.DllCharacteristics - (uint64_t) &h32) : ((uint64_t) &h64.DllCharacteristics - (uint64_t) &h64));
}
///------------------------

OptionalHdrTreeItem::OptionalHdrTreeItem(PeHandler *peHndl, level_t level, OptHdrWrapper::OptHdrFID role, OptionalHdrTreeItem *parent)
	: PeTreeItem(peHndl, role, FIELD_NONE, parent), 
	optHdr(peHndl->optHdrWrapper)
{
	this->setParent(parent);
	this->m_parentItem = parent;
	
	this->level = level;
	this->role = role;
	if (this->m_PE == NULL) {
		this->level = DESC;
		this->role = OptHdrWrapper::NONE;
		return;
	}
}

int OptionalHdrTreeItem::columnCount() const { 	return MAX_COL; }

QVariant OptionalHdrTreeItem::background(int column) const
{
	if (!m_PE) return QVariant();
	
	QColor changeCol = Qt::cyan;
	changeCol.setAlpha(50);

	if (column == COL_VALUE) {
		if (role == OptHdrWrapper::IMAGE_SIZE) {
			bool isOk;
			uint64_t rawImgSize = myPeHndl->optHdrWrapper.getNumValue(OptHdrWrapper::IMAGE_SIZE, &isOk);
			if (isOk && (m_PE->getImageSize() != rawImgSize)) {
				return changeCol;
			}
		}
		if (role == OptHdrWrapper::EP) {
			if (!m_PE->getContentAt(m_PE->getEntryPoint(Executable::RAW), 1)) {
				return errColor;
			}
		}
		
		if (role == OptHdrWrapper::CHECKSUM) {
			bool isOk;
			uint64_t checksum = myPeHndl->optHdrWrapper.getNumValue(OptHdrWrapper::CHECKSUM, &isOk);
			if (isOk) {
				if (this->myPeHndl->getCurrentChecksum() != QString::number(checksum, 16)) return errColor;
			}
		}
	}

	if (level == DETAILS && (column >= COL_VALUE && column <= COL_VALUE2)) {
		QColor flagsColorT = addrColors.flagsColor();
		flagsColorT.setAlpha(addrColors.flagsAlpha());
		return flagsColorT;
	}
	return QVariant();
}

QVariant OptionalHdrTreeItem::foreground(int column) const
{
	if (column == COL_OFFSET) return this->offsetFontColor;
	return QVariant();
}

QVariant OptionalHdrTreeItem::toolTip(int column) const
{
	int fieldIndx = column;
	if (!myPeHndl || !m_PE) return QVariant();

	if (column == COL_OFFSET) return tr("Right click to follow");

	if (role == OptHdrWrapper::DLL_CHARACT) {
		bool isOk;
		DWORD val = myPeHndl->optHdrWrapper.getNumValue(OptHdrWrapper::DLL_CHARACT, &isOk);
		if (!isOk) return QVariant();

		std::vector<DWORD> charactSet = OptHdrWrapper::splitDllCharact(val);
		QString tip = "";
		for (std::vector<DWORD>::const_iterator iter = charactSet.begin(); iter != charactSet.end(); ++iter) {
			QString name = OptHdrWrapper::translateDllCharacteristics(*iter);
			if (name.length()) continue;
			if (tip.size() > 0) tip +="\n";
			tip += name;
		}
		return tip;
	}
	return data(column);
}

offset_t OptionalHdrTreeItem::getContentOffset() const
{
	if (!m_PE) return 0;
	return optHdr.getFieldOffset(role);
}

bufsize_t OptionalHdrTreeItem::getContentSize() const
{
	if (!m_PE) return 0;
	return optHdr.getFieldSize(role);
}

QVariant OptionalHdrTreeItem::font(int col) const
{
	if (col != COL_NAME && col != COL_VALUE2 && this->role < OptHdrWrapper::DATA_DIR) {
		return this->offsetFont;
	}
	return QVariant();
}

QVariant OptionalHdrTreeItem::data(int column) const
{
	if (m_PE == NULL) return QVariant();

	if (this->level != DESC) return QVariant();
	if (!optHdr.getFieldPtr(role)) return QVariant();

	switch (column) {
		case COL_OFFSET : return QString::number(getContentOffset(), 16).toUpper();
		case COL_NAME : return optHdr.getFieldName(role);
		case COL_VALUE2 : return dataValMeanings();
		case COL_VALUE : 
		{
			bool isOk = false;
			uint64_t val = optHdr.getNumValue(role, column, &isOk);

			if (!isOk) return QVariant(); /* other type? */
			return QString::number(val, 16);
		}
	}
	return QVariant();
}

Qt::ItemFlags OptionalHdrTreeItem::flags(int column) const
{
	if (!optHdr.getFieldPtr(role)) return Qt::NoItemFlags;
    
	const Qt::ItemFlags fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if (column != COL_VALUE || level == DETAILS) return fl;
	return fl | Qt::ItemIsEditable;
}

bool OptionalHdrTreeItem::setDataValue(int column, const QVariant &value)
{
	if (this->m_PE == NULL) return false;
	if (column != COL_VALUE) return false;

	QString text = value.toString();
	bool isOk = false;
	ULONGLONG number = text.toLongLong(&isOk, 16);
	if (!isOk) return false;
	isOk = optHdr.setNumValue(role, column, number);
	return isOk;
}

QVariant OptionalHdrTreeItem::dataValMeanings() const
{
	if (this->m_PE == NULL) return QVariant();
	bool isOk = false;

	switch (role) {
		case OptHdrWrapper::MAGIC:
		{
			DWORD val = (DWORD) optHdr.getNumValue(OptHdrWrapper::MAGIC, &isOk);
			if (!isOk) return QVariant();
			return OptHdrWrapper::translateOptMagic(val);
		}
		case OptHdrWrapper::OSVER_MAJOR:
		{
			uint16_t* major = (uint16_t*) optHdr.getFieldPtr(OptHdrWrapper::OSVER_MAJOR);
			uint16_t* minor = (uint16_t*) optHdr.getFieldPtr(OptHdrWrapper::OSVER_MINOR);
			if (!major || !minor) return QVariant();

			return OptHdrWrapper::translateOSVersion(*major, *minor);
		}
		case OptHdrWrapper::SUBSYS:
		{
			DWORD val = (DWORD)optHdr.getNumValue(OptHdrWrapper::SUBSYS, &isOk);
			if (!isOk) return QVariant();
			return OptHdrWrapper::translateSubsystem(val);
		}	
	}
	return QVariant();
}


//-----------------------------------------------------------------------------
OptionalHdrTreeModel::OptionalHdrTreeModel(PeHandler *peHndl, QObject *parent)
	:  PeWrapperModel(peHndl, parent), dllCharact(NULL), dataDirItem(NULL)
{
	if (!m_PE) return;

	rootItem = new OptionalHdrTreeItem(peHndl);
	size_t fieldNum = OptHdrWrapper::FIELD_COUNTER;

	for (int i = 0; i < fieldNum; i++) {
	
		OptHdrWrapper::OptHdrFID role = OptHdrWrapper::OptHdrFID(i);

		if (role == OptHdrWrapper::DATA_DIR) {
			dataDirItem = new DataDirTreeItem(peHndl, OptionalHdrTreeItem::DESC);
			rootItem->appendChild(dataDirItem);
		}
		else if (role == OptHdrWrapper::DLL_CHARACT) {
			this->dllCharact = new OptHdrDllCharactTreeItem(peHndl, OptionalHdrTreeItem::DESC);
			rootItem->appendChild(dllCharact);
		} else
			rootItem->appendChild(new OptionalHdrTreeItem(peHndl, OptionalHdrTreeItem::DESC, role));
	}
	connect(peHndl, SIGNAL(modified()), this, SLOT(reload()));
}

void OptionalHdrTreeModel::reload()
{
	this->beginResetModel();

	this->dataDirItem->removeAllChildren();
	this->dllCharact->removeAllChildren();

	dataDirItem->init();
	this->dllCharact->init();
	
	this->endResetModel();
	emit modelUpdated();
}

QVariant OptionalHdrTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) return QVariant();
	if (role == Qt::ForegroundRole) {
		int fId = getFID(index);
		return this->addrColor(index);
	}
	if (role == Qt::ToolTipRole) return toolTip(index);

	OptionalHdrTreeItem *item = static_cast<OptionalHdrTreeItem*> (index.internalPointer());
	if (item == NULL) return QVariant();

	switch (role) {
		case Qt::FontRole :
			return item->font(index.column());
		case Qt::BackgroundRole :
			return item->background(index.column());
		case Qt::ForegroundRole :
			return item->foreground(index.column());
		case Qt::DisplayRole : 
		case Qt::EditRole :
			return item->data(index.column());
	}
	return QVariant();
}


bool OptionalHdrTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	OptionalHdrTreeItem *item = static_cast<OptionalHdrTreeItem*>(index.internalPointer());
	if (!item) return false;
	return item->setData(index.column(), value, role);
}


QVariant OptionalHdrTreeModel::headerData(int section, Qt::Orientation orien, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();

	switch (section) {
		case COL_OFFSET : return tr("Offset");
		case COL_NAME : return tr("Name");
		case COL_VALUE : return tr("Value");
		case COL_VALUE2 : return tr("Value");
	}
	return QVariant();
}

Qt::ItemFlags OptionalHdrTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	
	OptionalHdrTreeItem *item = static_cast<OptionalHdrTreeItem*>(index.internalPointer());
	if (!item)
		return Qt::NoItemFlags;
	
	return item->flags(index.column());
}

bool OptionalHdrTreeModel::containsValue(QModelIndex index) const
{
	OptionalHdrTreeItem *childItem = static_cast<OptionalHdrTreeItem*>(index.internalPointer());
	DataDirTreeItem *dataDirItem = dynamic_cast<DataDirTreeItem*>(childItem);

	if (dataDirItem) {
		if (dataDirItem->level != OptionalHdrTreeItem::DETAILS) return false;
		if (index.column() == COL_VALUE) return true;
	}
	if (index.column() == COL_VALUE) return true;
	return false;
}

int OptionalHdrTreeModel::getFID(const QModelIndex &index) const
{
	if (!index.isValid()) return -1;
	
	OptionalHdrTreeItem* item = static_cast<OptionalHdrTreeItem*>(index.internalPointer());
	if (item == NULL) return -1;
	int fId = item->role;

	DataDirTreeItem *dataDirItem = dynamic_cast<DataDirTreeItem*>(item);
	if (dataDirItem) fId = dataDirItem->recordNum;
	return fId;
}

int OptionalHdrTreeModel::getSID(const QModelIndex &index) const
{
	return index.column() - COL_VALUE;
}

ExeElementWrapper* OptionalHdrTreeModel::wrapperAt(QModelIndex index) const
{
	if (!index.isValid()) return NULL;
	OptionalHdrTreeItem* item = static_cast<OptionalHdrTreeItem*>(index.internalPointer());
	if (item == NULL) return NULL;

	ExeElementWrapper *wrapper = this->wrapper();
	DataDirTreeItem *dataDirItem = dynamic_cast<DataDirTreeItem*>(item);
	if (dataDirItem) {
		return &this->myPeHndl->dataDirWrapper;
	}
	return wrapper;
}
