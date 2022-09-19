#include "ResourceDirSplitter.h"
#include <QDataStream>
#include <QtGlobal>

#include "../TempBuffer.h"

using namespace pe;

#pragma pack(push,1)
#pragma pack(2)
typedef struct icon_hdr
{
	DWORD bitmap_offset;
	DWORD img_width;
	DWORD img_height;
	WORD color_planes;
	WORD colors_in_palette;
	DWORD reserved1;
	DWORD unk1;
} t_icon_hdr;

typedef struct cursor_hdr
{
	DWORD unk;
	t_icon_hdr icons;
} t_cursor_hdr;

typedef struct bmp_hdr
{
	WORD bmp_magic;
	DWORD bmp_size;
	WORD reserved1;
	WORD reserved2;
	DWORD start_offset;
} t_bmp_hdr;
#pragma pack(pop)// revert to default

void dump_icon_hdr(t_icon_hdr &hdr)
{
	std::cout << "bitmap_offset: " << std::dec << hdr.bitmap_offset << std::endl;
	std::cout << "img_width: " << std::dec << hdr.img_width << std::endl;
	std::cout << "img_height: " << std::dec << hdr.img_height << std::endl;
	std::cout << "color_planes: " << std::dec << hdr.color_planes << std::endl;
	std::cout << "colors_in_palette: " << std::dec << hdr.colors_in_palette << std::endl;
	std::cout << "\n";
}

//----------------------------------

ResourcesDirSplitter::ResourcesDirSplitter(PeHandler *peHndl, WrapperTableModel *upModel, WrapperTableModel *downModel, QWidget *parent)
	: DataDirWrapperSplitter(peHndl, pe::DIR_RESOURCE, parent), contentTab(-1), 
	saveAction(NULL)
{
	this->init(upModel, downModel);
}

ResourcesDirSplitter::~ResourcesDirSplitter()
{
	this->dock.setParent(this); // detach from leafTab
}

void ResourcesDirSplitter::normalizeSelectedId(size_t parentRowId)
{
	if (parentRowId >= ResourceDirWrapper::FIELD_COUNTER) {
		parentRowId -= ResourceDirWrapper::FIELD_COUNTER;
	} else {
		parentRowId = 0;
	}
	emit parentIdSelected(parentRowId);
}

void ResourcesDirSplitter::childIdSelected(int childId)
{
	ResourceLeafModel* leafModel = dynamic_cast<ResourceLeafModel*> (downModel);
	if (leafModel == NULL) return;
	
	leafModel->setLeafId(childId);
}

void ResourcesDirSplitter::changeView(bool isPix)
{
	this->contentPixmap.setVisible(isPix);
	this->contentText.setVisible(!isPix);
}

void setPixMap(QPixmap &mpix, QTextEdit &contentPixmap)
{
	QImage image = mpix.toImage();
	QTextCursor cursor = contentPixmap.textCursor();
	cursor.insertImage(image);
}

bool ResourcesDirSplitter::displayBitmap(ResourceContentWrapper *resContent)
{
	QByteArray arr;
	arr.setRawData((const char*)resContent->getPtr(), resContent->getSize());
	QPixmap mpix;
	bool isOk = false;
	if (mpix.loadFromData(arr, 0, Qt::AutoColor)) {
		setPixMap(mpix, contentPixmap);
		isOk = true;
	}
	return isOk;
}

bool ResourcesDirSplitter::displayText(ResourceContentWrapper *resContent)
{
	const char *content = (const char*)(resContent->getPtr());
	const size_t contentSize = resContent->getSize();
	if (!content || !contentSize) return false;

	QByteArray arr;
	arr.setRawData(content, contentSize);
	
	QString text(arr);
	if (text.size() == 0) return false;
	
	this->contentText.setPlainText(text);
	this->leafTab.setToolTip("");
	return true;
}


