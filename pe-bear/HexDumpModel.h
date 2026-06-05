#pragma once
#include <QtGlobal>
#include <QVector>
#include <QPair>

#include "QtCompat.h"
#include "gui_base/PeTableModel.h"
#include "ViewSettings.h"

class HexDumpModel : public PeTableModel//QAbstractTableModel, public PeViewItem
{
	Q_OBJECT

signals:
	void scrollReset();

public slots:
	void setHexView(bool isSet) { showHex = isSet; reset(); }

	void setShownContent(offset_t start, bufsize_t size);

	// Override: refresh in place instead of a full reset when the grid layout is
	// unchanged, so open cell editors are never released during (fast) editing.
	virtual void onNeedReset();

	// Records a changed file region for a precise, deferred dataChanged (see onNeedReset).
	void onContentReplaced(offset_t modOffset, bufsize_t modSize);

public:
	HexDumpModel(PeHandler *peHndl, bool isHex, QObject *parent = 0);
	
	Executable::addr_type getAddrType() { return this->addrType; }
	bool isHexView() const { return showHex; }

	bufsize_t getPageSize() { return pageSize; };
	offset_t getStartOff() { return startOff; }

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;

	QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &, const QVariant &, int role);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual offset_t contentOffsetAt(const QModelIndex &index) const;
	QVariant getRawContentAt(const QModelIndex &index) const;
	QVariant getElement(offset_t offset) const;

	virtual ViewSettings* getSettings()
	{
		return &settings;
	}

	void changeSettings(HexViewSettings &newSettings) 
	{
		settings = newSettings;
	}

protected:
	virtual void connectSignals();

	// Maps a [from, to) file-offset range to a clamped row/col rect and emits dataChanged.
	void emitRegionChanged(offset_t from, offset_t to, int rows, int cols);

	Executable::addr_type addrType;

private:
	HexViewSettings settings;
	bool showHex;
	offset_t startOff, endOff;
	bufsize_t pageSize;
	int m_lastRowCount; // tracks layout changes for onNeedReset()

	// Pending changed regions [from, to) awaiting a deferred, targeted dataChanged.
	QVector<QPair<offset_t, offset_t> > m_pendingRegions;
	static const int MAX_PENDING_REGIONS = 16;

friend class HexTableView;
friend class OffsetHeader;
};
