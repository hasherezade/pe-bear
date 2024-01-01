#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include <bearparser/bearparser.h>

//---
class PatternSearchWindow : public QDialog
{
	Q_OBJECT

public:
	PatternSearchWindow(QWidget *parent);
	~PatternSearchWindow() { }

	QString getSignature();

	void exec(offset_t offset, offset_t maxOffset)
	{
		startOffset.setRange(0, maxOffset);
		startOffset.setPrefix("0x");
		startOffset.setValue(offset);
		this->signPattern = "";
		QDialog::exec();
	}

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
