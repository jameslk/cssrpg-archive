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
#include <math.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"

#include "server_class.h"

#include "cssrpg.h"
#include "cssrpg_interface.h"
#include "cssrpg_menu.h"
#include "cssrpg_stats.h"
#include "cssrpg_commands.h"
#include "cssrpg_textdb.h"
#include "cssrpg_hacks.h"
#include "items/rpgi.h"
#include "cssrpg_console.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*	//////////////////////////////////////
	Server Console Commands
	////////////////////////////////////// */
CON_COMMAND(cssrpg_debug_playerlist, "List all RPG players") {
	unsigned int i = CRPG_Player::player_count;
	CRPG_Player *player;

	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			player = CRPG_Player::players[i];
			Msg("Player: %s, UserID: %d, Level: %d, Experience: %d/%d\n",
				player->name(), player->userid, player->level, player->exp, CRPG_StatsManager::LvltoExp(player->level));
		}
	}
}

CON_COMMAND(cssrpg_debug_menulist, "List all RPG menus") {
	unsigned int i = CRPG_Menu::menu_count;
	CRPG_Menu *menu;

	while(i--) {
		if(CRPG_Menu::menus[i] != NULL) {
			menu = CRPG_Menu::menus[i];
			Msg("Player: %s, Submenu: %d, Page: %d\n",
				menu->name(), menu->submenu, menu->page);
		}
	}
}

CON_COMMAND(cssrpg_debug_timers, "List all timers") {
	CRPG_Timer *timer;

	CRPG::DebugMsg("Current Time: %f Timers:", s_globals->curtime);

	for(timer = CRPG_Timer::ll_first;timer != NULL;timer = timer->ll_next)
		CRPG::DebugMsg("Timer Executes: %f", timer->next_tm);

	CRPG::DebugMsg("* End of timers list", s_globals->curtime);
	return ;
}

CON_COMMAND(cssrpg_debug_teamcount, "Player count for each team") {
	CRPG::DebugMsg("CT: %d T: %d", CRPG_TeamBalance::teamct_count, CRPG_TeamBalance::teamt_count);
	return ;
}

CON_COMMAND(cssrpg_debug_giveitem, "Gives a player a new Item Level") {
	CRPG_Player *player;
	int player_index, item_index;

	if(s_engine->Cmd_Argc() < 3) {
		CRPG::DebugMsg("cssrpg_debug_giveitem: Requires 2 arguments");
		return ;
	}

	player_index = CRPG::FindPlayer(s_engine->Cmd_Argv(1));
	if(player_index < 0) {
		CRPG::DebugMsg("cssrpg_debug_giveitem: Couldn't find player: %s", s_engine->Cmd_Argv(1));
		return ;
	}

	player = IndextoRPGPlayer(player_index);

	item_index = atoi(s_engine->Cmd_Argv(2));
	if(!player->GiveItem(item_index)) {
		CRPG::DebugMsg("cssrpg_debug_giveitem: Failed to give player %s Item: %d", player->name(), item_index);
		return ;
	}

	CRPG::DebugMsg("cssrpg_debug_giveitem: Gave player %s Item %s Level %d",
		player->name(), CRPG::item_types[item_index].name, player->items[item_index].level);

	return ;
}

CON_COMMAND(cssrpg_debug_listdir, "List a directory's contents") {
	struct file_info file;
	unsigned int i = 0;

	if(s_engine->Cmd_Argc() < 2) {
		CRPG::DebugMsg("cssrpg_debug_listdir: Please specify a path");
		return ;
	}

	while(CRPG::traverse_dir(file, s_engine->Cmd_Argv(1), i++) != END_OF_DIR) {
		if(file.type == file_normal)
			CRPG::DebugMsg("File: %s", file.name);
		else
			CRPG::DebugMsg("Directory: %s", file.name);
	}

	return ;
}

CON_COMMAND(cssrpg_debug_langkeys, "List a language's keys") {
	unsigned int i;
	CRPG_TextDB *txtdb;

	if(s_engine->Cmd_Argc() < 2) {
		CRPG::DebugMsg("cssrpg_debug_langkeys: Please specify a language file");
		return ;
	}

	txtdb = FiletoTextDB(s_engine->Cmd_Argv(1));
	if(txtdb == NULL) {
		CRPG::DebugMsg("cssrpg_debug_langkeys: Language file was not found");
		return ;
	}

	CRPG::DebugMsg("*** cssrpg_debug_langkeys: Start ***");
	CRPG::DebugMsg("Language Name: %s", txtdb->name);

	for(i = 0;i < TXTDB_KEY_COUNT;i++)
		CRPG::DebugMsg("Key %s: %s", "hello dear aunt", txtdb->txt.key_array[i]->s);

	CRPG::DebugMsg("*** cssrpg_debug_langkeys: End ***");

	return ;
}

