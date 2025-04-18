#pragma once

#include "../REbear.h"

size_t scrollSize = 20;
QString sliderHandleBg = "rgba(0,0,0,240);";
QString sliderBg = "rgba(62,43,62,128);";
QString sliderHoverColor = "rgba(255,255,255,112);";

QString g_DarkStyle = QString() +
"QWidget"
"{"
"    color: #eff0f1;"
"    background-color: #30353a;"
"    selection-background-color:#3daee9;"
"    selection-color: #eff0f1;"
"    background-clip: border;"
"    border-image: none;"
"    border: 0px transparent black;"
"    alternate-background-color: #353941;"
"    outline: 0;"
"    font: inherit;"
"}"
"QWidget:item:pressed"
"{"
"    background-color: #3daee9;"
"    color: #eff0f1;"
"}"
"QWidget:disabled"
"{"
"    color: gray;"
"}"
"QSpinBox,"
"QLineEdit,"
"QTreeView,"
"QListView,"
"QTextEdit"
"{"
"    border: 1px solid #76797c;"
"    font: inherit;"
"}"
"QLineEdit:hover"
"{"
"    border-color: white;"
"}"
"QTabBar"
"{"
"    font: inherit;"
"}"
"QTabBar::tab"
"{"
"    border: 1px solid #76797c;"
"    background-color: qlineargradient(x1: 0.5, y1: 0.5 x2: 0.5, y2: 1, stop: 0 #31363b, stop: 0.5 #3b4045);"
"    selection-background-color: #3daee9;"
"    border-radius: 2px;"
"    padding: 5px;"
"    font: inherit;"
"}"
"QTabBar::tab:hover {"
"    background-color: qlineargradient(x1: 0.5, y1: 0.5 x2: 0.5, y2: 1, stop: 0 #3b4045, stop: 0.5 black);"
"    border: 1px solid cyan;"
"}"
"QTabBar::tab:selected {"
"    border: 1px solid #734f96;"
"    border-radius: 2px;"
"    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 black, stop: 1 #3b4045);"
"}"
"QComboBox"
"{"
"    border: 1px solid #76797c;"
"    background-color: qlineargradient(x1: 0.5, y1: 0.5 x2: 0.5, y2: 1, stop: 0 #31363b, stop: 0.5 #3b4045);"
"    selection-background-color: #3daee9;"
"    border-radius: 2px;"
"}"
"QComboBox:drop-down {"
"	border: 3px solid " + sliderBg + ";"
"	border-radius: 4px;"
"	background: rgba(242, 242, 242, 150);"
"	background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #FFFFFD, stop: 0.3 #96ADB2);"
"}"
"QComboBox:drop-down:hover {"
"	border: 3px solid #5E749C;"
"}"

"QComboBox:down-arrow {"
"      border: 1px solid black;"
"      width: 1px;"
"      height: 1px;"
"      background: white;"
"}"
"QToolBar"
"{"
"    border: 1px solid #76797c;"
"    padding: 5px;"
"    selection-background-color: #3daee9;"
"    background-color: qlineargradient(x1: 0, y1: 0.5 x2: 0.5, y2: 1, stop: 0 #31363b, stop: 0.5 #3b4045);"
"}"
"QToolBar[dataDir=\"true\"] { background: #8b9095; border-radius: 3px; outline: 0; }"
"QToolButton{ "
"    background-color: transparent;"
"}"
"QTabBar QToolButton { "
"    border: 1px solid #76797c;"
"    border-radius: 2px;"
"    background-color: qlineargradient(x1: 0, y1: 0.5 x2: 0.5, y2: 1, stop: 0 #31363b, stop: 0.5 #3b4045);"
"}"
"QToolButton:hover"
"{"
"    border: 1px solid cyan;"
"    border-radius: 2px;"
"}"
"QToolButton:checked"
"{"
"    border: 1px solid;"
"    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 black, stop: 1 #3b4045);"
"    border-color: #734f96;"
"    border-radius: 2px;"
"}"
"QPushButton"
"{"
"    color: #eff0f1;"
"    background-color: qlineargradient(x1: 0.5, y1: 0.5 x2: 0.5, y2: 1, stop: 0 #3b4045, stop: 0.5 #31363b);"
"    border-width: 1px;"
"    border-color: #76797c;"
"    border-style: solid;"
"    padding: 5px;"
"    border-radius: 2px;"
"    outline: none;"
"}"
"QPushButton:pressed,"
"QToolButton:pressed"
"{"
"    background-color: #31363b;"
"    padding-top: -15px;"
"    padding-bottom: -17px;"
"}"
"QPushButton:checked"
"{"
"    background-color: #76797c;"
"    border-color: #6A6969;"
"}"
"QPushButton:hover"
"{"
"    border-color: cyan;"
"}"
"DisasmTreeView"
"{"
	"background-color: " + DISASMDMP_BG + ";"
	"alternate-background-color: " + HEXDMP_HBG + ";"
	"selection-background-color: darkblue;"
	"color: " + DISASMDMP_TXT + ";"
"}"
"QHeaderView"
"{"
"	background-color: #31363b;"
"	border: 1px transparent;"
"	border-radius: 0px;"
"	margin: 0px;"
"	padding: 0px;"
"}"
"QHeaderView::section "
"{"
	"background-color: #222222;"
"}"
"QTableCornerButton::section { background-color: black; }"
"ContentPreview"
"{"
	"background-color: #222222;"
	"alternate-background-color: cyan;"
	"color: white;"
"}"
"HexCompareView"
"{"
	"background-color: #584454;"
	"alternate-background-color: #3e2b3e;"
	"color: white;"
