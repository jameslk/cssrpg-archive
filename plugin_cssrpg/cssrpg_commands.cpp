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

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "engine/IEngineSound.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"

#include "cssrpg.h"
#include "cssrpg_bot.h"
#include "cssrpg_textdb.h"
#include "cssrpg_database.h"
#include "items/rpgi.h"
#include "cssrpg_commands.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*	//////////////////////////////////////
	CRPG_Commands Class
	////////////////////////////////////// */
template class CRPG_LinkedList<CRPG_Commands>;
template<> CRPG_Commands* CRPG_LinkedList<CRPG_Commands>::ll_first;
template<> CRPG_Commands* CRPG_LinkedList<CRPG_Commands>::ll_last;
template<> unsigned int CRPG_LinkedList<CRPG_Commands>::ll_count;

CRPG_Commands::CRPG_Commands(char *reg, char *desc, unsigned int req_argu, char *argu_template, rpgcmd_func *func) {
	strncpy(name, reg, 63);
	name[64] = '\0';
	strncpy(info, desc, 127);
	info[128] = '\0';

	req_argc = req_argu;
	strncpy(arg_template, argu_template, 127);
	arg_template[128] = '\0';

	call = func;

	this->ll_add();
	return ;
}

CRPG_Commands::~CRPG_Commands() {
	this->ll_del();

	return ;
}

void CRPG_Commands::ExecCmd(void) {
	CRPG_Commands *cmd;
	char *alias = (char*)CRPG::s_engine()->Cmd_Argv(0)+RPGCMD_PREFIX_LEN;
	char **argv;
	int argc = CRPG::s_engine()->Cmd_Argc()-1, i, result;

	if(argc) {
		argv = (char**)malloc(argc*sizeof(char*));
		for(i = 0;i < argc;i++)
			argv[i] = (char*)CRPG::s_engine()->Cmd_Argv(i+1);
	}

	for(cmd = ll_first;cmd != NULL;cmd = cmd->ll_next) {
		if(CRPG::istrcmp((char*)alias, cmd->name)) {
			if((unsigned int)argc >= cmd->req_argc)
				result = cmd->call(CRPG::s_engine()->Cmd_Argc()-1, argv,
					(char*)CRPG::s_engine()->Cmd_Args(), alias-RPGCMD_PREFIX_LEN);
			else
				CRPG::ConsoleMsg("%s%s %s", NULL, RPGCMD_PREFIX, cmd->name, cmd->arg_template);

			if(!result)
				CRPG::DebugMsg("Command \"%s\" failed", alias-RPGCMD_PREFIX_LEN);

			if(argc)
				free(argv);

			return ;
		}
	}

	CRPG::DebugMsg("Command not found: %s", alias-RPGCMD_PREFIX_LEN);

	if(argc)
		free(argv);

	return ;
}

/*	//////////////////////////////////////
	Commands
	////////////////////////////////////// */
#define GET_PLAYER_OR_RETURN(name, buf) \
	buf = IndextoRPGPlayer(CRPG::FindPlayer(name)); \
	if(buf == NULL) { \
		CRPG::ConsoleMsg("Couldn't find player: %s", thiscmd, name); \
		return 1; \
	}

#define GET_UPGRADE_OR_RETURN(upgrade, index, buf) \
	buf = NULL; \
	for(index = 0;index < ITEM_COUNT;index++) { \
		if(CRPG::istrcmp(CRPG::item_types[index].shortname, upgrade)) \
			buf = &CRPG::item_types[index]; \
	} \
	if(buf == NULL) { \
		for(index = 0;index < ITEM_COUNT;index++) { \
			if(CRPG::istrcmp(CRPG::item_types[index].name, upgrade)) \
				buf = &CRPG::item_types[index]; \
		} \
		if(buf == NULL) { \
			CRPG::ConsoleMsg("Couldn't find specified Upgrade: %s", thiscmd, upgrade); \
			return 1; \
		} \
	} \
	index = buf->index;

