#include "AIAssistantWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QScrollBar>
#include <QDateTime>
#include <QKeyEvent>
#include <QTimer>
#include <QApplication>
#include <QPalette>
#include <bearparser/bearparser.h>

// Custom chat input that handles Enter key
class ChatInputEdit : public QTextEdit {
public:
ChatInputEdit(QWidget *parent = nullptr) : QTextEdit(parent) {}

std::function<void()> onSendMessage;

protected:
void keyPressEvent(QKeyEvent *event) override {
if (event->key() == Qt::Key_Return && !(event->modifiers() & Qt::ShiftModifier)) {
if (onSendMessage) onSendMessage();
event->accept();
} else {
QTextEdit::keyPressEvent(event);
}
}
};

AIAssistantWindow::AIAssistantWindow(AISettings *settings, QWidget *parent)
: QDockWidget("AI Agent", parent), aiSettings(settings), currentPeHandler(nullptr), typingWidget(nullptr),
  isAgentMode(false), autoAnalysisEnabled(false), pendingAnalysisSteps(0)
{
aiAnalyzer = new AIAnalyzer(settings, this);

connect(aiAnalyzer, &AIAnalyzer::analysisComplete, this, &AIAssistantWindow::onAnalysisComplete);
connect(aiAnalyzer, &AIAnalyzer::analysisError, this, &AIAssistantWindow::onAnalysisError);
connect(aiAnalyzer, &AIAnalyzer::progressUpdate, this, &AIAssistantWindow::onProgressUpdate);

setupUI();
setWindowTitle("AI Agent");
setMinimumWidth(400);
setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
}

AIAssistantWindow::~AIAssistantWindow()
{
}

void AIAssistantWindow::keyPressEvent(QKeyEvent *event)
{
if (event->key() == Qt::Key_Escape) {
close();
event->accept();
} else {
QDockWidget::keyPressEvent(event);
}
}

