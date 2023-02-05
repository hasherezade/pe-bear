#include <new>
#include "PeHandler.h"

#include "../base/PeHandlersManager.h"
#include <bearparser/bearparser.h>

using namespace sig_ma;
using namespace pe;

enum operation_ids { SIMPLE = 0, OP_ADD_SECTION = 1 };


CalcThread::CalcThread(CalcThread::hash_type hType, PEFile* pe, offset_t checksumOffset)
	: m_PE(pe), hashType(hType), checksumOff(checksumOffset)
{
}

void CalcThread::run()
{
	QMutexLocker lock(&m_arrMutex);

	QString fileHash = "Cannot calculate!";
	if (!m_PE || !m_PE->getContent()) {
		emit gotHash(fileHash, hashType);
		return;
	}
	QCryptographicHash::Algorithm qHashType = QCryptographicHash::Md5;
	if (hashType == MD5) {
		qHashType = QCryptographicHash::Md5;
	} else if (hashType == SHA1) {
		qHashType = QCryptographicHash::Sha1;
	} 
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) //the feature was introduced in Qt5.0
	else if (hashType == SHA256) {
		qHashType = QCryptographicHash::Sha256;
	}
#endif
	try {
		if (hashType == CHECKSUM) {
			long checksum = PEFile::computeChecksum((BYTE*) m_PE->getContent(), m_PE->getContentSize(), checksumOff);
			fileHash = QString::number(checksum, 16);
		} else {
			QCryptographicHash calcHash(qHashType);
			calcHash.addData((char*) m_PE->getContent(), m_PE->getContentSize());
			fileHash = QString(calcHash.result().toHex());
		}
	} catch (...) {
		fileHash = "Cannot calculate!";
	}
	emit gotHash(fileHash, hashType);
}

//-------------------------------------------------
PeHandler::PeHandler(PEFile *pe, FileBuffer *fileBuffer)
	: QObject(), 
	m_fileModDate(QDateTime()), // init with empty
	m_loadedFileModDate(QDateTime()), // init with empty
	dosHdrWrapper(pe), 
	fileHdrWrapper(pe), optHdrWrapper(pe), richHdrWrapper(pe), dataDirWrapper(pe),
	exportDirWrapper(pe), importDirWrapper(pe), tlsDirWrapper(pe), relocDirWrapper(pe), 
	securityDirWrapper(pe), ldConfDirWrapper(pe), boundImpDirWrapper(pe),
	delayImpDirWrapper(pe), debugDirWrapper(pe), exceptDirWrapper(pe), clrDirWrapper(pe),
	resourcesAlbum(pe),
	resourcesDirWrapper(pe, &resourcesAlbum),
	signFinder(NULL), 
	modifHndl(pe->getFileBuffer(), this)
{
	if (!pe) return;

	m_PE = pe;
	m_fileBuffer = fileBuffer;

	markedTarget = INVALID_ADDR;
	markedOrigin = INVALID_ADDR;

	displayedOffset = 0;
	displayedSize = 0;

	this->hoveredOffset = INVALID_ADDR;
	this->hoveredSize = 0;
	this->hilightedOffset = INVALID_ADDR;
	this->hilightedSize = 0;
	pageStart = 0;
	pageSize = PREVIEW_SIZE;
	
	updateFileModifTime();
	m_loadedFileModDate = m_fileModDate; // init
	
	associateWrappers();
	this->wrapAlbum();
	
	for (int i = 0; i < CalcThread::HASHES_NUM; i++) {
		calcThread[i] = NULL;
		calcQueued[i] = false;
	}
	//---
	this->runHashesCalculation();
}

void PeHandler::associateWrappers()
{
	// clear associations
	for (int i = 0; i < DIR_ENTRIES_COUNT ; i++) {
		dataDirWrappers[i] = NULL;
	}
	// make associations
	dataDirWrappers[DIR_EXPORT] = &exportDirWrapper;
	dataDirWrappers[DIR_IMPORT] = &importDirWrapper;
	dataDirWrappers[DIR_TLS] = &tlsDirWrapper;
	dataDirWrappers[DIR_BASERELOC] = &relocDirWrapper;

	dataDirWrappers[DIR_SECURITY] = &securityDirWrapper;
	dataDirWrappers[DIR_LOAD_CONFIG] = &ldConfDirWrapper;
	dataDirWrappers[DIR_BOUND_IMPORT] = &boundImpDirWrapper;

	dataDirWrappers[DIR_DELAY_IMPORT] = &delayImpDirWrapper;
	dataDirWrappers[DIR_DEBUG] = &debugDirWrapper;
	dataDirWrappers[DIR_EXCEPTION] = &exceptDirWrapper;
	dataDirWrappers[DIR_RESOURCE] = &resourcesDirWrapper;
	dataDirWrappers[DIR_COM_DESCRIPTOR] = &clrDirWrapper;
}

