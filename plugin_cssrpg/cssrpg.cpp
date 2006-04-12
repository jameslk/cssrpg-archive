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

#include <stdio.h>
#include <time.h>

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"

#define GAME_DLL 1
#include "cbase.h"
#define GAME_DLL 1

#include "cssrpg_menu.h"
#include "cssrpg_console.h"
#include "cssrpg_database.h"

#include "items/rpgi.h"
#include "items/rpgi_regen.h"
#include "items/rpgi_resup.h"
#include "items/rpgi_hbonus.h"
#include "items/rpgi_vamp.h"
#include "items/rpgi_stealth.h"
#include "items/rpgi_ljump.h"
#include "items/rpgi_fnade.h"
#include "items/rpgi_icestab.h"

#include "cssrpg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*	//////////////////////////////////////
	CRPG Class 
	////////////////////////////////////// */
struct item_type CRPG::item_types[ITEM_COUNT];
CRPG_Database* CRPG::db;

void CRPG::Init(void) {
	init_item_types();
	init_database();

	return ;
}

void CRPG::ShutDown(void) {
	delete db;

	return;
}

void CRPG::init_item_types(void) {
	int i = ITEM_COUNT;
	while(i--)
		item_types[i].index = i;

	/* Regen */
	strcpy(item_types[ITEM_REGEN].name, "Regeneration");
	strcpy(item_types[ITEM_REGEN].shortname, "regen");
	item_types[ITEM_REGEN].maxlevelbarrier = 10;
	item_types[ITEM_REGEN].buy_item = CRPGI_Regen::BuyItem;
	item_types[ITEM_REGEN].sell_item = CRPGI_Regen::SellItem;

	/* HBonus */
	strcpy(item_types[ITEM_HBONUS].name, "Health+");
	strcpy(item_types[ITEM_HBONUS].shortname, "hbonus");
	item_types[ITEM_HBONUS].maxlevelbarrier = 16;
	item_types[ITEM_HBONUS].buy_item = CRPGI_HBonus::BuyItem;
	item_types[ITEM_HBONUS].sell_item = CRPGI_HBonus::SellItem;

	/* Resup */
	strcpy(item_types[ITEM_RESUP].name, "Resupply");
	strcpy(item_types[ITEM_RESUP].shortname, "resup");
	item_types[ITEM_RESUP].maxlevelbarrier = 10;
	item_types[ITEM_RESUP].buy_item = CRPGI_Resup::BuyItem;
	item_types[ITEM_RESUP].sell_item = CRPGI_Resup::SellItem;

	/* Vamp */
	strcpy(item_types[ITEM_VAMP].name, "Vampire");
	strcpy(item_types[ITEM_VAMP].shortname, "vamp");
	item_types[ITEM_VAMP].maxlevelbarrier = 10;
	item_types[ITEM_VAMP].buy_item = CRPGI_Vamp::BuyItem;
	item_types[ITEM_VAMP].sell_item = CRPGI_Vamp::SellItem;

	/* Stealth */
	strcpy(item_types[ITEM_STEALTH].name, "Stealth");
	strcpy(item_types[ITEM_STEALTH].shortname, "stealth");
	item_types[ITEM_STEALTH].maxlevelbarrier = 5;
	item_types[ITEM_STEALTH].buy_item = CRPGI_Stealth::BuyItem;
	item_types[ITEM_STEALTH].sell_item = CRPGI_Stealth::SellItem;

	/* LJump */
	strcpy(item_types[ITEM_LJUMP].name, "LongJump");
	strcpy(item_types[ITEM_LJUMP].shortname, "ljump");
	item_types[ITEM_LJUMP].maxlevelbarrier = 6;
	item_types[ITEM_LJUMP].buy_item = CRPGI_LJump::BuyItem;
	item_types[ITEM_LJUMP].sell_item = CRPGI_LJump::SellItem;

	/* FNade */
	strcpy(item_types[ITEM_FNADE].name, "FireGrenade");
	strcpy(item_types[ITEM_FNADE].shortname, "fnade");
	item_types[ITEM_FNADE].maxlevelbarrier = 6;
	item_types[ITEM_FNADE].buy_item = CRPGI_FNade::BuyItem;
	item_types[ITEM_FNADE].sell_item = CRPGI_FNade::SellItem;

	/* IceStab */
	strcpy(item_types[ITEM_ICESTAB].name, "IceStab");
	strcpy(item_types[ITEM_ICESTAB].shortname, "icestab");
	item_types[ITEM_ICESTAB].maxlevelbarrier = 5;
	item_types[ITEM_ICESTAB].buy_item = CRPGI_IceStab::BuyItem;
	item_types[ITEM_ICESTAB].sell_item = CRPGI_IceStab::SellItem;

	return ;
}

