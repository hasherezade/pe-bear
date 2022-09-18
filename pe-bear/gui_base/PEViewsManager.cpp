#include "PEViewsManager.h"

PEDockedWidget* PEViewsManager::getPeDockWidget(PeHandler* peHndl)
{
	if (!peHndl) return NULL;
	std::map<PeHandler*, PEDockedWidget*>::iterator found = this->PeViews.find(peHndl);
	if (found != this->PeViews.end()) {
		return found->second;
	}

	PEDockedWidget* p = new PEDockedWidget(peHndl, this);
	if (!p) return NULL; //should not happen
	
	p->hide();
	connect(this, SIGNAL(signalChangeHexViewSettings(HexViewSettings &)), p, SLOT(changeHexViewSettings(HexViewSettings &)) );
	connect(this, SIGNAL(signalChangeDisasmViewSettings(DisasmViewSettings &)), p, SLOT(changeDisasmViewSettings(DisasmViewSettings &)) );
	connect(this, SIGNAL(globalFontChanged()), p, SLOT(refreshFonts()) );
	
	p->changeHexViewSettings(this->hexSettings);
	p->changeDisasmViewSettings(this->disasmSettings);

	p->setAllowedAreas(Qt::AllDockWidgetAreas);// Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	p->setFloating(false);
	p->setFeatures( QDockWidget::DockWidgetMovable
	   | QDockWidget::DockWidgetClosable
	   | QDockWidget::DockWidgetFloatable 
	   | QDockWidget::DockWidgetVerticalTitleBar
	);
	this->addDockWidget(Qt::RightDockWidgetArea, p, Qt::Vertical);
	if (lastDock.size()) {
		this->tabifyDockWidget(lastDock.back(), p);
		p->setTabOrder(p, lastDock.back());
	}
	lastDock.push_back(p);
	PeViews[peHndl] = p;
	p->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	p->goToEntryPoint();
	return p;
}

bool PEViewsManager::removePeDockWidget(PeHandler* peHndl)
{
	std::map<PeHandler*, PEDockedWidget*>::iterator found = this->PeViews.find(peHndl);
	if (found == this->PeViews.end()) return false;

	PEDockedWidget* peDock = found->second;
	if (peDock) peDock->close();
	this->PeViews.erase(found);

	std::vector<PEDockedWidget*>::iterator itr;
	for (itr = lastDock.begin(); itr != lastDock.end(); itr++) {
		if (*itr == peDock) {
			lastDock.erase(itr);
			break;
		}
	}
	delete peDock;
	return true;
}

void PEViewsManager::clear()
{
	std::map<PeHandler*, PEDockedWidget*>::iterator vItr;
	for (vItr = this->PeViews.begin(); vItr != this->PeViews.end(); vItr++) {
		PeHandler* hndl = vItr->first;
		PEDockedWidget *peDock = vItr->second;
		if (peDock) peDock->close();
		delete peDock;
	}
	this->PeViews.clear();
}

PEViewsManager::~PEViewsManager()
{
	clear();
}
