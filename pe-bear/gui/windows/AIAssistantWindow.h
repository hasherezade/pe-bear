#pragma once

#include <QtWidgets>
#include "../../base/AIAnalyzer.h"
#include "../../base/AISettings.h"
#include "../../base/PeHandler.h"

class AIAssistantWindow : public QDockWidget
{
	Q_OBJECT

public:
	explicit AIAssistantWindow(AISettings *settings, QWidget *parent = nullptr);
	~AIAssistantWindow();
	
	void setAutoAnalyzeOnLoad(bool enabled);

public slots:
	void setCurrentPeHandler(PeHandler *peHandler);
	void onAnalyzeImports();
	void onAnalyzeExports();
	void onAnalyzeStrings();
	void onDetectMalware();
	void onAnalyzeVulnerabilities();
	void onAnalyzeSelectedCode();
	void onScanWithVirusTotal();
	void performAutoAnalysis();
	void onPeFileLoaded();

protected:
	void keyPressEvent(QKeyEvent *event) override;

private slots:
	void onAnalysisComplete(const QString &result);
	void onAnalysisError(const QString &error);
	void onProgressUpdate(const QString &status);
	void onClearResults();
	void onCopyResults();
	void onSaveResults();
	void onSendMessage();

private:
	void setupUI();
	void addMessage(const QString &type, const QString &text);
	void addWelcomeMessage();
	void addTypingIndicator();
	void removeTypingIndicator();
	QPushButton* createQuickButton(const QString &text, const QString &tooltip);
	
	// Agent functions - auto gather and analyze PE context
	// Gathers general information about the current PE file.
	QString gatherPEInfo();
	// Extracts information about the sections in the given PE file.
	QString extractSectionInfo(PEFile *pe);
	// Extracts import table details from the given PE file.
	QString extractImportInfo(PEFile *pe);
	// Extracts export table details from the given PE file.
	QString extractExportInfo(PEFile *pe);
	// Checks and summarizes security features present in the given PE file.
	QString checkSecurityFeatures(PEFile *pe);
	// Extracts a sample of strings found in the given PE file.
	QString extractStringSample(PEFile *pe);
	// Generates a summary report of the PE file's key characteristics.
	QString generatePESummary();
	
	AISettings *aiSettings;
	AIAnalyzer *aiAnalyzer;
	PeHandler *currentPeHandler;
	
	// UI Components
	QWidget *centralWidget;
	QScrollArea *chatScrollArea;
	QWidget *chatContainer;
	QVBoxLayout *chatLayout;
	QTextEdit *messageInput;
	QPushButton *sendBtn;
	QPushButton *analyzeImportsBtn;
	QPushButton *analyzeExportsBtn;
	QPushButton *analyzeStringsBtn;
	QPushButton *detectMalwareBtn;
	QPushButton *vulnAnalysisBtn;
	QPushButton *virusTotalBtn;
	QPushButton *autoAnalyzeBtn;
	QLabel *statusLabel;
	QProgressBar *progressBar;
	QWidget *typingWidget;
	
	bool isAgentMode;
	bool autoAnalysisEnabled;
	int pendingAnalysisSteps;
};
