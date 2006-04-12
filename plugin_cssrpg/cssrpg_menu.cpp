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
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"

#include "MRecipientFilter.h"
#include "cssrpg.h"
#include "cssrpg_menu.h"

#include "items/rpgi.h"
#include "items/rpgi_regen.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IVEngineServer *engine;

/*	//////////////////////////////////////
	CRPG_Menu Class 
	////////////////////////////////////// */
#define MSG_STATS_RESET "Your stats have been successfully reset."
#define MSG_MAX_LVL "You have reached the maximum level for this item."
#define MSG_CREDITS "You do not have enough credits to buy %s Lvl %d. (Requires %d Credits)"
#define MSG_ITEM_BOUGHT "%s Lvl %ld successfully purchased."
#define MSG_ITEM_SOLD "%s Lvl %ld has been sold."

#define LVL_STR "Lvl %ld"
#define LVL_MAX "Lvl MAX"
#define CR_STR "[%s: %ld]"
#define CR_MAX "[%s: MAX]"

template class CRPG_PlayerClass<CRPG_Menu>;
template<> CRPG_Menu** CRPG_PlayerClass<CRPG_Menu>::nodes;
CRPG_Menu** CRPG_Menu::menus;
unsigned int CRPG_Menu::menu_count;

char temp_out[1024];

void CRPG_Menu::GetUpgradesPage(void) {
	CRPG_Player *player;
	unsigned int i, offset, lvl;

	player = IndextoRPGPlayer(this->index);
	if(player == NULL) {
		BuildOutput(0, "Error");
		return ;
	}

	for(i = 0, offset = 0;i < ITEM_COUNT;i++) {
		if(!CRPG::item_types[i].enable) {
			offset++;
			continue;
		}

		if(i-offset) BuildOutput(0, "\n");
		lvl = player->items[i].level;
		if(lvl >= CRPG::item_types[i].maxlevel) {
			Q_snprintf(temp_out, 1024, "%d. %s %s %s", (i+1)-offset, CRPG::item_types[i].name, LVL_MAX, CR_MAX);
			BuildOutput(0, temp_out, "Cost");
		}
		else {
			Q_snprintf(temp_out, 1024, "->%d. %s %s %s", (i+1)-offset, CRPG::item_types[i].name, LVL_STR, CR_STR);
			if(!lvl)
				BuildOutput(0, temp_out, lvl+1, "Cost", CRPGI::GetItemCost(i, lvl+1));
			else
				BuildOutput(0, temp_out, lvl+1, "Cost", CRPGI::GetItemCost(i, lvl+1));
		}
		this->SetOptions((i+1)-offset);
	}

	return ;
}

void CRPG_Menu::UpgradesSelect(unsigned int option) {
	CRPG_Player *player;
	int offset;
	unsigned int i, credits, lvl, cost;
	struct item_type *item;

	if(option > ITEM_COUNT) {
		this->DelMenu();
		return ;
	}

	for(i = 0, offset = 0;i < ITEM_COUNT;i++) {
		if(!CRPG::item_types[i].enable) {
			offset++;
			continue;
		}

		if(option == ((i+1)-offset)) {
			option = i+1;
			offset = -1;
			break;
		}
	}

	if(offset != -1) {
		this->DelMenu();
		return ;
	}

	player = IndextoRPGPlayer(this->index);
	credits = player->credits;
	lvl = player->items[--option].level;
	item = &CRPG::item_types[option];
	
	cost = CRPGI::GetItemCost(option, lvl+1);

	if(lvl >= item->maxlevel) {
		CRPG::ChatAreaMsg(this->index, MSG_MAX_LVL);
	}
	else if(credits < cost) {
		CRPG::ChatAreaMsg(this->index, MSG_CREDITS, item->name, lvl+1, cost);
	}
	else {
		player->BuyItem(option);
		CRPG::ChatAreaMsg(this->index, MSG_ITEM_BOUGHT, item->name, lvl+1);
	}

	this->DelMenu();
	return ;
}

