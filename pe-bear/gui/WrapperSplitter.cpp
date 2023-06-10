#include "WrapperSplitter.h"

//---------------------------------------------------------------------------

WrapperSplitter::WrapperSplitter(QWidget *parent)
	: QSplitter(Qt::Vertical, parent), 
	upModel(NULL), downModel(NULL),
	upTree(NULL), downTree(NULL),
	dock(NULL),
	toolBar(this), title(""),
	recordId(0)
{
	//empty
}

WrapperSplitter::WrapperSplitter(PeTreeModel *upModel, PeTreeModel *downModel, QWidget *parent)
	: QSplitter(Qt::Vertical, parent), 
	upModel(NULL), downModel(NULL),
	upTree(NULL), downTree(NULL),
	dock(NULL),
	toolBar(this), title(""),
	recordId(0)
{ 
	init(upModel, downModel); 
}

void WrapperSplitter::init(PeTreeModel *upModel, PeTreeModel *downModel)
{
	this->upModel = upModel;
	this->downModel = downModel;

	toolBar.setMaximumHeight(20);
	addWidget(&toolBar);
	initToolbar();
	
	if (upModel) {
		upTree.setModel(this->upModel);
		this->upModel->setParent(&this->upTree);
		this->upTree.expandAll();
		addWidget(&upTree);
		connect(&upTree, SIGNAL(parentIdSelected(size_t)), this, SLOT(setParentIdInDocker(size_t)));
	}

	if (downModel) {
		connect(&upTree, SIGNAL(parentIdSelected(size_t)), downModel, SLOT(setParentId(size_t)));
		connect(downModel, SIGNAL(modelUpdated()), this, SLOT(reloadDocketTitle()));
		
		downTree.setModel(this->downModel);
		this->downModel->setParent(&downTree);
		downTree.expandAll();
		this->dock.setFeatures(QDockWidget::NoDockWidgetFeatures);
		dock.setWidget(&this->downTree);
		reloadDocketTitle();
		addWidget(&dock);
	}
}

void WrapperSplitter::reloadDocketTitle()
{
	if (!upModel) {
		dock.setWindowTitle("");
		return;
	}
	WrapperInterface *wrapperI = dynamic_cast<WrapperInterface*>(upModel);
	if (wrapperI == NULL) {
		return;
	}
	QString desc = wrapperI->makeDockerTitle(this->recordId);
	this->dock.setWindowTitle(desc);
}

void WrapperSplitter::setParentIdInDocker(size_t upId)
{
	this->recordId = upId;
	reloadDocketTitle();
}