bool ResourcesDirSplitter::displayIcon(ResourceContentWrapper *resContent, const pe::resource_type &restype)
{
	const char *contentPtr = (const char*)(resContent->getPtr());
	const size_t contentSize = resContent->getSize();
	char *content = (char*)contentPtr;

	TempBuffer tmp;
	size_t resOffset = 0;
	
	const size_t iconHdrSize = sizeof(t_icon_hdr);
	if (!content || contentSize <= iconHdrSize) {
		return false;
	}
	
	const t_icon_hdr *iconHdr = NULL;
	DWORD bitmapOffset = 0;
	if (restype == pe::RESTYPE_CURSOR) {
		const t_cursor_hdr *cursorHdr = (t_cursor_hdr *)content;
		iconHdr = &cursorHdr->icons;
		if (iconHdr) {
			resOffset = sizeof(DWORD);
			bitmapOffset = iconHdr->bitmap_offset + resOffset;
		}
	} else {
		iconHdr = (t_icon_hdr *)content;
		if (iconHdr) {
			bitmapOffset = iconHdr->bitmap_offset;
		}
	}
	if (!iconHdr) return false;

	if (bitmapOffset >= contentSize || resOffset >= contentSize) {
		return false;
	}

	size_t colors = iconHdr->colors_in_palette;
	switch (colors) {
		case 64:
		case 32:
		case 24:
		case 16:
		case 8:
		case 4:
		case 2:
		case 1:
			break;
		default:
			return false; // invalid color set
	}

	DWORD dimH = iconHdr->img_height;
	DWORD dimW = iconHdr->img_width;

	if (restype == pe::RESTYPE_ICON) {
		dimH = dimW;
		if (tmp.init((const BYTE*)contentPtr, contentSize)) {
			//replace pointer to the content by a temporay buffer:
			content = (char*)tmp.getContent();

			// overwrite the header in the temporary buffer:
			t_icon_hdr* iconHdr2 = (t_icon_hdr *)content;
			iconHdr2->img_height = dimH;
		}
	}

	QByteArray arr;
	bool isOk = false;
	if (colors == 32) {
		const char* bitmapPtr = (const char*)((ULONGLONG)content + bitmapOffset);
		const size_t bitmapSize = contentSize - bitmapOffset;
		
		arr.append(bitmapPtr, bitmapSize);

		QImage img = QImage((const uchar *)arr.constData(), dimW, dimH, QImage::Format_ARGB32);
		QPixmap mpix = QPixmap::fromImage(img.mirrored(false, true));
		setPixMap(mpix, contentPixmap);
		isOk = true;
	}
	else {
		const char* bitmapPtr = (const char*)((ULONGLONG)content + resOffset);
		const size_t bitmapSize = contentSize - resOffset;

		t_bmp_hdr hdr1 = { 0 };
		hdr1.bmp_magic = 0x4D42;
		hdr1.bmp_size = sizeof(t_bmp_hdr) + bitmapSize;
		hdr1.start_offset = bitmapOffset;

		arr.append((const char*)&hdr1, sizeof(t_bmp_hdr));
		arr.append(bitmapPtr, bitmapSize);

		QPixmap mpix;
		if (mpix.loadFromData(arr, 0, Qt::AutoColor)) {
			setPixMap(mpix, contentPixmap);
			isOk = true;
		}
	}
	if (!isOk) {
		return false;
	}
	QString info = "Dimensions: " + QString::number(dimW, 10) + " x " + QString::number(dimH, 10) 
		+ "\n" + "Colors: " + QString::number(colors, 10) + " bit";

	this->pixmapInfo.setText(info);
	this->pixmapInfo.setVisible(true);
	return true;
}

void ResourcesDirSplitter::clearContentDisplay()
{
	this->pixmapInfo.setVisible(false);
	this->contentPixmap.setVisible(false);
	this->contentPixmap.setText("");
	this->contentText.setVisible(false);
	this->contentText.setText("");
}

