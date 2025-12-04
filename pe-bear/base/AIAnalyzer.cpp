#include "AIAnalyzer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QHttpMultiPart>

AIAnalyzer::AIAnalyzer(AISettings *settings, QObject *parent)
	: QObject(parent), aiSettings(settings)
{
	networkManager = new QNetworkAccessManager(this);
}

AIAnalyzer::~AIAnalyzer()
{
}

QString AIAnalyzer::formatPromptForReverseEngineering(const QString &content, const QString &analysisType)
{
	QString prompt;
	
	if (analysisType == "code") {
		prompt = "You are an expert reverse engineer. Analyze the following assembly/code and provide:\n"
				 "1. High-level explanation of what the code does\n"
				 "2. Potential security implications\n"
				 "3. Notable patterns or techniques\n"
				 "4. Suspicious behavior if any\n\n"
				 "Code:\n" + content;
	}
	else if (analysisType == "function") {
		prompt = "You are an expert reverse engineer. Explain this function in detail:\n"
				 "1. What does it do?\n"
				 "2. What are the parameters and return value?\n"
				 "3. Any security concerns?\n"
				 "4. Is there obfuscation or anti-analysis techniques?\n\n"
				 "Function:\n" + content;
	}
	else if (analysisType == "malware") {
		prompt = "You are a malware analyst. Analyze this PE file data and indicators:\n"
				 "1. Assess maliciousness level (1-10)\n"
				 "2. Identify malware family if possible\n"
				 "3. List suspicious indicators\n"
				 "4. Suggest analysis approach\n\n"
				 "Indicators:\n" + content;
	}
	else if (analysisType == "deobfuscation") {
		prompt = "You are an expert in code deobfuscation. Analyze this potentially obfuscated code:\n"
				 "1. Identify obfuscation techniques used\n"
				 "2. Suggest deobfuscation approach\n"
				 "3. Explain what the code likely does\n\n"
				 "Code:\n" + content;
	}
	else if (analysisType == "imports") {
		prompt = "You are a reverse engineer. Analyze these API imports:\n"
				 "1. What capabilities do these imports suggest?\n"
				 "2. Any dangerous or suspicious APIs?\n"
				 "3. What might the program do?\n\n"
				 "Imports:\n" + content;
	}
	else if (analysisType == "strings") {
		prompt = "You are a malware analyst. Analyze these strings extracted from a binary:\n"
				 "1. Identify suspicious strings (URLs, IPs, commands, etc.)\n"
				 "2. Suggest what they reveal about functionality\n"
				 "3. Rate suspiciousness (1-10)\n\n"
				 "Strings:\n" + content;
	}
	else {
		prompt = "Analyze the following from a reverse engineering perspective:\n\n" + content;
	}
	
	return prompt;
}

void AIAnalyzer::sendOpenAIRequest(const QString &prompt)
{
	if (aiSettings->getOpenAIApiKey().isEmpty()) {
		emit analysisError("OpenAI API key not configured");
		return;
	}

	QUrl url("https://api.openai.com/v1/chat/completions");
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("Authorization", QString("Bearer %1").arg(aiSettings->getOpenAIApiKey()).toUtf8());

	QJsonObject message;
	message["role"] = "user";
	message["content"] = prompt;

	QJsonArray messages;
	messages.append(message);

	QJsonObject json;
	json["model"] = aiSettings->getOpenAIModel();
	json["messages"] = messages;
	json["temperature"] = 0.7;
	json["max_tokens"] = 2000;

	QJsonDocument doc(json);
	QByteArray data = doc.toJson();

	QNetworkReply *reply = networkManager->post(request, data);
	connect(reply, &QNetworkReply::finished, this, &AIAnalyzer::onOpenAIReplyFinished);
}