char *player_cols[] = {
	"player_id INTEGER PRIMARY KEY",
	"name TEXT DEFAULT ' '",
	"steamid TEXT DEFAULT '0'",
	"level INTEGER DEFAULT '1'",
	"exp INTEGER DEFAULT '0'",
	"credits INTEGER DEFAULT '0'",
	"lastseen INTEGER DEFAULT '0'",
	"items_id INTEGER DEFAULT '-1'"
};
char *player_col_types[] = {"player_id", "name", "steamid", "level", "exp", "credits", "lastseen", "items_id"};
unsigned int player_col_count = 8;
void CRPG::init_database(void) {
	unsigned int i, result;
	char *str;
	db = new CRPG_Database(CSSRPG_DB);

	result = db->TableExists(TBL_PLAYERS);
	if(!result) {
		str = new char[1024];
		memset(str, 0, 1024);
		for(i = 0;i < player_col_count;i++) {
			if(i)
				sprintf(str, "%s, %s", str, player_cols[i]);
			else
				sprintf(str, "%s", player_cols[i]);
		}

		db->Query("CREATE TABLE %s (%s)", TBL_PLAYERS, str);
		delete[] str;
	}
	else if(result == 1) {
		for(i = 1;i < player_col_count;i++) {
			if(!db->ColExists(player_col_types[i], TBL_PLAYERS))
				db->Query("ALTER TABLE %s ADD COLUMN %s INTEGER", TBL_PLAYERS, player_cols[i]);
		}
	}

	result = db->TableExists(TBL_ITEMS);
	if(!result) {
		str = new char[1024];
		memset(str, 0, 1024);
		sprintf(str, "items_id INTEGER PRIMARY KEY");
		for(i = 0;i < ITEM_COUNT;i++)
			sprintf(str, "%s, %s INTEGER DEFAULT '0'", str, CRPG::item_types[i].shortname);

		db->Query("CREATE TABLE %s (%s)", TBL_ITEMS, str);
		delete[] str;
	}
	else if(result == 1) {
		for(i = 0;i < ITEM_COUNT;i++) {
			if(!db->ColExists(CRPG::item_types[i].shortname, TBL_ITEMS))
				db->Query("ALTER TABLE %s ADD COLUMN %s INTEGER DEFAULT '0'", TBL_ITEMS, CRPG::item_types[i].shortname);
		}
	}

	return ;
}

void CRPG::DatabaseMaid(void) {
	tbl_result *result;
	int query_ok, i;

	if(!save_data)
		return ;

	/* Delete players who are Level 1 and haven't played for 3 days */
	db->Query("DELETE FROM %s WHERE level <= '1' AND lastseen <= '%d'", TBL_PLAYERS, time(NULL)-259200);

	if(player_expire) {
		query_ok = db->Query(&result, "SELECT items_id FROM %s WHERE lastseen <= '%d'", TBL_PLAYERS,
			time(NULL)-(86400*CRPG_GlobalSettings::player_expire));

		if(query_ok && (result != NULL)) {
			if(result->col_count == 1) {
				for(i = 1;i < result->row_count;i++) {
					db->Query("DELETE FROM %s WHERE items_id = '%s'", TBL_ITEMS, result->array[i][0]);
					db->Query("DELETE FROM %s WHERE items_id = '%s'", TBL_PLAYERS, result->array[i][0]);
				}
			}
			FreeResult(result);
		}
		else if(!query_ok) {
			CRPG::DebugMsg("DatabaseMaid: player expire query failed");
		}
	}

	db->Query("VACUUM %s", TBL_PLAYERS);
	db->Query("VACUUM %s", TBL_ITEMS);

	return ;
}

