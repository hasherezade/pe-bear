#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../../base/PeHandlersManager.h"

//---
class PatternSearchWindow : public QDialog
{
	Q_OBJECT

public:
	PatternSearchWindow(QWidget *parent, offset_t offset, offset_t maxOffset);
	~PatternSearchWindow() { }
	
	QString getSignature();

protected slots:
	void accept();

protected:
	void fetchSignature();

	QVBoxLayout topLayout;
	QHBoxLayout secPropertyLayout2;
	QHBoxLayout secPropertyLayout3;
	QHBoxLayout buttonLayout;

	QLabel patternLabel;
	QLineEdit patternEdit;

	QLabel offsetLabel;
	QSpinBox startOffset;

	QDialogButtonBox buttonBox;
	QString signPattern;
};