CON_COMMAND(cssrpg_debug_netprop, "Finds a network property given a class name and member") {
	int i = 0, props = 0, offset = 0;
	ServerClass *sc = s_gamedll->GetAllServerClasses();

	if(s_engine->Cmd_Argc() < 3) {
		CRPG::DebugMsg("cssrpg_debug_netprop: Requires 2 arguments");
		return ;
	}

	while(sc) {
		if(CRPG::istrcmp((char*)sc->GetName(), s_engine->Cmd_Argv(1))) {
			props = sc->m_pTable->GetNumProps();

			for(i = 0; i < props; i++) {
				if(CRPG::istrcmp((char*)sc->m_pTable->GetProp(i)->GetName(), s_engine->Cmd_Argv(2))) {
					if((offset = sc->m_pTable->GetProp(i)->GetOffset()) < 0)
						offset = offset * -1;
					
					CRPG::DebugMsg("Offset: %d", offset);
					return ;
				} 
			}

			CRPG::DebugMsg("Offset: Not Found");
			return ;
		}

		sc = sc->m_pNext;
	}

	CRPG::DebugMsg("Offset: Not Found");
	return ;
}

CON_COMMAND(cssrpg_debug_dump_netprops, "Dumps all network properties to netprops.txt") {
	FILE *fptr;

	fptr = fopen("netprops.txt", "w");
	if(fptr == NULL)
		CRPG::DebugMsg("Unable to open netprops.txt");

	CRPG_ExternProps::DumpNetProps(fptr, s_gamedll->GetAllServerClasses());
	CRPG::DebugMsg("All network properties dumped to netprops.txt");

	fclose(fptr);

	return ;
}

/*	//////////////////////////////////////
	Static Server Variables
	////////////////////////////////////// */
static ConVar cssrpg_version("cssrpg_version", CSSRPG_VERSION,
		FCVAR_REPLICATED | FCVAR_SPONLY | FCVAR_NOTIFY, "CSS:RPG plugin version");

/*	//////////////////////////////////////
	CRPG_Setting Class
	////////////////////////////////////// */
template class CRPG_StaticLinkedList<CRPG_Setting>;
template<> CRPG_Setting* CRPG_StaticLinkedList<CRPG_Setting>::ll_first;
template<> CRPG_Setting* CRPG_StaticLinkedList<CRPG_Setting>::ll_last;
template<> unsigned int CRPG_StaticLinkedList<CRPG_Setting>::ll_count;

void CRPG_Setting::Init(void) {
	ll_init();
	CRPG_GlobalSettings::InitSettings();

	return ;
}

void CRPG_Setting::FreeMemory(void) {
	CRPG_Setting *setting, *next;

	for(setting = ll_first;setting != NULL;setting = next) {
		next = setting->ll_next;
		delete setting;
	}

	return ;
}

void CRPG_Setting::setval_for_type(void) {
	switch(this->type) {
		case var_str:
			memset(this->val.s, '\0', 256);
			strncpy(this->val.s, this->var->GetString(), 255);
			break;

		case var_float:
			this->val.f = this->var->GetFloat();;
			break;

		case var_ufloat:
			this->val.uf = fabs(this->var->GetFloat());
			break;

		case var_int:
			this->val.i = this->var->GetInt();
			break;

		case var_uint:
			this->val.ui = abs(this->var->GetInt());
			break;

		case var_bool:
			this->val.b = this->var->GetBool();
			break;
	}

	if(this->flags & SETTING_HAS_UPDATEVAR) {
		switch(this->type) {
			case var_str:
				strncpy((char*)this->update, this->val.s, 255);
				break;

			case var_float:
				*(float*)this->update = this->val.f;
				break;

			case var_ufloat:
				*(float*)this->update = this->val.uf;
				break;

			case var_int:
				*(int*)this->update = this->val.i;
				break;

			case var_uint:
				*(unsigned int*)this->update = this->val.ui;
				break;

			case var_bool:
				*(bool*)this->update = this->val.b;
				break;
		}
	}

	return ;
}

void* CRPG_Setting::getval_for_type(void) {
	switch(this->type) {
		case var_str:
			return this->val.s;

		case var_float:
			return &this->val.f;

		case var_ufloat:
			return &this->val.uf;

		case var_int:
			return &this->val.i;

		case var_uint:
			return &this->val.ui;

		case var_bool:
			return &this->val.b;
	}

	return NULL;
}

