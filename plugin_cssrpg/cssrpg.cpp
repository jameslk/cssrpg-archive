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
	/* Regen */
	strcpy(item_types[ITEM_REGEN].name, "Regeneration");
	strcpy(item_types[ITEM_REGEN].shortname, "regen");
	item_types[ITEM_REGEN].maxlevel = 5;
	item_types[ITEM_REGEN].buy_item = CRPGI_Regen::BuyItem;
	item_types[ITEM_REGEN].sell_item = CRPGI_Regen::SellItem;

	/* HBonus */
	strcpy(item_types[ITEM_HBONUS].name, "Health+");
	strcpy(item_types[ITEM_HBONUS].shortname, "hbonus");
	item_types[ITEM_HBONUS].maxlevel = 16;
	item_types[ITEM_HBONUS].buy_item = CRPGI_HBonus::BuyItem;
	item_types[ITEM_HBONUS].sell_item = CRPGI_HBonus::SellItem;

	/* Resup */
	strcpy(item_types[ITEM_RESUP].name, "Resupply");
	strcpy(item_types[ITEM_RESUP].shortname, "resup");
	item_types[ITEM_RESUP].maxlevel = 5;
	item_types[ITEM_RESUP].buy_item = CRPGI_Resup::BuyItem;
	item_types[ITEM_RESUP].sell_item = CRPGI_Resup::SellItem;

	/* Vamp */
	strcpy(item_types[ITEM_VAMP].name, "Vampire");
	strcpy(item_types[ITEM_VAMP].shortname, "vamp");
	item_types[ITEM_VAMP].maxlevel = 10;
	item_types[ITEM_VAMP].buy_item = CRPGI_Vamp::BuyItem;
	item_types[ITEM_VAMP].sell_item = CRPGI_Vamp::SellItem;

	/* Stealth */
	strcpy(item_types[ITEM_STEALTH].name, "Stealth");
	strcpy(item_types[ITEM_STEALTH].shortname, "stealth");
	item_types[ITEM_STEALTH].maxlevel = 5;
	item_types[ITEM_STEALTH].buy_item = CRPGI_Stealth::BuyItem;
	item_types[ITEM_STEALTH].sell_item = CRPGI_Stealth::SellItem;

	/* LJump */
	strcpy(item_types[ITEM_LJUMP].name, "LongJump");
	strcpy(item_types[ITEM_LJUMP].shortname, "ljump");
	item_types[ITEM_LJUMP].maxlevel = 5;
	item_types[ITEM_LJUMP].buy_item = CRPGI_LJump::BuyItem;
	item_types[ITEM_LJUMP].sell_item = CRPGI_LJump::SellItem;

	/* FNade */
	strcpy(item_types[ITEM_FNADE].name, "FireGrenade");
	strcpy(item_types[ITEM_FNADE].shortname, "fnade");
	item_types[ITEM_FNADE].maxlevel = 5;
	item_types[ITEM_FNADE].buy_item = CRPGI_FNade::BuyItem;
	item_types[ITEM_FNADE].sell_item = CRPGI_FNade::SellItem;

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

	db->Query("DELETE FROM %s WHERE level <= '1'", TBL_PLAYERS);
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
CRPG_Player** CRPG_Player::players;
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
	AutoSave(1);
	free_nodes(player_count);

	return ;
}

void CRPG_Player::AutoSave(char force) { /* Each player is saved each frame */
	static int saveplayer = player_count;
	static float nextrun = gpGlobals->curtime+AUTOSAVE_DURATION;

	if(!force && (gpGlobals->curtime < nextrun))
		return ;

	if((saveplayer < 0) || !enable || !save_data) {
		saveplayer = player_count;
		nextrun = gpGlobals->curtime+AUTOSAVE_DURATION;
		return ;
	}

	CRPG::DebugMsg("Player: %d", saveplayer);

	if(players[saveplayer] != NULL)
		players[saveplayer]->SaveData();

	saveplayer--;
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
	if(player == NULL)
		return NULL;

	CRPG::DebugMsg("New Player: %s Index: %d UserID: %d SteamID: %s",
		player->name(), player->index, player->userid, player->steamid());

	player->credits = credits_start;
	player->LoadData();

	for(i = 0;i < ITEM_COUNT;i++) {
		maxlevel = CRPG::item_types[i].maxlevel;
		while(player->items[i].level > maxlevel) {
			/* Give player their credit's back */
			player->credits += CRPGI::GetItemCost(i, player->items[i].level--);
		}
	}

	return player;
}

unsigned int CRPG_Player::DelPlayer(void) {
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

void CRPG_Player::LoadData(void) {
	unsigned int retval, i;
	struct tbl_result *result;

	if(!enable)
		return ;

	if(!bot_enable && (CRPG::istrcmp((char*)this->steamid(), "BOT")))
		return ;

	/* Player Info */
	retval = CRPG::db->Query(&result, "SELECT * FROM %s WHERE name = '%q' AND steamid = '%q'",
		TBL_PLAYERS, this->name(), this->steamid());

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

unsigned int CRPG_Player::BuyItem(unsigned int item_index) {
	unsigned int cost;

	cost = CRPGI::GetItemCost(item_index, this->items[item_index].level+1);

	if(cost > this->credits)
		return 0;

	this->credits -= cost;
	this->items[item_index].level++;
	CRPG::item_types[item_index].buy_item(this); /* do any item-specific activation */

	CRPG::DebugMsg("%s bought item %s Lvl %d",
		this->name(), CRPG::item_types[item_index].name, this->items[item_index].level);

	CRPGI::PlayerUpdate(this);

	return 1;
}

unsigned int CRPG_Player::SellItem(unsigned int item_index) {
	this->credits += CRPGI::GetItemSale(item_index, this->items[item_index].level);
	this->items[item_index].level--;
	CRPG::item_types[item_index].sell_item(this); /* do any item-specific deactivation */

	CRPG::DebugMsg("%s sold item %s Lvl %d",
		this->name(), CRPG::item_types[item_index].name, this->items[item_index].level+1);

	CRPGI::PlayerUpdate(this);

	return 1;
}