RPG_CMD(help, "List of CSS:RPG commands and help for specific commands", 0, "[command name]") {
	CRPG_Commands *cmd = NULL;

	if(argc) {
		for(cmd = CRPG_Commands::ll_first;cmd != NULL;cmd = cmd->ll_next) {
			if((strlen(argv[0]) == (RPGCMD_PREFIX_LEN+strlen(cmd->name))) && CRPG::istrcmp(argv[0]+RPGCMD_PREFIX_LEN, cmd->name)) {
				CRPG::ConsoleMsg("%s%s %s", NULL, RPGCMD_PREFIX, cmd->name, cmd->arg_template);
				CRPG::ConsoleMsg("- %s", NULL, cmd->info);
				break;
			}
		}
		if(cmd == NULL)
			CRPG::ConsoleMsg("Command '%s' not found", thiscmd, argv[0]);
	}
	else {
		CRPG::ConsoleMsg("", "Commands");
		for(cmd = CRPG_Commands::ll_first;cmd != NULL;cmd = cmd->ll_next)
			CRPG::ConsoleMsg("%s%s - %s", NULL, RPGCMD_PREFIX, cmd->name, cmd->info);
	}

	return 1;
}

RPG_CMD(player, "Get info about a certain player", 1, "<player name | userid | steamid>") {
	CRPG_Player *player;
	unsigned int i;

	GET_PLAYER_OR_RETURN(argv[0], player)

	CRPG::ConsoleMsg("----------", NULL);
	CRPG::ConsoleMsg("", (char*)player->name());

	CRPG::ConsoleMsg("Index: %ld, UserID: %ld, SteamID: %s, Database ID: %ld:%ld", "Info",
		player->index, player->userid, player->steamid(), player->dbinfo.player_id, player->dbinfo.items_id);

	CRPG::ConsoleMsg("Level: %ld, Experience: %ld/%ld, Credits: %ld, Rank: %ld/%ld", "Stats",
		player->level, player->exp, CRPG_StatsManager::LvltoExp(player->level), player->credits,
		CRPG_RankManager::GetPlayerRank(player), CRPG_RankManager::GetRankCount());

	if(player->lang_is_set)
		CRPG::ConsoleMsg("Language: %s", "Settings", player->lang->name);
	else
		CRPG::ConsoleMsg("Language: %s (Default)", "Settings", player->lang->name);

	CRPG::ConsoleMsg("", "Upgrades");
	for(i = 0;i < ITEM_COUNT;i++) {
		CRPG::ConsoleMsg("%s Level %ld", "-", player->items[i].type->name, player->items[i].level);
	}

	CRPG::ConsoleMsg("----------", NULL);

	return 1;
}

RPG_CMD(resetstats, "Reset a player's Level, Credits, Experience, and Upgrades (this cannot be undone!)",
		1, "<player name | userid | steamid>") {
	CRPG_Player *player;

	GET_PLAYER_OR_RETURN(argv[0], player)

	player->ResetStats();

	CRPG::ConsoleMsg("%s's stats have been permanently reset", thiscmd, player->name());

	return 1;
}

RPG_CMD(resetexp, "Reset a player's Experience", 1, "<player name | userid | steamid>") {
	CRPG_Player *player;

	GET_PLAYER_OR_RETURN(argv[0], player)

	player->exp = 0;

	CRPG::ConsoleMsg("%s's Experience has been reset", thiscmd, player->name());

	return 1;
}

RPG_CMD(setlvl, "Set a player's Level", 2, "<player name | userid | steamid> <new level>") {
	CRPG_Player *player;
	unsigned int newlvl, oldlvl;

	newlvl = abs(atoi(argv[1]));
	if(!newlvl) {
		CRPG::ConsoleMsg("The level must be greater than 0", thiscmd);
		return 1;
	}

	GET_PLAYER_OR_RETURN(argv[0], player)

	oldlvl = player->level;

	player->level = newlvl;
	player->exp = 0;

	if(newlvl > oldlvl) {
		player->credits += (newlvl-oldlvl)*CRPG_GlobalSettings::credits_inc;
		if(player->isfake())
			CRPG_Bot::PickUpgrade(player);
	}

	CRPG::ConsoleMsg("%s has been set to Level %d (previously Level %d)", thiscmd, player->name(), player->level, oldlvl);

	if(CRPG_GlobalSettings::announce_newlvl)
		CRPG::ChatAreaMsg(0, TXTDB_ID(newlvl.msg1), player->name(), player->level);

	return 1;
}