void AIAnalyzer::sendGeminiRequest(const QString &prompt)
{
	if (aiSettings->getGeminiApiKey().isEmpty()) {
		emit analysisError("Gemini API key not configured");
		return;
	}

	// Use v1beta endpoint with proper model name
	QString model = aiSettings->getGeminiModel();
	// Migrate old model names to current ones
	if (model == "gemini-pro" || model == "gemini-1.5-flash" || model == "gemini-1.5-pro") {
		model = "gemini-2.0-flash";  // Update to current model
	}
	QString urlStr = QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
					 .arg(model)
					 .arg(aiSettings->getGeminiApiKey());
	
	QUrl url(urlStr);
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QJsonObject textPart;
	textPart["text"] = prompt;

	QJsonArray parts;
	parts.append(textPart);

	QJsonObject content;
	content["parts"] = parts;

	QJsonArray contents;
	contents.append(content);

	QJsonObject json;
	json["contents"] = contents;

	QJsonDocument doc(json);
	QByteArray data = doc.toJson();

	QNetworkReply *reply = networkManager->post(request, data);
	connect(reply, &QNetworkReply::finished, this, &AIAnalyzer::onGeminiReplyFinished);
}

void AIAnalyzer::onOpenAIReplyFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	if (reply->error() == QNetworkReply::NoError) {
		QByteArray response = reply->readAll();
		QJsonDocument doc = QJsonDocument::fromJson(response);
		QJsonObject obj = doc.object();
		
		if (obj.contains("choices")) {
			QJsonArray choices = obj["choices"].toArray();
			if (!choices.isEmpty()) {
				QJsonObject choice = choices[0].toObject();
				QJsonObject message = choice["message"].toObject();
				QString content = message["content"].toString();
				emit analysisComplete(content);
			}
		} else if (obj.contains("error")) {
			QString error = obj["error"].toObject()["message"].toString();
			emit analysisError("OpenAI Error: " + error);
		}
	} else {
		QString errorStr = reply->errorString();
		QByteArray responseData = reply->readAll();
		if (!responseData.isEmpty()) {
			QJsonDocument errDoc = QJsonDocument::fromJson(responseData);
			if (errDoc.object().contains("error")) {
				errorStr = errDoc.object()["error"].toObject()["message"].toString();
			}
		}
		emit analysisError("Network error: " + errorStr);
	}
	
	reply->deleteLater();
}

void AIAnalyzer::onGeminiReplyFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	QByteArray response = reply->readAll();
	
	if (reply->error() == QNetworkReply::NoError) {
		QJsonDocument doc = QJsonDocument::fromJson(response);
		QJsonObject obj = doc.object();
		
		if (obj.contains("candidates")) {
			QJsonArray candidates = obj["candidates"].toArray();
			if (!candidates.isEmpty()) {
				QJsonObject candidate = candidates[0].toObject();
				QJsonObject content = candidate["content"].toObject();
				QJsonArray parts = content["parts"].toArray();
				if (!parts.isEmpty()) {
					QString text = parts[0].toObject()["text"].toString();
					emit analysisComplete(text);
				} else {
					emit analysisError("Gemini returned empty response");
				}
			} else {
				emit analysisError("Gemini returned no candidates");
			}
		} else if (obj.contains("error")) {
			QString error = obj["error"].toObject()["message"].toString();
			emit analysisError("Gemini API Error: " + error);
		} else {
			emit analysisError("Unexpected Gemini response format");
		}
	} else {
		QString errorStr = reply->errorString();
		// Try to extract error message from response body
		if (!response.isEmpty()) {
			QJsonDocument errDoc = QJsonDocument::fromJson(response);
			if (errDoc.object().contains("error")) {
				errorStr = errDoc.object()["error"].toObject()["message"].toString();
			}
		}
		emit analysisError("Gemini Network Error: " + errorStr);
	}
	
	reply->deleteLater();
}

