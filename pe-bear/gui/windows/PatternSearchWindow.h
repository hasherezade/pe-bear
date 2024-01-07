#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include <bearparser/bearparser.h>

#include "../../base/PeHandler.h"
#include "../../base/threads/SignFinderThread.h"

//---
class PatternSearchWindow : public QDialog
{
	Q_OBJECT

public:
	PatternSearchWindow(QWidget *parent, PeHandler* peHndl);
	~PatternSearchWindow()
	{
		if (threadMngr) {
			delete threadMngr;
		}
	}

	int exec()
	{
		if (!m_peHndl) return QDialog::Rejected;

		PEFile* peFile = this->m_peHndl->getPe();
		if (!peFile) return QDialog::Rejected;

		offset_t maxOffset = peFile->getContentSize();
		offset_t offset = m_peHndl->getDisplayedOffset();

		startOffsetBox.setRange(0, maxOffset);
		startOffsetBox.setValue(offset);
		return QDialog::exec();
	}

protected slots:
	void onSearchClicked();
	void onStopClicked();

	void matchesFound(MatchesCollection *thread);
	void onProgressUpdated(int progress);

protected:
	QString fetchSignature();

	QVBoxLayout topLayout;
	QHBoxLayout secPropertyLayout2;
	QHBoxLayout secPropertyLayout3;
	QHBoxLayout secPropertyLayout4;
	QHBoxLayout buttonLayout;

	QLabel patternLabel;
	QLineEdit patternEdit;

	QLabel offsetLabel;
	QSpinBox startOffsetBox;
	QProgressBar progressBar;
	
	QPushButton searchButton;
	QPushButton stopButton;
	
	PeHandler* m_peHndl;
	SignFinderThreadManager *threadMngr;
};