bool PeHandler::hasDirectory(dir_entry dirNum)
{
	if (dirNum >= DIR_ENTRIES_COUNT) return false;

	if (!this->m_PE || !this->m_PE->hasDirectory(dirNum)) return false;

	if (!dataDirWrappers[dirNum]) return false;
	if (!dataDirWrappers[dirNum]->getPtr()) return false;
	return true;
}

QString PeHandler::getCurrentHash(CalcThread::hash_type type)
{
	QMutexLocker ml(&m_hashMutex[type]);
	return hash[type];
}

void PeHandler::onHashReady(QString hash, int hType)
{
	if (hType >= CalcThread::HASHES_NUM) return;
	QMutexLocker ml(&m_hashMutex[hType]);
	this->hash[hType] = hash;
	emit hashChanged();
}

void PeHandler::onCalcThreadFinished()
{
	for (int hType = 0; hType < CalcThread::HASHES_NUM; hType++) {
		if (calcThread[hType] != NULL && calcThread[hType]->isFinished()) {
			delete calcThread[hType];
			calcThread[hType] = NULL;
			if (calcQueued[hType]) {
				//printf("starting queued\n");
				calculateHash((CalcThread::hash_type) hType);
			}
		}
	}
}

void PeHandler::deleteThreads()
{
	for (int hType = 0; hType < CalcThread::HASHES_NUM; hType++) {
		if (calcThread[hType] != NULL) {
			while (calcThread[hType]->isFinished() == false) {
				calcThread[hType]->wait();
			}
			delete calcThread[hType];
			calcThread[hType] = NULL;
		}
	}
}

void PeHandler::calculateHash(CalcThread::hash_type type)
{
	if (type >= CalcThread::HASHES_NUM) return;
	
	if (calcThread[type] != NULL) {
		calcQueued[type] = true;
		return; //previous thread didn't finished
	}
	const char* content = (char*) m_PE->getContent();
	const offset_t size = m_PE->getRawSize();
	const bool isSizeAcceptable = (offset_t(int(size)) == size) ? true : false;

	if (!content || !size || !isSizeAcceptable) {
		hash[type] = "Cannot calculate!";
	} else {
		hash[type] = "Calculating...";
		offset_t checksumOffset = this->optHdrWrapper.getFieldOffset(OptHdrWrapper::CHECKSUM);
		this->calcThread[type] = new CalcThread(type, m_PE, checksumOffset);
		QObject::connect(calcThread[type], SIGNAL(gotHash(QString, int)), this, SLOT(onHashReady(QString, int)));
		QObject::connect(calcThread[type], SIGNAL(finished()), this, SLOT(onCalcThreadFinished()));
		calcThread[type]->start();
		calcQueued[type] = false;
	}
}

void PeHandler::setPackerSignFinder(SigFinder *sFinder)
{
	this->signFinder = sFinder;
	findPackerSign(m_PE->getEntryPoint(), Executable::RVA, sig_ma::FIXED);
}

bool PeHandler::isPacked()
{
	return (this->packerAtOffset.size() > 0);
}

PckrSign* PeHandler::findPackerSign(offset_t startAddr, Executable::addr_type aT, match_direction md)
{
	if (!signFinder || !m_PE) return NULL;
	BYTE* content = m_PE->getContent();
	if (!content) return NULL;
	size_t contentSize = m_PE->getRawSize();

	offset_t startingRaw = m_PE->toRaw(startAddr, aT);
	if (startingRaw == INVALID_ADDR) return NULL;

	sig_ma::matched matchedSet = signFinder->getMatching(content, contentSize, startingRaw, md);
	int foundCount = matchedSet.signs.size();
	if (foundCount == 0) return NULL;

	PckrSign* packer = *(matchedSet.signs.begin());
	if (!packer) foundCount = 0;

	if (foundCount > 0) {
		FoundPacker pckr(startingRaw + matchedSet.match_offset, packer);
		std::vector<FoundPacker>::iterator itr = std::find(this->packerAtOffset.begin(), this->packerAtOffset.end(), pckr);

		if (itr != this->packerAtOffset.end()) { //already exist
			FoundPacker &found = *itr;
			packer = found.signaturePtr;
		} else {
			this->packerAtOffset.push_back(pckr);
		}
	}

	emit foundSignatures(foundCount, 0);
	return packer;
}


