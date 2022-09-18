#include "ViewSettings.h"

QPixmap ViewSettings::makeScaledPixMap(const QString &resource, int width, int height)
{
	QPixmap pix(resource);
	pix = pix.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	return pix;
}

QIcon ViewSettings::makeScaledIcon(const QString &resource, int width, int height)
{
	return QIcon(ViewSettings::makeScaledPixMap(resource, width, height));
}
