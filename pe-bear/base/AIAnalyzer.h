#pragma once

#include <QtCore>
#include <QtNetwork>
#include "../base/AISettings.h"

class AIAnalyzer : public QObject
{
	Q_OBJECT

signals:
	void analysisComplete(const QString &result);
	void analysisError(const QString &error);
	void progressUpdate(const QString &status);

public:
	explicit AIAnalyzer(AISettings *settings, QObject *parent = nullptr);
	~AIAnalyzer();

	// Analysis types
	void analyzeCode(const QString &code, const QString &context = "");
	void analyzeBinaryData(const QByteArray &data, const QString &context = "");
	void explainFunction(const QString &functionCode, const QString &functionName = "");
	void detectMalware(const QByteArray &fileData, const QStringList &strings, const QStringList &imports);
	void suggestDeobfuscation(const QString &code);
	void analyzeImports(const QStringList &imports);
	void analyzeExports(const QStringList &exports);
	void analyzeSuspiciousStrings(const QStringList &strings);
	
	// Security Analysis
	void analyzeVulnerabilities(const QStringList &imports, const QStringList &strings, const QString &securityFeatures);
	void analyzeSecurityPosture(const QString &peContext);
	
	// VirusTotal integration
	void scanFileHash(const QString &hash);
	void uploadFileToVirusTotal(const QString &filePath);

private slots:
	void onOpenAIReplyFinished();
	void onGeminiReplyFinished();
	void onVirusTotalReplyFinished();

private:
	AISettings *aiSettings;
	QNetworkAccessManager *networkManager;
	
	void sendOpenAIRequest(const QString &prompt);
	void sendGeminiRequest(const QString &prompt);
	void sendVirusTotalRequest(const QString &endpoint, const QByteArray &data = QByteArray());
	
	QString formatPromptForReverseEngineering(const QString &content, const QString &analysisType);
	
	QMap<QNetworkReply*, QString> replyContextMap;
};