RPG_CMD(addlvl, "Add Level(s) to a player's current Level", 2, "<player name | userid | steamid> <levels>") {
	CRPG_Player *player;
	unsigned int newlvl, oldlvl;

	newlvl = abs(atoi(argv[1]));
	if(!newlvl) {
		CRPG::ConsoleMsg("No Levels added", thiscmd);
		return 1;
	}

	GET_PLAYER_OR_RETURN(argv[0], player)

	oldlvl = player->level;

	player->level += newlvl;
	player->exp = 0;

	player->credits += newlvl*CRPG_GlobalSettings::credits_inc;
	if(player->isfake())
		CRPG_Bot::PickUpgrade(player);

	newlvl += oldlvl;

	CRPG::ConsoleMsg("%s has been set to Level %d (previously Level %d)", thiscmd, player->name(), player->level, oldlvl);

	if(CRPG_GlobalSettings::announce_newlvl)
		CRPG::ChatAreaMsg(0, TXTDB_ID(newlvl.msg1), player->name(), player->level);

	return 1;
}

RPG_CMD(setexp, "Set a player's Experience", 2, "<player name | userid | steamid> <new exp>") {
	CRPG_Player *player;
	unsigned int newexp, oldexp, oldlvl;

	GET_PLAYER_OR_RETURN(argv[0], player)

	oldlvl = player->level;
	oldexp = player->exp;
	newexp = abs(atoi(argv[1]));

	if(newexp > player->exp)
		player->AddExp(newexp-oldexp);
	else
		player->exp = newexp;

	CRPG::ConsoleMsg("%s is now Level %d and has %d/%d Experience (previously Level %d with %d/%d Experience)",
		thiscmd, player->name(), player->level, player->exp, CRPG_StatsManager::LvltoExp(player->level),
		oldlvl, oldexp, CRPG_StatsManager::LvltoExp(oldlvl));

	return 1;
}

RPG_CMD(addexp, "Give a player Experience", 2, "<player name | userid | steamid> <new exp>") {
	CRPG_Player *player;
	unsigned int newexp, oldexp, oldlvl;

	newexp = abs(atoi(argv[1]));
	if(!newexp) {
		CRPG::ConsoleMsg("No Experience added", thiscmd);
		return 1;
	}

	GET_PLAYER_OR_RETURN(argv[0], player)

	oldlvl = player->level;
	oldexp = player->exp;

	player->AddExp(newexp);

	newexp += oldexp;

	CRPG::ConsoleMsg("%s is now Level %d and has %d/%d Experience (previously Level %d with %d/%d Experience)",
		thiscmd, player->name(), player->level, player->exp, CRPG_StatsManager::LvltoExp(player->level),
		oldlvl, oldexp, CRPG_StatsManager::LvltoExp(oldlvl));

	return 1;
}

RPG_CMD(setcredits, "Set a player's Credits", 2, "<player name | userid | steamid> <new credits>") {
	CRPG_Player *player;
	unsigned int oldcredits;

	GET_PLAYER_OR_RETURN(argv[0], player)

	oldcredits = player->credits;
	player->credits = abs(atoi(argv[1]));

	CRPG::ConsoleMsg("%s now has %ld Credits (previously had %ld Credits)",
		thiscmd, player->name(), player->credits, oldcredits);

	return 1;
}