PckrSign* PeHandler::findPackerInArea(offset_t rawOff, size_t areaSize, sig_ma::match_direction md)
{
	if (!signFinder || !m_PE) return NULL;
	
	BYTE *content = NULL;

	bool isDeepSearch = false;
	offset_t foundOffset = 0;
	PckrSign* packer = NULL;
	int foundCount = 0;

	for (size_t step = 0; step < areaSize; step++) {

		size_t size = areaSize - step;
		content = m_PE->getContentAt(rawOff + step, Executable::RAW, size);
		if (content == NULL) {
			//printf("content is NULL\n");
			break;
		}
		sig_ma::matched matchedSet = signFinder->getMatching(content, size, 0, md);
		
		foundCount += matchedSet.signs.size();
		if (matchedSet.signs.size() == 0) break;

		packer = *(matchedSet.signs.begin());
		if (!packer) break;
		
		foundOffset = step + matchedSet.match_offset + rawOff;
		//printf("Found %s, at %x searching next...\n", packer->get_name().c_str(), foundOffset);

		step += matchedSet.match_offset;
		FoundPacker pckr(foundOffset , packer);
		std::vector<FoundPacker>::iterator itr = std::find(this->packerAtOffset.begin(), this->packerAtOffset.end(), pckr);
		if (itr != this->packerAtOffset.end()) {
			//already exist
			FoundPacker &found = *itr;
			packer = found.signaturePtr;
			continue;
		} else {
			this->packerAtOffset.push_back(pckr);
		}
		if (isDeepSearch == false) break;
	}

	emit foundSignatures(foundCount, 1);
	return packer;
}


void PeHandler::setHilighted(offset_t hOffset, uint32_t hSize)
{
	this->hilightedOffset = hOffset;
	this->hilightedSize = hSize;
}

void PeHandler::setHovered(bool isRVA, offset_t hOffset, uint32_t hSize)
{
	if (!m_PE) return;

	Executable::addr_type aType = isRVA ? Executable::RVA : Executable::RAW;
	offset_t raw =  m_PE->toRaw(hOffset, aType);
	if (raw == INVALID_ADDR) return;
	
	hOffset = raw;
	if (this->hoveredOffset == hOffset && this->hoveredSize == hSize) return;

	this->hoveredOffset = hOffset;
	this->hoveredSize = hSize;
	emit hovered();
}

bool PeHandler::setDisplayed(bool isRVA, offset_t dOffset, bufsize_t dSize)
{
	if (!m_PE) return false;

	Executable::addr_type aType = isRVA ? Executable::RVA : Executable::RAW;
	offset_t raw =  m_PE->toRaw(dOffset, aType);
	if (raw == INVALID_ADDR) return false;

	dOffset = raw;
	if (dOffset >= this->m_PE->getRawSize()) { //out of scope!
		dOffset = this->m_PE->getRawSize();
		dSize = 0;
	}
	/* store previous */
	offset_t prevOff = this->displayedOffset;
	if (prevOff >= 0) this->prevOffsets.push(prevOff);

	/* set current */ 
	this->displayedOffset = dOffset;
	if (dSize != SIZE_UNLIMITED) {
		this->displayedSize = dSize;
	}
	setPageOffset(this->displayedOffset);
	return true;
}

void  PeHandler::undoDisplayOffset()
{
	if (this->prevOffsets.size() == 0) return;

	uint32_t prevOff =  this->prevOffsets.top();
	this->prevOffsets.pop();
	this->displayedOffset = prevOff;
//	emit displayAreaModified(this->displayedOffset, this->displayedSize);
	setPageOffset(this->displayedOffset);
}

void PeHandler::setPageOffset(offset_t pageO)
{
	pageStart = pageO;
	emit pageOffsetModified(this->pageStart, this->pageSize);
}

void PeHandler::advanceOffset(int increment)
{ 
	offset_t page = pageStart;

	if (increment < 0) {
		increment *= (-1);
		if (increment > page) 
			page = 0;
		else 
			page -= increment;
	} else {
		offset_t max = m_PE->getRawSize();
		if (page + increment > max) 
			page = max;
		else 
			page += increment;
	}

	setPageOffset(page);
}


bool PeHandler::setDisplayedEP()
{
	if (!this->m_PE) return false;
	bool isOk = true;
	offset_t epRVA = this->m_PE->getEntryPoint();
	//
	offset_t epOff = 0;
	try {
		epOff = m_PE->rvaToRaw(epRVA);
	} catch (CustomException &e) {
		isOk = false;
	}
	if (!isOk) return false;
	//
	offset_t dispO = epOff;
	bufsize_t dispS = this->m_PE->getRawSize() - epOff;
	setDisplayed(false, dispO, dispS);
	return true;
}

bool PeHandler::isDataDirModified(offset_t modO, bufsize_t modS)
{
	BYTE *content = m_PE->getContent();
	bufsize_t contentSize = m_PE->getRawSize();
	IMAGE_DATA_DIRECTORY* dir = this->m_PE->getDataDirectory();
	if (content && dir) {
		offset_t offsetS = ((BYTE*) dir) - content;
		offset_t offsetE = offsetS + (sizeof(IMAGE_DATA_DIRECTORY) * pe::DIR_ENTRIES_COUNT);
		
		offset_t modE = modO + modS;
		if ((modO >= offsetS && modO <= offsetE) || (modE >= offsetS && modE <= offsetE)) {
				return true;
		}
	}
	return false;
}

