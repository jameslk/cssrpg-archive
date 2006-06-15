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
#include "cssrpg_textdb.h"
#include "cssrpg_menu.h"

#include "items/rpgi.h"
#include "items/rpgi_regen.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern IVEngineServer *engine;

/*	//////////////////////////////////////
	CRPG_MenuOptions Class 
	////////////////////////////////////// */
template class CRPG_DynLinkedList<CRPG_MenuOptions>;

void CRPG_MenuOptions::update_options(CRPG_Menu *menu) {
	CRPG_MenuOptions *opt;
	unsigned int i = 1;

	WARN_IF(menu == NULL, return)

	for(opt = menu->opt_first;opt != NULL;opt = opt->ll_next, i++) {
		opt->index = i;
		opt->page = i / 8;

		if(((int)menu->opt_count-(int)opt->index) >= 1)
			opt->nextpage = opt->page+1;
		else
			opt->nextpage = -1;

		if(opt->index > 7)
			opt->prevpage = opt->page-1;
		else
			opt->prevpage = -1;

		if((opt->ll_prev == NULL) || (opt->ll_prev->page_index >= 7))
			opt->page_index = 1;
		else
			opt->page_index = opt->ll_prev->page_index+1;
	}

	return ;
}

CRPG_MenuOptions* CRPG_MenuOptions::AddOption(CRPG_Menu *opt_menu, char opt_enabled, char *strf, ...) {
	CRPG_MenuOptions *opt;
	va_list ap;

	WARN_IF(opt_menu == NULL, return NULL)
	WARN_IF((strf == NULL) || !*strf, return NULL)

	opt = new CRPG_MenuOptions();

	opt->menu = opt_menu;

	va_start(ap, strf);
	Q_vsnprintf(opt->str, 1024, strf, ap);
	va_end(ap);

	opt->enabled = (opt_enabled ? 1 : 0);

	opt->assign_pointers(&opt_menu->opt_first, &opt_menu->opt_last, &opt_menu->opt_count);
	opt->ll_add();
	update_options(opt_menu);

	return opt;
}

CRPG_MenuOptions* CRPG_MenuOptions::PageIndextoOpt(CRPG_Menu *menu, int opt_page, int opt_page_index) {
	CRPG_MenuOptions *opt;

	WARN_IF(menu == NULL, return NULL)

	for(opt = menu->opt_first;opt != NULL;opt = opt->ll_next) {
		if((opt->page == opt_page) && (opt->page_index == opt_page_index))
			return opt;
	}

	return NULL;
}

char* CRPG_MenuOptions::MakeOutputStr(void) {
	memset(this->output, '\0', 1024);

	if(this->enabled)
		CRPG::snprintf(this->output, 1024, "->%d. %s", this->page_index, this->str);
	else
		CRPG::snprintf(this->output, 1024, "%d. %s", this->page_index, this->str);

	return this->output;
}

void CRPG_MenuOptions::DelOption(void) {
	CRPG_Menu *opt_menu = this->menu;
	this->ll_del();
	delete this;

	update_options(opt_menu);
	return ;
}

void CRPG_MenuOptions::FreeOptions() {
	CRPG_MenuOptions *opt, *next;

	for(opt = menu->opt_first;opt != NULL;opt = next) {
		next = opt->ll_next;
		opt->ll_del();
		delete opt;
	}

	return ;
}