void AIAnalyzer::analyzeCode(const QString &code, const QString &context)
{
	emit progressUpdate("Analyzing code with AI...");
	QString prompt = formatPromptForReverseEngineering(code, "code");
	if (!context.isEmpty()) {
		prompt += "\n\nAdditional context: " + context;
	}
	
	if (aiSettings->isUseOpenAI()) {
		sendOpenAIRequest(prompt);
	} else if (aiSettings->isUseGemini()) {
		sendGeminiRequest(prompt);
	} else {
		emit analysisError("No AI provider enabled. Please configure API keys in settings.");
	}
}

void AIAnalyzer::analyzeBinaryData(const QByteArray &data, const QString &context)
{
	emit progressUpdate("Analyzing binary data...");
	QString hexData = data.toHex(' ');
	if (hexData.length() > 2000) {
		hexData = hexData.left(2000) + "... [truncated]";
	}
	analyzeCode(hexData, context);
}

void AIAnalyzer::explainFunction(const QString &functionCode, const QString &functionName)
{
	emit progressUpdate("Analyzing function...");
	QString prompt = formatPromptForReverseEngineering(functionCode, "function");
	if (!functionName.isEmpty()) {
		prompt = "Function: " + functionName + "\n\n" + prompt;
	}
	
	if (aiSettings->isUseOpenAI()) {
		sendOpenAIRequest(prompt);
	} else if (aiSettings->isUseGemini()) {
		sendGeminiRequest(prompt);
	} else {
		emit analysisError("No AI provider enabled.");
	}
}

void AIAnalyzer::detectMalware(const QByteArray &fileData, const QStringList &strings, const QStringList &imports)
{
	emit progressUpdate("Analyzing for malware indicators...");
	
	QString indicators;
	indicators += "File size: " + QString::number(fileData.size()) + " bytes\n\n";
	
	if (!imports.isEmpty()) {
		indicators += "Suspicious imports:\n";
		for (const QString &imp : imports) {
			indicators += "- " + imp + "\n";
		}
		indicators += "\n";
	}
	
	if (!strings.isEmpty()) {
		indicators += "Suspicious strings:\n";
		int count = 0;
		for (const QString &str : strings) {
			if (count++ > 50) {
				indicators += "... [truncated]\n";
				break;
			}
			indicators += "- " + str + "\n";
		}
	}
	
	QString prompt = formatPromptForReverseEngineering(indicators, "malware");
	
	if (aiSettings->isUseOpenAI()) {
		sendOpenAIRequest(prompt);
	} else if (aiSettings->isUseGemini()) {
		sendGeminiRequest(prompt);
	} else {
		emit analysisError("No AI provider enabled.");
	}
}

void AIAnalyzer::suggestDeobfuscation(const QString &code)
{
	emit progressUpdate("Analyzing obfuscation...");
	QString prompt = formatPromptForReverseEngineering(code, "deobfuscation");
	
	if (aiSettings->isUseOpenAI()) {
		sendOpenAIRequest(prompt);
	} else if (aiSettings->isUseGemini()) {
		sendGeminiRequest(prompt);
	} else {
		emit analysisError("No AI provider enabled.");
	}
}

void AIAnalyzer::analyzeImports(const QStringList &imports)
{
	emit progressUpdate("Analyzing imports...");
	QString importList;
	for (const QString &imp : imports) {
		importList += imp + "\n";
	}
	
	QString prompt = formatPromptForReverseEngineering(importList, "imports");
	
	if (aiSettings->isUseOpenAI()) {
		sendOpenAIRequest(prompt);
	} else if (aiSettings->isUseGemini()) {
		sendGeminiRequest(prompt);
	} else {
		emit analysisError("No AI provider enabled.");
	}
}

void AIAnalyzer::analyzeExports(const QStringList &exports)
{
	emit progressUpdate("Analyzing exports...");
	QString exportList;
	for (const QString &exp : exports) {
		exportList += exp + "\n";
	}
	
	analyzeCode(exportList, "These are exported functions from a PE file");
}