RPG_CMD(addcredits, "Add to player's Credits", 2, "<player name | userid | steamid> <new credits>") {
	CRPG_Player *player;
	unsigned int oldcredits;

	GET_PLAYER_OR_RETURN(argv[0], player)

	oldcredits = player->credits;
	player->credits += abs(atoi(argv[1]));

	CRPG::ConsoleMsg("%s now has %ld Credits (previously had %ld Credits)",
		thiscmd, player->name(), player->credits, oldcredits);

	return 1;
}

RPG_CMD(setupgradelvl, "Set a player's Upgrade Level", 3, "<player name | userid | steamid> <upgrade> <level>") {
	CRPG_Player *player;
	struct item_type *item = NULL;
	unsigned int index, newlvl, oldlvl;

	GET_PLAYER_OR_RETURN(argv[0], player)
	GET_UPGRADE_OR_RETURN(argv[1], index, item)

	newlvl = abs(atoi(argv[2]));

	if(CRPG::istrcmp("max", argv[2]) || (newlvl > item->maxlevel))
		newlvl = item->maxlevel;

	oldlvl = player->items[index].level;
	player->items[index].level = newlvl;
	item->sell_item((void*)player); /* do any item-specific deactivation */
	CRPGI::PlayerUpdate(player);

	CRPG::ConsoleMsg("%s now has %s Level %d (previously Level %d)",
		thiscmd, player->name(), item->name, newlvl, oldlvl);

	return 1;
}

RPG_CMD(giveupgrade, "Give a player an Upgrade (increment)", 2, "<player name | userid | steamid> <upgrade>") {
	CRPG_Player *player;
	struct item_type *item = NULL;
	unsigned int index, oldlvl;

	GET_PLAYER_OR_RETURN(argv[0], player)
	GET_UPGRADE_OR_RETURN(argv[1], index, item)

	if(player->items[index].level >= item->maxlevel) {
		CRPG::ConsoleMsg("%s has the maximum Level for %s (Level %d)",
			thiscmd, player->name(), item->name, item->maxlevel);
		return 1;
	}

	oldlvl = player->items[index].level;
	if(!player->GiveItem(index))
		return 0;

	CRPG::ConsoleMsg("%s now has %s Level %d (previously Level %d)",
		thiscmd, player->name(), item->name, player->items[index].level, oldlvl);

	return 1;
}

RPG_CMD(giveall, "Give a player all the Upgrades available", 1, "<player name | userid | steamid>") {
	CRPG_Player *player;
	unsigned int i = ITEM_COUNT;

	GET_PLAYER_OR_RETURN(argv[0], player)

	while(i--) {
		if(CRPG::item_types[i].enable) {
			player->items[i].level = CRPG::item_types[i].maxlevel;
			CRPG::item_types[i].buy_item((void*)player);
			CRPGI::PlayerUpdate(player);
		}
	}

	CRPG::ConsoleMsg("%s now has all Upgrades", thiscmd, player->name());

	return 1;
}

RPG_CMD(takeupgrade, "Take an Upgrade from a player (decrement)", 2, "<player name | userid | steamid> <upgrade>") {
	CRPG_Player *player;
	struct item_type *item = NULL;
	unsigned int index, oldlvl;

	GET_PLAYER_OR_RETURN(argv[0], player)
	GET_UPGRADE_OR_RETURN(argv[1], index, item)

	if(player->items[index].level <= 0) {
		CRPG::ConsoleMsg("%s doesn't have %s", thiscmd, player->name(), item->name);
		return 1;
	}

	oldlvl = player->items[index].level;
	if(!player->TakeItem(index))
		return 0;

	CRPG::ConsoleMsg("%s now has %s Level %d (previously Level %d)",
		thiscmd, player->name(), item->name, player->items[index].level, oldlvl);

	return 1;
}