void AIAssistantWindow::setupUI()
{
// Create central widget for the dock
centralWidget = new QWidget(this);
setWidget(centralWidget);

QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
mainLayout->setContentsMargins(4, 4, 4, 4);
mainLayout->setSpacing(4);

// Chat area
chatScrollArea = new QScrollArea(centralWidget);
chatScrollArea->setWidgetResizable(true);
chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
chatScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
chatScrollArea->setFrameShape(QFrame::NoFrame);

chatContainer = new QWidget();
chatLayout = new QVBoxLayout(chatContainer);
chatLayout->setContentsMargins(8, 8, 8, 8);
chatLayout->setSpacing(8);
chatLayout->addStretch();

chatScrollArea->setWidget(chatContainer);
mainLayout->addWidget(chatScrollArea, 1);

// Quick actions bar
QWidget *quickActions = new QWidget(centralWidget);
QHBoxLayout *quickLayout = new QHBoxLayout(quickActions);
quickLayout->setContentsMargins(8, 4, 8, 4);
quickLayout->setSpacing(4);

QLabel *quickLabel = new QLabel("Quick:", centralWidget);
quickLayout->addWidget(quickLabel);

autoAnalyzeBtn = createQuickButton("Agent Analyze", "Full automatic security analysis");
analyzeImportsBtn = createQuickButton("Imports", "Analyze imported functions");
analyzeExportsBtn = createQuickButton("Exports", "Analyze exported functions");
analyzeStringsBtn = createQuickButton("Strings", "Analyze strings");
detectMalwareBtn = createQuickButton("Malware", "Detect malware indicators");
vulnAnalysisBtn = createQuickButton("Vulns", "Analyze security vulnerabilities");
virusTotalBtn = createQuickButton("VT Scan", "Scan with VirusTotal");

connect(autoAnalyzeBtn, &QPushButton::clicked, this, &AIAssistantWindow::performAutoAnalysis);
connect(analyzeImportsBtn, &QPushButton::clicked, this, &AIAssistantWindow::onAnalyzeImports);
connect(analyzeExportsBtn, &QPushButton::clicked, this, &AIAssistantWindow::onAnalyzeExports);
connect(analyzeStringsBtn, &QPushButton::clicked, this, &AIAssistantWindow::onAnalyzeStrings);
connect(detectMalwareBtn, &QPushButton::clicked, this, &AIAssistantWindow::onDetectMalware);
connect(vulnAnalysisBtn, &QPushButton::clicked, this, &AIAssistantWindow::onAnalyzeVulnerabilities);
connect(virusTotalBtn, &QPushButton::clicked, this, &AIAssistantWindow::onScanWithVirusTotal);

quickLayout->addWidget(autoAnalyzeBtn);
quickLayout->addWidget(analyzeImportsBtn);
quickLayout->addWidget(analyzeExportsBtn);
quickLayout->addWidget(analyzeStringsBtn);
quickLayout->addWidget(detectMalwareBtn);
quickLayout->addWidget(vulnAnalysisBtn);
quickLayout->addWidget(virusTotalBtn);
quickLayout->addStretch();

mainLayout->addWidget(quickActions);

// Input area
QWidget *inputArea = new QWidget(centralWidget);
QVBoxLayout *inputLayout = new QVBoxLayout(inputArea);
inputLayout->setContentsMargins(8, 4, 8, 8);
inputLayout->setSpacing(4);

QHBoxLayout *inputRowLayout = new QHBoxLayout();
inputRowLayout->setSpacing(4);

ChatInputEdit *inputEdit = new ChatInputEdit(centralWidget);
inputEdit->setPlaceholderText("Ask about the PE file... (Enter to send)");
inputEdit->setMinimumHeight(40);
inputEdit->setMaximumHeight(80);
messageInput = inputEdit;

sendBtn = new QPushButton("Send", centralWidget);
sendBtn->setFixedHeight(40);
sendBtn->setToolTip("Send message");

inputEdit->onSendMessage = [this]() { onSendMessage(); };
connect(sendBtn, &QPushButton::clicked, this, &AIAssistantWindow::onSendMessage);

inputRowLayout->addWidget(messageInput);
inputRowLayout->addWidget(sendBtn);
inputLayout->addLayout(inputRowLayout);

// Status bar
QHBoxLayout *statusLayout = new QHBoxLayout();
statusLayout->setSpacing(4);

statusLabel = new QLabel("Ready", centralWidget);

progressBar = new QProgressBar(centralWidget);
progressBar->setMaximum(0);
progressBar->setMinimum(0);
progressBar->setFixedHeight(3);
progressBar->setTextVisible(false);
progressBar->setVisible(false);

statusLayout->addWidget(statusLabel);
statusLayout->addWidget(progressBar, 1);
inputLayout->addLayout(statusLayout);

mainLayout->addWidget(inputArea);

addWelcomeMessage();
}

QPushButton* AIAssistantWindow::createQuickButton(const QString &text, const QString &tooltip)
{
QPushButton *btn = new QPushButton(text, centralWidget);
btn->setToolTip(tooltip);
return btn;
}

void AIAssistantWindow::addWelcomeMessage()
{
addMessage("ai", "AI Reverse Engineering Agent\n\n"
"I can automatically analyze PE files for security issues.\n\n"
"Agent Mode (Recommended):\n"
"- Click 'Agent Analyze' for full automatic analysis\n"
"- Gathers imports, exports, strings, security features\n"
"- Provides comprehensive threat & vulnerability assessment\n\n"
"Quick Actions:\n"
"- Imports/Exports - API analysis\n"
"- Strings - Extract and analyze strings\n"
"- Malware - Detect malicious patterns\n"
"- Vulns - Security vulnerability analysis\n"
"- VT Scan - VirusTotal reputation check\n\n"
"Load a PE file and click Agent Analyze to begin!");
}

