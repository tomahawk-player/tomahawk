/*
   Copyright (C) 2009 by Aurélien Gâteau <aurelien.gateau@canonical.com>
   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */
#include "ImageConverter.h"

#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusMetaType>
#include <QtGui/QImage>

#include "utils/Logger.h"

namespace ImageConverter
{

/**
 * A structure representing an image which can be marshalled to fit the
 * notification spec.
 */
struct SpecImage
{
	int width, height, rowStride;
	bool hasAlpha;
	int bitsPerSample, channels;
	QByteArray data;
};

QDBusArgument &operator<<(QDBusArgument &argument, const SpecImage &image)
{
	argument.beginStructure();
	argument << image.width << image.height << image.rowStride << image.hasAlpha;
	argument << image.bitsPerSample << image.channels << image.data;
	argument.endStructure();
	return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, SpecImage &image)
{
	argument.beginStructure();
	argument >> image.width >> image.height >> image.rowStride >> image.hasAlpha;
	argument >> image.bitsPerSample >> image.channels >> image.data;
	argument.endStructure();
	return argument;
}

} // namespace

// This must be before the QVariant::fromValue below (#211726)
Q_DECLARE_METATYPE(ImageConverter::SpecImage)

namespace ImageConverter
{
QVariant variantForImage(const QImage &_image)
{
	qDBusRegisterMetaType<SpecImage>();

	QImage image = _image.convertToFormat(QImage::Format_ARGB32);

	int rowStride = image.width() * 4;

	// Notification spec stores pixels in R,G,B,A order, regardless of
	// endianess
	// Qt represents pixels as 32 bit unsigned int. So the order depend on
	// endianess:
	// - In big endian the order is A,R,G,B
	// - In little endian the order is B,G,R,A
	QByteArray data;
	data.resize(rowStride * image.height());
	char* dst = data.data();
	for (int y=0; y<image.height(); ++y) {
		QRgb* src = (QRgb*)image.scanLine(y);
		QRgb* end = src + image.width();
		for (;src != end; ++src) {
			// Probably slow, but free of endianess issues
			*dst++ = qRed(*src);
			*dst++ = qGreen(*src);
			*dst++ = qBlue(*src);
			*dst++ = qAlpha(*src);
		}
	}

	SpecImage specImage;
	specImage.width = image.width();
	specImage.height = image.height();
	specImage.rowStride = rowStride;
	specImage.hasAlpha = true;
	specImage.bitsPerSample = 8;
	specImage.channels = 4;
	specImage.data = data;

	return QVariant::fromValue(specImage);
}

} // namespace

