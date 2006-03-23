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

#ifndef CSSRPG_H
#define CSSRPG_H

#define CSSRPG_VERSION "1.0.2"

#define CSSRPG_DB "cssrpg.db"
/* Columns: [int player_id] [string name] [string steamid] [int level] [int exp] [int credits] [int lastseen] [int items_id] */
#define TBL_PLAYERS "players"
/* Columns: [int items_id] [regen] [hbonus] [...] */
#define TBL_ITEMS "items"

#define ITEM_REGEN		0
#define ITEM_HBONUS		1
#define ITEM_RESUP		2
#define ITEM_VAMP		3
#define ITEM_STEALTH	4
#define ITEM_LJUMP		5
#define ITEM_FNADE		6
#define ITEM_ICESTAB	7

#define ITEM_COUNT		8

struct item_type {
	char name[16];
	char shortname[8];

	char enable;
	unsigned int maxlevel;

	unsigned int start_cost;
	unsigned int inc_cost;

	void (*buy_item)(void *ptr);
	void (*sell_item)(void *ptr);
};

#include "cssrpg_console.h"
class CRPG_GlobalSettings {
public:
	static bool enable;
	static bool bot_enable;
	static bool debug_mode;
	static bool save_data;
	static bool steamid_save;
	static unsigned int save_interval;
	static unsigned int player_expire;
	static bool announce_newlvl;

	static bool exp_notice;
	static unsigned int exp_max;
	static unsigned int exp_start;
	static unsigned int exp_inc;

	static float exp_damage;
	static float exp_knifedmg;
	static float exp_kill;
	static float exp_headshot;

	static float exp_teamwin;
	static float exp_bombplanted;
	static float exp_bombdefused;
	static float exp_bombexploded;
	static float exp_hostage;
	static float exp_vipescaped;

	static unsigned int credits_inc;
	static unsigned int credits_start;
	static float sale_percent;

	static void InitSettings(void);
};

#include "cssrpg_misc.h"
class CRPG_Database;
class CRPG: public CRPG_Utils, private CRPG_GlobalSettings {
	/* Private Functions */
	static void init_item_types(void);
	static void init_database(void);

public:
	/* Public Variables */
	static struct item_type item_types[ITEM_COUNT];
	static CRPG_Database *db;

	/* Public Functions */
	static void Init(void);
	static void ShutDown(void);

	static void DatabaseMaid(void);
};

enum cssteam_t {team_ct, team_t, team_none};

#include "cssrpg_stats.h"
class IPlayerInfo;
class CBasePlayer;
class CRPG_Timer;
class CRPG_Player: public CRPG_PlayerClass<CRPG_Player>, private CRPG_GlobalSettings {
public:
	/* Public Variables */
	static CRPG_Player **players;
	static unsigned int player_count;

	unsigned int level;
	unsigned int exp;
	unsigned long credits;

	struct {
		struct item_type *type;
		unsigned int level;
	} items[ITEM_COUNT];

	struct {
		int player_id;
		int items_id;
	} dbinfo;

	struct {
		cssteam_t team;
		char isdead;
	} css;

	/* Public Functions */
	CRPG_Player(): level(1), exp(0), credits(0) {
		unsigned int i = ITEM_COUNT;
		while(i--) {
			items[i].type = &CRPG::item_types[i];
			items[i].level = 0;
		}
		index = 0;
		userid = 0;

		dbinfo.player_id = -1;
		dbinfo.items_id = -1;

		css.team = team_none;
		css.isdead = 1;
	}

	static void Init(void);
	static void ShutDown(void);

	static void AutoSave(void);
	static void SaveAll(void);

	static CRPG_Player* AddPlayer(edict_t *e);
	unsigned int DelPlayer(void);

	void InsertPlayer(void);
	void LoadData(void);
	void SaveData(void);

	void AddLevel(unsigned int level) {
		CRPG_StatsManager::player_new_lvl(this, level);
	}
	void AddExp(unsigned int exp) {
		CRPG_StatsManager::add_exp(this, exp);
	}

	void ResetStats(void);

	unsigned int GiveItem(unsigned int item_index);
	unsigned int TakeItem(unsigned int item_index);
	unsigned int BuyItem(unsigned int item_index);
	unsigned int SellItem(unsigned int item_index);
};

CRPG_Player* IndextoRPGPlayer(int index);
CRPG_Player* EdicttoRPGPlayer(edict_t *e);
CRPG_Player* UserIDtoRPGPlayer(int userid);
CRPG_Player* SteamIDtoRPGPlayer(const char *steamid);

#define IF_BOT_ENABLED(x) if(!x->isfake() || (x->isfake() && CRPG_GlobalSettings::bot_enable))
#define IF_BOT_NENABLED(x) if(x->isfake() && !CRPG_GlobalSettings::bot_enable)

#endif
