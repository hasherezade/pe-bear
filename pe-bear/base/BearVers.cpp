#include "BearVers.h"
#include "../QtCompat.h"
using namespace pe_bear;

BearVers::BearVers(int ma, int mi, int p, int s, const QString &desc)
	: vMajor(ma), vMinor(mi), vPatch(p), vSub(s), vDesc(desc), 
	valid(true)
{
}

BearVers::BearVers(QString replyString)
	: vMajor(0), vMinor(0), vPatch(0), vSub(0), vDesc(""), valid(false)
{
	replyString = replyString.trimmed();
	QStringList strings = replyString.split(".", QT_SkipEmptyParts);
	if (strings.length() < 3) return;

	this->vMajor = strings[0].toInt();
	this->vMinor = strings[1].toInt();
	this->vPatch = strings[2].toInt();
	this->vSub = (strings.length() > 3) ? 0 : strings[3].toInt();
	valid = true;
}

QString BearVers::toString()
{
	if (!isValid()) return "";
	QString str = QString::number(vMajor) + "." + QString::number(vMinor) + "." + QString::number(vPatch);
	if (vSub != 0) {
		str += "." + QString::number(vSub);
	}
	if (vDesc.length() > 0) {
		str += "-" + vDesc;
	}
	return str;
}

int BearVers::operator==(BearVers const &ver2) const
{
	return ((ver2.vMajor == this->vMajor) 
		&& (ver2.vMinor == this->vMinor) 
		&& (ver2.vPatch == this->vPatch)
		&& (ver2.vSub == this->vSub));
}

bool BearVers::operator< (BearVers const &ver2) const
{
	if (ver2.vMajor > this->vMajor) return true;
	if (ver2.vMajor < this->vMajor) return false;

	if (ver2.vMinor > this->vMinor) return true;
	if (ver2.vMinor < this->vMinor) return false;

	if (ver2.vPatch > this->vPatch) return true;
	if (ver2.vPatch < this->vPatch) return false;
	
	if (ver2.vSub > this->vSub) return true;
	if (ver2.vSub < this->vSub) return false;
	return false;
}

BearVers::VersionStatus BearVers::compare(BearVers &ver)
{
	if (!this->isValid() || !ver.isValid()) return BearVers::VER_INVALID;
	if ((*this) == ver) {
		return BearVers::VER_OK;
	}
	if (ver > (*this)) {
		return BearVers::VER_OLD;
	}
	return BearVers::VER_NEW;
}