void CRPG_Menu::GetStatsPage(void) {
	CRPG_Player *player = IndextoRPGPlayer(this->index);

	if(player == NULL) {
		BuildOutput(0, "Error");
		return ;
	}

	BuildOutput(0, "->1. Level: %ld", player->level);
	BuildOutput(0, "\n->2. Exp: %ld/%ld", player->exp, CRPG_StatsManager::LvltoExp(player->level));
	BuildOutput(0, "\n->3. Credits: %ld", player->credits);
	BuildOutput(0, "\n->4. Rank: %ld/%ld",
		CRPG_RankManager::GetPlayerRank(player), CRPG_RankManager::GetRankCount());

	SetOptions(1, 2, 3, 4, 5, 6, 7, 8, 9);
	return ;
}

void CRPG_Menu::GetSellPage(void) {
	CRPG_Player *player;
	unsigned int i, offset;

	player = IndextoRPGPlayer(this->index);
	if(player == NULL) {
		BuildOutput(0, "Error");
		return ;
	}

	if(this->data != NULL) {
		BuildOutput(0, "Are you sure?\n->1. Yes\n->2. No");
		this->options = 0;
		this->SetOptions(1, 2);
		return ;
	}

	for(i = 0, offset = 0;i < ITEM_COUNT;i++) {
		if(!player->items[i].level || !CRPG::item_types[i].enable) {
			offset++;
			continue;
		}

		if(i-offset) BuildOutput(0, "\n");
		Q_snprintf(temp_out, 1024, "->%d. %s %s %s",
			(i+1)-offset, CRPG::item_types[i].name, LVL_STR, CR_STR);
		BuildOutput(0, temp_out, player->items[i].level, "Sale", CRPGI::GetItemSale(i, player->items[i].level));
		this->SetOptions((i+1)-offset);
	}

	if(i == offset) {
		BuildOutput(0, "No items available.");
		this->SetOptions(0, 0);
	}

	return ;
}

void CRPG_Menu::SellSelect(unsigned int option) {
	int i, offset;
	CRPG_Player *player;

	player = IndextoRPGPlayer(this->index);
	WARN_IF(player == NULL, return)

	if(option > ITEM_COUNT) {
		this->DelMenu();
		return ;
	}

	if(this->data == NULL) {
		/* Player selected to sell an item */
		this->data = (void*)option;
		this->CreateMenu();
	}
	else {
		/* Player selected option on confirm menu */
		if(option != 1) {
			this->DelMenu();
			return ;
		}

		option = (unsigned int)this->data;
		for(i = 0, offset = 0;i < ITEM_COUNT;i++) {
			if(!player->items[i].level || !CRPG::item_types[i].enable) {
				offset++;
				continue;
			}

			if(option == ((i+1)-offset)) {
				option = i;
				offset = -1;
				break;
			}
		}

		if(offset != -1) {
			this->DelMenu();
			return ;
		}

		/* Item sold */
		player->SellItem(option);
		CRPG::ChatAreaMsg(this->index, MSG_ITEM_SOLD, CRPG::item_types[i].name, player->items[i].level+1);
		this->DelMenu();
	}

	return ;
}

void CRPG_Menu::GetSettingsPage(void) {
	this->options = 0;

	if(this->data != NULL) {
		BuildOutput(0, "WARNING: You will lose all Levels, Credits, and Experience.\n");
		BuildOutput(0, "Reset stats permanently?\n->1. Yes\n->2. No");
		this->SetOptions(1, 2);
		return ;
	}

	BuildOutput(0, "->1. Reset Stats");
	this->SetOptions(1);

	return ;
}