"}"
"ColorSettings {"
"	qproperty-rawColor: #2FA530; qproperty-rvaColor: #0080ff; qproperty-vaColor: #c364c5; "
"	qproperty-flagsColor: #584454;"
"	qproperty-dataDirColor: #584454;"
"	qproperty-dataDirNameColor: #452f5b;"
"}"
"QLabel[hasUrl=\"true\"] { background: #5b6065; border-radius: 1px; }"
"QLineEdit[readOnly=\"true\"]{"
"	border: 2px ridge black;"
"	background-color: #353941;"
"}"
"QCheckBox::indicator:unchecked {"
"	border: 1px solid white;"
"	border-radius: 2px;"
"}"
""
"QScrollBar:horizontal {"
"     border: 1px solid #222222;"
"     background: "+ sliderBg + ";"
"     height: 13px;"
"     margin: 0px " + QString::number(scrollSize) + "px 0 " + QString::number(scrollSize) + "px;"
"}"
""
"QScrollBar::handle:horizontal"
"{"
"      background: " + sliderHandleBg + ";"
"      min-height: " + QString::number(scrollSize) + "px;"
"      border-radius: 2px;"
"}"
"QScrollBar::handle:horizontal:hover"
"{"
"      border: 1px solid black;"
"      background: " + sliderHoverColor + ";"
"      min-height: " + QString::number(scrollSize) + "px;"
"      border-radius: 2px;"
"}"
""
"QScrollBar::add-line:horizontal {"
"      border: 1px solid #1b1b19;"
"      border-radius: 2px;"
"      background: " + sliderBg + ";"
"      width: " + QString::number(scrollSize) + "px;"
"      subcontrol-position: right;"
"      subcontrol-origin: margin;"
"}"
""
"QScrollBar::sub-line:horizontal {"
"      border: 1px solid #1b1b19;"
"      border-radius: 2px;"
"      background: " + sliderBg + ";"
"      width: " + QString::number(scrollSize) + "px;"
"      subcontrol-position: left;"
"      subcontrol-origin: margin;"
"}"
""
"QScrollBar::right-arrow:horizontal, QScrollBar::left-arrow:horizontal"
"{"
"      border: 1px solid black;"
"      width: 1px;"
"      height: 1px;"
"      background: white;"
"}"
""
"QSpinBox:vertical"
"{"
"      background: " + sliderBg + ";"
"      border: 1px solid #222222;"
"}"
""
"QScrollBar:vertical"
"{"
"      background: " + sliderBg + ";"
"      width: 13px;"
"      margin: " + QString::number(scrollSize) + "px 0 " + QString::number(scrollSize) + "px 0;"
"      border: 1px solid #222222;"
"}"
""
"QScrollBar::handle:vertical"
"{"
"      background: " + sliderHandleBg + ";"
"      min-height: " + QString::number(scrollSize) + "px;"
"      border-radius: 5px;"
"}"
"QScrollBar::handle:vertical:hover"
"{"
"      border: 1px solid black;"
"      background: " + sliderHoverColor + ";"
"      min-height: " + QString::number(scrollSize) + "px;"
"      border-radius: 2px;"
"}"
""
"QScrollBar::add-line:vertical"
"{"
"      border: 1px solid #1b1b19;"
"      border-radius: 2px;"
"      background: " + sliderBg + ";"
"      height: " + QString::number(scrollSize) + "px;"
"      subcontrol-position: bottom;"
"      subcontrol-origin: margin;"
"}"
""
"QScrollBar::sub-line:vertical"
"{"
"      border: 1px solid #1b1b19;"
"      border-radius: 2px;"
"      background: " + sliderBg + ";"
"      height: " + QString::number(scrollSize) + "px;"
"      subcontrol-position: top;"
"      subcontrol-origin: margin;"
"}"
""
"QSpinBox::up-arrow:vertical, QSpinBox::down-arrow:vertical"
"{"
"      border: 1px solid black;"
"      border-radius: 2px;"
"      width: 1px;"
"      height: 1px;"
"      background: white;"
"}"
"QSpinBox::up-arrow:vertical:disabled, QSpinBox::down-arrow:vertical:disabled"
"{"
"      background: gray;"
"}"
""
"QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical"
"{"
"      border: 1px solid black;"
"      width: 1px;"
"      height: 1px;"
"      background: white;"
"}"
""
"QScrollBar::up-arrow:vertical:disabled, QScrollBar::down-arrow:vertical:disabled"
"{"
"      background: gray;"
"}"
""
"QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal"
"{"
"      background: none;"
"}"
""
"QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical"
"{"
"      background: none;"
"}"
""
"QProgressBar"
"{"
"      text-align: center;"
"      border: 1px solid #1b1b19;"
"      border-radius: 2px;"
"      background: " + sliderBg + ";"
"      color: yellow;"
"}"
""
"QProgressBar:chunk"
"{"
"      background: " + sliderHandleBg + ";"
"}"
""
"QMenuBar::item"
"{"
"    border-radius: 2px;"
"    padding: 5px;"
"}"
"QMenu"
"{"
"    background-color: " + sliderBg + ";"
"    border: 1px solid black;"
"}"
"QMenu::item"
"{"
"    background-color: transparent;"
"}"
"QMenuBar::item::selected,"
"QMenu::item::selected"
"{"
"    background-color: qlineargradient(x1: 0, y1: 0.5 x2: 0.5, y2: 1, stop: 0 #31363b, stop: 0.5 #3b4045);"
"}"
"QTabWidget::pane {"
"   border: 0px;"
"}"
;
