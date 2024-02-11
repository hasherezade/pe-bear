#include "PatternSearchWindow.h"

PatternSearchWindow::PatternSearchWindow(QWidget *parent, PeHandler* peHndl)
	: QDialog(parent, Qt::Dialog),
	m_peHndl(peHndl), threadMngr(nullptr)
{
	setModal(true);
	setWindowTitle(tr("Define search"));
	offsetLabel.setText(tr("Starting from the offset:" ));
	secPropertyLayout2.addWidget(&offsetLabel);
	secPropertyLayout2.addWidget(&startOffsetBox);

	startOffsetBox.setRange(0, 0);
	startOffsetBox.setPrefix("0x");
	startOffsetBox.setValue(0);

	QRegExpValidator *validator = new QRegExpValidator(QRegExp("([0-9A-Fa-f\\?]{0,2} {0,1})*"), this);
	patternEdit.setValidator(validator);
	patternEdit.setToolTip(tr("Hexadecimal with wild characters, i.e.")+ "\"55 8B ?? 8B 45 0C\"");
	patternLabel.setText(tr("Signature to search:"));

	patternLabel.setBuddy(&patternEdit);
	secPropertyLayout3.addWidget(&patternLabel);
	secPropertyLayout3.addWidget(&patternEdit);
	secPropertyLayout4.addWidget(&progressBar);
	
	topLayout.addLayout(&secPropertyLayout2);
	topLayout.addLayout(&secPropertyLayout3);
	topLayout.addLayout(&secPropertyLayout4);
	topLayout.addLayout(&buttonLayout);

	progressBar.setRange(0, 1000);
	progressBar.setVisible(false);
	topLayout.addStretch();
	setLayout(&topLayout);

	searchButton.setText(tr("Search"));
	buttonLayout.addWidget(&searchButton);
	connect(&searchButton, SIGNAL(clicked()), this, SLOT(onSearchClicked()));
	patternEdit.setFocus();
}

QString PatternSearchWindow::fetchSignature()
{
	return patternEdit.text();
}

void PatternSearchWindow::onSearchClicked()
{
	QString text = fetchSignature();;
	if (!text.length()) {
		return;
	}
	
	if (!this->m_peHndl) return;
	PEFile* peFile = m_peHndl->getPe();
	if (!peFile) return;

	offset_t maxOffset = peFile->getContentSize();
	offset_t offset = startOffsetBox.value();

	size_t fullSize = peFile->getContentSize();
	if (offset >= fullSize) return;
	
	if (!this->threadMngr) {
		threadMngr = new SignFinderThreadManager(peFile);
	}
	threadMngr->setStartOffset(offset);
	if (!threadMngr->loadSignature(tr("Searched"), text)) {
		QMessageBox::information(this, tr("Info"), tr("Could not parse the signature!"), QMessageBox::Ok);
		return;
	}
	connect(threadMngr, SIGNAL(gotMatches(MatchesCollection* )), 
		this, SLOT(matchesFound(MatchesCollection *)), Qt::UniqueConnection);
	connect(threadMngr, SIGNAL(progressUpdated(int )), 
		this, SLOT(onProgressUpdated(int )), Qt::UniqueConnection);
	connect(threadMngr, SIGNAL(searchStarted(bool )), 
		this, SLOT(onSearchStarted(bool )), Qt::UniqueConnection);
		
	progressBar.setVisible(true);
	progressBar.setValue(0);
	threadMngr->recreateThread();
}

void PatternSearchWindow::onProgressUpdated(int progress)
{
	progressBar.setValue(progress);
}

void PatternSearchWindow::onSearchStarted(bool isStarted)
{
	searchButton.setEnabled(!isStarted);
}

void PatternSearchWindow::matchesFound(MatchesCollection *matches)
{
	if (!threadMngr) return; //should never happen
	
	if (!matches || !matches->packerAtOffset.size()) {
		QMessageBox::information(this, tr("Info"), tr("Signature not found!"), QMessageBox::Ok);
		threadMngr->stopThread();
		return;
	}
	const QList<MatchedSign> &signAtOffset = matches->packerAtOffset;
	if (!signAtOffset.size()) {
		QMessageBox::information(this, tr("Info"), tr("Signature not found!"), QMessageBox::Ok);
		threadMngr->stopThread();
		return;
	}
	MatchedSign match = *(signAtOffset.begin());
	const size_t offset = match.offset;
	const size_t signLen = match.len;
	m_peHndl->setDisplayed(false, offset, signLen);
	m_peHndl->setHilighted(offset, signLen);
	startOffsetBox.setValue(offset);
	if (QMessageBox::question(this, tr("Info"), tr("Signature found at:") + " 0x" + QString::number(offset, 16) + "\n"+ 
		tr("Search next?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
	{
		threadMngr->stopThread();
		return;
	}
	threadMngr->setStartOffset(offset + 1);
	threadMngr->recreateThread();
}