void ResourcesDirSplitter::refreshLeafContent()
{
	bool isOk = false;
	clearContentDisplay();
	
	ResourceLeafModel* leafModel = dynamic_cast<ResourceLeafModel*> (downModel);
	if (leafModel == NULL) return;

	size_t childId = leafModel->getLeafId();
	int x = leafModel->getParentId();

	ResourceEntryWrapper* eDir = dynamic_cast<ResourceEntryWrapper*> (myPeHndl->resourcesDirWrapper.getEntryAt(x));
	if (eDir == NULL) return; //should never happen!

	std::vector<ResourceLeafWrapper*> *vec = myPeHndl->resourcesAlbum.entriesAt(x);
	if (!vec || vec->size() <= childId) return;
	//----------------

	ResourceLeafWrapper* rsrc = vec->at(childId);
	if (!rsrc) return;
	
	ResourceContentWrapper *resContent = myPeHndl->resourcesAlbum.getContentWrapper(rsrc);
	if (!resContent) {
		return;
	}

	pe::resource_type dirType = resContent->getType();
	
	switch(dirType) {
		case 0:
		case RESTYPE_RCDATA:
		case RESTYPE_HTML :
		case RESTYPE_MANIFEST :
		{
			if (displayBitmap(resContent)) {
				isOk = true;
				changeView(true);
			} else {
				isOk = displayText(resContent);
				changeView(false);
			}
			break;
		}
		case RESTYPE_CURSOR:
		case RESTYPE_BITMAP:
		case RESTYPE_ICON: {
			if (displayBitmap(resContent)) {
				isOk = true;
				changeView(true);
			}
			else if (displayIcon(resContent, dirType)) {
				isOk = true;
				changeView(true);
			}
			if (isOk) {
				this->leafTab.setTabEnabled(this->contentTab, true);
				this->leafTab.setToolTip("Image Preview");
			}
			break;
		}
	}
	if (isOk) {
		this->leafTab.setTabEnabled(this->contentTab, true);
		return;
	}
	this->leafTab.setTabEnabled(this->contentTab, false);
	this->leafTab.setToolTip("Preview not implemented yet");
	this->contentText.setText("Error 501: Preview not implemented yet.");
}

void ResourcesDirSplitter::init(WrapperTableModel *upModel, WrapperTableModel *downModel)
{
	this->upModel = upModel;
	this->downModel = downModel;
	
	addWidget(&toolBar);
	initToolbar();

	if (upModel) {
		upTree.setModel(this->upModel);
		addWidget(&upTree);
		connect(&upTree, SIGNAL(parentIdSelected(size_t)), this, SLOT(normalizeSelectedId(size_t)));
		connect(this, SIGNAL(parentIdSelected(size_t)), this, SLOT(onUpIdSelected(size_t)));
		if (downModel) {
			connect(&upTree, SIGNAL(parentIdSelected(size_t)), this, SLOT(normalizeSelectedId(size_t)));
			connect(this, SIGNAL(parentIdSelected(size_t)), downModel, SLOT(setParentId(size_t)));
		}
	}
	//elementsList.setMaximumHeight(30);
	//elementsList.setMaxVisibleItems(5);
	this->addWidget(&elementsList);
	connect(&elementsList, SIGNAL(currentIndexChanged(int)), this, SLOT(childIdSelected(int)));
	connect(downModel, SIGNAL(modelUpdated()), this, SLOT(refreshLeafContent()));

	if (downModel) {
		int num = leafTab.addTab(&dock, "Table");
		this->contentTab = leafTab.addTab(&contentDock, "Content");
		dock.setFeatures(QDockWidget::NoDockWidgetFeatures);
		
		downTree.setModel(this->downModel);
		dock.setWidget(&this->downTree);
		dock.setWindowTitle("Resources");
		addWidget(&leafTab);
	}
	this->contentText.setReadOnly(true);
	this->contentPixmap.setReadOnly(true);
	this->contentDock.addWidget(&contentText);
	this->contentDock.addWidget(&contentPixmap);
	this->contentDock.addWidget(&pixmapInfo);
	refreshLeafContent();
}

void ResourcesDirSplitter::resizeComponents()
{
	const int iconDim = ViewSettings::getSmallIconDim(QApplication::font());
	QIcon saveIco = ViewSettings::makeScaledIcon(":/icons/save_black.ico", iconDim, iconDim);
	saveAction->setIcon(saveIco);

	const int MAX_HEIGHT = iconDim * 2;
	const int NUM_EDIT_HEIGHT = iconDim * 3;
	toolBar.setMaximumHeight(MAX_HEIGHT);
	elementsList.setMaximumHeight(NUM_EDIT_HEIGHT);
}