/*	//////////////////////////////////////
	CRPG_Player Class 
	////////////////////////////////////// */
template class CRPG_PlayerClass<CRPG_Player>;
template<> CRPG_Player** CRPG_PlayerClass<CRPG_Player>::nodes;
CRPG_Player** CRPG_Player::players = NULL;
unsigned int CRPG_Player::player_count;

void CRPG_Player::Init(void) {
	unsigned int i;
	player_count = (unsigned int)CRPG::maxClients();
	i = player_count;

	players = new CRPG_Player *[player_count];
	while(i--)
		players[i] = NULL;

	nodes = players;

	return ;
}

void CRPG_Player::ShutDown(void) {
	SaveAll();
	free_nodes(player_count);
	players = NULL;

	return ;
}

/* Each player is saved each frame */
void CRPG_Player::AutoSave(void) {
	static int index = player_count-1; /* the player array starts at 0 */
	static float nextrun = gpGlobals->curtime+(float)save_interval;

	if(gpGlobals->curtime < nextrun)
		return ;

	if(!enable || !save_data || !save_interval || (players == NULL)) {
		index = player_count-1;
		nextrun = gpGlobals->curtime+60; /* Check again in 1 minute */
		return ;
	}
	else if(index < 0) {
		index = player_count-1;
		nextrun = gpGlobals->curtime+save_interval;
		return ;
	}

	if(players[index] != NULL)
		players[index]->SaveData();

	index--;
	return ;
}

/* All players are saved on the same frame */
void CRPG_Player::SaveAll(void) {
	unsigned int i = player_count;

	if(!enable || !save_data || (players == NULL))
		return ;

	while(i--) {
		if(players[i] != NULL)
			players[i]->SaveData();
	}

	return ;
}

CRPG_Player* IndextoRPGPlayer(int index) {
	return CRPG_Player::IndextoHandle(index);
}

CRPG_Player* EdicttoRPGPlayer(edict_t *e) {
	return CRPG_Player::EdicttoHandle(e);
}

CRPG_Player* UserIDtoRPGPlayer(int userid) {
	unsigned int i = CRPG_Player::player_count;

	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			if(CRPG_Player::players[i]->userid == userid)
				return CRPG_Player::players[i];
		}
	}

	return NULL;
}

CRPG_Player* SteamIDtoRPGPlayer(const char *steamid) {
	unsigned int i = CRPG_Player::player_count;

	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			if(!strcmp(CRPG_Player::players[i]->steamid(), steamid))
				return CRPG_Player::players[i];
		}
	}

	return NULL;
}

CRPG_Player* CRPG_Player::AddPlayer(edict_t *e) {
	CRPG_Player *player;
	unsigned int i, maxlevel;

	player = new_node(e);
	WARN_IF(player == NULL, return NULL)

	CRPG::DebugMsg(1, "New Player: %s Index: %d UserID: %d SteamID: %s",
		player->name(), player->index, player->userid, player->steamid());

	player->credits = credits_start;
	player->LoadData(1);

	for(i = 0;i < ITEM_COUNT;i++) {
		maxlevel = CRPG::item_types[i].maxlevel;
		while(player->items[i].level > maxlevel) {
			/* Give player their credit's back */
			/* TakeItem isn't necessary since the player hasn't even been completely added yet */
			player->credits += CRPGI::GetItemCost(i, player->items[i].level--);
		}
	}

	return player;
}