void AIAssistantWindow::addMessage(const QString &type, const QString &text)
{
QWidget *messageWidget = new QWidget();
QHBoxLayout *msgLayout = new QHBoxLayout(messageWidget);
msgLayout->setContentsMargins(4, 4, 4, 4);
msgLayout->setSpacing(8);

bool isUser = (type == "user");
bool isError = (type == "error");

if (!isUser) {
QLabel *avatar = new QLabel(isError ? "!" : "AI", messageWidget);
avatar->setFixedSize(24, 24);
avatar->setAlignment(Qt::AlignCenter);
avatar->setFrameStyle(QFrame::Box);
msgLayout->addWidget(avatar, 0, Qt::AlignTop);
} else {
msgLayout->addSpacing(32);
}

QLabel *content = new QLabel(messageWidget);
content->setWordWrap(true);
content->setTextFormat(Qt::PlainText);
content->setText(text);
content->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
content->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
content->setMargin(8);

msgLayout->addWidget(content, 1);

if (isUser) {
QLabel *avatar = new QLabel("U", messageWidget);
avatar->setFixedSize(24, 24);
avatar->setAlignment(Qt::AlignCenter);
avatar->setFrameStyle(QFrame::Box);
msgLayout->addWidget(avatar, 0, Qt::AlignTop);
} else {
msgLayout->addSpacing(32);
}

int insertPos = chatLayout->count() - 1;
chatLayout->insertWidget(insertPos, messageWidget);

QTimer::singleShot(50, [this]() {
chatScrollArea->verticalScrollBar()->setValue(
chatScrollArea->verticalScrollBar()->maximum()
);
});
}

void AIAssistantWindow::addTypingIndicator()
{
typingWidget = new QWidget();
QHBoxLayout *layout = new QHBoxLayout(typingWidget);
layout->setContentsMargins(4, 4, 4, 4);
layout->setSpacing(8);

QLabel *avatar = new QLabel("AI", typingWidget);
avatar->setFixedSize(24, 24);
avatar->setAlignment(Qt::AlignCenter);
avatar->setFrameStyle(QFrame::Box);
layout->addWidget(avatar, 0, Qt::AlignTop);

QLabel *dots = new QLabel("Analyzing...", typingWidget);
layout->addWidget(dots);
layout->addStretch();

int insertPos = chatLayout->count() - 1;
chatLayout->insertWidget(insertPos, typingWidget);

QTimer::singleShot(50, [this]() {
chatScrollArea->verticalScrollBar()->setValue(
chatScrollArea->verticalScrollBar()->maximum()
);
});
}

void AIAssistantWindow::removeTypingIndicator()
{
if (typingWidget) {
chatLayout->removeWidget(typingWidget);
delete typingWidget;
typingWidget = nullptr;
}
}

void AIAssistantWindow::onSendMessage()
{
QString text = messageInput->toPlainText().trimmed();
if (text.isEmpty()) return;

addMessage("user", text);
messageInput->clear();

addTypingIndicator();
aiAnalyzer->analyzeCode(text, "User question about the loaded PE file");
}

void AIAssistantWindow::setCurrentPeHandler(PeHandler *peHandler)
{
currentPeHandler = peHandler;
if (peHandler) {
QString fileName = peHandler->getShortName();
addMessage("ai", QString("Loaded: %1\n\nI'm ready to analyze this file. Use the quick actions below or ask me anything!").arg(fileName));
}
}

void AIAssistantWindow::onAnalyzeImports()
{
if (!currentPeHandler) {
addMessage("error", "Please load a PE file first.");
return;
}

PEFile *pe = currentPeHandler->getPe();
if (!pe) return;

QStringList imports;

ImportDirWrapper *importDir = dynamic_cast<ImportDirWrapper*>(pe->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_IMPORT));
if (!importDir) {
addMessage("ai", "This file has no imports.");
return;
}

size_t impNum = importDir->getEntriesCount();
for (size_t i = 0; i < impNum; i++) {
ImportEntryWrapper *impEntry = dynamic_cast<ImportEntryWrapper*>(importDir->getEntryAt(i));
if (!impEntry) continue;

QString dllName = QString::fromStdString(impEntry->getLibraryName());

size_t funcNum = impEntry->getEntriesCount();
for (size_t j = 0; j < funcNum && j < 100; j++) {
ImportedFuncWrapper *func = dynamic_cast<ImportedFuncWrapper*>(impEntry->getEntryAt(j));
if (!func) continue;

QString funcName = func->getName();
if (!funcName.isEmpty()) {
imports.append(dllName + "!" + funcName);
}
}
}

