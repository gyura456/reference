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
/** \file numkeys.c
  * \brief  Numeric keyboard source.
  */
#include <numkeys.h>

static const GVSpecialKey Eng1SKeys[] = {
			{ "\001", 0, GVKEY_SINGLESET, 1 },				// \001 (1)	= Shift Lower to Upper
			{ "\001", 0, GVKEY_INVERT|GVKEY_LOCKSET, 2 },	// \002 (2)	= Shift Upper to Upper Lock
			{ "\002", 0, GVKEY_INVERT|GVKEY_LOCKSET, 0 },	// \003 (3)	= Shift Upper Lock to Lower
			{ "123", 0, GVKEY_LOCKSET, 3 },					// \004 (4)	= Change to Numbers
			{ "\010", "\b", 0, 0 },							// \005 (5)	= Backspace
			{ "\015", "\r", 0, 0 },							// \006 (6)	= Enter 1
			{ "\015", "\r", 0, 0 },							// \007 (7)	= Enter 2 (Short keycap)
			{ "Sym", 0, GVKEY_LOCKSET, 4 },					// \010 (8)	= Change to Symbols
			{ "aA", 0, GVKEY_LOCKSET, 0 },					// \011 (9)	= Change to Lower Alpha
	};
	static const char NumSetRow0[] = "\0050\006";
	static const char NumSetRow1[] = "123";
	static const char NumSetRow2[] = "456";
	static const char NumSetRow3[] = "789";
	static const char *NumSet[] = {NumSetRow3, NumSetRow2, NumSetRow1, NumSetRow0, 0};
	static const GVKeySet NumSets[] = {NumSet, 0};
const GVKeyTable NumKeys = {Eng1SKeys, NumSets};