/*	//////////////////////////////////////
	CRPG_Menu Class 
	////////////////////////////////////// */
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
	unsigned int i, lvl;
	CRPG_MenuOptions *opt, *temp;
	txtkey_t *key;

	player = IndextoRPGPlayer(this->index);
	if(player == NULL) {
		BuildOutput(0, "Error");
		return ;
	}

	if(!this->opt_count) {
		for(i = 0;i < ITEM_COUNT;i++) {
			if(!CRPG::item_types[i].enable)
				continue;

			key = player->lang->NametoKey("items.%s", CRPG::item_types[i].shortname);
			WARN_IF(key == NULL, continue)

			lvl = player->items[i].level;
			if(lvl >= CRPG::item_types[i].maxlevel) {
				CRPG::snprintf(temp_out, 1024, "%s %s %s", key->s, LVL_MAX, CR_MAX);
				opt = CRPG_MenuOptions::AddOption(this, 0, temp_out, TXTDB(player, menu_txt.cost));
			}
			else {
				CRPG::snprintf(temp_out, 1024, "%s %s %s", key->s, LVL_STR, CR_STR);
				opt = CRPG_MenuOptions::AddOption(this, 1, temp_out, lvl+1, TXTDB(player, menu_txt.cost), CRPGI::GetItemCost(i, lvl+1));
			}

			WARN_IF(opt == NULL, continue)
			opt->data.i = CRPG::item_types[i].index;
		}
	}

	if(!this->opt_count) {
		BuildOutput(0, TXTDB(player, menu_misc.no_items));
		return ;
	}

	for(opt = this->opt_first;opt != NULL;opt = opt->ll_next) {
		if(opt->page == this->page) {
			if(opt->page_index > 1)
				BuildOutput(0, "\n");

			temp = opt;
			BuildOutput(0, opt->MakeOutputStr());
			SetOptions(opt->page_index);
		}
	}

	if((temp->prevpage < 0) && (temp->nextpage >= 0)) {
		BuildOutput(0, "\n8. %s\n->9. %s", TXTDB(player, menu_opt.previous), TXTDB(player, menu_opt.next));
		this->SetOptions(9);
	}
	else if((temp->prevpage >= 0) && (temp->nextpage < 0)) {
		BuildOutput(0, "\n->8. %s\n9. %s", TXTDB(player, menu_opt.previous), TXTDB(player, menu_opt.next));
		this->SetOptions(8);
		this->data.ia = 1;
	}
	else if((temp->prevpage >= 0) && (temp->nextpage >= 0)) {
		BuildOutput(0, "\n->8. %s\n->9. %s", TXTDB(player, menu_opt.previous), TXTDB(player, menu_opt.next));
		this->SetOptions(8);
		this->SetOptions(9);
	}

	return ;
}

void CRPG_Menu::UpgradesSelect(unsigned int option) {
	CRPG_Player *player;
	CRPG_MenuOptions *opt;
	unsigned int credits, lvl, cost;
	struct item_type *item = NULL;

	if(option == 8) {
		if(this->page)
			this->page--;
		this->data.ia = 0;
		this->CreateMenu();
		return ;
	}
	else if(option == 9) {
		if(!this->data.ia)
			this->page++;
		this->CreateMenu();
		return ;
	}

	for(opt = this->opt_first;opt != NULL;opt = opt->ll_next) {
		if(opt->page == this->page) {
			if(option == opt->page_index) {
				item = &CRPG::item_types[opt->data.i];
				break;
			}
		}
	}

	WARN_IF(item == NULL, this->DelMenu(); return)

	player = IndextoRPGPlayer(this->index);
	credits = player->credits;
	lvl = player->items[item->index].level;
	
	cost = CRPGI::GetItemCost(item->index, lvl+1);

	if(lvl >= item->maxlevel) {
		CRPG::ChatAreaMsg(this->index, TXTDB(player, menu_result.max_lvl));
	}
	else if(credits < cost) {
		CRPG::ChatAreaMsg(this->index, TXTDB(player, menu_result.not_enough_credits), item->name, lvl+1, cost);
	}
	else {
		player->BuyItem(item->index);
		CRPG::ChatAreaMsg(this->index, TXTDB(player, menu_result.item_bought), item->name, lvl+1);
	}

	this->DelMenu();
	return ;
}

