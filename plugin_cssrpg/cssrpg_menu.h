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

#ifndef CSSRPG_MENU_H
#define CSSRPG_MENU_H

#include "cssrpg_misc.h"
class CRPG_Menu;
class CRPG_MenuOptions: public CRPG_DynLinkedList<CRPG_MenuOptions> {
private:
	/* Private Variables */
	CRPG_Menu *menu;
	char output[1024];

	/* Private Functions */
	static void update_options(CRPG_Menu *menu);

public:
	/* Public Variables */
	char str[1024];
	int index;
	int page_index;
	char enabled;

	int page;
	int nextpage;
	int prevpage;

	struct {
		char s[256];
		int i;
		float f;
		void *p;
	} data; /* option data */

	/* Public Functions */
	CRPG_MenuOptions(): index(1), page_index(1), enabled(1), page(1) {
		memset(str, '\0', 1024);
		memset(&data, '\0', sizeof(data));
	}

	~CRPG_MenuOptions() {
		if(str != NULL)
			free(str);
	}

	static CRPG_MenuOptions* AddOption(CRPG_Menu *opt_menu, char opt_enabled, char *strf, ...); /* Add option to end */
	static CRPG_MenuOptions* PageIndextoOpt(CRPG_Menu *menu, int opt_page, int opt_index);
	char* MakeOutputStr(void);
	void DelOption(void);
	void FreeOptions(void);
};

enum submenu_t {
	/* Default RPG Menu submenus */
	none, upgrades, sell, stats, settings, help,
	/* These menus are hidden */
	top10
};

#include "cssrpg.h"
class CRPG_Menu: public CRPG_PlayerClass<CRPG_Menu>, private CRPG_GlobalSettings {
	/* Private Variables */
	struct {
		char s[256];
		int ia;
		int ib;
		int ic;
		float f;
		void *p;
	} data; /* menu to menu data */

	char menu_out[200];
	int menu_outlen;

	/* Private Functions */
	void GetUpgradesPage(void);
	void UpgradesSelect(unsigned int option);
	void GetSellPage(void);
	void SellSelect(unsigned int option);
	void GetStatsPage(void);
	void GetSettingsPage(void);
	void SettingsSelect(unsigned int option);
	void GetHelpPage(void);
	void HelpSelect(unsigned int option);

	void GetTop10Page(void);

	void GetMenu(void);
	void SetOptions(char bit1, char bit2, char bit3 = 0, char bit4 = 0, char bit5 = 0, char bit6 = 0, char bit7 = 0, char bit8 = 0, char bit9 = 0);
	void SetOptions(unsigned int opt_num);
	void BuildOutput(char finalize, char *str, ...);
	void SendOutput(char finalize);

public:
	/* Public Variables */
	static CRPG_Menu **menus;
	static unsigned int menu_count;

	submenu_t submenu;
	CRPG_MenuOptions *opt_first;
	CRPG_MenuOptions *opt_last;
	unsigned int opt_count;
	int page;
	unsigned int options;

	char header; /* Show the Credits header for this menu (on by default) */

	/* Public Functions */
	CRPG_Menu(): menu_outlen(0), submenu(none), opt_first(NULL), opt_last(NULL), opt_count(0), page(0), options(0), header(1) {
		memset(&data, 0, sizeof(data));
		index = 0;
		userid = 0;
	}

	static void Init(void);
	static void ShutDown(void);

	void CreateMenu(void);
	void SelectOption(unsigned int option);

	static CRPG_Menu* AddMenu(edict_t *e);
	unsigned int DelMenu(void);
};

CRPG_Menu* IndextoRPGMenu(int index);
CRPG_Menu* EdicttoRPGMenu(edict_t *e);

#endif