void CRPG_Menu::SettingsSelect(unsigned int option) {
	CRPG_Player *player;

	if(option > ITEM_COUNT) {
		this->DelMenu();
		return ;
	}

	if(this->data == NULL) {
		if(option == 1) {
			this->data = (void*)option;
			this->CreateMenu();
		}
	}
	else {
		if(option != 1) {
			this->DelMenu();
			return ;
		}

		player = IndextoRPGPlayer(this->index);
		WARN_IF(player == NULL, return)

		player->ResetStats();
		CRPG::ChatAreaMsg(player->index, MSG_STATS_RESET);
		this->DelMenu();
	}

	return ;
}

#define HELP_LINK_COUNT 4
struct {
	char *name;
	char *link;
} help_links[HELP_LINK_COUNT] = {
	{"About CSS:RPG", "http://cssrpg.sourceforge.net/help/cssrpg.html"},
	{"CSS:RPG Upgrades", "http://cssrpg.sourceforge.net/help/upgrades.html"},
	{"CSS:RPG Commands", "http://cssrpg.sourceforge.net/help/commands.html"},
	{"CSS:RPG Acronyms", "http://cssrpg.sourceforge.net/help/acronyms.html"}
};

void CRPG_Menu::GetHelpPage(void) {
	unsigned int i;

	for(i = 0;i < HELP_LINK_COUNT;i++) {
		if(i)
			BuildOutput(0, "\n");
		BuildOutput(0, "->%d. %s", i+1, help_links[i].name);
		SetOptions(i+1);
	}

	return ;
}

void CRPG_Menu::HelpSelect(unsigned int option) {
	if(option > HELP_LINK_COUNT || option < 1) {
		this->DelMenu();
		return ;
	}
	option--;

	CRPG::ShowMOTD(this->index, help_links[option].name, help_links[option].link, motd_url);
	this->DelMenu();
	return ;
}

void CRPG_Menu::GetTop10Page(void) {
	unsigned int i;
	struct ranklist **ranks;

	CRPG_RankManager::GetTop10Players(&ranks);
	WARN_IF(ranks == NULL, return)

	BuildOutput(0, "Top 10 CSS:RPG Players\n-----\n");

	for(i = 0;i < 10;i++) {
		if(i)
			BuildOutput(0, "\n");

		BuildOutput(0, "->%d. %s Lvl: %ld Exp: %ld Cr: %ld",
			i+1, ranks[i]->name, ranks[i]->level, ranks[i]->exp, ranks[i]->credits);
	}
	SetOptions(1, 2, 3, 4, 5, 6, 7, 8, 9);

	CRPG_RankManager::FreeRanksList(ranks);
	return ;
}

void CRPG_Menu::GetMenu(void) {
	switch(this->submenu) {
		case none:
			SetOptions(1, 2, 3, 4, 5);
			BuildOutput(0, "->1. Upgrades\n->2. Sell\n->3. Stats\n->4. Settings\n->5. Help");
			break;

		case upgrades:
			GetUpgradesPage();
			break;

		case sell:
			GetSellPage();
			break;

		case stats:
			GetStatsPage();
			break;

		case settings:
			GetSettingsPage();
			break;
		
		case help:
			GetHelpPage();
			break;

		case top10:
			GetTop10Page();
			break;

		default:
			BuildOutput(0, "Unknown");
			break;
	}

	return ;
}

void CRPG_Menu::SetOptions(char bit1, char bit2, char bit3, char bit4, char bit5, char bit6, char bit7, char bit8, char bit9) {
	unsigned int i = 9;
	char opt_array[] = {bit1, bit2, bit3, bit4, bit5, bit6, bit7, bit8, bit9};
	this->options = 0;

	while(i--)
		this->options |= (opt_array[i] ? 1 << i : 0);

	return ;
}

void CRPG_Menu::SetOptions(unsigned int opt_num) {
	this->options |= 1 << (opt_num-1);
	return ;
}