bool PeHandler::isSectionsHeadersModified(offset_t modO, bufsize_t modSize)
{
	BYTE *content = m_PE->getContent();
	size_t contentSize = m_PE->getRawSize();
	if (!content) return false;
	offset_t modE = modO + modSize;

	offset_t optOff = this->fileHdrWrapper.getFieldOffset(FileHdrWrapper::OPTHDR_SIZE);
	bufsize_t optSize = this->fileHdrWrapper.getFieldSize(FileHdrWrapper::OPTHDR_SIZE);
	
	if (modO >= optOff && modE <= (optOff + optSize)) {
		//printf("OPTHDR_SIZE modified!\n");
		return true;
	}
	int bgn = m_PE->secHdrsOffset();
	int end = m_PE->secHdrsEndOffset();

	if (modO >= bgn && modO < end) return true;
	if (modE >= bgn && modO < end) return true;
	return false;
}

bool PeHandler::isInActiveArea(offset_t offset)
{
	bool isActiveArea = true;
	if (this->displayedSize != ULONG_MAX) {

		offset_t highO = this->hilightedOffset;
		offset_t highS = this->hilightedSize;
			
		if (highO != (-1) && highS != (-1) && (offset < highO || offset >= (highO + highS))) {
			isActiveArea = false;
		}
	}
	return isActiveArea;
}

bool PeHandler::isInModifiedArea(offset_t offset)
{
	return this->modifHndl.isInLastModifiedArea(offset);
}

bool PeHandler::resize(bufsize_t newSize)
{
	if (m_PE->resize(newSize)) {
		updatePeOnResized();
		emit modified();
		return true;
	}
	return false;
}

bool PeHandler::resizeImage(bufsize_t newSize)
{
	if (m_PE->getImageSize() == newSize) return false; //nothing to change

	offset_t modOffset = this->optHdrWrapper.getFieldOffset(OptHdrWrapper::IMAGE_SIZE);
	bufsize_t modSize = this->optHdrWrapper.getFieldSize(OptHdrWrapper::IMAGE_SIZE);
	this->modifHndl.backupModification(modOffset, modSize, false);

	m_PE->setImageSize(newSize);
	return true;
}

SectionHdrWrapper* PeHandler::addSection(QString name, bufsize_t rSize, bufsize_t vSize) //throws exception
{
	offset_t modOffset = this->optHdrWrapper.getFieldOffset(OptHdrWrapper::IMAGE_SIZE);
	bufsize_t modSize = this->optHdrWrapper.getFieldSize(OptHdrWrapper::IMAGE_SIZE);
	this->modifHndl.backupModification(modOffset, modSize, false);

	modOffset = this->fileHdrWrapper.getFieldOffset(FileHdrWrapper::SEC_NUM);
	modSize = this->fileHdrWrapper.getFieldSize(FileHdrWrapper::SEC_NUM);
	this->modifHndl.backupModification(modOffset, modSize, true);

	//--
	modOffset = m_PE->secHdrsEndOffset();
	modSize = sizeof(IMAGE_SECTION_HEADER);
	this->modifHndl.backupModification(modOffset, modSize, true);
	//---
	SectionHdrWrapper* newSec = NULL;
	try {
		newSec = m_PE->addNewSection(name, rSize, vSize);
	} catch (CustomException e) {
		this->modifHndl.unStoreLast();
		throw (e);
	}

	if (!newSec) {
		this->modifHndl.unStoreLast();
		throw CustomException("Cannot add new section!");
	}
	emit modified();
	emit secHeadersModified();
	return newSec;
}

offset_t PeHandler::loadSectionContent(SectionHdrWrapper* sec, QFile &fIn, bool continueLastOperation)
{
	if (!m_PE || !sec) return 0;

	offset_t modifOffset = sec->getRawPtr();
	bufsize_t modifSize = sec->getContentSize(Executable::RAW, true);
	backupModification(modifOffset, modifSize, continueLastOperation);

	AbstractByteBuffer *buf = m_PE->getFileBuffer();
	if (!buf) return 0;

	offset_t loaded = buf->substFragmentByFile(modifOffset, modifSize, fIn);

	setDisplayed(false, modifOffset, modifSize);
	setBlockModified(modifOffset, loaded);
	return loaded;
}

bool PeHandler::_moveDataDirEntry(pe::dir_entry dirNum, offset_t targetRaw, bool continueLastOperation)
{
	if (dirNum >= DIR_ENTRIES_COUNT) return false;
	if (!dataDirWrappers[dirNum]) return false;

	const offset_t fieldOffset = this->dataDirWrapper.getFieldOffset(dirNum);
	const bufsize_t fieldSize = this->dataDirWrapper.getFieldSize(dirNum, FIELD_NONE);
	if (fieldOffset == INVALID_ADDR) return false;

	// current directory:
	BYTE* ptr = (BYTE*)dataDirWrappers[dirNum]->getPtr();
	const offset_t dirOffset = m_PE->getOffset(ptr);
	bufsize_t dirSize = getDirSize(dirNum);

	if (!ptr || !dirSize || dirOffset == INVALID_ADDR) return false;

	backupModification(dirOffset, dirSize, continueLastOperation); // backup current area
	backupModification(targetRaw, dirSize, true); // backup the target area
	backupModification(fieldOffset, fieldSize, true); //backup the offset
	bool isOk = false;
	try {
		isOk = m_PE->moveDataDirEntry(dirNum, targetRaw, Executable::RAW);
	}
	catch (CustomException e) {
		isOk = false;
	}
	if (!isOk) {
		unbackupLastModification();
		return false;
	}
	dataDirWrappers[dirNum]->wrap();
	return true;
}