void CRPG_Menu::GetSellPage(void) {
	CRPG_Player *player;
	unsigned int i;
	CRPG_MenuOptions *opt, *temp;
	txtkey_t *key;

	player = IndextoRPGPlayer(this->index);
	if(player == NULL) {
		BuildOutput(0, "Error");
		return ;
	}

	if(this->data.ia) {
		BuildOutput(0, "%s\n->1. %s\n->2. %s", TXTDB(player, menu_confirm.sell_confirm),
			TXTDB(player, menu_opt.yes), TXTDB(player, menu_opt.no));
		this->options = 0;
		this->SetOptions(1, 2);
		return ;
	}

	if(!this->opt_count) {
		for(i = 0;i < ITEM_COUNT;i++) {
			if(!player->items[i].level || !CRPG::item_types[i].enable)
				continue;

			key = player->lang->NametoKey("items.%s", CRPG::item_types[i].shortname);
			WARN_IF(key == NULL, continue)

			CRPG::snprintf(temp_out, 1024, "%s %s %s", key->s, LVL_STR, CR_STR);
			opt = CRPG_MenuOptions::AddOption(this, 1, temp_out,
				player->items[i].level, TXTDB(player, menu_txt.sale), CRPGI::GetItemSale(i, player->items[i].level));

			WARN_IF(opt == NULL, continue)
			opt->data.i = CRPG::item_types[i].index;
		}
	}

	if(!this->opt_count) {
		BuildOutput(0, TXTDB(player, menu_misc.no_items));
		return ;
	}

	for(opt = this->opt_first;opt != NULL;opt = opt->ll_next) {
		if(opt->page == this->page) {
			if(opt->page_index > 1)
				BuildOutput(0, "\n");

			temp = opt;
			BuildOutput(0, opt->MakeOutputStr());
			SetOptions(opt->page_index);
		}
	}

	if((temp->prevpage < 0) && (temp->nextpage >= 0)) {
		BuildOutput(0, "\n8. %s\n->9. %s", TXTDB(player, menu_opt.previous), TXTDB(player, menu_opt.next));
		this->SetOptions(9);
	}
	else if((temp->prevpage >= 0) && (temp->nextpage < 0)) {
		BuildOutput(0, "\n->8. %s\n9. %s", TXTDB(player, menu_opt.previous), TXTDB(player, menu_opt.next));
		this->SetOptions(8);
	}
	else if((temp->prevpage >= 0) && (temp->nextpage >= 0)) {
		BuildOutput(0, "\n->8. %s\n->9. %s", TXTDB(player, menu_opt.previous), TXTDB(player, menu_opt.next));
		this->SetOptions(8);
		this->SetOptions(9);
	}

	return ;
}

void CRPG_Menu::SellSelect(unsigned int option) {
	CRPG_Player *player;
	CRPG_MenuOptions *opt;
	struct item_type *item = NULL;

	player = IndextoRPGPlayer(this->index);
	WARN_IF(player == NULL, this->DelMenu(); return)

	if(!this->data.ia) {
		/* Player selected to sell an item */

		if(option == 8) {
			if(this->page)
				this->page--;
			this->data.ia = 0;
			this->CreateMenu();
			return ;
		}
		else if(option == 9) {
			if(!this->data.ia)
				this->page++;
			this->CreateMenu();
			return ;
		}

		this->data.ia = option;
		this->CreateMenu();
	}
	else {
		/* Player selected option on confirm menu */
		if(option != 1) {
			this->DelMenu();
			return ;
		}

		for(opt = this->opt_first;opt != NULL;opt = opt->ll_next) {
			if(opt->page == this->page) {
				if(this->data.ia == opt->page_index) {
					item = &CRPG::item_types[opt->data.i];
					break;
				}
			}
		}

		WARN_IF(item == NULL, this->DelMenu(); return)

		/* Item sold */
		player->SellItem(item->index);
		CRPG::ChatAreaMsg(this->index, TXTDB(player, menu_result.item_sold), item->name, player->items[item->index].level+1);
		this->DelMenu();
	}

	return ;
}

void CRPG_Menu::GetStatsPage(void) {
	CRPG_Player *player = IndextoRPGPlayer(this->index);

	if(player == NULL) {
		BuildOutput(0, "Error");
		return ;
	}

	BuildOutput(0, "->1. %s: %ld", TXTDB(player, menu_txt.level), player->level);
	BuildOutput(0, "\n->2. %s: %ld/%ld", TXTDB(player, menu_txt.exp_short), player->exp, CRPG_StatsManager::LvltoExp(player->level));
	BuildOutput(0, "\n->3. %s: %ld", TXTDB(player, menu_txt.credits), player->credits);
	BuildOutput(0, "\n->4. %s: %ld/%ld", TXTDB(player, menu_txt.rank),
		CRPG_RankManager::GetPlayerRank(player), CRPG_RankManager::GetRankCount());

	SetOptions(1, 2, 3, 4, 5, 6, 7, 8, 9);
	return ;
}

