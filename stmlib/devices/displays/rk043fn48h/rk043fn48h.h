/*
 *   Copyright (C) 2017  Gyorgy Stercz
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/** \file rk043fn48h.h
  * \brief Rk043fn48h constants and handler functions.
  *
  * \author Gyorgy Stercz
  */
#ifndef RK043FN48H_H_INCLUDED
#define RK043FN48H_H_INCLUDED

/** \brief LCD size in pixel.
  *
  * \{
  */
#define LCD_WIDTH 480   /* Active width*/
#define LCD_HEIGHT 272   /* Active height*/
/** \} */

/** \brief LCD timing values.
  *
  * \{
  */
#define HSYNC 41    /* Horizontal synchronization */
#define VSYNC 10    /* Vertical synchronization */
#define HBP 13      /* Horizontal back porch */
#define VBP 2       /* Vertical back porch */
#define HFP 32      /* Horizontal front porch */
#define VFP 2       /* Vertical front porch */
/** \} */

#endif //RK043FN48H_H_INCULDED