bool PeHandler::moveDataDirEntry(pe::dir_entry dirNum, offset_t targetRaw)
{
	if (!_moveDataDirEntry(dirNum, targetRaw, false)) {
		return false;
	}
	emit modified();
	return true;
}

size_t PeHandler::getDirSize(dir_entry dirNum)
{
	if (dirNum >= DIR_ENTRIES_COUNT) return 0;
	if (dataDirWrappers[dirNum] == NULL) return 0;
	
	bufsize_t dirSize = dataDirWrappers[dirNum]->getSize();
	return dirSize;
}

bool PeHandler::canAddImportsLib(size_t libsCount)
{
	const size_t kRequiredFreeRecords = libsCount + 1;
	const bufsize_t tableSize = importDirWrapper.getSize();

	offset_t impDirOffset = importDirWrapper.getOffset();
	if (impDirOffset == INVALID_ADDR) return false;

	const bufsize_t fieldSize = sizeof(IMAGE_IMPORT_DESCRIPTOR);

	if (!m_PE->getContentAt(impDirOffset, tableSize + fieldSize)) return false;

	offset_t fieldOffset = (tableSize < fieldSize) ? impDirOffset : impDirOffset + tableSize - fieldSize; // substract the terminator record from the table
	BYTE *ptr = m_PE->getContentAt(fieldOffset, fieldSize * kRequiredFreeRecords); // space for the new records + the terminator
	if (!ptr) return false;

	if (!pe_util::isSpaceClear(ptr, fieldSize * kRequiredFreeRecords)) {
		return false;
	}
	return true;
}

bool PeHandler::addImportLib(bool continueLastOperation)
{
	if (!canAddImportsLib(1)) {
		return false;
	}
	const bufsize_t tableSize = importDirWrapper.getSize();

	offset_t impDirOffset = importDirWrapper.getOffset();
	if (impDirOffset == INVALID_ADDR) return false;

	const bufsize_t fieldSize = sizeof(IMAGE_IMPORT_DESCRIPTOR);

	offset_t fieldOffset = (tableSize < fieldSize) ? impDirOffset : (impDirOffset + tableSize - fieldSize); // substract the terminator record from the table
	BYTE *ptr = m_PE->getContentAt(fieldOffset, fieldSize * 2); // space for the new record + terminator
	if (!ptr) return false;

	backupModification(fieldOffset, fieldSize, continueLastOperation); // backup Section Header

	IMAGE_IMPORT_DESCRIPTOR* field = (IMAGE_IMPORT_DESCRIPTOR*)ptr;
	memset(field, 0, fieldSize);

	field->OriginalFirstThunk = (-1);
	field->FirstThunk = (-1);
	field->Name = (-1);

	importDirWrapper.wrap();
	//---
	emit modified();
	return true;
}

ImportEntryWrapper* PeHandler::_autoAddLibrary(const QString &name, size_t importedFuncsCount, size_t expectedDllsCount, offset_t &storageOffset)
{
	//add new library wrapper:
	ImportDirWrapper* imports = dynamic_cast<ImportDirWrapper*> (m_PE->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_IMPORT));
	
	ImportEntryWrapper* lastWr = dynamic_cast<ImportEntryWrapper*> (imports->getLastEntry());
	if (lastWr == NULL) return NULL;
	
	//backup only the spacer:
	backupModification(lastWr->getOffset() + lastWr->getSize(), lastWr->getSize(), true);
	ImportEntryWrapper* libWr = dynamic_cast<ImportEntryWrapper*> (imports->addEntry(NULL));
	if (libWr == NULL) {
		this->unbackupLastModification();
		throw CustomException("Failed to add a DLL entry!");
		return NULL;
	}
	const size_t PADDING = libWr->getSize() * (expectedDllsCount + 1); // leave the space for further entries
	storageOffset = imports->getOffset() + imports->getContentSize() + PADDING;
	offset_t nameOffset = storageOffset;
	const size_t kRecordSize = libWr->getFieldSize(ImportEntryWrapper::FIRST_THUNK) * 2; // TODO: calculate it better
	const size_t kNameRecordPadding = (kRecordSize * (importedFuncsCount + 1)) + 1; // leave space for X thunks + terminator + string '\0' terminator
	const size_t nameTotalSize = name.length() + 1;
	while (true) {
		BYTE *ptr = m_PE->getContentAt(nameOffset, nameTotalSize + kNameRecordPadding);
		if (!ptr) {
			this->unbackupLastModification();
			throw CustomException("Failed to get a free space to fill the entry!");
			return NULL;
		}
		if (!pe_util::isSpaceClear(ptr, nameTotalSize + kNameRecordPadding)) {
			nameOffset++; //move the pointer...
			continue;
		}
		nameOffset += kNameRecordPadding; //leave the padding between the previous element and the current record
		break;
	}
	
	backupModification(nameOffset, nameTotalSize, true);
	if (m_PE->setStringValue(nameOffset, name) == false) {
		this->unbackupLastModification();
		throw CustomException("Failed to fill library name!");
		return NULL;
    }
    storageOffset = nameOffset + nameTotalSize;

    offset_t firstThunk = m_PE->convertAddr(storageOffset, Executable::RAW, Executable::RVA);

    libWr->setNumValue(ImportEntryWrapper::FIRST_THUNK, firstThunk);
    libWr->setNumValue(ImportEntryWrapper::ORIG_FIRST_THUNK, firstThunk);
    libWr->wrap();

    storageOffset += kNameRecordPadding;
    offset_t nameRva = m_PE->convertAddr(nameOffset, Executable::RAW, Executable::RVA);
	
	backupModification(libWr->getFieldOffset(ImportEntryWrapper::NAME), libWr->getFieldSize(ImportEntryWrapper::NAME), true);
    libWr->setNumValue(ImportEntryWrapper::NAME, nameRva);

    return libWr;
}

