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

#ifndef CSSRPG_TEXTDB_H
#define CSSRPG_TEXTDB_H

#ifdef WIN32
#define TEXTDB_PATH "\\cfg\\cssrpg\\languages\\"
#else
#define TEXTDB_PATH "/cfg/cssrpg/languages/"
#endif

#define TXTDB_KEY_COUNT 51 /* REMEMBER TO UPDATE THIS AS MORE KEYS ARE ADDED!!! */

typedef struct {
	char *s;
	char name[32];
	unsigned int id;
} txtkey_t;

typedef struct {
	txtkey_t *key_array[TXTDB_KEY_COUNT];

	struct {
		txtkey_t msg1;
	} greeting;

	struct {
		txtkey_t regen;
		txtkey_t hbonus;
		txtkey_t resup;
		txtkey_t vamp;
		txtkey_t stealth;
		txtkey_t ljump;
		txtkey_t fnade;
		txtkey_t icestab;
		txtkey_t fpistol;
		txtkey_t denial;
		txtkey_t impulse;
	} items;

	struct {
		txtkey_t msg1;
		txtkey_t msg2;
	} newlvl;

	struct {
		txtkey_t msg1;
		txtkey_t msg2;
	} newbielvl;

	struct {
		txtkey_t msg1;
	} exphint;

	struct {
		txtkey_t msg1;
	} rpgrank;

	struct {
		txtkey_t msg1;
	} rpgtop10;

	struct {
		txtkey_t level;
		txtkey_t exp_short;
		txtkey_t exp_long;
		txtkey_t credits;
		txtkey_t rank;
		txtkey_t cost;
		txtkey_t sale;
	} menu_txt;

	struct {
		txtkey_t stats_reset;
		txtkey_t max_lvl;
		txtkey_t not_enough_credits;
		txtkey_t item_bought;
		txtkey_t item_sold;
		txtkey_t lang_changed;
	} menu_result;

	struct {
		txtkey_t upgrades;
		txtkey_t sell;
		txtkey_t stats;
		txtkey_t settings;
		txtkey_t help;
		txtkey_t next;
		txtkey_t previous;
		txtkey_t yes;
		txtkey_t no;
		txtkey_t reset_stats;
		txtkey_t language;
		txtkey_t exit;
	} menu_opt;

	struct {
		txtkey_t sell_confirm;
		txtkey_t reset_confirm;
	} menu_confirm;

	struct {
		txtkey_t about;
		txtkey_t upgrades;
		txtkey_t commands;
		txtkey_t acronyms;
	} menu_help;

	struct {
		txtkey_t no_items;
	} menu_misc;
} txt_keys;

#include "cssrpg_misc.h"
class CRPG_TextDB: public CRPG_StaticLinkedList<CRPG_TextDB> {
	/* Private Functions */
	void init_keyarray();

	static unsigned int assign_key(txtkey_t *key, const char *str);
	void parse_txtfile(KeyValues *kv, unsigned int phase = 0);

public:
	/* Public Variables */
	static CRPG_TextDB *deflang; /* default langauge */

	file_info *file;
	char *name;
	char hidden;
	txt_keys txt;

	/* Public Functions */
	CRPG_TextDB(void): file(NULL), name(NULL), hidden(0) {
		init_keyarray();
	}

	~CRPG_TextDB(void);

	static void Init(void);
	static void ShutDown(void);

	txtkey_t* NametoKey(char *namef, ...);
	txtkey_t* IDtoKey(unsigned int id);

	static void LoadDefault(void);
	static void LoadLanguages(void);
	static void UnloadLanguages(void);
};

CRPG_TextDB *FiletoTextDB(char *filename);

#define TXTDB(player, type) player->lang->txt.type.s
#define TXTDB_ID(type) CRPG_TextDB::deflang->txt.type.id

#endif