bool ResourcesDirSplitter::initToolbar()
{
	toolBar.clear();
	toolBar.setProperty("dataDir", true);
	saveAction = new QAction( QString("&Save entries"), this);
	connect(saveAction, SIGNAL(triggered()), this, SLOT(onSaveEntries()) );
	resizeComponents();
	toolBar.addAction(saveAction);
	return true;
}

void ResourcesDirSplitter::onSaveEntries()
{
	if (m_PE == NULL || myPeHndl == NULL) return;

	QString pathDir = chooseDumpOutDir();
	if (pathDir.length() == 0) return;
	
	size_t dumped = 0;
	size_t failed = 0;
	size_t invalid = 0;

	size_t dirsNum = myPeHndl->resourcesAlbum.dirsCount();

	for (int x = 0; x < dirsNum; x++) {
		std::vector<ResourceLeafWrapper*> *vec = myPeHndl->resourcesAlbum.entriesAt(x);
		if (!vec) break;
		size_t vecSize = vec->size();

		ResourceEntryWrapper* eDir = dynamic_cast<ResourceEntryWrapper*> (myPeHndl->resourcesDirWrapper.getEntryAt(x));
		if (eDir == NULL) continue; //should never happen!

		WORD dirType = eDir->getID();

		for (int y = 0; y < vecSize; y++) {
			ResourceLeafWrapper* rsrc = vec->at(y);
			if (rsrc == NULL) {
				invalid++;
				continue;
			}
			IMAGE_RESOURCE_DATA_ENTRY* entry = rsrc->leafEntryPtr();
			if (entry == NULL) {
				invalid++;
				continue;
			}
			offset_t dataRva = entry->OffsetToData;
			bufsize_t dataSize = entry->Size;
			Executable::addr_type aT = m_PE->detectAddrType(dataRva, Executable::RVA);

			offset_t dataOffset = m_PE->toRaw(dataRva, aT);
			if (dataOffset == INVALID_ADDR){
				invalid++;
				continue;
			}
#if QT_VERSION >= 0x050000
			QString fileName = QString::asprintf("_%x_%lx", x, dataRva);
#else
			QString fileName;
			fileName.sprintf("_%x_%lx", x, dataRva);
#endif
			QString path = pathDir + QDir::separator()
				+ ResourceEntryWrapper::translateType(dirType) 
				+ fileName;

			if (m_PE->dumpFragment(dataOffset, dataSize, path)) dumped++;
			else failed++;
		}
	}
	
	if (failed == 0) {
		QString infoStr = "Dumped " + QString::number(dumped) + " entries to: " + pathDir;
		if (invalid) infoStr += "\nUnable to dump entries: " + QString::number(invalid);
		QMessageBox::information(this,"Dumped", infoStr);
	} else {
		QMessageBox::warning(this, "Failed", "Failed to dump!");
	}
}

void ResourcesDirSplitter::onUpIdSelected(size_t upId)
{
	elementsList.clear();
	WrapperInterface *upInterface = dynamic_cast<WrapperInterface*>(upModel);
	if (upInterface == NULL) {
		return;
	}
	ExeNodeWrapper* node = dynamic_cast<ExeNodeWrapper*>(upInterface->wrapper());
	if (node == NULL) {
		return;
	}
	ExeNodeWrapper *childEntry = node->getEntryAt(upId);
	if (childEntry == NULL) {
		this->dock.setWindowTitle("-");
		return;
	}

	QString desc = childEntry->getName(); 
	this->dock.setWindowTitle(desc);

	size_t num = myPeHndl->resourcesAlbum.entriesCountAt(upId);
	fillList(num);
}

void ResourcesDirSplitter::fillList(size_t num)
{
	elementsList.clear();
	for (size_t i = 0; i < num; i++) {
		elementsList.addItem("Entry number: " + QString::number(i));
	}
}
