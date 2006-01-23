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

#ifndef CSSRPG_CONSOLE_H
#define CSSRPG_CONSOLE_H

class ConVar;
class CRPG_Setting;

typedef void (var_callback)(CRPG_Setting *setting, char *str, char *oldstr);
#define VAR_FUNC(x) void x(CRPG_Setting *setting, char *str, char *oldstr)

#define SETTING_HAS_CALLBACK	0x01
#define SETTING_HAS_UPDATEVAR	0x02

enum var_type {var_str, var_float, var_ufloat, var_int, var_uint, var_bool};

#include "cssrpg_misc.h"
class CRPG_Setting: public CRPG_LinkedList<CRPG_Setting> {
	/* Private Functions */
	void setval_for_type(void);
	void* getval_for_type(void);
	static CRPG_Setting* new_setting(char *name, char *defaultval, char *desc, var_type type);

	/* Private Variables */
	var_type type;
	union {
		char s[256];
		float f;
		float uf;
		int i;
		unsigned int ui;
		bool b;
	} val;

	int flags;

public:
	/* Public Variables */
	ConVar *var;

	char name[32];
	char desc[512];
	char defaultval[256];

	void *update;
	var_callback *func;

	/* Public Functions */
	static void Init(void);
	static void FreeMemory(void);

	static void SettingChange(ConVar *var, char const *pOldString);

	static CRPG_Setting* CreateVar(char *name, char *defaultval, char *desc, var_type type);
	static CRPG_Setting* CreateVar(char *name, char *defaultval, char *desc, var_type type, void *update);
	static CRPG_Setting* CreateVar(char *name, char *defaultval, char *desc, var_type type, var_callback *func);
	static CRPG_Setting* CreateVar(char *name, char *defaultval, char *desc, var_type type, void *update, var_callback *func);

	void* value(void) {
		return getval_for_type();
	}
};

#endif