RPG_CMD(buyupgrade, "Force a player to buy an Upgrade", 2, "<player name | userid | steamid> <upgrade>") {
	CRPG_Player *player;
	struct item_type *item = NULL;
	unsigned int index, cost, oldlvl;

	GET_PLAYER_OR_RETURN(argv[0], player)
	GET_UPGRADE_OR_RETURN(argv[1], index, item)

	oldlvl = player->items[index].level;
	if(oldlvl >= item->maxlevel) {
		CRPG::ConsoleMsg("%s has the maximum Level for %s (Level %d)",
			thiscmd, player->name(), item->name, item->maxlevel);
		return 1;
	}

	cost = CRPGI::GetItemCost(index, oldlvl);
	if(cost > player->credits) {
		CRPG::ConsoleMsg("%s doesn't have enough credits to purchase %s (%d/%d)",
			thiscmd, player->name(), item->name, player->credits, cost);
		return 1;
	}

	if(!player->BuyItem(index))
		return 0;

	CRPG::ConsoleMsg("%s now has %s Level %d (previously Level %d)",
		thiscmd, player->name(), item->name, player->items[index].level, oldlvl);

	return 1;
}

RPG_CMD(sellupgrade, "Force a player to sell an Upgrade (full refund)", 2, "<player name | userid | steamid> <upgrade>") {
	CRPG_Player *player;
	struct item_type *item = NULL;
	unsigned int index, oldlvl;

	GET_PLAYER_OR_RETURN(argv[0], player)
	GET_UPGRADE_OR_RETURN(argv[1], index, item)

	oldlvl = player->items[index].level;
	if(oldlvl <= 0) {
		CRPG::ConsoleMsg("%s doesn't have %s", thiscmd, player->name(), item->name);
		return 1;
	}

	if(!player->TakeItem(index))
		return 0;

	player->credits += CRPGI::GetItemCost(index, oldlvl);

	CRPG::ConsoleMsg("%s now has %s Level %d (previously Level %d)",
		thiscmd, player->name(), item->name, player->items[index].level, oldlvl);

	return 1;
}

RPG_CMD(sellall, "Force a player to sell all their Upgrades (full refund)", 1, "<player name | userid | steamid>") {
	CRPG_Player *player;
	unsigned int index = ITEM_COUNT, level;

	GET_PLAYER_OR_RETURN(argv[0], player)

	while(index--) {
		level = player->items[index].level;
		while(level) {
			player->TakeItem(index);
			player->credits += CRPGI::GetItemCost(index, level--);
		}
	}

	CRPG::ConsoleMsg("%s has sold all Upgrades and has %ld Credits", thiscmd, player->name(), player->credits);

	return 1;
}

RPG_CMD(db_delplayer, "Delete a player entry from both tables in the database (this cannot be undone!)", 1, "<full name | player db id | steamid>") {
	CRPG_Player *player;
	struct tbl_result *result;
	char *items_id;

	if(CRPG::steamid_check(argv[0])) {
		player = SteamIDtoRPGPlayer(argv[0]);
		if(player != NULL)
			player->ResetStats();

		if(!CRPG::db->Query(&result, "SELECT items_id, name FROM %s WHERE steamid = '%s'", TBL_PLAYERS, argv[0])) {
			CRPG::ConsoleMsg("An error occured while attempting to execute query", thiscmd);
			return 0;
		}

		if(result != NULL)
			goto delete_player;
	}
	else if(atoi(argv[0])) {
		if(!CRPG::db->Query(&result, "SELECT items_id, name FROM %s WHERE player_id = '%s'", TBL_PLAYERS, argv[0])) {
			CRPG::ConsoleMsg("An error occured while attempting to execute query", thiscmd);
			return 0;
		}

		if(result != NULL)
			goto delete_player;
	}

	/* Else try to match as name */

	player = NametoRPGPlayer(argv[0]);
	if(player != NULL)
		player->ResetStats();

	if(!CRPG::db->Query(&result, "SELECT items_id, name FROM %s WHERE name = '%s'", TBL_PLAYERS, argv[0])) {
		CRPG::ConsoleMsg("An error occured while attempting to execute query", thiscmd);
		return 0;
	}

	if(result == NULL) {
		CRPG::ConsoleMsg("Unable to find the specified player in the database", thiscmd);
		return 1;
	}

delete_player:
	items_id = GetCell(result, "items_id");

	if(CRPG_GlobalSettings::save_data) {
		CRPG::db->Query("DELETE FROM %s WHERE items_id = '%s'", TBL_ITEMS, items_id);
		CRPG::db->Query("DELETE FROM %s WHERE items_id = '%s'", TBL_PLAYERS, items_id);
	}
	else {
		CRPG::ConsoleMsg("Notice: cssrpg_save_data is set to '0', command had no effect", thiscmd);
		CRPG::ConsoleMsg("Notice: Ignore the proceeding message", thiscmd);
	}

	CRPG::ConsoleMsg("Player '%s' has been deleted from the database", thiscmd, GetCell(result, "name"));

	FreeResult(result);

	return 1;
}