bool PeHandler::_autoFillFunction(ImportEntryWrapper* libWr, ImportedFuncWrapper* fWr, const QString& name, offset_t &storageOffset)
{
	if (m_PE == NULL || fWr == NULL) {
		return false;
	}
	const offset_t thunkRVA = m_PE->convertAddr(storageOffset, Executable::RAW, Executable::RVA);
	if (thunkRVA == INVALID_ADDR) {
		return false;
	}
	backupModification(fWr->getOffset(), fWr->getSize(), true);
	fWr->setNumValue(ImportedFuncWrapper::THUNK, thunkRVA);
	fWr->setNumValue(ImportedFuncWrapper::ORIG_THUNK, thunkRVA);

	storageOffset += sizeof(WORD); //add sizeof Hint
	const size_t nameTotalLen = name.length() + 1;
	backupModification(storageOffset, nameTotalLen, true);
    if (m_PE->setStringValue(storageOffset, name) == false) {
		throw CustomException("Failed to fill the function name");
		this->unbackupLastModification();
		return false;
	}
	storageOffset += nameTotalLen;
	return true;
}

bool PeHandler::autoAddImports(const ImportsAutoadderSettings &settings)
{
	const QStringList dllsList = settings.dllFunctions.keys();
	const size_t dllsCount = dllsList.size();
	const bool shouldMoveTable = (canAddImportsLib(dllsCount)) ? false : true;
	
	const size_t SEC_PADDING = 10;
	size_t impDirSize = this->getDirSize(pe::DIR_IMPORT);
	if (!impDirSize) {
		throw CustomException("Import Directory does not exist");
		return false;
	}
	
	//TODO: calculate the needed size basing on settings:
	size_t newImpSize = pe_util::roundup(impDirSize * 2, 0x1000);;
	
	SectionHdrWrapper *stubHdr = NULL;
	offset_t newImpOffset = INVALID_ADDR;

	if (settings.addNewSec) {
		QString name = "new_imp";
		stubHdr = this->addSection(name, newImpSize, newImpSize);
		if (!stubHdr) {
			throw CustomException("Cannot add a new section");
			return false;
		}
		newImpOffset = stubHdr->getRawPtr();
	} else {
		stubHdr = m_PE->getLastSection();
		if (stubHdr == NULL) {
			throw CustomException("Cannot fetch last section!");
			return false;
		}
		// resize section
		offset_t secROffset = stubHdr->getContentOffset(Executable::RAW, true);
		offset_t realSecSize = m_PE->getContentSize() - secROffset;

		stubHdr = m_PE->extendLastSection(newImpSize + SEC_PADDING);
		if (stubHdr == NULL) {
			throw CustomException("Cannot fetch last section!");
			return false;
		}
		const offset_t SEC_RVA = stubHdr->getContentOffset(Executable::RVA);
		newImpOffset  = secROffset + realSecSize;
	}

	if (shouldMoveTable) {
		if (newImpOffset == INVALID_ADDR) {
			return false;
		}
		backupModification(stubHdr->getFieldOffset(SectionHdrWrapper::CHARACT), stubHdr->getFieldSize(SectionHdrWrapper::CHARACT), true);
		const DWORD oldCharact = stubHdr->getCharacteristics();
		if (!stubHdr->setCharacteristics(oldCharact | 0xE0000000)) {
			this->unbackupLastModification();
			throw CustomException("Cannot modify section characteristics");
			return false;
		}
		if (!this->_moveDataDirEntry(pe::DIR_IMPORT, newImpOffset, true)) {
			throw CustomException("Cannot move the data dir");
			return false;
		}
	}

	QMap<QString, ImportEntryWrapper*> addedWrappers;
	offset_t storageOffset = 0;

	for (auto itr = dllsList.begin(); itr != dllsList.end(); ++itr) {
		
		QString library = *itr;
		const size_t funcCount = settings.dllFunctions[library].size();
		if (!funcCount) continue;
		
		ImportEntryWrapper* libWr = _autoAddLibrary(library, funcCount, dllsCount, storageOffset);
		if (libWr == NULL) {
			throw CustomException("Adding library failed!");
			return false;
		}
		addedWrappers[library] = libWr;
	}

	for (auto itr = addedWrappers.begin(); itr != addedWrappers.end(); ++itr) {
		ImportEntryWrapper* libWr = itr.value();
		const QString library = itr.key();
		for (auto fItr = settings.dllFunctions[library].begin(); fItr != settings.dllFunctions[library].end(); ++fItr) {
			ImportedFuncWrapper* func = _addImportFunc(libWr, true);
			if (!func) {
				break;
			}
			
			QString funcName = *fItr;
			_autoFillFunction(libWr, func, funcName, storageOffset);
			delete func; func = NULL; // delete the temporary wrapper
			libWr->wrap();
		}
	}
	
	ImportDirWrapper* imports = dynamic_cast<ImportDirWrapper*> (m_PE->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_IMPORT));
	if (imports == NULL) {
		throw CustomException("Cannot fetch imports!");
		return false;
	}
	importDirWrapper.wrap();
	
	emit modified();
	return true;
}