void AIAnalyzer::analyzeSuspiciousStrings(const QStringList &strings)
{
	emit progressUpdate("Analyzing strings...");
	QString stringList;
	int count = 0;
	for (const QString &str : strings) {
		if (count++ > 100) {
			stringList += "... [truncated]\n";
			break;
		}
		stringList += str + "\n";
	}
	
	QString prompt = formatPromptForReverseEngineering(stringList, "strings");
	
	if (aiSettings->isUseOpenAI()) {
		sendOpenAIRequest(prompt);
	} else if (aiSettings->isUseGemini()) {
		sendGeminiRequest(prompt);
	} else {
		emit analysisError("No AI provider enabled.");
	}
}

void AIAnalyzer::analyzeVulnerabilities(const QStringList &imports, const QStringList &strings, const QString &securityFeatures)
{
	emit progressUpdate("Analyzing for security vulnerabilities...");
	
	QString context;
	context += "=== VULNERABILITY ANALYSIS REQUEST ===\n\n";
	context += "Security Features Status:\n" + securityFeatures + "\n\n";
	
	if (!imports.isEmpty()) {
		context += "Imported APIs:\n";
		for (const QString &imp : imports) {
			context += "- " + imp + "\n";
		}
		context += "\n";
	}
	
	if (!strings.isEmpty()) {
		context += "Extracted Strings (sample):\n";
		int count = 0;
		for (const QString &str : strings) {
			if (count++ > 30) break;
			context += "- " + str + "\n";
		}
	}
	
	QString prompt = "You are a security vulnerability analyst. Analyze this Windows PE executable for potential security vulnerabilities:\n\n" + context + "\n\n";
	prompt += "Please identify:\n";
	prompt += "1. **Buffer Overflow Risks**: Unsafe string/memory functions (strcpy, sprintf, gets, etc.)\n";
	prompt += "2. **Format String Vulnerabilities**: Improper use of printf-family functions\n";
	prompt += "3. **Integer Overflow Risks**: Arithmetic operations that could overflow\n";
	prompt += "4. **Use-After-Free Potential**: Memory management issues\n";
	prompt += "5. **DLL Hijacking Risk**: Unsafe DLL loading patterns\n";
	prompt += "6. **Hardcoded Credentials**: Passwords, keys, or tokens in strings\n";
	prompt += "7. **Insecure Network Communication**: HTTP instead of HTTPS, weak crypto\n";
	prompt += "8. **Missing Security Mitigations**: ASLR, DEP, CFG status\n";
	prompt += "9. **Command Injection Risks**: Shell execution patterns\n";
	prompt += "10. **Path Traversal Risks**: File path handling issues\n\n";
	prompt += "For each vulnerability found, provide:\n";
	prompt += "- Severity (Critical/High/Medium/Low)\n";
	prompt += "- Evidence (which API/string indicates this)\n";
	prompt += "- Exploitation scenario\n";
	prompt += "- Mitigation recommendation\n";
	
	if (aiSettings->isUseOpenAI()) {
		sendOpenAIRequest(prompt);
	} else if (aiSettings->isUseGemini()) {
		sendGeminiRequest(prompt);
	} else {
		emit analysisError("No AI provider enabled.");
	}
}

void AIAnalyzer::analyzeSecurityPosture(const QString &peContext)
{
	emit progressUpdate("Analyzing security posture...");
	
	QString prompt = "You are a binary security analyst. Provide a comprehensive security assessment of this PE executable:\n\n";
	prompt += peContext + "\n\n";
	prompt += "Analyze and report on:\n";
	prompt += "1. **Overall Security Score**: Rate 1-10 with justification\n";
	prompt += "2. **Attack Surface**: What attack vectors does this binary expose?\n";
	prompt += "3. **Exploit Mitigations**: Which protections are enabled/disabled?\n";
	prompt += "4. **Vulnerability Summary**: List potential vulnerabilities by severity\n";
	prompt += "5. **Malware Indicators**: Any suspicious behaviors or patterns?\n";
	prompt += "6. **Recommendations**: Prioritized security improvements\n";
	prompt += "7. **Risk Assessment**: Who should be concerned about this binary?\n";
	
	if (aiSettings->isUseOpenAI()) {
		sendOpenAIRequest(prompt);
	} else if (aiSettings->isUseGemini()) {
		sendGeminiRequest(prompt);
	} else {
		emit analysisError("No AI provider enabled.");
	}
}