void CRPG_Menu::SendOutput(char finalize) {
	MRecipientFilter filter;
	bf_write *buffer;

	filter.AddRecipient(this->index);
	buffer = engine->UserMessageBegin(&filter, 10);

	buffer->WriteShort((finalize ? this->options : 1 << 9)); //Sets how many options the menu has
	buffer->WriteChar(-1); //Sets how long the menu stays open -1 for stay until option selected
	buffer->WriteByte((finalize ? 0 : 1)); // 0 = Draw Immediately, 1 = Draw Later
	buffer->WriteString(this->menu_out); //The text shown on the menu

	engine->MessageEnd();

	memset(this->menu_out, 0, 200);
	this->menu_outlen = 0;
	return ;
}

void CRPG_Menu::BuildOutput(char finalize, char *strf, ...) {
	static char str[1024];
	char *ptr = str;
	va_list ap;

	va_start(ap, strf);
	Q_vsnprintf(str, 1024, strf, ap);
	va_end(ap);

	if(menu_outlen >= 200)
		SendOutput(0); /* flush the output */

	while(*ptr) {
		this->menu_out[this->menu_outlen++] = *ptr++;
		if(this->menu_outlen >= 200) {
			this->menu_out[this->menu_outlen] = 0;
			if(!*ptr)
				SendOutput(finalize);
			else
				SendOutput(0);
		}
	}

	this->menu_out[this->menu_outlen] = 0;
	if(finalize)
		SendOutput(finalize);

	return ;
}

void CRPG_Menu::CreateMenu(void) {
	CRPG_Player *player;

	if(!enable) {
		this->DelMenu();
		return ;
	}

	player = IndextoRPGPlayer(this->index);
	if(player == NULL) {
		CRPG::DebugMsg("Warning: Player of index %d is NULL in function CreateMenu()", this->index);
		this->DelMenu();
		return ;
	}

	this->options = 0;
	
	if(header)
		BuildOutput(0, "Credits %ld\n-----\n", player->credits);

	GetMenu();

	/* Finalize Menu */
	this->SetOptions(10);
	BuildOutput(1, "\n0. Exit");
	return ;
}

void CRPG_Menu::SelectOption(unsigned int option) {
	CRPG::DebugMsg(1, "%s selected option %d in submenu %d on page %d",
		this->name(), option, this->submenu, this->page);

	/* Emit menu sound */
	CRPG::EmitSound(this->index, "buttons/button14.wav");

	if(!option) {
		this->DelMenu();
		return ;
	}

	switch(this->submenu) {
		case none:
			if(option <= 5) {
				this->submenu = (enum submenu_t)option;
				this->page = 0;
				CreateMenu();
			}
			else {
				this->DelMenu();
				return ;
			}
			break;

		case upgrades:
			UpgradesSelect(option);
			break;

		case sell:
			SellSelect(option);
			break;

		case stats:
			this->DelMenu();
			break;

		case settings:
			SettingsSelect(option);
			break;

		case help:
			HelpSelect(option);
			break;

		case top10:
			this->DelMenu();
			break;

		default:
			this->DelMenu();
			break;
	}

	return ;
}

void CRPG_Menu::Init(void) {
	unsigned int i;
	menu_count = (unsigned int)CRPG::maxClients();
	i = menu_count;

	menus = new CRPG_Menu *[menu_count];
	while(i--)
		menus[i] = NULL;

	nodes = menus;

	return ;
}

CRPG_Menu* IndextoRPGMenu(int index) {
	return CRPG_Menu::IndextoHandle(index);
}

CRPG_Menu* EdicttoRPGMenu(edict_t *e) {
	return CRPG_Menu::EdicttoHandle(e);
}

CRPG_Menu* CRPG_Menu::AddMenu(edict_t *e) {
	CRPG_Menu *menu;

	WARN_IF((e == NULL) || !CRPG::IsValidEdict(e), return NULL)

	menu = EdicttoRPGMenu(e);
	if(menu != NULL)
		menu->DelMenu();

	menu = new_node(e);
	CRPG::DebugMsg(1, "Player %s created menu", menu->name());

	return menu;
}

unsigned int CRPG_Menu::DelMenu(void) {
	return del_node(this);
}

void CRPG_Menu::ShutDown(void) {
	free_nodes(menu_count);

	return ;
}
