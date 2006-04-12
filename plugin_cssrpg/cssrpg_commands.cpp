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

RPG_CMD(help, "List of CSS:RPG commands", 0, "") {
	CRPG_Commands *cmd;

	CRPG::ConsoleMsg("", "Commands");
	for(cmd = CRPG_Commands::ll_first;cmd != NULL;cmd = cmd->ll_next)
		CRPG::ConsoleMsg("%s%s - %s", NULL, RPGCMD_PREFIX, cmd->name, cmd->info);

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
		player->credits = (newlvl-oldlvl)*CRPG_GlobalSettings::credits_inc;
		if(player->isfake())
			CRPG_Bot::PickUpgrade(player);
	}

	CRPG::ConsoleMsg("%s has been set to Level %d (previously Level %d)", thiscmd, player->name(), player->level, oldlvl);

	if(CRPG_GlobalSettings::announce_newlvl)
		CRPG::ChatAreaMsg(0, "%s is now Level %d", player->name(), player->level);

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

	player->credits = newlvl*CRPG_GlobalSettings::credits_inc;
	if(player->isfake())
		CRPG_Bot::PickUpgrade(player);

	newlvl += oldlvl;

	CRPG::ConsoleMsg("%s has been set to Level %d (previously Level %d)", thiscmd, player->name(), player->level, oldlvl);

	if(CRPG_GlobalSettings::announce_newlvl)
		CRPG::ChatAreaMsg(0, "%s is now Level %d", player->name(), player->level);

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
	return 1;
}