void AIAnalyzer::scanFileHash(const QString &hash)
{
	if (aiSettings->getVirusTotalApiKey().isEmpty()) {
		emit analysisError("VirusTotal API key not configured");
		return;
	}
	
	emit progressUpdate("Querying VirusTotal...");
	
	QString urlStr = QString("https://www.virustotal.com/api/v3/files/%1").arg(hash);
	QUrl url(urlStr);
	QNetworkRequest request(url);
	request.setRawHeader("x-apikey", aiSettings->getVirusTotalApiKey().toUtf8());
	
	QNetworkReply *reply = networkManager->get(request);
	connect(reply, &QNetworkReply::finished, this, &AIAnalyzer::onVirusTotalReplyFinished);
}

void AIAnalyzer::uploadFileToVirusTotal(const QString &filePath)
{
	if (aiSettings->getVirusTotalApiKey().isEmpty()) {
		emit analysisError("VirusTotal API key not configured");
		return;
	}
	
	emit progressUpdate("Uploading file to VirusTotal...");
	
	QFile *file = new QFile(filePath);
	if (!file->open(QIODevice::ReadOnly)) {
		emit analysisError("Cannot open file for upload");
		delete file;
		return;
	}
	
	QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
	
	QHttpPart filePart;
	filePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
		QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(QFileInfo(filePath).fileName())));
	filePart.setBodyDevice(file);
	file->setParent(multiPart);
	
	multiPart->append(filePart);
	
	QUrl url("https://www.virustotal.com/api/v3/files");
	QNetworkRequest request(url);
	request.setRawHeader("x-apikey", aiSettings->getVirusTotalApiKey().toUtf8());
	
	QNetworkReply *reply = networkManager->post(request, multiPart);
	multiPart->setParent(reply);
	
	connect(reply, &QNetworkReply::finished, this, &AIAnalyzer::onVirusTotalReplyFinished);
}

void AIAnalyzer::onVirusTotalReplyFinished()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	if (reply->error() == QNetworkReply::NoError) {
		QByteArray response = reply->readAll();
		QJsonDocument doc = QJsonDocument::fromJson(response);
		QJsonObject obj = doc.object();
		
		QString result = "VirusTotal Report:\n\n";
		
		if (obj.contains("data")) {
			QJsonObject data = obj["data"].toObject();
			QJsonObject attributes = data["attributes"].toObject();
			
			if (attributes.contains("last_analysis_stats")) {
				QJsonObject stats = attributes["last_analysis_stats"].toObject();
				result += QString("Malicious: %1\n").arg(stats["malicious"].toInt());
				result += QString("Suspicious: %1\n").arg(stats["suspicious"].toInt());
				result += QString("Undetected: %1\n").arg(stats["undetected"].toInt());
				result += QString("Harmless: %1\n\n").arg(stats["harmless"].toInt());
			}
			
			if (attributes.contains("last_analysis_results")) {
				result += "Detection details:\n";
				QJsonObject results = attributes["last_analysis_results"].toObject();
				for (auto it = results.begin(); it != results.end(); ++it) {
					QJsonObject engine = it.value().toObject();
					if (engine["category"].toString() == "malicious") {
						result += QString("- %1: %2\n").arg(it.key()).arg(engine["result"].toString());
					}
				}
			}
			
			emit analysisComplete(result);
		} else {
			emit analysisError("No data in VirusTotal response");
		}
	} else {
		emit analysisError("VirusTotal error: " + reply->errorString());
	}
	
	reply->deleteLater();
}