if (imports.isEmpty()) {
addMessage("ai", "No imports found in this file.");
return;
}

addMessage("user", "Analyze imports");
addTypingIndicator();
aiAnalyzer->analyzeImports(imports);
}

void AIAssistantWindow::onAnalyzeExports()
{
if (!currentPeHandler) {
addMessage("error", "Please load a PE file first.");
return;
}

PEFile *pe = currentPeHandler->getPe();
if (!pe) return;

QStringList exports;

ExportDirWrapper *exportDir = dynamic_cast<ExportDirWrapper*>(pe->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_EXPORT));
if (!exportDir) {
addMessage("ai", "This file has no exports.");
return;
}

size_t expNum = exportDir->getEntriesCount();
for (size_t i = 0; i < expNum && i < 100; i++) {
ExeNodeWrapper *node = exportDir->getEntryAt(i);
if (!node) continue;

QString funcName = node->getName();
if (!funcName.isEmpty()) {
exports.append(funcName);
}
}

if (exports.isEmpty()) {
addMessage("ai", "No exports found in this file.");
return;
}

addMessage("user", "Analyze exports");
addTypingIndicator();
aiAnalyzer->analyzeExports(exports);
}

void AIAssistantWindow::onAnalyzeStrings()
{
if (!currentPeHandler) {
addMessage("error", "Please load a PE file first.");
return;
}

StringsCollection &strings = currentPeHandler->stringsMap;
if (strings.size() == 0) {
addMessage("ai", "No strings found. Please run string extraction first.");
return;
}

QStringList strList;
QList<offset_t> offsets = strings.getOffsets();
for (offset_t offset : offsets) {
if (strList.size() >= 100) break;
QString str = strings.getString(offset);
if (str.length() > 3) {
strList.append(str);
}
}

addMessage("user", "Analyze strings");
addTypingIndicator();
aiAnalyzer->analyzeSuspiciousStrings(strList);
}

void AIAssistantWindow::onDetectMalware()
{
if (!currentPeHandler) {
addMessage("error", "Please load a PE file first.");
return;
}

PEFile *pe = currentPeHandler->getPe();
if (!pe) return;

QStringList imports;
ImportDirWrapper *importDir = dynamic_cast<ImportDirWrapper*>(pe->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_IMPORT));
if (importDir) {
size_t impNum = importDir->getEntriesCount();
for (size_t i = 0; i < impNum; i++) {
ImportEntryWrapper *impEntry = dynamic_cast<ImportEntryWrapper*>(importDir->getEntryAt(i));
if (!impEntry) continue;

size_t funcNum = impEntry->getEntriesCount();
for (size_t j = 0; j < funcNum && j < 50; j++) {
ImportedFuncWrapper *func = dynamic_cast<ImportedFuncWrapper*>(impEntry->getEntryAt(j));
if (!func) continue;

QString funcName = func->getName();
if (!funcName.isEmpty()) {
imports.append(funcName);
}
}
}
}

QStringList strings;
StringsCollection &foundStrings = currentPeHandler->stringsMap;
QList<offset_t> strOffsets = foundStrings.getOffsets();
for (offset_t offset : strOffsets) {
if (strings.size() >= 50) break;
QString str = foundStrings.getString(offset);
if (str.length() > 3) {
strings.append(str);
}
}

QByteArray fileData;
AbstractByteBuffer *buf = pe->getFileBuffer();
if (buf) {
size_t size = buf->getContentSize();
fileData.resize(qMin(size, (size_t)10000));
memcpy(fileData.data(), buf->getContent(), fileData.size());
}

addMessage("user", "Detect malware indicators");
addTypingIndicator();
aiAnalyzer->detectMalware(fileData, strings, imports);
}

