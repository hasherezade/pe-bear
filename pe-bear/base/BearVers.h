#pragma once

#include <QtCore>

namespace pe_bear {
class BearVers
{
public:
	enum VersionStatus { VER_INVALID = (-1), VER_OK, VER_OLD, VER_NEW };

	BearVers(int major, int minor, int patch, int sub, QString desc="");
	BearVers(QString s);
	bool isValid() { return valid; }

	VersionStatus compare(BearVers &latestVer);
	QString toString();

	int operator==(BearVers const &ver) const;
	int operator!=(BearVers const &ver) const { return !((*this) == ver); }

	bool operator< (BearVers const &ver) const;
	bool operator> (BearVers const &ver) const { if ((*this) == ver) return false; return !((*this) < ver); }

protected:
	int vMajor, vMinor, vPatch, vSub;
	QString vDesc;
	bool valid;
};
}; // namespace pe_bear