void CRPG_Setting::SettingChange(ConVar *var, char const *pOldString) {
	CRPG_Setting *setting;

	for(setting = ll_first;setting != NULL;setting = setting->ll_next) {
		if(setting->var == var) {
			setting->setval_for_type();
			if(setting->flags & SETTING_HAS_CALLBACK)
				setting->func(setting, (char*)setting->var->GetString(), (char*)pOldString);
		}
	}

	return ;
}

CRPG_Setting* CRPG_Setting::new_setting(char *name, char *defaultval, char *desc, var_type type) {
	CRPG_Setting *setting;

	setting = new CRPG_Setting;

	CRPG::snprintf(setting->name, 32, "cssrpg_%s", name);
	memset(setting->val.s, '\0', 256);
	Q_strncpy(setting->defaultval, defaultval, 255);
	Q_strncpy(setting->desc, desc, 512);

	setting->var = new ConVar(setting->name, setting->defaultval, 0, setting->desc, CRPG_Setting::SettingChange);

	setting->type = type;

	setting->update = NULL;
	setting->func = NULL;

	setting->ll_add();
	return setting;
}

CRPG_Setting* CRPG_Setting::CreateVar(char *name, char *defaultval, char *desc, var_type type) {
	CRPG_Setting *setting = new_setting(name, defaultval, desc, type);

	setting->flags = 0;
	setting->setval_for_type();

	return setting;
}

CRPG_Setting* CRPG_Setting::CreateVar(char *name, char *defaultval, char *desc, var_type type, void *update) {
	CRPG_Setting *setting = new_setting(name, defaultval, desc, type);

	setting->update = update;
	setting->flags = SETTING_HAS_UPDATEVAR;
	setting->setval_for_type();
	SettingChange(setting->var, setting->var->GetString());

	return setting;
}

CRPG_Setting* CRPG_Setting::CreateVar(char *name, char *defaultval, char *desc, var_type type, var_callback *func) {
	CRPG_Setting *setting = new_setting(name, defaultval, desc, type);

	setting->func = func;
	setting->flags = SETTING_HAS_CALLBACK;
	setting->setval_for_type();
	SettingChange(setting->var, setting->var->GetString());

	return setting;
}

CRPG_Setting* CRPG_Setting::CreateVar(char *name, char *defaultval, char *desc, var_type type, void *update, var_callback *func) {
	CRPG_Setting *setting = new_setting(name, defaultval, desc, type);

	setting->update = update;
	setting->func = func;
	setting->flags = (SETTING_HAS_UPDATEVAR | SETTING_HAS_CALLBACK);
	setting->setval_for_type();
	SettingChange(setting->var, setting->var->GetString());

	return setting;
}

/*	//////////////////////////////////////
	CRPG_GlobalSettings Class
	////////////////////////////////////// */
bool CRPG_GlobalSettings::enable;
bool CRPG_GlobalSettings::bot_enable;
bool CRPG_GlobalSettings::debug_mode;
bool CRPG_GlobalSettings::save_data;
bool CRPG_GlobalSettings::steamid_save;
char CRPG_GlobalSettings::default_lang[256];
unsigned int CRPG_GlobalSettings::save_interval;
unsigned int CRPG_GlobalSettings::player_expire;
unsigned int CRPG_GlobalSettings::bot_maxlevel;
bool CRPG_GlobalSettings::announce_newlvl;

unsigned int CRPG_GlobalSettings::icestab_lmtdmg;
char CRPG_GlobalSettings::denial_restrict[1024];

bool CRPG_GlobalSettings::exp_notice;
unsigned int CRPG_GlobalSettings::exp_max;
unsigned int CRPG_GlobalSettings::exp_start;
unsigned int CRPG_GlobalSettings::exp_inc;

float CRPG_GlobalSettings::exp_damage;
float CRPG_GlobalSettings::exp_knifedmg;
float CRPG_GlobalSettings::exp_kill;
float CRPG_GlobalSettings::exp_headshot;

float CRPG_GlobalSettings::exp_teamwin;
float CRPG_GlobalSettings::exp_bombplanted;
float CRPG_GlobalSettings::exp_bombdefused;
float CRPG_GlobalSettings::exp_bombexploded;
float CRPG_GlobalSettings::exp_hostage;
float CRPG_GlobalSettings::exp_vipescaped;

unsigned int CRPG_GlobalSettings::credits_inc;
unsigned int CRPG_GlobalSettings::credits_start;
float CRPG_GlobalSettings::sale_percent;

