#include "PatternSearchWindow.h"

PatternSearchWindow::PatternSearchWindow(QWidget *parent, offset_t offset, offset_t maxOffset)
	: QDialog(0, Qt::Dialog),
	buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
	signPattern("")
{
	//setWindowFlags(Qt::Dialog);
	setModal(true);
	offsetLabel.setText(tr("Search starting from the offset:" ));
	secPropertyLayout2.addWidget(&offsetLabel);
	secPropertyLayout2.addWidget(&startOffset);

	startOffset.setDisplayIntegerBase(16);
	startOffset.setRange(0, maxOffset);
	startOffset.setPrefix("0x");
	startOffset.setValue(offset);

	QRegExpValidator *validator = new QRegExpValidator(QRegExp("([0-9A-Fa-f]{2} {0,1})*"), this);
	patternEdit.setValidator(validator);
	patternLabel.setText(tr("Signature to search:"));
	patternLabel.setBuddy(&patternEdit);
	secPropertyLayout3.addWidget(&patternLabel);
	secPropertyLayout3.addWidget(&patternEdit);

	topLayout.addLayout(&secPropertyLayout2);
	topLayout.addLayout(&secPropertyLayout3);
	topLayout.addLayout(&buttonLayout);

	topLayout.addStretch();
	setLayout(&topLayout);
	setWindowTitle(tr("Define search"));

	buttonLayout.addWidget(&buttonBox);
	connect(&buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(&buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString PatternSearchWindow::getSignature()
{
	return this->signPattern;
}

void PatternSearchWindow::fetchSignature()
{
	this->signPattern = patternEdit.text();
}

void PatternSearchWindow::accept()
{
	fetchSignature();
	this->close();
}