ImportedFuncWrapper* PeHandler::_addImportFunc(ImportEntryWrapper *lib, bool continueLastOperation)
{
	if (!lib) return NULL;

	const size_t funcNum = lib->getEntriesCount();
	
	bool isOk = false;
	offset_t callVia = lib->getNumValue(ImportEntryWrapper::FIRST_THUNK, &isOk);
	if (!isOk) {
		return NULL;
	}

	{ //scope0
		// create a temporary wrapper to check if adding a terminating record is possible:
		ImportedFuncWrapper *nextFuncSpacer = new ImportedFuncWrapper(m_PE, lib, funcNum + 1);
		if (nextFuncSpacer->getThunkValue()) {
			// not an empty space
			delete nextFuncSpacer;
			return NULL;
		}
		// space check - OK! enough space for the terminating record
		delete nextFuncSpacer;
	} //!scope0

	ImportedFuncWrapper *nextFunc = NULL;
	bool isSet = false;

	{ //scope1
		// create a temporary wrapper to help filling in the space:
		nextFunc = new ImportedFuncWrapper(m_PE, lib, funcNum);
		offset_t offset = nextFunc->getFieldOffset(ImportEntryWrapper::FIRST_THUNK);
		bufsize_t fieldSize = nextFunc->getFieldSize(ImportEntryWrapper::FIRST_THUNK);

		if (offset && fieldSize) {
			backupModification(offset, fieldSize, continueLastOperation);
			isSet = nextFunc->setNumValue(ImportEntryWrapper::FIRST_THUNK, (-1));
		}
	} //!scope1
	
	if (!isSet) {
		delete nextFunc; nextFunc = NULL;
		this->unbackupLastModification();
		return NULL;
	}
	return nextFunc;
}

bool PeHandler::addImportFunc(size_t libNum)
{
	ImportEntryWrapper *lib = dynamic_cast<ImportEntryWrapper*>(importDirWrapper.getEntryAt(libNum));
	ImportedFuncWrapper* func = _addImportFunc(lib);
	if (!func) {
		return false;
	}
	delete func; //delete the temporary wrapper
	lib->wrap();
	//---
	emit modified();
	return true;
}

void PeHandler::setEP(offset_t newEpRva)
{
	if (!m_PE) return;

	if (newEpRva >= m_PE->getImageSize()) return;
	offset_t epOffset = this->m_PE->peOptHdrOffset();
	
	// TODO: implement it in better way
	Executable::exe_bits mode = this->m_PE->getBitMode();
	if (mode != Executable::BITS_32 && mode != Executable::BITS_64) return;

	static IMAGE_OPTIONAL_HEADER32 h32;
	static IMAGE_OPTIONAL_HEADER64 h64;
	epOffset += (mode == Executable::BITS_32 ? ((uint64_t) &h32.AddressOfEntryPoint - (uint64_t) &h32) : ((uint64_t) &h64.AddressOfEntryPoint - (uint64_t) &h64));

	backupModification(epOffset , sizeof(DWORD)); // backup
	if (m_PE->setEntryPoint(newEpRva, Executable::RVA)) {
		emit modified();
		return;
	}
	this->unbackupLastModification(); // unbackup
}

bool PeHandler::clearBlock(offset_t offset, uint64_t size)
{
	return fillBlock(offset, size, 0);
}

bool PeHandler::fillBlock(offset_t offset, uint64_t size, BYTE val)
{
	if (!m_PE) return false;

	this->backupModification(offset, size);
	BYTE *buf = m_PE->getContentAt(offset, size);
	if (!buf) {
		this->unbackupLastModification();
		return false;
	}
	::memset(buf, val, size);
	return this->setBlockModified(offset, size);
}


