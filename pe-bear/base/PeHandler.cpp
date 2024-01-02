#include <new>
#include "PeHandler.h"

#include "../base/PeHandlersManager.h"
#include <bearparser/bearparser.h>
#include "../disasm/PeDisasm.h"

#define MIN_STRING_LEN 5

using namespace sig_ma;
using namespace pe;

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
	signFinder(nullptr), 
	modifHndl(pe->getFileBuffer(), this),
	stringThread(nullptr), stringExtractQueued(false)
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
	connect(this, SIGNAL(modified()), this, SLOT(runHashesCalculation()));
	
	this->runStringsExtraction();
	connect(this, SIGNAL(modified()), this, SLOT(runStringsExtraction()));
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

bool PeHandler::hasDirectory(dir_entry dirNum) const
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
		if (calcThread[hType] && calcThread[hType]->isFinished()) {
			delete calcThread[hType];
			calcThread[hType] = nullptr;
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
		if (calcThread[hType]) {
			while (calcThread[hType]->isFinished() == false) {
				calcThread[hType]->wait();
			}
			delete calcThread[hType];
			calcThread[hType] = nullptr;
		}
	}
	// delete strign extraction threads
	if (this->stringThread) {
		while (stringThread->isFinished() == false) {
			stringThread->wait();
		}
		delete stringThread;
		stringThread = nullptr;
	}
}

bool PeHandler::runStringsExtraction()
{
	if (this->stringThread) {
		stringExtractQueued = true;
		return false; //previous thread didn't finished
	}
	this->stringThread = new StringExtThread(m_PE, MIN_STRING_LEN);
	QObject::connect(stringThread, SIGNAL(gotStrings(StringsCollection* )), this, SLOT(onStringsReady(StringsCollection* )));
	QObject::connect(stringThread, SIGNAL(finished()), this, SLOT(stringExtractionFinished()));
	stringThread->start();
	stringExtractQueued = false;
	return true;
}

void PeHandler::onStringsReady(StringsCollection* mapToFill)
{
	if (!mapToFill) {
		return;
	}
	mapToFill->incRefCntr();
	this->stringsMap.fill(*mapToFill);
	mapToFill->release();
	stringsUpdated();
}

void PeHandler::stringExtractionFinished()
{
	if (stringThread && stringThread->isFinished()) {
		delete stringThread;
		stringThread = nullptr;
	}
	if (stringExtractQueued) {
		runStringsExtraction();
	}
}