void CRPG_GlobalSettings::InitSettings(void) {
	struct item_type *type;

	CRPG_Setting::CreateVar("enable", "1", "If set to 1, CSSRPG is enabled, if 0, CSSRPG is disabled", var_bool, &enable, CRPG::CVAREnable);
	CRPG_Setting::CreateVar("bot_enable", "1", "If set to 1, bots will be able to use the CSSRPG plugin", var_bool, &bot_enable);
	CRPG_Setting::CreateVar("debug", "0", "Turns on debug mode for this plugin", var_bool, &debug_mode);
	CRPG_Setting::CreateVar("save_data", "1", "If disabled, the database won't be updated (this means player data won't be saved!)", var_bool, &save_data);
	CRPG_Setting::CreateVar("steamid_save", "1", "Save by SteamID instead of by SteamID and name", var_bool, &steamid_save);
	CRPG_Setting::CreateVar("default_lang", "english.txt", "Default language file (e.g. english.txt)", var_str, default_lang);
	CRPG_Setting::CreateVar("save_interval", "150", "Interval (in seconds) that player data is auto saved (0 = off)", var_uint, &save_interval);
	CRPG_Setting::CreateVar("player_expire", "30", "Sets how many days until an unused player account is deleted (0 = never)", var_uint, &player_expire);
	CRPG_Setting::CreateVar("bot_maxlevel", "250", "The maximum level a bot can reach until its stats are reset (0 = infinite)", var_uint, &bot_maxlevel);
	CRPG_Setting::CreateVar("announce_newlvl", "1", "Global announcement when a player reaches a new level (1 = enable, 0 = disable)", var_bool, &announce_newlvl);

	type = &CRPG::item_types[ITEM_REGEN];
	CRPG_Setting::CreateVar("regen_enable", "1", "Sets the Regeneration item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("regen_maxlevel", "5", "Regeneration item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("regen_cost", "5", "Regeneration item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("regen_icost", "10", "Regeneration item cost increment for each level", var_uint, &type->inc_cost);
	
	type = &CRPG::item_types[ITEM_HBONUS];
	CRPG_Setting::CreateVar("hbonus_enable", "1", "Sets the Health Bonus (Health+) item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("hbonus_maxlevel", "16", "Health Bonus (Health+) item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("hbonus_cost", "10", "Health Bonus (Health+) item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("hbonus_icost", "10", "Health Bonus (Health+) item cost increment for each level", var_uint, &type->inc_cost);
	
	type = &CRPG::item_types[ITEM_RESUP];
	CRPG_Setting::CreateVar("resup_enable", "1", "Sets the Resupply item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("resup_maxlevel", "5", "Resupply item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("resup_cost", "5", "Resupply item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("resup_icost", "15", "Resupply item cost increment for each level", var_uint, &type->inc_cost);
	
	type = &CRPG::item_types[ITEM_VAMP];
	CRPG_Setting::CreateVar("vamp_enable", "1", "Sets the Vampire item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("vamp_maxlevel", "10", "Vampire item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("vamp_cost", "15", "Vampire item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("vamp_icost", "10", "Vampire item cost increment for each level", var_uint, &type->inc_cost);

	type = &CRPG::item_types[ITEM_STEALTH];
	CRPG_Setting::CreateVar("stealth_enable", "1", "Sets the Stealth item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("stealth_maxlevel", "5", "Stealth item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("stealth_cost", "15", "Stealth item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("stealth_icost", "10", "Stealth item cost increment for each level", var_uint, &type->inc_cost);

	type = &CRPG::item_types[ITEM_LJUMP];
	CRPG_Setting::CreateVar("ljump_enable", "1", "Sets the LongJump item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("ljump_maxlevel", "5", "LongJump item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("ljump_cost", "20", "LongJump item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("ljump_icost", "15", "LongJump item cost increment for each level", var_uint, &type->inc_cost);

	type = &CRPG::item_types[ITEM_FNADE];
	CRPG_Setting::CreateVar("fnade_enable", "1", "Sets the FireGrenade item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("fnade_maxlevel", "5", "FireGrenade item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("fnade_cost", "15", "FireGrenade item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("fnade_icost", "10", "FireGrenade item cost increment for each level", var_uint, &type->inc_cost);

	type = &CRPG::item_types[ITEM_ICESTAB];
	CRPG_Setting::CreateVar("icestab_enable", "1", "Sets the IceStab item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("icestab_maxlevel", "3", "IceStab item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("icestab_cost", "20", "IceStab item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("icestab_icost", "30", "IceStab item cost increment for each level", var_uint, &type->inc_cost);
	CRPG_Setting::CreateVar("icestab_limit_dmg", "10", "Maximum damage that can be done upon icestabbed victims (0 = disable)", var_uint, &icestab_lmtdmg);

	type = &CRPG::item_types[ITEM_FPISTOL];
	CRPG_Setting::CreateVar("fpistol_enable", "1", "Sets the FrostPistol item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("fpistol_maxlevel", "10", "FrostPistol item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("fpistol_cost", "20", "FrostPistol item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("fpistol_icost", "15", "FrostPistol item cost increment for each level", var_uint, &type->inc_cost);

	type = &CRPG::item_types[ITEM_DENIAL];
	CRPG_Setting::CreateVar("denial_enable", "1", "Sets the Denial item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("denial_maxlevel", "3", "Denial item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("denial_cost", "25", "Denial item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("denial_icost", "50", "Denial item cost increment for each level", var_uint, &type->inc_cost);
	CRPG_Setting::CreateVar("denial_restrict", "", "Space delimited list of restricted weapons (e.g. awp g3sg1 m249)", var_str, denial_restrict);

	type = &CRPG::item_types[ITEM_IMPULSE];
	CRPG_Setting::CreateVar("impulse_enable", "1", "Sets the Impulse item to enabled (1) or disabled (0)", var_bool, &type->enable);
	CRPG_Setting::CreateVar("impulse_maxlevel", "5", "Impulse item maximum level", var_uint, &type->maxlevel, CRPGI::CVARItemMaxLvl);
	CRPG_Setting::CreateVar("impulse_cost", "20", "Impulse item start cost", var_uint, &type->start_cost);
	CRPG_Setting::CreateVar("impulse_icost", "20", "Impulse item cost increment for each level", var_uint, &type->inc_cost);

	CRPG_Setting::CreateVar("exp_notice", "1", "Sets notifications to players when they gain Experience", var_bool, &exp_notice);
	CRPG_Setting::CreateVar("exp_max", "50000", "Maximum experience that will ever be required", var_uint, &exp_max);
	CRPG_Setting::CreateVar("exp_start", "250", "Experience required for Level 1", var_uint, &exp_start);
	CRPG_Setting::CreateVar("exp_inc", "50", "Incriment experience requied for each level (until cssrpg_exp_max)", var_uint, &exp_inc);

	CRPG_Setting::CreateVar("exp_damage", "1.0", "Experience for hurting an enemy multiplied by the damage done", var_ufloat, &exp_damage);
	CRPG_Setting::CreateVar("exp_knifedmg", "8.0", "Experience for knifing an enemy multiplied by the damage done (must be higher than exp_damage)", var_ufloat, &exp_knifedmg);
	CRPG_Setting::CreateVar("exp_kill", "15", "Experience for a kill multiplied by the victim's level", var_ufloat, &exp_kill);
	CRPG_Setting::CreateVar("exp_headshot", "50", "Experience extra for a headshot", var_ufloat, &exp_headshot);

	CRPG_Setting::CreateVar("exp_teamwin", "0.15", "Experience multipled by the experience required and the team ratio given to a team for completing the objective", var_ufloat, &exp_teamwin);
	CRPG_Setting::CreateVar("exp_bombplanted", "0.15", "Experience multipled by the experience required and the team ratio given for planting the bomb", var_ufloat, &exp_bombplanted);
	CRPG_Setting::CreateVar("exp_bombdefused", "0.30", "Experience multipled by the experience required and the team ratio given for defusing the bomb", var_ufloat, &exp_bombdefused);
	CRPG_Setting::CreateVar("exp_bombexploded", "0.20", "Experience multipled by the experience required and the team ratio given to the bomb planter when it explodes", var_ufloat, &exp_bombexploded);
	CRPG_Setting::CreateVar("exp_hostage", "0.10", "Experience multipled by the experience required and the team ratio for rescuing a hostage", var_ufloat, &exp_hostage);
	CRPG_Setting::CreateVar("exp_vipescaped", "0.35", "Experience multipled by the experience required and the team ratio given to the vip when the vip escapes", var_ufloat, &exp_vipescaped);

	CRPG_Setting::CreateVar("credits_inc", "5", "Credits given to each new level", var_uint, &credits_inc);
	CRPG_Setting::CreateVar("credits_start", "0", "Starting credits for Level 1", var_uint, &credits_start);
	CRPG_Setting::CreateVar("sale_percent", "0.75", "Percentage of credits a player gets for selling an item", var_ufloat, &sale_percent);

	return ;
}
