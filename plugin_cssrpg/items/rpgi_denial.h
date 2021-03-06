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

#ifndef RPGI_DENIAL_H
#define RPGI_DENIAL_H

/*
	Reference:
	primammo, secammo, vest, vesthelm, defuser, nvgs, flashbang, hegrenade
	smokegrenade, galil, ak47, scout, sg552, awp, g3sg1, famas, m4a1, aug
	sg550, glock, usp, p228, deagle, elite, fiveseven, m3, xm1014, mac10
	tmp, mp5navy, ump45, p90, m249
*/

struct css_inventory {
	char primary[24];
	char secondary[24];
	char second_default[24];

	struct {
		bool flashbang;
		bool hegrenade;
		bool smokegrenade;
		bool defuser;
		bool nvgs;
	} equip;
};

#include "../cssrpg_misc.h"
class CRPGI_Denial: public CRPG_PlayerClass<CRPGI_Denial> {
	/* Private Variables */
	struct css_inventory inv;

	/* Private Functions */
	static bool is_restricted(char *weapon);

public:
	/* Public Variables */
	bool was_dead;

	static CRPGI_Denial** players;
	static unsigned int player_count;
	static bool round_end;
	static bool players_spawned;
	static int last_bomb_index;

	/* Public Functions */
	CRPGI_Denial() {
		memset(inv.primary, '\0', 24);
		memset(inv.secondary, '\0', 24);
		memset(inv.second_default, '\0', 24);
		memset(&inv.equip, 0, sizeof(inv.equip));
		was_dead = 0;
	}

	static void Init(void);
	static void ShutDown(void);
	static bool BuyItem(class CRPG_Player *player);
	static bool SellItem(class CRPG_Player *player);

	void ResetItems(void);

	static void ItemPickup(CRPG_Player *player, char *item);
	static void NextFrame(void);
	static void NadeDetonate(CRPG_Player *player, char *nade);
};

CRPGI_Denial* IndextoDenial(int index);

#endif