void PeHandler::calculateHash(CalcThread::hash_type type)
{
	if (type >= CalcThread::HASHES_NUM) return;
	
	if (calcThread[type]) {
		calcQueued[type] = true;
		return; //previous thread didn't finished
	}
	const char* content = (char*) m_PE->getContent();
	const offset_t size = m_PE->getRawSize();
	const bool isSizeAcceptable = (offset_t(int(size)) == size) ? true : false;

	if (!content || !size || !isSizeAcceptable) {
		hash[type] = "Cannot calculate!";
		return;
	}
	hash[type] = "Calculating...";
	offset_t checksumOffset = this->optHdrWrapper.getFieldOffset(OptHdrWrapper::CHECKSUM);
	this->calcThread[type] = new CalcThread(type, m_PE, checksumOffset);
	QObject::connect(calcThread[type], SIGNAL(gotHash(QString, int)), this, SLOT(onHashReady(QString, int)));
	QObject::connect(calcThread[type], SIGNAL(finished()), this, SLOT(onCalcThreadFinished()));
	calcThread[type]->start();
	calcQueued[type] = false;
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

	const size_t contentSize = m_PE->getRawSize();

	offset_t startingRaw = m_PE->toRaw(startAddr, aT);
	if (startingRaw == INVALID_ADDR) return NULL;

	sig_ma::matched matchedSet = signFinder->getMatching(content, contentSize, startingRaw, md);
	size_t foundCount = matchedSet.signs.size();
	if (foundCount == 0) return NULL;

	PckrSign* packer = NULL;
	for (auto sItr = matchedSet.signs.begin(); sItr != matchedSet.signs.end(); ++sItr) {
		packer = *sItr;
		if (!packer) continue;
		
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

size_t PeHandler::findSignatureInArea(offset_t rawOff, size_t areaSize, sig_ma::SigFinder &localSignFinder, std::vector<sig_ma::FoundPacker> &signAtOffset, bool isDeepSearch)
{
	if (!m_PE) return 0;
	
	for (size_t step = 0; step < areaSize; step++) {

		size_t size = areaSize - step;
		BYTE * content = m_PE->getContentAt(rawOff + step, Executable::RAW, size);
		if (!content) break;

		sig_ma::matched matchedSet = localSignFinder.getMatching(content, size, 0, sig_ma::FIXED);
		if (matchedSet.signs.size() == 0) continue;

		PckrSign *packer = *(matchedSet.signs.begin());
		if (!packer) continue;
		
		offset_t foundOffset = step + matchedSet.match_offset + rawOff;
		//printf("Found %s, at %x searching next...\n", packer->get_name().c_str(), foundOffset);

		step += matchedSet.match_offset;
		
		FoundPacker pckr(foundOffset , packer);
		std::vector<FoundPacker>::iterator itr = std::find(signAtOffset.begin(), signAtOffset.end(), pckr);
		if (itr != signAtOffset.end()) {
			//already exist
			FoundPacker &found = *itr;
			packer = found.signaturePtr;
			continue;
		} else {
			signAtOffset.push_back(pckr);
		}
		if (isDeepSearch == false) break;
	}
	return signAtOffset.size();
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

bool PeHandler::resize(bufsize_t newSize, bool continueLastOperation)
{
	try {
		this->backupResize(newSize, continueLastOperation);
	} catch (CustomException &e) {
		std::cerr << "Resize backup fail: " << e.what() << std::endl;
	}
	if (m_PE->resize(newSize)) {
		updatePeOnResized();
		emit modified();
		return true;
	}
	return false;
}

bool PeHandler::resizeImage(bufsize_t newSize)
{
	{ //scope0
		QMutexLocker lock(&m_UpdateMutex);
		if (!m_PE || m_PE->getImageSize() == newSize) return false; //nothing to change

		offset_t modOffset = this->optHdrWrapper.getFieldOffset(OptHdrWrapper::IMAGE_SIZE);
		bufsize_t modSize = this->optHdrWrapper.getFieldSize(OptHdrWrapper::IMAGE_SIZE);
		this->modifHndl.backupModification(modOffset, modSize, false);
		m_PE->setImageSize(newSize);
	} //!scope0
	
	updatePeOnResized();
	emit modified();
	return true;
}

bool PeHandler::setByte(offset_t offset, BYTE val)
{
	{//scope0
		QMutexLocker lock(&m_UpdateMutex);

		BYTE* contentPtr = m_PE->getContentAt(offset, 1);
		if (!contentPtr) {
			return false;
		}
		BYTE prev_val = contentPtr[0];
		if (prev_val == val) {
			return false; // nothing has changed
		}
		this->backupModification(offset, 1);
		contentPtr[0] = val;
	}//!scope0
	this->setBlockModified(offset, 1);
	return true;
}

#include <iostream>
bool PeHandler::isVirtualFormat()
{
	ImportDirWrapper* imp = m_PE->getImports();
	if (imp && imp->isValid()) {
		return false;
	}
	const size_t count = this->m_PE->getSectionsCount();
	if (!count) return false;
	
	bool isDump = false;
	SectionHdrWrapper *sec = this->m_PE->getSecHdr(0);
	offset_t v = sec->getVirtualPtr();
	offset_t r = sec->getRawPtr();
	if (r > v) return false; //this is an anomaly...
	if (v == r) return false;
	offset_t diff = v - r;
	if (m_PE->isAreaEmpty(r, diff)) {
		isDump = true;
	}
	return isDump;
}

bool PeHandler::isVirtualEqualRaw()
{
	const size_t count = this->m_PE->getSectionsCount();
	const offset_t modOffset = m_PE->secHdrsOffset();
	if (!count || modOffset == INVALID_ADDR) return true;
	
	for (size_t i = 0; i < count; i++) {
		SectionHdrWrapper *sec = this->m_PE->getSecHdr(i);
		if (!sec) break;
		if (sec->getVirtualPtr() != sec->getRawPtr()) {
			return false;
		}
	}
	return true;
}

bool PeHandler::copyVirtualSizesToRaw()
{
	const size_t count = this->m_PE->getSectionsCount();
	const offset_t modOffset = m_PE->secHdrsOffset();
	if (!count || modOffset == INVALID_ADDR) return false;
	
	size_t secHdrsSize = m_PE->secHdrsEndOffset() - modOffset;
	this->modifHndl.backupModification(modOffset, secHdrsSize, false);
	for (size_t i = 0; i < count; i++) {
		SectionHdrWrapper *sec = this->m_PE->getSecHdr(i);
		if (!sec) break;
		sec->setNumValue(SectionHdrWrapper::RPTR, sec->getVirtualPtr());
		sec->setNumValue(SectionHdrWrapper::RSIZE, sec->getContentSize(Executable::RVA, false));
	}

	bool isOk = false;
	uint64_t val = this->optHdrWrapper.getNumValue(OptHdrWrapper::SEC_ALIGN, &isOk);
	if (isOk) {
		this->modifHndl.backupModification(
			this->optHdrWrapper.getFieldOffset(OptHdrWrapper::FILE_ALIGN), 
			this->optHdrWrapper.getFieldSize(OptHdrWrapper::FILE_ALIGN),
			true
		);
		this->optHdrWrapper.setNumValue(OptHdrWrapper::FILE_ALIGN, val);
	}
	rewrapDataDirs();
	emit modified();
	emit secHeadersModified();
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
		const bufsize_t roundedRawEnd = buf_util::roundupToUnit(m_PE->getMappedSize(Executable::RAW), m_PE->getAlignment(Executable::RAW));
		const bufsize_t newSize = roundedRawEnd + rSize;
		this->modifHndl.backupResize(newSize, true);
		
		newSec = m_PE->addNewSection(name, rSize, vSize);
	} catch (CustomException &e) {
		this->modifHndl.unStoreLast();
		throw (e); // rethrow the exception to exit from the function
	}

	if (!newSec) { // in case if adding section has failed, but not because of an exception thrown
		this->modifHndl.unStoreLast();
		throw CustomException("Cannot add new section!");
	}
	emit modified();
	emit secHeadersModified();
	return newSec;
}

offset_t PeHandler::loadSectionContent(SectionHdrWrapper* sec, QFile &fIn, bool continueLastOperation)
{
	if (!sec) return 0;
	offset_t loaded = 0;
	offset_t modifOffset = INVALID_ADDR;
	bufsize_t modifSize = 0;
	{ //scope0
		QMutexLocker lock(&m_UpdateMutex);
		if (!m_PE) return 0;

		AbstractByteBuffer *buf = m_PE->getFileBuffer();
		if (!buf) return 0;
		
		modifOffset = sec->getRawPtr();
		modifSize = sec->getContentSize(Executable::RAW, true);
		if (modifOffset == INVALID_ADDR|| modifSize == 0) {
			return 0;
		}
		backupModification(modifOffset, modifSize, continueLastOperation);
		loaded = buf->substFragmentByFile(modifOffset, modifSize, fIn);
	} //!scope0
	setBlockModified(modifOffset, loaded);
	setDisplayed(false, modifOffset, modifSize);
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
		if (!continueLastOperation) {
			unbackupLastModification(); // operation was not performed, so remove the backup
		}
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

ImportEntryWrapper* PeHandler::_autoAddLibrary(const QString &name, size_t importedFuncsCount, size_t expectedDllsCount, offset_t &storageOffset, bool separateOFT, bool continueLastOperation)
{
	ImportDirWrapper* imports = dynamic_cast<ImportDirWrapper*> (m_PE->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_IMPORT));
	if (!imports) return NULL;
	
	ImportEntryWrapper* lastWr = dynamic_cast<ImportEntryWrapper*> (imports->getLastEntry());
	if (!lastWr) return NULL;
	
	if (storageOffset == 0) {
		//throw CustomException("The storage offset was not supplied!");
		return NULL;
	}
	// backup only the spacer:
	backupModification(lastWr->getOffset() + lastWr->getSize(), lastWr->getSize(), continueLastOperation);
	// get a new entry to be filled:
	ImportEntryWrapper* libWr = dynamic_cast<ImportEntryWrapper*> (imports->addEntry(NULL));
	if (libWr == NULL) {
		this->unModify();
		throw CustomException("Failed to add a DLL entry!");
		return NULL;
	}
	// search for a sufficient space for name and thunks:
	offset_t nameOffset = storageOffset;
	const size_t kThunkSize = _getThunkSize();
	const size_t kThunkSeries = separateOFT ? 2 : 1;
	const size_t kThunkSeriesSize = kThunkSize * (importedFuncsCount + 1); // leave space for X thunks + terminator
	const size_t kNameRecordPadding = kThunkSeriesSize * kThunkSeries; 
	const size_t nameTotalSize = name.length() + 1; // name + string '\0' terminator
	while (true) {
		BYTE *ptr = m_PE->getContentAt(nameOffset, nameTotalSize + kNameRecordPadding);
		if (!ptr) {
			this->unModify();
			throw CustomException("Failed to get a free space to fill the entry!");
			return NULL;
		}
		if (!pe_util::isSpaceClear(ptr, nameTotalSize + kNameRecordPadding)) {
			nameOffset++; //move the pointer...
			continue;
		}
		break;
	}
	// fill in the name
	backupModification(nameOffset, nameTotalSize, true);
	if (m_PE->setStringValue(nameOffset, name) == false) {
		this->unModify();
		throw CustomException("Failed to fill library name!");
		return NULL;
	}

	// move the storage offset after the filled name:
	storageOffset = nameOffset + nameTotalSize;

	// fill in the thunks lists pointers:
	offset_t origFirstThunk = 0;
	const offset_t firstThunk = m_PE->convertAddr(storageOffset, Executable::RAW, Executable::RVA);
	if (separateOFT) {
		origFirstThunk = m_PE->convertAddr((storageOffset + kThunkSeriesSize), Executable::RAW, Executable::RVA);
	}
	libWr->setNumValue(ImportEntryWrapper::FIRST_THUNK, firstThunk);
	libWr->setNumValue(ImportEntryWrapper::ORIG_FIRST_THUNK, origFirstThunk);
	libWr->wrap();

	// leave the space for a needed number of thunks:
	storageOffset += kNameRecordPadding;
	
	// fill in the name RVA:
	const offset_t nameRva = m_PE->convertAddr(nameOffset, Executable::RAW, Executable::RVA);

	backupModification(libWr->getFieldOffset(ImportEntryWrapper::NAME), libWr->getFieldSize(ImportEntryWrapper::NAME), true);
	libWr->setNumValue(ImportEntryWrapper::NAME, nameRva);

	return libWr;
}

bool PeHandler::_autoFillFunction(ImportEntryWrapper* libWr, ImportedFuncWrapper* fWr, const QString& name, const WORD ordinal, offset_t &storageOffset)
{
	if (m_PE == NULL || fWr == NULL) {
		return false;
	}

	backupModification(fWr->getOffset(), fWr->getSize(), true);
	if (name.length()) {
		if (!pe_util::validateFuncName(name.toStdString().c_str(), name.length())) {
			this->unModify();
			throw CustomException("Invalid function name supplied!");
		}
		const offset_t thunkRVA = m_PE->convertAddr(storageOffset, Executable::RAW, Executable::RVA);
		if (thunkRVA == INVALID_ADDR) {
			return false;
		}
		fWr->setNumValue(ImportedFuncWrapper::THUNK, thunkRVA);
		fWr->setNumValue(ImportedFuncWrapper::ORIG_THUNK, thunkRVA);
		fWr->setNumValue(ImportedFuncWrapper::HINT, ordinal);
		
		storageOffset += sizeof(WORD); //add sizeof Hint
		const size_t nameTotalLen = name.length() + 1;
		backupModification(storageOffset, nameTotalLen, true);
		if (m_PE->setStringValue(storageOffset, name) == false) {
			this->unModify();
			throw CustomException("Failed to fill the function name");
			return false;
		}
		storageOffset += nameTotalLen;
		return true;
	}
	// import by ordinal:
	const uint64_t ord_flag = m_PE->isBit32() ? ORDINAL_FLAG32 : ORDINAL_FLAG64;
	fWr->setNumValue(ImportedFuncWrapper::THUNK, ordinal ^ ord_flag);
	fWr->setNumValue(ImportedFuncWrapper::ORIG_THUNK, ordinal ^ ord_flag);
	return true;
}

bool PeHandler::autoAddImports(const ImportsAutoadderSettings &settings)
{
	const QStringList dllsList = settings.dllFunctions.keys();
	const size_t dllsCount = dllsList.size();
	if (dllsCount == 0) return false;

	const size_t kDllRecordsSpace = sizeof(IMAGE_IMPORT_DESCRIPTOR) * (dllsCount + 1); // space for Import descriptor for each needed DLL
	const bool shouldMoveTable = (canAddImportsLib(dllsCount)) ? false : true;
	
	const size_t impDirSize = this->getDirSize(pe::DIR_IMPORT);
	if (!impDirSize) {
		throw CustomException("Import Directory does not exist");
		return false;
	}
	const size_t newImpDirSize = (shouldMoveTable) ? (impDirSize + kDllRecordsSpace) : 0;
	const size_t thunksCount = settings.calcThunksCount();
	
	const size_t thunksSeries = settings.separateOFT ? 2 : 1;
	const size_t dllRecordsSize = settings.calcDllNamesSpace() + (thunksCount * this->_getThunkSize() * thunksSeries);
	const size_t funcRecordsSize = settings.calcFuncNamesSpace() + (sizeof(WORD) * thunksCount);

	const size_t SEC_PADDING = 10;
	size_t kNeededSize = newImpDirSize + dllRecordsSize + funcRecordsSize;
	if (!settings.addNewSec) kNeededSize += SEC_PADDING;
	size_t newImpSize = kNeededSize;
	
	SectionHdrWrapper *stubHdr = NULL;
	offset_t newImpOffset = INVALID_ADDR;
	bool continueLastOperation = false; //this is the first stored operation in the series
	if (settings.addNewSec) {
		QString name = "new_imp";
		stubHdr = this->addSection(name, newImpSize, newImpSize);
		if (!stubHdr) {
			throw CustomException("Cannot add a new section");
			return false;
		}
		continueLastOperation = true;
		newImpOffset = stubHdr->getRawPtr();
	} else {
		stubHdr = m_PE->getLastSection();
		if (stubHdr == NULL) {
			throw CustomException("Cannot fetch the last section!");
			return false;
		}
		// resize section
		const offset_t secROffset = stubHdr->getContentOffset(Executable::RAW, true);
		const offset_t realSecSize = m_PE->getContentSize() - secROffset; // the size including the eventual padding

		backupModification(stubHdr->getFieldOffset(SectionHdrWrapper::VSIZE), stubHdr->getFieldSize(SectionHdrWrapper::VSIZE), continueLastOperation);
		continueLastOperation = true;
		backupModification(stubHdr->getFieldOffset(SectionHdrWrapper::RSIZE), stubHdr->getFieldSize(SectionHdrWrapper::RSIZE), continueLastOperation);
		OptHdrWrapper* optHdr = dynamic_cast<OptHdrWrapper*>(m_PE->getWrapper(PEFile::WR_OPTIONAL_HDR));
		if (optHdr) {
			backupModification(optHdr->getFieldOffset(OptHdrWrapper::IMAGE_SIZE), optHdr->getFieldSize(OptHdrWrapper::IMAGE_SIZE), continueLastOperation);
		}
		// do the operation:
		this->modifHndl.backupResize(this->m_PE->getContentSize() + newImpSize, continueLastOperation);
		stubHdr = m_PE->extendLastSection(newImpSize);
		if (stubHdr == NULL) {
			throw CustomException("Cannot extend the last section!");
			return false;
		}
		newImpOffset  = secROffset + realSecSize + SEC_PADDING;
	}
	
	offset_t storageOffset = 0;
	
	if (shouldMoveTable) {
		if (newImpOffset == INVALID_ADDR) {
			return false;
		}
		backupModification(stubHdr->getFieldOffset(SectionHdrWrapper::CHARACT), stubHdr->getFieldSize(SectionHdrWrapper::CHARACT), continueLastOperation);
		continueLastOperation = true;
		const DWORD oldCharact = stubHdr->getCharacteristics();
		const DWORD requiredCharact = SCN_MEM_READ | SCN_MEM_WRITE;
		if (!stubHdr->setCharacteristics(oldCharact | requiredCharact)) {
			this->unModify();
			throw CustomException("Cannot modify the section characteristics");
			return false;
		}
		if (!this->_moveDataDirEntry(pe::DIR_IMPORT, newImpOffset, continueLastOperation)) {
			this->unModify();
			throw CustomException("Cannot move the data dir");
			return false;
		}
		// the table was moved, use the space just after the table for the storage:
		ImportDirWrapper* imports = dynamic_cast<ImportDirWrapper*> (m_PE->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_IMPORT));
		if (imports) {
			storageOffset = imports->getOffset() + imports->getContentSize() + kDllRecordsSpace;
		}
	} else {
		// the table was NOT moved, use the newly added space directly for the new records:
		storageOffset = newImpOffset;
	}

	QMap<QString, ImportEntryWrapper*> addedWrappers;

	for (auto itr = dllsList.begin(); itr != dllsList.end(); ++itr) {
		
		QString library = *itr;
		const size_t funcCount = settings.dllFunctions[library].size();
		if (!funcCount) continue;
		
		ImportEntryWrapper* libWr = _autoAddLibrary(library, funcCount, dllsCount, storageOffset, settings.separateOFT, continueLastOperation);
		if (libWr == NULL) {
			throw CustomException("Adding library failed!");
			return false;
		}
		continueLastOperation = true;
		addedWrappers[library] = libWr;
	}

	for (auto itr = addedWrappers.begin(); itr != addedWrappers.end(); ++itr) {
		ImportEntryWrapper* libWr = itr.value();
		const QString library = itr.key();
		for (auto fItr = settings.dllFunctions[library].begin(); fItr != settings.dllFunctions[library].end(); ++fItr) {
			ImportedFuncWrapper* func = _addImportFunc(libWr, continueLastOperation);
			if (!func) {
				break;
			}
			continueLastOperation = true;
			const QString funcName = *fItr;

			if (funcName.startsWith("#")) {
				QString ordStr = funcName.mid(1); 
				const int ordinal = ordStr.toInt();
				_autoFillFunction(libWr, func, "", ordinal, storageOffset);
			} else {
				_autoFillFunction(libWr, func, funcName, 0, storageOffset);
			}
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
		this->unModify();
		throw CustomException("Failed to add imported function!");
		return NULL;
	}
	return nextFunc;
}

bool PeHandler::addImportFunc(size_t libNum)
{
	ImportEntryWrapper *lib = dynamic_cast<ImportEntryWrapper*>(importDirWrapper.getEntryAt(libNum));
	ImportedFuncWrapper* func = NULL;
	try {
		func = _addImportFunc(lib);
	} catch (CustomException &e) {
		func = NULL;
	}
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

void PeHandler::backupResize(bufsize_t newSize, bool continueLastOperation)
{
	modifHndl.backupResize(newSize, continueLastOperation);
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

	bool isSecHdrModified = false;
	bool baseHdrModified = false;

	{ //scope0
		QMutexLocker lock(&m_UpdateMutex);
		m_PE->wrap();
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
	}//!scope0
	
	if (isSecHdrModified) {
		emit secHeadersModified();
	}
	return true;
}

void PeHandler::updatePeOnResized()
{
	{ //scope0
		QMutexLocker lock(&m_UpdateMutex);
		rewrapDataDirs();
	}//!scope0
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

bool PeHandler::exportDisasm(const QString &path, const offset_t startOff, const size_t previewSize)
{
	PEFile *pe = this->getPe();
	if (!pe) return false;
	
	if (!pe->getContentAt(startOff, previewSize)) {
		return false;
	}
	
	QFile fOut(path);
	if (fOut.open(QFile::WriteOnly | QFile::Text) == false) {
		return false;
	}
	
	pe_bear::PeDisasm myDisasm(pe, previewSize);
	myDisasm.init(startOff, pe->getBitMode());
	myDisasm.fillTable();

	QTextStream disasmStream(&fOut);
	for (int index = 0; index < myDisasm.chunksCount(); ++index ) {
		QString str = myDisasm.mnemStr(index);
		if (myDisasm.isBranching(index)) {
			str = myDisasm.translateBranching(index);
		}

		//resolve target functions:
		bool isOk = false;
		const offset_t tRva =  myDisasm.getTargetRVA(index, isOk);
		QString funcName = "";
		QString refStr = "";
		if (isOk) {
			funcName = importDirWrapper.thunkToFuncName(tRva, false);
			if (funcName.length() == 0 ) {
				funcName = delayImpDirWrapper.thunkToFuncName(tRva, false);
			}
			refStr = myDisasm.getStringAt(tRva);
		}
		
		offset_t VA = pe->rvaToVa(myDisasm.getRvaAt(index));
		QString vaStr = QString::number(VA, 16);
		
		// stream to the file:
		disasmStream << vaStr << " : " <<  str;
		if (funcName.length()) {
			disasmStream << " : " <<  funcName;
		}
		else if (refStr.length()) {
			disasmStream << " : " <<  refStr;
		}
		disasmStream << "\n";
		if (myDisasm.isBranching(index)) {
			disasmStream << "\n"; // add a separator line
		}
	}
	fOut.close();
	return true;
}