bool PeHandler::substBlock(offset_t offset, uint64_t size, BYTE* buf)
{
	if (!m_PE) return false;
	BYTE *content = m_PE->getContent();
	bufsize_t fileSize = m_PE->getRawSize();
	if (offset > fileSize) return false;

	offset_t maxSize = ((offset_t)fileSize) - offset;
	if (size > maxSize) size = maxSize;
	
	this->backupModification(offset, size);
	if (buf) {
		memcpy(content + offset, buf, size);
	} else {
		memset(content + offset, 0, size);
	}
	return this->setBlockModified(offset, size);
}

void PeHandler::backupModification(offset_t modifOffset, bufsize_t modifSize, bool continueLastOperation)
{
	modifHndl.backupModification(modifOffset, modifSize, continueLastOperation);
}

void PeHandler::unbackupLastModification()
{
	modifHndl.unStoreLast();
}

bool PeHandler::setBlockModified(offset_t modO, bufsize_t modSize)
{
	bool isOk = false;
	try {
		updatePeOnModified(modO, modSize); // throws exception
		isOk = true;

	} catch (CustomException &e) {
		this->unModify();
		std::cerr << "Unacceptable modification: " << e.what() << "\n";
		isOk = false;
	}
	if (isOk) {
		emit modified();
	}
	return isOk;
}

bool PeHandler::isBaseHdrModif(offset_t modO, bufsize_t modSize)
{
	bool baseHdrsModified = false;
	if (this->dosHdrWrapper.intersectsBlock(modO, modSize)) {
		//std::cout << "DOS header affected[" << std::hex << modO  << " - " << modO + modSize  << "]!\n";
		baseHdrsModified = true;
	}
	if (this->fileHdrWrapper.intersectsBlock(modO, modSize)) {
		//std::cout << "File Header affected[" << std::hex << modO << " - " << modO + modSize << "]!\n";
		baseHdrsModified = true;
	}
	if (this->optHdrWrapper.intersectsBlock(modO, modSize)) {
		//std::cout << "Optional header affected[" << std::hex << modO << " - " << modO + modSize << "]!\n";
		baseHdrsModified = true;
	}

	return baseHdrsModified;
}

bool PeHandler::rewrapDataDirs()
{
	bool anyModified = false;

	//std::cout << "Rewrapped: ";
	for (size_t i = 0; i < DIR_ENTRIES_COUNT; i++) {
		if (!dataDirWrappers[i]) continue;

		bool result = dataDirWrappers[i]->wrap();
		if (result) {
			//std::cout << i << " ";
			anyModified = true;
		}
	}
	//std::cout <<  "\n";
	this->wrapAlbum();
	return anyModified;
}


bool PeHandler::updatePeOnModified(offset_t modO, bufsize_t modSize)// throws exception
{
	if (!m_PE) return false;

	m_PE->wrap();

	bool isSecHdrModified = false;
	bool baseHdrModified = false;

	if (modO != INVALID_ADDR && modSize) { // if modification offset is specified
		if (isBaseHdrModif(modO, modSize)) {
			baseHdrModified = true;
			isSecHdrModified = true;
		}
		if (this->m_PE->getSectionsCount() != m_PE->hdrSectionsNum()) {
			isSecHdrModified = true;
		}
		if (isSectionsHeadersModified(modO, modSize)) {
			isSecHdrModified = true;
		}
	}
	else {
		// if the modification offset is unknown, assume modified:
		baseHdrModified = true;
		isSecHdrModified = true;
	}

	if (baseHdrModified) {
		fileHdrWrapper.wrap();
		optHdrWrapper.wrap();
	}

	rewrapDataDirs();

	runHashesCalculation();

	if (isSecHdrModified) {
		emit secHeadersModified();
	}
	return true;
}

void PeHandler::updatePeOnResized()
{
	rewrapDataDirs();
	runHashesCalculation();

	emit secHeadersModified();
}


void PeHandler::runHashesCalculation()
{
	for (int i = 0; i < CalcThread::HASHES_NUM; i++) {
		CalcThread::hash_type hType = static_cast<CalcThread::hash_type>(i);
		calculateHash(hType);
	}
}

void PeHandler::unModify()
{
	if (this->modifHndl.undoLastOperation()) {
		try {
			updatePeOnModified();
		}
		catch (CustomException &e)
		{
			std::cerr << "Failed to update PE on modification: " << e.what() << std::endl;
		}
		emit modified();
	}
}

bool PeHandler::markedBranching(offset_t cRva, offset_t tRva)
{
	if (m_PE == NULL) return false;
	//validate...
	if (m_PE->toRaw(cRva, Executable::RVA) == INVALID_ADDR) return false;
	if (m_PE->toRaw(tRva, Executable::RVA) == INVALID_ADDR) return false;

	markedTarget = cRva;
	markedOrigin = tRva;

	emit marked();
	return true;
}