void CRPG_Menu::GetSettingsPage(void) {
	CRPG_TextDB *txtdb;
	CRPG_MenuOptions *opt, *temp;
	CRPG_Player *player = IndextoRPGPlayer(this->index);;
	unsigned int i = 0;

	this->options = 0;

	if(this->data.ia == 1) {
		if(!this->opt_count) {
			WARN_IF(player == NULL, this->DelMenu(); return)

			for(txtdb = CRPG_TextDB::ll_first;txtdb != NULL;txtdb = txtdb->ll_next) {
				if(!txtdb->hidden) {
					if(!CRPG::istrcmp(player->lang->file->name, txtdb->file->name))
						opt = CRPG_MenuOptions::AddOption(this, 1, txtdb->name);
					else
						opt = CRPG_MenuOptions::AddOption(this, 0, txtdb->name);

					WARN_IF(opt == NULL, continue)

					opt->data.i = i++;
				}
			}
		}

		if(!this->opt_count) {
			BuildOutput(0, TXTDB(player, menu_misc.no_items));
			return ;
		}

		for(opt = this->opt_first;opt != NULL;opt = opt->ll_next) {
			if(opt->page == this->page) {
				if(opt->page_index > 1)
					BuildOutput(0, "\n");

				temp = opt;
				BuildOutput(0, opt->MakeOutputStr());

				if(opt->enabled)
					SetOptions(opt->page_index);
			}
		}

		if((temp->prevpage < 0) && (temp->nextpage >= 0)) {
			BuildOutput(0, "\n8. %s\n->9. %s", TXTDB(player, menu_opt.previous), TXTDB(player, menu_opt.next));
			this->SetOptions(9);
		}
		else if((temp->prevpage >= 0) && (temp->nextpage < 0)) {
			BuildOutput(0, "\n->8. %s\n9. %s", TXTDB(player, menu_opt.previous), TXTDB(player, menu_opt.next));
			this->SetOptions(8);
		}
		else if((temp->prevpage >= 0) && (temp->nextpage >= 0)) {
			BuildOutput(0, "\n->8. %s\n->9. %s", TXTDB(player, menu_opt.previous), TXTDB(player, menu_opt.next));
			this->SetOptions(8);
			this->SetOptions(9);
		}
	}
	else if(this->data.ia == 2) {
		BuildOutput(0, "%s", TXTDB(player, menu_confirm.reset_confirm));
		BuildOutput(0, "\n->1. %s\n->2. %s", TXTDB(player, menu_opt.yes), TXTDB(player, menu_opt.no));
		this->SetOptions(1, 2);
	}
	else {
		if(player->lang_is_set)
			BuildOutput(0, "->1. %s", TXTDB(player, menu_opt.language));
		else
			BuildOutput(0, "->1. /!\\ %s", TXTDB(player, menu_opt.language));
		BuildOutput(0, "\n->2. %s", TXTDB(player, menu_opt.reset_stats));
		this->SetOptions(1, 2);
	}

	return ;
}

void CRPG_Menu::SettingsSelect(unsigned int option) {
	CRPG_Player *player;
	CRPG_TextDB *txtdb;
	CRPG_MenuOptions *opt;
	unsigned int i = 0;

	if(option > ITEM_COUNT) {
		this->DelMenu();
		return ;
	}

	if(!this->data.ia) {
		this->data.ia = option;
		this->CreateMenu();
	}
	else if(this->data.ia == 1) {
		/* Language */

		if(option == 8) {
			if(this->page)
				this->page--;
			this->data.ib = 0;
			this->CreateMenu();
			return ;
		}
		else if(option == 9) {
			if(!this->data.ib)
				this->page++;
			this->CreateMenu();
			return ;
		}

		player = IndextoRPGPlayer(this->index);
		WARN_IF(player == NULL, this->DelMenu(); return)

		for(opt = this->opt_first;opt != NULL;opt = opt->ll_next) {
			if(opt->page == this->page) {
				if(opt->page_index == option) {
					for(txtdb = CRPG_TextDB::ll_first;txtdb != NULL;txtdb = txtdb->ll_next) {
						if(i++ == opt->index)
							break;
					}
				}
			}
		}

		WARN_IF(txtdb == NULL, this->DelMenu(); return)

		player->lang = txtdb;
		player->lang_is_set = 1;
		CRPG::ChatAreaMsg(player->index, TXTDB(player, menu_result.lang_changed), txtdb->name);
		this->DelMenu();
	}
	else if(this->data.ia == 2) {
		/* Reset Stats */

		if(option != 1) {
			this->DelMenu();
			return ;
		}

		player = IndextoRPGPlayer(this->index);
		WARN_IF(player == NULL, this->DelMenu(); return)

		player->ResetStats();
		CRPG::ChatAreaMsg(player->index, TXTDB(player, menu_result.stats_reset));
		this->DelMenu();
	}
	else {
		this->DelMenu();
	}

	return ;
}

