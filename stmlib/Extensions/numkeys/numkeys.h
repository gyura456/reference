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
/** \file numkeys.h
  * \brief Numeric keyboard.
  *
  * \author Gyorgy Stercz
  */
#ifndef NUMKEYS_H_INCLUDED
#define NUMKEYS_H_INCLUDED

#include <stdint.h>
/**
 * A GVKeyTable is a set of definitions that define how the keyboard lays out
 * its keys. A GVKeyTable consists of a number of GVKeySets and a special key table.
 *
 * A GVKeySet is a set of keys that make up the currently visible keyboard.
 * Special keys in the GVKeySet can be used to switch between GVKeySets within
 * the GVKeyTable. An example is a shift key which switches between the GVKeySet of
 * lower case keys and the GVKeySet of upper case keys. GVKeySet number 0 is special
 * in that it is the default GVKeySet when the keyboard is first displayed.
 *
 * A GVKeySet is made up of GVKeyRow's. Each GVKeyRow describes the keys on one row
 * of the keyboard.
 *
 * Each GVKeyRow covers a number of key columns. Different rows can have different numbers of columns.
 * eg. 'Q' -> 'P' has 10 keys while 'A' to 'L' has 9. Additionally each key can cover more than one
 * column position eg a wide space bar.
 *
 * Each GVKeyRow is just a string. Each character is the caption for one key. Using the same
 * character for two or more adjacent keys merges the keys into one big key covering multiple key columns.
 * Characters \001 to \037 (1 to 31) are special keys. How to handle and draw those is described by the
 * special key structure array. Special keys do things like changing keysets, returning characters less than 32,
 * have multiple character keycaps.
 *
 * Note: keycaps from the special key table with a single character from 1 to 31 in them may invoke special drawn
 * symbols eg. character 13 may cause a special symbol to be drawn for the enter key. Other than those characters
 * which are drawn as symbols by the keyboard draw function, all other characters for keycaps are drawn using the
 * current widget font.
 *
 * Special keycaps handled by the standard draw:
 * \001 (1)		- Shift (up arrow)
 * \002 (2)		- Shift locked (up arrow - bold)
 * \010 (8)		- Tab (right arrow)
 * \011 (9)		- BackSpace (left arrow)
 * \015 (13)	- Carriage Return (hooked left arrow)
 */

typedef struct GVSpecialKey {
	const char *keycap;				// The caption on the key
	const char *sendkey;				// The key to send (NULL means none)
	uint8_t		flags;						// Flags
		#define GVKEY_INVERT		0x01		//		Invert the color
		#define GVKEY_SINGLESET		0x02		//		Change set when this key is pressed but only for a single keystroke
		#define GVKEY_LOCKSET		0x04		//		Change set when this key is pressed but stay there until the set is changed by the user
	uint8_t		newset;							//		The new set to change to
	} GVSpecialKey;

typedef const char **GVKeySet;				// Array of Rows - Null indicates the end
typedef struct GVKeyTable {
	const GVSpecialKey *skeys;				// Array of Special Key structures
	const GVKeySet	*ksets;					// Array of KeySets - Null indicates the end
	} GVKeyTable;

extern const GVKeyTable NumKeys;

#endif // NUMKEYS_H_INCLUDED