unsigned int CRPG_Player::DelPlayer(void) {
	if(this->css.team == team_t)
		CRPG_TeamBalance::teamt_count--;
	else if(this->css.team == team_ct)
		CRPG_TeamBalance::teamct_count--;

	this->SaveData();
	return del_node(this);
}

void CRPG_Player::InsertPlayer(void) {
	unsigned int i;

	if(!enable || !save_data)
		return ;

	CRPG::db->Query("INSERT INTO %s (name, steamid, level, exp, credits, lastseen) VALUES ('%q', '%q', '%ld', '%ld', '%ld', '%ld')",
		TBL_PLAYERS, this->name(), this->steamid(), this->level, this->exp, this->credits, time(NULL));
	this->dbinfo.player_id = CRPG::db->GetInsertKey();

	char *str1 = new char[1024], *str2 = new char[1024];
	memset(str1, 0, 1024);
	memset(str2, 0, 1024);

	for(i = 0;i < ITEM_COUNT;i++) {
		if(i) {
			sprintf(str1, "%s, %s", str1, CRPG::item_types[i].shortname);
			sprintf(str2, "%s, '0'", str2);
		}
		else {
			sprintf(str1, "%s", CRPG::item_types[i].shortname);
			sprintf(str2, "'0'");
		}
	}

	CRPG::db->Query("INSERT INTO %s (%s) VALUES (%s)", TBL_ITEMS, str1, str2);
	delete[] str1, str2;
	this->dbinfo.items_id = CRPG::db->GetInsertKey();

	CRPG::db->Query("UPDATE %s SET items_id = '%d' WHERE player_id = '%d'",
		TBL_PLAYERS, this->dbinfo.items_id, this->dbinfo.player_id);

	return ;
}

void CRPG_Player::LoadData(char init) {
	unsigned int retval, i;
	struct tbl_result *result;

	if(!enable)
		return ;

	if(!bot_enable && (CRPG::istrcmp((char*)this->steamid(), "BOT")))
		return ;

	/* Player Info */
	if(steamid_save && CRPG::steamid_check((char*)this->steamid())) {
		retval = CRPG::db->Query(&result,
			"SELECT * FROM %s WHERE steamid = '%q' ORDER BY level DESC LIMIT 1",
			TBL_PLAYERS, this->steamid());
	}
	else {
		retval = CRPG::db->Query(&result,
			"SELECT * FROM %s WHERE name = '%q' AND steamid = '%q' ORDER BY level DESC LIMIT 1",
			TBL_PLAYERS, this->name(), this->steamid());
	}

	if(!retval) {
		CRPG::ConsoleMsg("Unable to load player data for %s", MTYPE_ERROR, this->name());
		FreeResult(result);
		return ;
	}

	if(result == NULL) {
		this->InsertPlayer();
		return ;
	}

	this->dbinfo.player_id = atoi(GetCell(result, "player_id"));
	this->dbinfo.items_id = atoi(GetCell(result, "items_id"));
	
	this->level = atoi(GetCell(result, "level"));
	this->exp = atoi(GetCell(result, "exp"));
	this->credits = atoi(GetCell(result, "credits"));

	FreeResult(result);

	/* Player Items */
	retval = CRPG::db->Query(&result, "SELECT * FROM %s WHERE items_id = '%d'",
		TBL_ITEMS, this->dbinfo.items_id);

	if(!retval) {
		CRPG::ConsoleMsg("Unable to load item data for %s", MTYPE_ERROR, this->name());
		FreeResult(result);
		return ;
	}

	if(result == NULL) {
		this->InsertPlayer();
		return ;
	}

	for(i = 0;i < ITEM_COUNT;i++)
		this->items[i].level = atoi(GetCell(result, CRPG::item_types[i].shortname));

	if(!init)
		CRPGI::PlayerUpdate(this);

	FreeResult(result);
	return ;
}

