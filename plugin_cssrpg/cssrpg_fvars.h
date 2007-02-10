/*
# Copyright (c) 2005-2006 CSS:RPG Mod for Counter-Strike Source(TM) Developement Team
#
# zlib/libpng License
# This software is provided 'as-is', without any express or implied warranty. In no event
# will the authors be held liable for any damages arising from the use of this software.
# Permission is granted to anyone to use this software for any purpose, including
# commercial applications, and to alter it and redistribute it freely, subject to the
# following restrictions:
#   1. The origin of this software must not be misrepresented; you must not claim that
#      you wrote the original software. If you use this software in a product, an
#      acknowledgment in the product documentation would be appreciated but is not required.
#   2. Altered source versions must be plainly marked as such, and must not be
#      misrepresented as being the original software.
#   3. This notice may not be removed or altered from any source distribution.
*/

#ifndef CSSRPG_FVARS_H
#define CSSRPG_FVARS_H

#define FVAR_DBNAME "filevars.txt"

/**
 * @brief File Variable manager class for constant variables that are loaded
 *        from the File Variable Database (FVAR_DBNAME).
 */
class CRPG_FileVar: public CRPG_StaticLinkedList<CRPG_FileVar> {
	/* Private Variables */
	char *path;
	char *key;
	unsigned int pathlen;

	struct {
		char *s;
		long l;
		unsigned long ul;
		float f;
		float uf;
		bool b;
	} value;

	/* Private Functions */
	void set_value(char *value);
	static class KeyValues* scan_kvtree(class KeyValues *kv, char *name);
	static void parse_fvdb(class KeyValues *kv);

public:
	/* Public Variables */
	char *name;

	/* Public Functions */
	CRPG_FileVar(char *name, char *defval, char *path);
	~CRPG_FileVar();

	static void LoadFVars(void);

	char *String(void) const {
		return value.s;
	}

	long Long(void) const {
		return value.l;
	}

	unsigned long ULong(void) const {
		return value.ul;
	}

	double Float(void) const {
		return value.f;
	}

	double UFloat(void) const {
		return value.uf;
	}

	bool Bool(void) const {
		return value.b;
	}
};

#endif