#define HELP_LINK_COUNT 4
struct {
	char *key;
	char *link;
} help_links[HELP_LINK_COUNT] = {
	{"menu_help.about", "http://cssrpg.sourceforge.net/help/cssrpg.html"},
	{"menu_help.upgrades", "http://cssrpg.sourceforge.net/help/upgrades.html"},
	{"menu_help.commands", "http://cssrpg.sourceforge.net/help/commands.html"},
	{"menu_help.acronyms", "http://cssrpg.sourceforge.net/help/acronyms.html"}
};

void CRPG_Menu::GetHelpPage(void) {
	unsigned int i;
	CRPG_Player *player = IndextoRPGPlayer(this->index);
	txtkey_t *key;

	WARN_IF(player == NULL, return)

	for(i = 0;i < HELP_LINK_COUNT;i++) {
		if(i)
			BuildOutput(0, "\n");

		key = player->lang->NametoKey(help_links[i].key);
		WARN_IF(key == NULL, continue)

		BuildOutput(0, "->%d. %s", i+1, key->s);
		SetOptions(i+1);
	}

	return ;
}

void CRPG_Menu::HelpSelect(unsigned int option) {
	CRPG_Player *player;
	txtkey_t *key;

	if(option > HELP_LINK_COUNT || option < 1) {
		this->DelMenu();
		return ;
	}
	option--;

	player = IndextoRPGPlayer(this->index);
	WARN_IF(player == NULL, return)

	key = player->lang->NametoKey(help_links[option].key);
	WARN_IF(key == NULL, return)

	CRPG::ShowMOTD(this->index, key->s, help_links[option].link, motd_url);
	this->DelMenu();
	return ;
}

void CRPG_Menu::GetTop10Page(void) {
	unsigned int i;
	struct ranklist **ranks;
	CRPG_Player *player = IndextoRPGPlayer(this->index);

	WARN_IF(player == NULL, return)

	CRPG_RankManager::GetTop10Players(&ranks);
	WARN_IF(ranks == NULL, return)

	BuildOutput(0, "%s\n-----\n", TXTDB(player, rpgtop10.msg1));

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
	CRPG_Player *player;

	switch(this->submenu) {
		case none:
			player = IndextoRPGPlayer(this->index);
			WARN_IF(player == NULL, return)
			SetOptions(1, 2, 3, 4, 5);
			BuildOutput(0, "->1. %s\n->2. %s\n->3. %s",
				TXTDB(player, menu_opt.upgrades), TXTDB(player, menu_opt.sell), TXTDB(player, menu_opt.stats));

			if(player->lang_is_set)
				BuildOutput(0, "\n->4. %s", TXTDB(player, menu_opt.settings));
			else
				BuildOutput(0, "\n->4. /!\\ %s", TXTDB(player, menu_opt.settings));
			
			BuildOutput(0, "\n->5. %s", TXTDB(player, menu_opt.help));
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
		BuildOutput(0, "%s %ld\n-----\n", TXTDB(player, menu_txt.credits), player->credits);

	GetMenu();

	/* Finalize Menu */
	this->SetOptions(10);
	BuildOutput(1, "\n0. %s", TXTDB(player, menu_opt.exit));
	return ;
}

void CRPG_Menu::SelectOption(unsigned int option) {
	CRPG::DebugMsg(1, "%s selected option %d in submenu %d on page %d",
		this->name(), option, this->submenu, this->page);

	/* Emit menu sound */
	CRPG::EmitSound(this->index, "buttons/button14.wav");

	if(!option || (option == 10)) {
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
	if(this->opt_first != NULL)
		this->opt_first->FreeOptions();

	return del_node(this);
}

void CRPG_Menu::ShutDown(void) {
	free_nodes(menu_count);

	return ;
}
