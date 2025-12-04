#pragma once
#include <QtGlobal>

#include "../../QtCompat.h"
#include "../../base/MainSettings.h"
#include "../../base/AISettings.h"

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
	void setAISettings(AISettings *settings);

	t_reload_mode getReloadMode();
	void setReloadMode(const t_reload_mode rMode);

protected slots:
	void refreshSettingsView();
	void onDirChose();

private slots:
	void onOkClicked();

protected:
	void showEvent(QShowEvent * ev) { refreshSettingsView(); QDialog::showEvent(ev); }

private:
	int getLanguageIndex(const QString &lang);
	int loadAvailableTranslations(const QString &rootDir);
	void setupAITab();

	QTabWidget *tabWidget;
	QWidget *generalTab;
	QWidget *aiTab;
	
	QVBoxLayout topLayout;
	QPushButton dirButton, okButton, cancelButton;
	QLabel uddDirLabel;
	DirEdit uddDirEdit;
	QLabel languageLabel;
	QComboBox languageEdit;

	QLabel reloadFileLabel;
	QComboBox reloadFileStates;
	QCheckBox autoSaveTagsCBox;

	// AI Settings widgets
	QLineEdit *openaiKeyEdit;
	QLineEdit *geminiKeyEdit;
	QLineEdit *virusTotalKeyEdit;
	QComboBox *openaiModelCombo;
	QComboBox *geminiModelCombo;
	QCheckBox *useOpenAICBox;
	QCheckBox *useGeminiCBox;

	MainSettings *settings;
	AISettings *aiSettings;
};