void AIAssistantWindow::onAnalyzeVulnerabilities()
{
if (!currentPeHandler) {
addMessage("error", "Please load a PE file first.");
return;
}

PEFile *pe = currentPeHandler->getPe();
if (!pe) {
addMessage("error", "Invalid PE file.");
return;
}

// Gather imports
QStringList imports;
DataDirEntryWrapper *importDir = dynamic_cast<DataDirEntryWrapper*>(
pe->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_IMPORT));

if (importDir) {
size_t impNum = importDir->getEntriesCount();
for (size_t i = 0; i < impNum; i++) {
ImportEntryWrapper *impEntry = dynamic_cast<ImportEntryWrapper*>(importDir->getEntryAt(i));
if (!impEntry) continue;

size_t funcNum = impEntry->getEntriesCount();
for (size_t j = 0; j < funcNum && j < 100; j++) {
ImportedFuncWrapper *func = dynamic_cast<ImportedFuncWrapper*>(impEntry->getEntryAt(j));
if (!func) continue;

QString funcName = func->getName();
if (!funcName.isEmpty()) {
imports.append(funcName);
}
}
}
}

// Gather strings
QStringList strings;
StringsCollection &foundStrings = currentPeHandler->stringsMap;
QList<offset_t> strOffsets = foundStrings.getOffsets();
for (offset_t offset : strOffsets) {
if (strings.size() >= 100) break;
QString str = foundStrings.getString(offset);
if (str.length() > 3) {
strings.append(str);
}
}

// Get security features
QString securityFeatures = checkSecurityFeatures(pe);

addMessage("user", "Analyze for security vulnerabilities");
addTypingIndicator();
aiAnalyzer->analyzeVulnerabilities(imports, strings, securityFeatures);
}

void AIAssistantWindow::onAnalyzeSelectedCode()
{
bool ok;
QString code = QInputDialog::getMultiLineText(this, "Analyze Code",
  "Paste assembly or code to analyze:",
  "", &ok);
if (ok && !code.isEmpty()) {
addMessage("user", code);
addTypingIndicator();
aiAnalyzer->analyzeCode(code, "User-provided code from PE-bear");
}
}

void AIAssistantWindow::onScanWithVirusTotal()
{
if (!currentPeHandler) {
addMessage("error", "Please load a PE file first.");
return;
}

QString md5 = currentPeHandler->getCurrentMd5();
if (md5.isEmpty()) {
addMessage("error", "Could not calculate file hash.");
return;
}

addMessage("user", QString("Scan with VirusTotal\nHash: %1").arg(md5));
addTypingIndicator();
aiAnalyzer->scanFileHash(md5);
}

void AIAssistantWindow::onAnalysisComplete(const QString &result)
{
removeTypingIndicator();
progressBar->setVisible(false);
statusLabel->setText("Ready");

addMessage("ai", result);
}

void AIAssistantWindow::onAnalysisError(const QString &error)
{
removeTypingIndicator();
progressBar->setVisible(false);
statusLabel->setText("Error");

QString errorMsg = "Error: " + error;
if (error.contains("API key")) {
errorMsg += "\n\nPlease configure your API keys in AI Assistant > AI Settings.";
}
addMessage("error", errorMsg);
}

void AIAssistantWindow::onProgressUpdate(const QString &status)
{
statusLabel->setText(status);
progressBar->setVisible(true);
}

void AIAssistantWindow::onClearResults()
{
while (chatLayout->count() > 1) {
QLayoutItem *item = chatLayout->takeAt(0);
if (item->widget()) {
delete item->widget();
}
delete item;
}
addWelcomeMessage();
statusLabel->setText("Ready");
}

void AIAssistantWindow::onCopyResults()
{
QString allText;
for (int i = 0; i < chatLayout->count() - 1; i++) {
QLayoutItem *item = chatLayout->itemAt(i);
if (item && item->widget()) {
QList<QLabel*> labels = item->widget()->findChildren<QLabel*>();
for (QLabel *label : labels) {
if (!label->text().isEmpty() && label->text().length() > 2) {
allText += label->text() + "\n\n";
}
}
}
}
QClipboard *clipboard = QApplication::clipboard();
clipboard->setText(allText);
statusLabel->setText("Chat copied to clipboard");
}

