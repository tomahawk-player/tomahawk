/*
   Copyright (C) 2009 by Aurélien Gâteau <aurelien.gateau@canonical.com>
   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
   
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
#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

class QVariant;
class QImage;

namespace ImageConverter
{

/**
 * Returns a variant representing an image using the format describe in the
 * galago spec
 */
QVariant variantForImage(const QImage &image);

} // namespace

#endif /* IMAGECONVERTER_H */