RPG_CMD(db_mass_sell, "Force everyone in the database (and playing) to sell a specific upgrade", 1, "<upgrade>") {
	struct item_type *item = NULL;
	unsigned int index, oldlvl, i = CRPG_Player::player_count;
	CRPG_Player *player;
	struct tbl_result *result;
	int item_col, id_col;
	char *items_id;

	GET_UPGRADE_OR_RETURN(argv[0], index, item)

	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			player = CRPG_Player::players[i];

			oldlvl = player->items[index].level;
			if(oldlvl <= 0)
				continue; /* skip */

			if(!player->TakeItem(index))
				return 0;

			player->credits += CRPGI::GetItemCost(index, oldlvl);
		}
	}

	CRPG::db->Query(&result, "SELECT items_id, %s FROM %s WHERE %s > '0'", item->shortname, TBL_ITEMS, item->shortname);

	id_col = GetColIndex(result, "items_id");
	item_col = GetColIndex(result, item->shortname);

	WARN_IF(id_col == -1, FreeResult(result); return 0)
	WARN_IF(item_col == -1, FreeResult(result); return 0)

	if(CRPG_GlobalSettings::save_data) {
		for(i = 1;(int)i < result->row_count;i++) {
			oldlvl = abs(atoi(result->array[i][item_col]));
			WARN_IF(!oldlvl, oldlvl = 1)
			WARN_IF(oldlvl > item->maxlevel, oldlvl = item->maxlevel)

			items_id = result->array[i][id_col];

			CRPG::db->Query("UPDATE %s SET %s = '0' WHERE items_id = '%s'", TBL_ITEMS, item->shortname, items_id);
			CRPG::db->Query("UPDATE %s SET credits = (credits + %d) WHERE items_id = '%s'",
				TBL_PLAYERS, CRPGI::GetItemCost(index, oldlvl), items_id);
		}
	}
	else {
		CRPG::ConsoleMsg("Notice: cssrpg_save_data is set to '0', command had no effect", thiscmd);
	}

	CRPG::ConsoleMsg("All (%d) players in the database with Upgrade '%s' have been refunded their credits",
		thiscmd, result->row_count, item->name);

	FreeResult(result);

	return 1;
}

RPG_CMD(db_write, "Write current player data to the database", 0, "") {
	CRPG_Player::SaveAll();
	CRPG::ConsoleMsg("All player data has been saved to the database", thiscmd);

	return 1;
}

RPG_CMD(cvarlist, "Build a list of CSS:RPG's CVARs for a config file", 0, "[use default values (1 or 0)]") {
	CRPG_Setting *setting;

	for(setting = CRPG_Setting::ll_first;setting != NULL;setting = setting->ll_next) {
		if(argc && atoi(argv[0]) == 1)
			CRPG::ConsoleMsg("%s '%s' //%s", NULL, setting->name, setting->defaultval, setting->desc);
		else
			CRPG::ConsoleMsg("%s '%s' //%s", NULL, setting->name, setting->var->GetString(), setting->desc);
	}

	return 1;
}

RPG_CMD(reloadlangs, "Reload all languages for CSS:RPG", 0, "") {
	CRPG_TextDB::LoadLanguages();

	return 1;
}