void AIAssistantWindow::onSaveResults()
{
QString fileName = QFileDialog::getSaveFileName(this, "Save Chat",
"", "Text Files (*.txt);;All Files (*)");
if (fileName.isEmpty()) return;

QString allText;
for (int i = 0; i < chatLayout->count() - 1; i++) {
QLayoutItem *item = chatLayout->itemAt(i);
if (item && item->widget()) {
QList<QLabel*> labels = item->widget()->findChildren<QLabel*>();
for (QLabel *label : labels) {
if (!label->text().isEmpty() && label->text().length() > 2) {
allText += label->text() + "\n\n---\n\n";
}
}
}
}

QFile file(fileName);
if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
QTextStream out(&file);
out << allText;
file.close();
statusLabel->setText("Chat saved");
} else {
QMessageBox::critical(this, "Error", "Could not save file: " + file.errorString());
}
}

// Agent Mode Methods

QString AIAssistantWindow::gatherPEInfo()
{
if (!currentPeHandler) return "No PE file loaded.";

PEFile *pe = currentPeHandler->getPe();
if (!pe) return "Invalid PE file.";

QString info;
info += "=== PE FILE ANALYSIS CONTEXT ===\n\n";

// Basic Info
info += "[BASIC INFO]\n";
info += QString("- File: %1\n").arg(currentPeHandler->getShortName());
info += QString("- MD5: %1\n").arg(currentPeHandler->getCurrentMd5());

if (pe->isBit32()) {
info += "- Architecture: 32-bit (PE32)\n";
} else {
info += "- Architecture: 64-bit (PE32+)\n";
}

// DOS Header
info += "\n[DOS HEADER]\n";
MappedExe *mpe = dynamic_cast<MappedExe*>(pe);
if (mpe) {
info += QString("- Entry Point: 0x%1\n").arg(mpe->getEntryPoint(), 0, 16);
}

// Sections
info += extractSectionInfo(pe);

// Imports
info += extractImportInfo(pe);

// Exports
info += extractExportInfo(pe);

// Security Features
info += checkSecurityFeatures(pe);

// Strings sample
info += extractStringSample(pe);

return info;
}

QString AIAssistantWindow::extractSectionInfo(PEFile *pe)
{
QString info = "\n[SECTIONS]\n";

size_t secCount = pe->getSectionsCount(false);
info += QString("- Section Count: %1\n").arg(secCount);

for (size_t i = 0; i < secCount && i < 20; i++) {
SectionHdrWrapper *sec = pe->getSecHdr(i);
if (sec) {
QString name = sec->mappedName;
bufsize_t rawSize = sec->getContentSize(Executable::RAW, false);
bufsize_t virtSize = sec->getContentSize(Executable::RVA, false);
DWORD characteristics = 0;

bool readable = false, writable = false, executable = false;
WrappedValue wVal = sec->getWrappedValue(SectionHdrWrapper::CHARACT);
if (wVal.isValid()) {
characteristics = wVal.getQVariant().toULongLong();
executable = characteristics & 0x20000000;
readable = characteristics & 0x40000000;
writable = characteristics & 0x80000000;
}

QString perms;
if (readable) perms += "R";
if (writable) perms += "W";
if (executable) perms += "X";

info += QString("  %1: Raw=0x%2, Virt=0x%3 [%4]\n")
.arg(name, -10)
.arg(rawSize, 0, 16)
.arg(virtSize, 0, 16)
.arg(perms);
}
}

return info;
}

QString AIAssistantWindow::extractImportInfo(PEFile *pe)
{
QString info = "\n[IMPORTS]\n";

DataDirEntryWrapper *importDir = dynamic_cast<DataDirEntryWrapper*>(
pe->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_IMPORT));

if (!importDir) {
info += "- No imports found\n";
return info;
}

size_t dllCount = importDir->getEntriesCount();
info += QString("- DLL Count: %1\n").arg(dllCount);

QStringList suspiciousDlls;
QStringList suspiciousFuncs;

