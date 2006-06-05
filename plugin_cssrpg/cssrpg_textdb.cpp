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
#include <stdlib.h>

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

#include "cssrpg.h"
#include "cssrpg_textdb.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

template class CRPG_LinkedList<CRPG_TextDB>;
template<> CRPG_TextDB* CRPG_LinkedList<CRPG_TextDB>::ll_first;
template<> CRPG_TextDB* CRPG_LinkedList<CRPG_TextDB>::ll_last;
template<> unsigned int CRPG_LinkedList<CRPG_TextDB>::ll_count;
CRPG_TextDB* CRPG_TextDB::deflang = NULL;

#define ADD_KEY(key) txt.key_array[i] = &txt.key; \
	strncpy(txt.key.name, #key, 31); \
	txt.key_array[i++]->id = i;

void CRPG_TextDB::init_keyarray(void) {
	unsigned int i = 0;

	ADD_KEY(greeting.msg1);
	ADD_KEY(items.regen);
	ADD_KEY(items.hbonus);
	ADD_KEY(items.resup);
	ADD_KEY(items.vamp);
	ADD_KEY(items.stealth);
	ADD_KEY(items.ljump);
	ADD_KEY(items.fnade);
	ADD_KEY(items.icestab);
	ADD_KEY(items.fpistol);
	ADD_KEY(items.denial);
	ADD_KEY(newlvl.msg1);
	ADD_KEY(newlvl.msg2);
	ADD_KEY(newbielvl.msg1);
	ADD_KEY(newbielvl.msg2);
	ADD_KEY(exphint.msg1);
	ADD_KEY(rpgrank.msg1);
	ADD_KEY(rpgtop10.msg1);
	ADD_KEY(menu_txt.level);
	ADD_KEY(menu_txt.exp_short);
	ADD_KEY(menu_txt.exp_long);
	ADD_KEY(menu_txt.credits);
	ADD_KEY(menu_txt.rank);
	ADD_KEY(menu_txt.cost);
	ADD_KEY(menu_txt.sale);
	ADD_KEY(menu_result.stats_reset);
	ADD_KEY(menu_result.max_lvl);
	ADD_KEY(menu_result.not_enough_credits);
	ADD_KEY(menu_result.item_bought);
	ADD_KEY(menu_result.item_sold);
	ADD_KEY(menu_result.lang_changed);
	ADD_KEY(menu_opt.upgrades);
	ADD_KEY(menu_opt.sell);
	ADD_KEY(menu_opt.stats);
	ADD_KEY(menu_opt.settings);
	ADD_KEY(menu_opt.help);
	ADD_KEY(menu_opt.next);
	ADD_KEY(menu_opt.previous);
	ADD_KEY(menu_opt.yes);
	ADD_KEY(menu_opt.no);
	ADD_KEY(menu_opt.reset_stats);
	ADD_KEY(menu_opt.language);
	ADD_KEY(menu_opt.exit);
	ADD_KEY(menu_confirm.sell_confirm);
	ADD_KEY(menu_confirm.reset_confirm);
	ADD_KEY(menu_help.about);
	ADD_KEY(menu_help.upgrades);
	ADD_KEY(menu_help.commands);
	ADD_KEY(menu_help.acronyms);
	ADD_KEY(menu_misc.no_items);

	while(i--)
		txt.key_array[i]->s = NULL;

	return ;
}

void CRPG_TextDB::Init(void) {
	LoadLanguages();

	return ;
}

void CRPG_TextDB::ShutDown(void) {
	UnloadLanguages();

	return ;
}

CRPG_TextDB *FiletoTextDB(char *filename) {
	CRPG_TextDB *txtdb;

	WARN_IF(filename == NULL, return NULL)

	for(txtdb = CRPG_TextDB::ll_first;txtdb != NULL;txtdb = txtdb->ll_next) {
		if(CRPG::istrcmp(filename, txtdb->file->name))
			return txtdb;
	}

	return NULL;
}

key_t* CRPG_TextDB::NametoKey(char *namef, ...) {
	unsigned int i = TXTDB_KEY_COUNT;
	char name[1024];
	va_list ap;

	WARN_IF(namef == NULL, return NULL)

	va_start(ap, namef);
	Q_vsnprintf(name, 1024, namef, ap);
	va_end(ap);

	while(i--) {
		if(CRPG::istrcmp(this->txt.key_array[i]->name, name))
			return this->txt.key_array[i];
	}

	return NULL;
}

key_t* CRPG_TextDB::IDtoKey(unsigned int id) {
	unsigned int i = TXTDB_KEY_COUNT;

	WARN_IF(id > TXTDB_KEY_COUNT, return NULL)

	while(i--) {
		if(this->txt.key_array[i]->id == id)
			return this->txt.key_array[i];
	}

	return NULL;
}

#define ASSIGN(x, y) assign_key(&deflang->txt.x, y);
void CRPG_TextDB::LoadDefault(void) {
	deflang = new CRPG_TextDB();

	deflang->name = (char*)calloc(sizeof(char), strlen("Default")+1);
	strcpy(deflang->name, "Default");

	deflang->hidden = 1;
	deflang->ll_add();

	ASSIGN(greeting.msg1, "Type \"rpgmenu\" to bring up your options.");
	ASSIGN(items.regen, "Regeneration");
	ASSIGN(items.hbonus, "Health+");
	ASSIGN(items.resup, "Resupply");
	ASSIGN(items.vamp, "Vampire");
	ASSIGN(items.stealth, "Stealth");
	ASSIGN(items.ljump, "LongJump");
	ASSIGN(items.fnade, "FireGrenade");
	ASSIGN(items.icestab, "IceStab");
	ASSIGN(items.fpistol, "FrostPistol");
	ASSIGN(items.denial, "Denial");
	ASSIGN(newlvl.msg1, "%s is now Level %d");
	ASSIGN(newlvl.msg2, "You have new credits (%ld total). Type \"rpgmenu\" to buy upgrades.");
	ASSIGN(newbielvl.msg1, "\x04You have gained a new Level! This means you can buy Upgrades which give you an advantage over your opponents.\x01");
	ASSIGN(newbielvl.msg2, "\x04Type \"\x03rpgmenu\x04\" in chat, or type it into the console to bring up a menu from which you can buy Upgrades.\x01");
	ASSIGN(exphint.msg1, "Experience Gained: %ld+\nExperience Quota: %ld/%ld");
	ASSIGN(rpgrank.msg1, "%s is Level %ld, ranked %ld/%ld with %ld/%ld Experience and %ld Credits");
	ASSIGN(rpgtop10.msg1, "Top 10 CSS:RPG Players");
	ASSIGN(menu_txt.level, "Level");
	ASSIGN(menu_txt.exp_short, "Exp");
	ASSIGN(menu_txt.exp_long, "Experience");
	ASSIGN(menu_txt.credits, "Credits");
	ASSIGN(menu_txt.rank, "Rank");
	ASSIGN(menu_txt.cost, "Cost");
	ASSIGN(menu_txt.sale, "Sale");
	ASSIGN(menu_result.stats_reset, "Your stats have been successfully reset.");
	ASSIGN(menu_result.max_lvl, "You have reached the maximum level for this item.");
	ASSIGN(menu_result.not_enough_credits, "You do not have enough credits to buy %s Lvl %d. (Requires %d Credits)");
	ASSIGN(menu_result.item_bought, "%s Level %ld successfully purchased.");
	ASSIGN(menu_result.item_sold, "%s Level %ld has been sold.");
	ASSIGN(menu_result.lang_changed, "CSS:RPG text will now appear in the %s language.");
	ASSIGN(menu_opt.upgrades, "Upgrades");
	ASSIGN(menu_opt.sell, "Sell");
	ASSIGN(menu_opt.stats, "Stats");
	ASSIGN(menu_opt.settings, "Settings");
	ASSIGN(menu_opt.help, "Help");
	ASSIGN(menu_opt.next, "Next");
	ASSIGN(menu_opt.previous, "Previous");
	ASSIGN(menu_opt.yes, "Yes");
	ASSIGN(menu_opt.no, "No");
	ASSIGN(menu_opt.reset_stats, "Reset Stats");
	ASSIGN(menu_opt.language, "Language");
	ASSIGN(menu_opt.exit, "Exit");
	ASSIGN(menu_confirm.sell_confirm, "Are you sure?");
	ASSIGN(menu_confirm.reset_confirm, "WARNING: You will lose all Levels, Credits, and Experience.\nReset stats permanently?");
	ASSIGN(menu_help.about, "About CSS:RPG");
	ASSIGN(menu_help.upgrades, "CSS:RPG Upgrades");
	ASSIGN(menu_help.commands, "CSS:RPG Commands");
	ASSIGN(menu_help.acronyms, "CSS:RPG Acronyms");
	ASSIGN(menu_misc.no_items, "No items available");

	return ;
}

void CRPG_TextDB::LoadLanguages(void) {
	char path[MAX_PATH], *buf;
	file_info *file = NULL;
	unsigned int i = 0, keyc, result, fsize;
	FILE *fptr;
	CRPG_TextDB *txtdb;

	UnloadLanguages();
	LoadDefault();

	memset(path, '\0', MAX_PATH);

	CRPG::s_engine()->GetGameDir(path, MAX_PATH);
	Q_snprintf(path, MAX_PATH, "%s%s", path, TEXTDB_PATH);

	do {
		if(file == NULL)
			file = (struct file_info*)malloc(sizeof(struct file_info));

		result = CRPG::traverse_dir(*file, path, i++);
		if((result == 1) && (file->type == file_normal) && CRPG::istrcmp(file->ext, "txt")) {
			fptr = fopen(file->fullpath, "rb");
			if(!fptr) {
				CRPG::ConsoleMsg("Failed to open language file: %s", MTYPE_WARNING, file->name);
				continue;
			}

			fseek(fptr, 0, SEEK_END);
			fsize = ftell(fptr);
			rewind(fptr);

			if(fsize < 5) {
				fclose(fptr);
				CRPG::ConsoleMsg("Language file %s is too small", MTYPE_WARNING, file->name);
				continue;
			}

			buf = (char*)calloc(fsize+1, sizeof(char));
			fread(buf, sizeof(char), fsize, fptr);
			fclose(fptr);

			/* Check for "EF BB BF" UTF-8 BOM */
			if(!memcmp(buf, "\xEF\xBB\xBF", 3))
				buf += 3;

			KeyValues *kv = new KeyValues(file->fullpath);
			kv->UsesEscapeSequences(true);
			if(!kv->LoadFromBuffer(file->name, buf)) {
				CRPG::ConsoleMsg("Failed to parse language file: %s", MTYPE_WARNING, file->name);
				continue;
			}

			free(buf);

			txtdb = new CRPG_TextDB();
			txtdb->file = file;
			txtdb->ll_add();

			keyc = TXTDB_KEY_COUNT;
			while(keyc--) { /* Set the default strings */
				if(!assign_key(txtdb->txt.key_array[keyc], deflang->txt.key_array[keyc]->s))
					CRPG::DebugMsg("Failed to assign default key #%d to new TextDB", keyc);
			}

			txtdb->parse_txtfile(kv);

			kv->deleteThis();
			file = NULL;
		}
	} while(result == 1);

	if(!result)
		CRPG::ConsoleMsg("Error when loading languages from: %s", MTYPE_ERROR, path);

	free(file);
	return ;
}

void CRPG_TextDB::UnloadLanguages(void) {
	CRPG_TextDB *txt, *next;

	for(txt = ll_first;txt != NULL;txt = next) {
		next = txt->ll_next;
		txt->ll_del();
		delete txt;
	}

	return ;
}

unsigned int CRPG_TextDB::assign_key(key_t *key, const char *str) {
	unsigned int len;

	WARN_IF(str == NULL, return 0)

	len = strlen(str);
	WARN_IF(!len, return 0)

	if((*key).s != NULL)
		free((*key).s);

	(*key).s = (char*)calloc(sizeof(char), len+1);
	strcpy((*key).s, str);

	return 1;
}

#undef ASSIGN
#define ASSIGN(x, y) if(!assign_key(&x, subkey->GetString(y))) \
	CRPG::DebugMsg("Failed to assign key \"%s\" from file \"%s\"", #x, this->file->name)
void CRPG_TextDB::parse_txtfile(KeyValues *kv, unsigned int phase) {
	KeyValues *subkey = NULL;

	if(!phase) {
		for(subkey = kv;subkey != NULL;subkey = subkey->GetNextTrueSubKey()) {
			if(CRPG::istrcmp((char*)subkey->GetName(), "Language")) {
				this->name = (char*)calloc(sizeof(char), strlen(subkey->GetString("name"))+1);
				strcpy(this->name, subkey->GetString("name"));
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "Greeting")) {
				ASSIGN(txt.greeting.msg1, "greeting");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "Items")) {
				ASSIGN(txt.items.denial, "denial");
				ASSIGN(txt.items.fnade, "fnade");
				ASSIGN(txt.items.fpistol, "fpistol");
				ASSIGN(txt.items.hbonus, "hbonus");
				ASSIGN(txt.items.icestab, "icestab");
				ASSIGN(txt.items.ljump, "ljump");
				ASSIGN(txt.items.regen, "regen");
				ASSIGN(txt.items.resup, "resup");
				ASSIGN(txt.items.stealth, "stealth");
				ASSIGN(txt.items.vamp, "vamp");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "NewLevel")) {
				ASSIGN(txt.newlvl.msg1, "new_lvl1");
				ASSIGN(txt.newlvl.msg2, "new_lvl2");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "NewbieText")) {
				ASSIGN(txt.newbielvl.msg1, "newbie_lvl1");
				ASSIGN(txt.newbielvl.msg2, "newbie_lvl2");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "ExpHintBox")) {
				ASSIGN(txt.exphint.msg1, "exp_hint");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "rpgrank")) {
				ASSIGN(txt.rpgrank.msg1, "rpgrank");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "rpgtop10")) {
				ASSIGN(txt.rpgtop10.msg1, "top10_title");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "Menu")) {
				parse_txtfile(subkey->GetFirstSubKey(), 1);
				break;
			}
		}
	}
	else { /* Menu */
		for (subkey = kv;subkey != NULL;subkey = subkey->GetNextTrueSubKey()) {
			if(CRPG::istrcmp((char*)subkey->GetName(), "Text")) {
				ASSIGN(txt.menu_txt.cost, "cost");
				ASSIGN(txt.menu_txt.credits, "credits");
				ASSIGN(txt.menu_txt.exp_long, "exp_long");
				ASSIGN(txt.menu_txt.exp_short, "exp_short");
				ASSIGN(txt.menu_txt.level, "level");
				ASSIGN(txt.menu_txt.rank, "rank");
				ASSIGN(txt.menu_txt.sale, "sale");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "Result")) {
				ASSIGN(txt.menu_result.item_bought, "item_bought");
				ASSIGN(txt.menu_result.item_sold, "item_sold");
				ASSIGN(txt.menu_result.max_lvl, "max_lvl");
				ASSIGN(txt.menu_result.not_enough_credits, "not_enough_credits");
				ASSIGN(txt.menu_result.stats_reset, "stats_reset");
				ASSIGN(txt.menu_result.lang_changed, "lang_changed");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "Options")) {
				ASSIGN(txt.menu_opt.help, "help");
				ASSIGN(txt.menu_opt.next, "next");
				ASSIGN(txt.menu_opt.no, "no");
				ASSIGN(txt.menu_opt.previous, "previous");
				ASSIGN(txt.menu_opt.reset_stats, "reset_stats");
				ASSIGN(txt.menu_opt.sell, "sell");
				ASSIGN(txt.menu_opt.settings, "settings");
				ASSIGN(txt.menu_opt.stats, "stats");
				ASSIGN(txt.menu_opt.upgrades, "upgrades");
				ASSIGN(txt.menu_opt.yes, "yes");
				ASSIGN(txt.menu_opt.language, "language");
				ASSIGN(txt.menu_opt.exit, "exit");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "Confirm")) {
				ASSIGN(txt.menu_confirm.reset_confirm, "reset_confirm");
				ASSIGN(txt.menu_confirm.sell_confirm, "sell_confirm");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "Help")) {
				ASSIGN(txt.menu_help.about, "about");
				ASSIGN(txt.menu_help.commands, "commands");
				ASSIGN(txt.menu_help.acronyms, "acronyms");
				ASSIGN(txt.menu_help.upgrades, "upgrades");
			}
			else if(CRPG::istrcmp((char*)subkey->GetName(), "Misc")) {
				ASSIGN(txt.menu_misc.no_items, "no_items");
			}
		}
	}

	return ;
}
