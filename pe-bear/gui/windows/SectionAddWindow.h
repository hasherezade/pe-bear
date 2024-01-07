#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../../base/PeHandlersManager.h"
#include "../../gui_base/HexSpinBox.h"

class SectionAddWindow : public QDialog
{
	Q_OBJECT

public:
	SectionAddWindow(QWidget *parent);
	~SectionAddWindow() { }

public slots:
	void onAddSectionToPe(PeHandler *peHndl);
	void onOkClicked();
	void onFileChose();

protected:
	QVBoxLayout topLayout;
	QHBoxLayout secPropertyLayout;
	QHBoxLayout secPropertyLayout2;
	QHBoxLayout secPropertyLayout3;
	QHBoxLayout buttonLayout;

	QLabel secRsizeLabel;
	HexSpinBox secRsizeEdit;

	QLabel secVsizeLabel;
	HexSpinBox secVsizeEdit;
	
	QLabel secNameLabel;
	QLineEdit secNameEdit;

	enum access { READ = 0, WRITE, EXEC, ACCESS_NUM };
	QCheckBox fileCBox;
	QCheckBox rightsCBox[ACCESS_NUM];

	QPushButton fileButton, okButton, cancelButton;
	PeHandler *currPeHndl;
	QString filePath;
};