for (size_t i = 0; i < dllCount && i < 50; i++) {
ImportDirWrapper *imp = dynamic_cast<ImportDirWrapper*>(importDir->getEntryAt(i));
if (!imp) continue;

QString dllName = imp->getName();
info += QString("  [%1]\n").arg(dllName);

// Check suspicious DLLs
QString dllLower = dllName.toLower();
if (dllLower.contains("ws2_32") || dllLower.contains("wininet") ||
dllLower.contains("winhttp") || dllLower.contains("crypt")) {
suspiciousDlls.append(dllName);
}

size_t funcCount = imp->getEntriesCount();
for (size_t j = 0; j < funcCount && j < 20; j++) {
ImportEntryWrapper *entry = dynamic_cast<ImportEntryWrapper*>(imp->getEntryAt(j));
if (entry) {
QString funcName = entry->getName();
info += QString("    - %1\n").arg(funcName);

// Check suspicious functions
QString funcLower = funcName.toLower();
if (funcLower.contains("virtualalloc") || funcLower.contains("writeprocessmemory") ||
funcLower.contains("createremotethread") || funcLower.contains("loadlibrary") ||
funcLower.contains("getprocaddress") || funcLower.contains("ntquery")) {
suspiciousFuncs.append(funcName);
}
}
}

if (funcCount > 20) {
info += QString("    ... and %1 more functions\n").arg(funcCount - 20);
}
}

if (!suspiciousDlls.isEmpty() || !suspiciousFuncs.isEmpty()) {
info += "\n[SUSPICIOUS INDICATORS]\n";
if (!suspiciousDlls.isEmpty()) {
info += QString("- Suspicious DLLs: %1\n").arg(suspiciousDlls.join(", "));
}
if (!suspiciousFuncs.isEmpty()) {
info += QString("- Suspicious APIs: %1\n").arg(suspiciousFuncs.join(", "));
}
}

return info;
}

QString AIAssistantWindow::extractExportInfo(PEFile *pe)
{
QString info = "\n[EXPORTS]\n";

DataDirEntryWrapper *exportDir = dynamic_cast<DataDirEntryWrapper*>(
pe->getWrapper(PEFile::WR_DIR_ENTRY + pe::DIR_EXPORT));

if (!exportDir) {
info += "- No exports (likely an executable, not a DLL)\n";
return info;
}

ExportDirWrapper *expWrap = dynamic_cast<ExportDirWrapper*>(exportDir);
if (expWrap) {
QString libName = expWrap->getName();
info += QString("- Library Name: %1\n").arg(libName);
}

size_t expCount = exportDir->getEntriesCount();
info += QString("- Export Count: %1\n").arg(expCount);

for (size_t i = 0; i < expCount && i < 30; i++) {
ExeNodeWrapper *entry = exportDir->getEntryAt(i);
if (entry) {
QString name = entry->getName();
info += QString("  - %1\n").arg(name);
}
}

if (expCount > 30) {
info += QString("  ... and %1 more exports\n").arg(expCount - 30);
}

return info;
}

QString AIAssistantWindow::checkSecurityFeatures(PEFile *pe)
{
QString info = "\n[SECURITY FEATURES]\n";

// Check for common security mitigations
bool hasAslr = false;
bool hasDep = false;
bool hasCfg = false;

// These would be in the DllCharacteristics field
OptHdrWrapper *optHdr = dynamic_cast<OptHdrWrapper*>(pe->getWrapper(PEFile::WR_OPTIONAL_HDR));
if (optHdr) {
WrappedValue dllCharVal = optHdr->getWrappedValue(OptHdrWrapper::DLL_CHARACT);
if (dllCharVal.isValid()) {
WORD dllCharact = dllCharVal.getQVariant().toULongLong();
hasAslr = dllCharact & 0x0040;  // DYNAMIC_BASE
hasDep = dllCharact & 0x0100;   // NX_COMPAT
hasCfg = dllCharact & 0x4000;   // GUARD_CF
}
}

info += QString("- ASLR (Address Space Layout Randomization): %1\n").arg(hasAslr ? "Enabled" : "DISABLED");
info += QString("- DEP/NX (Data Execution Prevention): %1\n").arg(hasDep ? "Enabled" : "DISABLED");
info += QString("- CFG (Control Flow Guard): %1\n").arg(hasCfg ? "Enabled" : "Disabled");

return info;
}

