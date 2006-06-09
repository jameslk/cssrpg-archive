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

#define TXTDB_KEY_COUNT 50 /* REMEMBER TO UPDATE THIS AS MORE KEYS ARE ADDED!!! */

typedef struct {
	char *s;
	char name[32];
	unsigned int id;
} key_t;

typedef struct {
	key_t *key_array[TXTDB_KEY_COUNT];

	struct {
		key_t msg1;
	} greeting;

	struct {
		key_t regen;
		key_t hbonus;
		key_t resup;
		key_t vamp;
		key_t stealth;
		key_t ljump;
		key_t fnade;
		key_t icestab;
		key_t fpistol;
		key_t denial;
	} items;

	struct {
		key_t msg1;
		key_t msg2;
	} newlvl;

	struct {
		key_t msg1;
		key_t msg2;
	} newbielvl;

	struct {
		key_t msg1;
	} exphint;

	struct {
		key_t msg1;
	} rpgrank;

	struct {
		key_t msg1;
	} rpgtop10;

	struct {
		key_t level;
		key_t exp_short;
		key_t exp_long;
		key_t credits;
		key_t rank;
		key_t cost;
		key_t sale;
	} menu_txt;

	struct {
		key_t stats_reset;
		key_t max_lvl;
		key_t not_enough_credits;
		key_t item_bought;
		key_t item_sold;
		key_t lang_changed;
	} menu_result;

	struct {
		key_t upgrades;
		key_t sell;
		key_t stats;
		key_t settings;
		key_t help;
		key_t next;
		key_t previous;
		key_t yes;
		key_t no;
		key_t reset_stats;
		key_t language;
		key_t exit;
	} menu_opt;

	struct {
		key_t sell_confirm;
		key_t reset_confirm;
	} menu_confirm;

	struct {
		key_t about;
		key_t upgrades;
		key_t commands;
		key_t acronyms;
	} menu_help;

	struct {
		key_t no_items;
	} menu_misc;
} txt_keys;

#include "cssrpg_misc.h"
class CRPG_TextDB: public CRPG_LinkedList<CRPG_TextDB> {
	/* Private Functions */
	void init_keyarray();

	static unsigned int assign_key(key_t *key, const char *str);
	void parse_txtfile(KeyValues *kv, unsigned int phase = 0);

public:
	/* Public Variables */
	static CRPG_TextDB *deflang; /* default langauge */

	file_info *file;
	char *name;
	char hidden;
	txt_keys txt;

	/* Public Functions */
	CRPG_TextDB(void) {
		file = NULL;
		name = NULL;
		hidden = 0;
		init_keyarray();
	}

	~CRPG_TextDB(void) {
		unsigned int i = TXTDB_KEY_COUNT;

		if(file != NULL)
			free(file);

		if(name != NULL)
			free(name);

		while(i--) {
			if(txt.key_array[i] != NULL)
				free(txt.key_array[i]->s);
		}
	}

	static void Init(void);
	static void ShutDown(void);

	key_t* NametoKey(char *namef, ...);
	key_t* IDtoKey(unsigned int id);

	static void LoadDefault(void);
	static void LoadLanguages(void);
	static void UnloadLanguages(void);
};

CRPG_TextDB *FiletoTextDB(char *filename);

#define TXTDB(player, type) player->lang->txt.type.s
#define TXTDB_ID(type) CRPG_TextDB::deflang->txt.type.id

#endif
