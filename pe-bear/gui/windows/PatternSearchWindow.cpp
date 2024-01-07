#include "PatternSearchWindow.h"

PatternSearchWindow::PatternSearchWindow(QWidget *parent, PeHandler* peHndl)
	: QDialog(parent, Qt::Dialog), m_peHndl(peHndl),
	buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
	signPattern("")
{
	//setWindowFlags(Qt::Dialog);
	setModal(true);
	setWindowTitle(tr("Define search"));
	offsetLabel.setText(tr("Starting from the offset:" ));
	secPropertyLayout2.addWidget(&offsetLabel);
	secPropertyLayout2.addWidget(&startOffset);
	
	startOffset.setDisplayIntegerBase(16);
	startOffset.setRange(0, 0);
	startOffset.setPrefix("0x");
	startOffset.setValue(0);

	QRegExpValidator *validator = new QRegExpValidator(QRegExp("([0-9A-Fa-f\\?]{0,2} {0,1})*"), this);
	patternEdit.setValidator(validator);
	patternEdit.setToolTip("Hexadecimal with wild characters, i.e. \"55 8B ?? 8B 45 0C\"");
	patternLabel.setText(tr("Signature to search:"));

	patternLabel.setBuddy(&patternEdit);
	secPropertyLayout3.addWidget(&patternLabel);
	secPropertyLayout3.addWidget(&patternEdit);

	topLayout.addLayout(&secPropertyLayout2);
	topLayout.addLayout(&secPropertyLayout3);
	topLayout.addLayout(&buttonLayout);

	topLayout.addStretch();
	setLayout(&topLayout);

	buttonLayout.addWidget(&buttonBox);
	connect(&buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(&buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	patternEdit.setFocus();
}

QString PatternSearchWindow::getSignature()
{
	return this->signPattern;
}

QString PatternSearchWindow::fetchSignature()
{
	this->signPattern = patternEdit.text();
	return this->signPattern;
}

void PatternSearchWindow::accept()
{
	QString text = fetchSignature();;
	if (!text.length()) {
		return;
	}
	
	if (!this->m_peHndl) return;
	PEFile* peFile = m_peHndl->getPe();
	if (!peFile) return;
	
	offset_t maxOffset = peFile->getContentSize();
	offset_t offset = m_peHndl->getDisplayedOffset();
	
	size_t fullSize = peFile->getContentSize();
	if (offset >= fullSize) return;

	SignFinderThread *thread = new SignFinderThread(peFile, offset);
	if (!thread->signFinder.loadSignature("Searched", text.toStdString())) {
		QMessageBox::information(this, tr("Info"), tr("Could not parse the signature!"), QMessageBox::Ok);
		delete thread;
		return;
	}
	connect(thread, SIGNAL(gotMatches(SignFinderThread* )), 
		this, SLOT(matchesFound(SignFinderThread *)), Qt::UniqueConnection);
	thread->start();
	//QDialog::accept();
}

void PatternSearchWindow::matchesFound(SignFinderThread *thread)
{
	if (!thread || !thread->packerAtOffset.size()) {
		QMessageBox::information(this, tr("Info"), tr("Signature not found!"), QMessageBox::Ok);
		delete thread;
		return;
	}
	const QList<sig_ma::FoundPacker> &signAtOffset = thread->packerAtOffset;
	if (!signAtOffset.size()) {
		QMessageBox::information(this, tr("Info"), tr("Signature not found!"), QMessageBox::Ok);
		delete thread;
		return;
	}
	const sig_ma::FoundPacker &pckr = *(signAtOffset.begin());
	m_peHndl->setDisplayed(false, pckr.offset, pckr.signaturePtr->length());
	m_peHndl->setHilighted(pckr.offset, pckr.signaturePtr->length());
	startOffset.setValue(pckr.offset);
	if (QMessageBox::question(this, tr("Info"), tr("Signature found at:") + " 0x" + QString::number(pckr.offset, 16) + "\n"+ 
		tr("Search next?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
	{
		delete thread;
		return;
	}
	thread->setStartOffset(pckr.offset + 1);
	thread->start();
}