QString AIAssistantWindow::extractStringSample(PEFile *pe)
{
Q_UNUSED(pe);
QString info = "\n[STRINGS SAMPLE]\n";

// Use currentPeHandler's string collection which is already populated
StringsCollection &strings = currentPeHandler->stringsMap;

QList<offset_t> offsets = strings.getOffsets();
int count = 0;
int maxStrings = 50;

QStringList suspiciousStrings;

for (const offset_t &offset : offsets) {
if (count >= maxStrings) break;

QString str = strings.getString(offset);
if (str.length() >= 5 && str.length() <= 200) {
info += QString("  0x%1: %2\n").arg(offset, 0, 16).arg(str);
count++;

// Check for suspicious strings
QString strLower = str.toLower();
if (strLower.contains("http://") || strLower.contains("https://") ||
strLower.contains("cmd.exe") || strLower.contains("powershell") ||
strLower.contains("password") || strLower.contains("encrypt") ||
strLower.contains(".onion") || strLower.contains("bitcoin")) {
suspiciousStrings.append(str);
}
}
}

if (offsets.size() > maxStrings) {
info += QString("  ... and %1 more strings\n").arg(offsets.size() - maxStrings);
}

if (!suspiciousStrings.isEmpty()) {
info += "\n[SUSPICIOUS STRINGS]\n";
for (const QString &s : suspiciousStrings) {
info += QString("  ! %1\n").arg(s);
}
}

return info;
}

QString AIAssistantWindow::generatePESummary()
{
if (!currentPeHandler) return "";

return QString(
"Analyzing PE file: %1\n"
"MD5: %2\n"
"This is a %3-bit Windows executable.\n"
).arg(currentPeHandler->getShortName())
.arg(currentPeHandler->getCurrentMd5())
.arg(currentPeHandler->getPe()->isBit32() ? "32" : "64");
}

void AIAssistantWindow::performAutoAnalysis()
{
if (!currentPeHandler) {
addMessage("error", "No PE file loaded for auto-analysis.");
return;
}

isAgentMode = true;
addMessage("ai", "**Agent Mode Activated**\n\n"
"I'm automatically analyzing the loaded PE file. "
"I'll examine imports, exports, strings, security features, and look for suspicious patterns.\n\n"
"Please wait while I gather information...");

addTypingIndicator();
statusLabel->setText("Agent: Gathering PE context...");
progressBar->setVisible(true);

// Gather all PE context
QString context = gatherPEInfo();

// Create comprehensive analysis prompt
QString prompt = QString(
"You are a reverse engineering AI agent analyzing a PE (Portable Executable) file. "
"Based on the following extracted information, provide a comprehensive security analysis:\n\n"
"%1\n\n"
"Please analyze:\n"
"1. **File Type & Purpose**: What kind of application is this? (installer, malware, tool, etc.)\n"
"2. **Suspicious Indicators**: Any red flags in imports, strings, or structure?\n"
"3. **Network Capability**: Can this binary communicate over the network?\n"
"4. **Persistence Mechanisms**: Any signs of autostart or persistence?\n"
"5. **Code Injection**: Any capability for injecting code into other processes?\n"
"6. **Encryption/Packing**: Is the binary packed or encrypted?\n"
"7. **Risk Assessment**: Overall risk level (Low/Medium/High/Critical) with reasoning\n"
"8. **Recommendations**: Next steps for deeper analysis\n\n"
"Be specific and technical. Reference the actual APIs and strings you see."
).arg(context);

aiAnalyzer->analyzeCode(prompt);
}

void AIAssistantWindow::setAutoAnalyzeOnLoad(bool enabled)
{
autoAnalysisEnabled = enabled;
}

void AIAssistantWindow::onPeFileLoaded()
{
if (autoAnalysisEnabled && currentPeHandler) {
QTimer::singleShot(500, this, &AIAssistantWindow::performAutoAnalysis);
}
}
