#include "Version.h"
#include "../rebear_ver_short.h"

using namespace pe_bear::updater;

const int Version::ComponentCount;
const int Version::MaxComponentDigits;

namespace {

	bool isAllDigits(const QString &s)
	{
		if (s.isEmpty()) return false;
		for (int i = 0; i < s.length(); i++) {
			const QChar c = s.at(i);
			/* deliberately ASCII-only: QChar::isDigit() also accepts other scripts */
			if (c < QLatin1Char('0') || c > QLatin1Char('9')) return false;
		}
		return true;
	}

}; // namespace

Version::Version()
	: m_valid(false)
{
	for (int i = 0; i < ComponentCount; i++) m_parts[i] = 0;
}

Version::Version(int major, int minor, int micro, int patch)
	: m_valid(true)
{
	m_parts[0] = major;
	m_parts[1] = minor;
	m_parts[2] = micro;
	m_parts[3] = patch;
	for (int i = 0; i < ComponentCount; i++) {
		if (m_parts[i] < 0) {
			m_valid = false;
			m_parts[i] = 0;
		}
	}
}

bool Version::looksLikePrerelease(const QString &text)
{
	const QString t = text.trimmed();
	/* SemVer prerelease ("-rc1") and build metadata ("+build") markers,
	   plus the loose "0.7.2rc1" and "0.7.2 beta" spellings seen in the wild */
	if (t.contains(QLatin1Char('-')) || t.contains(QLatin1Char('+'))
		|| t.contains(QLatin1Char('~')) || t.contains(QLatin1Char(' ')))
	{
		return true;
	}
	QString body = t;
	if (body.startsWith(QLatin1Char('v')) || body.startsWith(QLatin1Char('V'))) {
		body.remove(0, 1);
	}
	for (int i = 0; i < body.length(); i++) {
		const QChar c = body.at(i);
		if (c == QLatin1Char('.')) continue;
		if (c >= QLatin1Char('0') && c <= QLatin1Char('9')) continue;
		return true;
	}
	return false;
}

Version Version::fromString(const QString &text)
{
	const QString trimmed = text.trimmed();
	if (trimmed.isEmpty()) return Version();
	if (looksLikePrerelease(trimmed)) return Version();

	QString body = trimmed;
	if (body.startsWith(QLatin1Char('v')) || body.startsWith(QLatin1Char('V'))) {
		body.remove(0, 1);
	}
	if (body.isEmpty()) return Version();

	/* KeepEmptyParts: an empty component ("0..1", "0.7.") must be rejected,
	   not silently collapsed */
	const QStringList parts = body.split(QLatin1Char('.'));
	if (parts.size() < 2 || parts.size() > ComponentCount) return Version();

	int values[ComponentCount] = { 0, 0, 0, 0 };
	for (int i = 0; i < parts.size(); i++) {
		const QString &p = parts.at(i);
		if (!isAllDigits(p)) return Version();
		if (p.length() > MaxComponentDigits) return Version();
		bool ok = false;
		const int v = p.toInt(&ok);
		if (!ok || v < 0) return Version();
		values[i] = v;
	}
	return Version(values[0], values[1], values[2], values[3]);
}

Version Version::current()
{
	return Version(REBEAR_MAJOR_VERSION, REBEAR_MINOR_VERSION,
		REBEAR_MICRO_VERSION, REBEAR_PATCH_VERSION);
}

QString Version::toString() const
{
	if (!m_valid) return QString();

	int last = 2; /* always render at least major.minor.micro */
	for (int i = ComponentCount - 1; i > last; i--) {
		if (m_parts[i] != 0) {
			last = i;
			break;
		}
	}
	QString out;
	for (int i = 0; i <= last; i++) {
		if (i) out += QLatin1Char('.');
		out += QString::number(m_parts[i]);
	}
	return out;
}

int Version::compare(const Version &other) const
{
	for (int i = 0; i < ComponentCount; i++) {
		if (m_parts[i] < other.m_parts[i]) return -1;
		if (m_parts[i] > other.m_parts[i]) return 1;
	}
	return 0;
}
