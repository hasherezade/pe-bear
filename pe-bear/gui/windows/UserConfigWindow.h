#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../../base/MainSettings.h"

//----------------------------------------------------

class DirEdit : public QLineEdit
{
	Q_OBJECT
public:
	DirEdit(QWidget* parent = 0);

protected slots:
	void validateDir(const QString &path);
};

//----------------------------------------------------

class UserConfigWindow : public QDialog
{
    Q_OBJECT

public:
	UserConfigWindow(QWidget *parent = 0);
	void setMainSettings(MainSettings *settings);

	t_reload_mode getReloadMode();
	void setReloadMode(const t_reload_mode rMode);

protected slots:
	void refrehSettingsView();
	void onDirChose();

private slots:
	void onOkClicked();

protected:
	void showEvent(QShowEvent * ev) { refrehSettingsView(); QDialog::showEvent(ev); }

private:
	QVBoxLayout topLayout;
	QPushButton dirButton, okButton, cancelButton;
	QLabel uddDirLabel;
	DirEdit uddDirEdit;
	QLabel languageLabel;
	QComboBox languageEdit;

	QLabel reloadFileLabel;
	QComboBox reloadFileStates;
	QCheckBox autoSaveTagsCBox;

	MainSettings *settings;
};

