#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "../../base/PeHandlersManager.h"
#include "../../base/ImportsAutoadderSettings.h"

class ImpAdderSettingsTableModel : public QAbstractTableModel
{
    Q_OBJECT

public slots:
    void modelChanged()
    { 
        this->beginResetModel(); 
        this->endResetModel();
    }

public:
	enum COLS
	{
		COL_LIB = 0,
		COL_FUNC,
		COUNT_COL
	};

	ImpAdderSettingsTableModel(QObject *v_parent, ImportsAutoadderSettings& _settings)
		: QAbstractTableModel(v_parent), m_Settings(_settings)
	{
		reloadSettings();
	}

	virtual ~ImpAdderSettingsTableModel() { }

	QVariant headerData(int section, Qt::Orientation orientation, int role) const
	{
		if (role != Qt::DisplayRole) return QVariant();
		if (orientation != Qt::Horizontal) return QVariant();
		switch (section) {
			case COL_LIB : return "Lib";
			case COL_FUNC : return "Functions";
		}
		return QVariant();
	}
	
	Qt::ItemFlags flags(const QModelIndex &index) const
	{
		if (!index.isValid()) return Qt::NoItemFlags;
		const Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		return f;
	}

	int columnCount(const QModelIndex &parent) const { return COUNT_COL; }
	int rowCount(const QModelIndex &parent) const { return countElements(); }

	QVariant data(const QModelIndex &index, int role) const
	{
		const int elNum = index.row();
		
		if (role == Qt::UserRole) {
			if (!index.isValid() || elNum > countElements()) {
				return (-1);
			}
			return elNum;
		}
		if (!index.isValid() || elNum > countElements()) {
			return QVariant();
		}

		int attribute = index.column();
		if (attribute >= COUNT_COL) return QVariant();

		if (role == Qt::DisplayRole) {
			auto valPair = dllAndFunc.at(elNum);
			if (attribute == COL_LIB) {
				return valPair.first;
			}
			if (attribute == COL_FUNC) {
				return valPair.second;
			}
		}

		return QVariant();
	}
	
	bool setData(const QModelIndex &, const QVariant &, int) { return false; }

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
	{
		//no index item pointer
		return createIndex(row, column);
	}

	QModelIndex parent(const QModelIndex &index) const { return QModelIndex(); } // no parent

	void reloadSettings()
	{
		beginResetModel();
		dllAndFunc.clear();
		for (auto dItr = m_Settings.dllFunctions.begin(); dItr != m_Settings.dllFunctions.end(); ++dItr) {
			QString dllName = dItr.key();
			auto funcs = dItr.value();
			for (auto fItr = funcs.begin(); fItr != funcs.end(); ++fItr) {
				QString funcName = *fItr;
				dllAndFunc.append(QPair<QString,QString>(dllName, funcName));
			}
		}
		endResetModel();
	}

	int countElements() const
	{
		return dllAndFunc.size();
	}

	QPair<QString,QString> getPairAt(int elNum)
	{
		return dllAndFunc.at(elNum);
	}

protected:
	QList<QPair<QString,QString>> dllAndFunc;
	ImportsAutoadderSettings& m_Settings;
};

///---
class ImportsAddWindow : public QDialog
{
	Q_OBJECT

public:
	ImportsAddWindow(ImportsAutoadderSettings& _settings, QWidget *parent);
	~ImportsAddWindow() { }

public slots:
	void onAddClicked();
	void onRemoveClicked();
	void onSaveClicked();
	void onTableSelectionChanged(const QItemSelection &selected);

protected:
	QVBoxLayout topLayout;
	QHBoxLayout propertyLayout0,
		propertyLayout1, 
		propertyLayout2,
		propertyLayout3,
		propertyLayout4,
		propertyLayout5,
		propertyLayout6;

	QRegExpValidator *funcNameValidator;

	QHBoxLayout buttonLayout1;
	QHBoxLayout buttonLayout2;
	
	QLabel dllNameLabel;
	QLineEdit dllNameEdit;

	QLabel funcNameLabel;
	QLineEdit funcNameEdit;

	QPushButton addButton, removeButton,
		okButton, cancelButton;

	QLabel addSecLabel, separateOFTLabel;
	QCheckBox addSecCBox, separateOFTBox;

private:
	ImpAdderSettingsTableModel *tableModel;
	QTableView *ui_elementsView;
	ImportsAutoadderSettings& settings;
};