void CRPG_Player::SaveData(void) {
	unsigned int result, i;
	char *set_str;

	if(!enable || !save_data)
		return ;

	if(!bot_enable && (CRPG::istrcmp((char*)this->steamid(), "BOT")))
		return ;

	if(this->dbinfo.player_id < 0) {
		this->InsertPlayer();
		return ;
	}

	set_str = (char*)calloc(1024, sizeof(char));
	sprintf(set_str, "level = '%ld', exp = '%ld', credits = '%ld', lastseen = '%ld'",
		this->level, this->exp, this->credits, time(NULL));

	result = CRPG::db->Query("UPDATE %s SET %s WHERE player_id = '%d'",
		TBL_PLAYERS, set_str, this->dbinfo.player_id);

	if(!result)
		CRPG::ConsoleMsg("Failed to save player data for %s", MTYPE_ERROR, this->name());

	memset(set_str, 0, 1024);
	for(i = 0;i < ITEM_COUNT;i++) {
		if(i)
			sprintf(set_str, "%s, %s = '%d'", set_str, CRPG::item_types[i].shortname, this->items[i].level);
		else
			sprintf(set_str, "%s = '%d'", CRPG::item_types[i].shortname, this->items[i].level);
	}

	result = CRPG::db->Query("UPDATE %s SET %s WHERE items_id = '%d'",
		TBL_ITEMS, set_str, this->dbinfo.items_id);

	if(!result)
		CRPG::ConsoleMsg("Failed to save item data for %s", MTYPE_ERROR, this->name());

	free(set_str);
	return ;
}

void CRPG_Player::ResetStats(void) {
	unsigned int i = ITEM_COUNT;

	CRPG::DebugMsg("Stats have been reset for player: %s", this->name());

	while(i--) {
		items[i].level = 0;
		CRPG::item_types[i].sell_item(this);
	}

	this->level = 1;
	this->exp = 0;
	this->credits = 0;

	CRPGI::PlayerUpdate(this);

	return ;
}

unsigned int CRPG_Player::GiveItem(unsigned int item_index) {
	WARN_IF(item_index >= ITEM_COUNT, return 0)

	if(this->items[item_index].level >= CRPG::item_types[item_index].maxlevel)
		return 0;

	this->items[item_index].level++;
	CRPG::item_types[item_index].buy_item(this); /* do any item-specific activation */
	CRPGI::PlayerUpdate(this);

	return 1;
}

unsigned int CRPG_Player::TakeItem(unsigned int item_index) {
	WARN_IF(item_index >= ITEM_COUNT, return 0)

	if(this->items[item_index].level <= 0)
		return 0;

	this->items[item_index].level--;
	CRPG::item_types[item_index].sell_item(this); /* do any item-specific deactivation */
	CRPGI::PlayerUpdate(this);

	return 1;
}

unsigned int CRPG_Player::BuyItem(unsigned int item_index) {
	unsigned int cost;

	WARN_IF(item_index >= ITEM_COUNT, return 0)

	if(this->items[item_index].level >= CRPG::item_types[item_index].maxlevel)
		return 0;

	cost = CRPGI::GetItemCost(item_index, this->items[item_index].level+1);

	if(cost > this->credits)
		return 0;

	CRPG::DebugMsg(1, "%s bought item %s Lvl %d",
		this->name(), CRPG::item_types[item_index].name, this->items[item_index].level);

	if(!this->GiveItem(item_index))
		return 0;

	this->credits -= cost;

	return 1;
}

unsigned int CRPG_Player::SellItem(unsigned int item_index) {
	WARN_IF(item_index >= ITEM_COUNT, return 0)

	if(this->items[item_index].level <= 0)
		return 0;

	CRPG::DebugMsg(1, "%s sold item %s Lvl %d",
		this->name(), CRPG::item_types[item_index].name, this->items[item_index].level+1);

	if(!this->TakeItem(item_index))
		return 0;

	this->credits += CRPGI::GetItemSale(item_index, this->items[item_index].level);

	return 1;
}
