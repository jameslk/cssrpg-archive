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
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"

#define GAME_DLL 1
#include "cbase.h"
#define GAME_DLL 1

#include "../cssrpg.h"
#include "../cssrpg_hacks.h"
#include "rpgi.h"
#include "rpgi_denial.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PRIM_TYPES_COUNT 18
#define SEC_TYPES_COUNT 6

char *prim_types[PRIM_TYPES_COUNT] = {
	"galil", "ak47", "scout", "sg552", "awp", "g3sg1", "famas",
	"m4a1", "aug", "sg550", "m3", "xm1014", "mac10", "tmp", 
	"mp5navy", "ump45", "p90", "m249"
};

char *sec_types[SEC_TYPES_COUNT] = {
	"glock", "usp", "p228", "deagle", "elite", "fiveseven"
};

template class CRPG_PlayerClass<CRPGI_Denial>;
template<> CRPGI_Denial** CRPG_PlayerClass<CRPGI_Denial>::nodes;
CRPGI_Denial** CRPGI_Denial::players;
unsigned int CRPGI_Denial::player_count;
bool CRPGI_Denial::round_end = 0;
bool CRPGI_Denial::players_spawned = 0;
int CRPGI_Denial::last_bomb_index = -1;

bool CRPGI_Denial::is_restricted(char *weapon) {
	char *restricted = CRPG_GlobalSettings::denial_restrict;
	unsigned int i, ii = 0, skip = 0,
		rlen = strlen(restricted), wlen = strlen(weapon);

	if(!rlen)
		return 0;

	if((wlen > 7) && !memcmp(weapon, "weapon_", 7)) {
		weapon += 7;
		wlen -= 7;
	}

	WARN_IF(!wlen, return 0);

	for(i = 0;i < rlen;i++) {
		if(!skip) {
			if(restricted[i] == ' ') {
				if(ii == wlen) {
					return 1;
				}
				else {
					ii = 0;
					continue;
				}
			}
			else if(ii > wlen) {
				skip = 1;
			}
			else if(restricted[i] == weapon[ii]) {
                ii++;
				continue;
			}
			else if(tolower(restricted[i]) == tolower(weapon[ii])) {
                ii++;
				continue;
			}
			else {
				skip = 1;
			}
		}
		else if(restricted[i] == ' ') {
			ii = 0;
			skip = 0;
		}
	}

	if((ii == wlen) && !skip)
        return 1;

	return 0;
}

void CRPGI_Denial::Init(void) {
	unsigned int i;
	player_count = (unsigned int)CRPG::maxClients();
	i = player_count;

	players = new CRPGI_Denial *[player_count];
	while(i--)
		players[i] = NULL;

	nodes = players;

	last_bomb_index = -1;
	return ;
}

void CRPGI_Denial::ShutDown(void) {
	free_nodes(player_count);

	return ;
}

bool CRPGI_Denial::BuyItem(CRPG_Player *player) {
	return true;
}

bool CRPGI_Denial::SellItem(CRPG_Player *player) {
	return true;
}

CRPGI_Denial* IndextoDenial(int index) {
	return CRPGI_Denial::IndextoHandle(index);
}

void CRPGI_Denial::ResetItems(void) {
	memset(this->inv.primary, '\0', 24);
	memset(this->inv.secondary, '\0', 24);
	memset(this->inv.second_default, '\0', 24);
	memset(&this->inv.equip, 0, sizeof(this->inv.equip));

	return ;
}

void CRPGI_Denial::ItemPickup(CRPG_Player *player, char *item) {
	CRPGI_Denial *dn;
	unsigned int i;

	WARN_IF((player == NULL) || (item == NULL), return)

	dn = IndextoDenial(player->index);
	if(dn == NULL) {
		dn = new_node(player->e());
	}
	else if(dn->userid != player->userid) {
		del_node(dn);
		dn = new_node(player->e());
	}

	if(round_end) {
		if(CRPG::istrcmp(item, "glock") || CRPG::istrcmp(item, "usp")) {
			memset(dn->inv.second_default, '\0', 24);
			CRPG::snprintf(dn->inv.second_default, 23, "weapon_%s", item);
			return ;
		}
	}

	if(CRPG::istrcmp(item, "c4")) {
		last_bomb_index = dn->index;
		return ;
	}

	i = PRIM_TYPES_COUNT;
	while(i--) {
		if(CRPG::istrcmp(item, prim_types[i])) {
			memset(dn->inv.primary, '\0', 24);
			CRPG::snprintf(dn->inv.primary, 23, "weapon_%s", item);
			return ;
		}
	}

	i = SEC_TYPES_COUNT;
	while(i--) {
		if(CRPG::istrcmp(item, sec_types[i])) {
			memset(dn->inv.secondary, '\0', 24);
			CRPG::snprintf(dn->inv.secondary, 23, "weapon_%s", item);
			return ;
		}
	}

	if(CRPG::istrcmp(item, "flashbang"))
		dn->inv.equip.flashbang = 1;
	else if(CRPG::istrcmp(item, "hegrenade"))
		dn->inv.equip.hegrenade = 1;
	else if(CRPG::istrcmp(item, "smokegrenade"))
		dn->inv.equip.smokegrenade = 1;
	else if(CRPG::istrcmp(item, "defuser"))
		dn->inv.equip.defuser = 1;
	else if(CRPG::istrcmp(item, "nvgs"))
		dn->inv.equip.nvgs = 1;

	return ;
}

void CRPGI_Denial::NextFrame(void) {
	CRPGI_Denial *dn;
	CRPG_Player *player;
	unsigned int i;
	static bool players_stripped = 0;

	if(!players_spawned)
		return ;

	i = player_count;

	/* Reset player Denial data while Denial is disabled */
	IF_ITEM_NENABLED(ITEM_DENIAL) {
		while(i--) {
			if(CRPG_Player::players[i] != NULL) {
				player = CRPG_Player::players[i];
				dn = IndextoDenial(player->index);
				if(dn == NULL)
					continue;

				dn->ResetItems();
				dn->was_dead = 0;
			}
		}

		players_stripped = 0;
		players_spawned = 0;
		return ;
	}

	/* Strip player weapons this frame */
	if(!players_stripped) {
		while(i--) {
			if(CRPG_Player::players[i] != NULL) {
				player = CRPG_Player::players[i];

				IF_BOT_NENABLED(player)
					continue;

				if(player->items[ITEM_DENIAL].level < 1)
					continue;

				dn = IndextoDenial(player->index);
				if(dn == NULL) /* don't warn */
					continue;

				if(!dn->was_dead)
					continue;

				if(player->items[ITEM_DENIAL].level >= 2) { /* Strip all weapons to make room for ours */
					CRPG::SetCheats(1);
					s_helpers->ClientCommand(player->e(), "give player_weaponstrip\n");
					s_helpers->ClientCommand(player->e(), "ent_fire player_weaponstrip Strip\n");
					CRPG::SetCheats(0);
				}
			}
		}

		players_stripped = 1;
		return ;
	}

	/* Give players their Denial weapons this frame */
	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			player = CRPG_Player::players[i];

			IF_BOT_NENABLED(player)
				continue;

			dn = IndextoDenial(player->index);
			if(dn == NULL) /* don't warn */
				continue;

			if(!dn->was_dead)
				continue;

			dn->was_dead = 0;
			CBasePlayer_GiveNamedItem(player->cbp(), "weapon_knife");

			if(dn->index == last_bomb_index) {
				CBasePlayer_GiveNamedItem(player->cbp(), "weapon_c4");
				last_bomb_index = -1;
			}

			/* Level 1 */
			if(player->items[ITEM_DENIAL].level >= 1) {
				if(dn->inv.equip.flashbang && !is_restricted("flashbang"))
					CBasePlayer_GiveNamedItem(player->cbp(), "weapon_flashbang");
				if(dn->inv.equip.hegrenade && !is_restricted("hegrenade"))
					CBasePlayer_GiveNamedItem(player->cbp(), "weapon_hegrenade");
				if(dn->inv.equip.smokegrenade && !is_restricted("smokegrenade"))
					CBasePlayer_GiveNamedItem(player->cbp(), "weapon_smokegrenade");
				if(dn->inv.equip.defuser && !is_restricted("defuser"))
					CBasePlayer_GiveNamedItem(player->cbp(), "item_defuser");
				if(dn->inv.equip.nvgs && !is_restricted("nvgs"))
					CBasePlayer_GiveNamedItem(player->cbp(), "item_nvgs");
			}
			else {
				goto reset_equip;
			}
			/* *** */

			/* Level 2 */
			if(player->items[ITEM_DENIAL].level >= 2) {
				if(*dn->inv.secondary && !is_restricted(dn->inv.secondary))
					CBasePlayer_GiveNamedItem(player->cbp(), dn->inv.secondary);
				else if(*dn->inv.second_default)
					CBasePlayer_GiveNamedItem(player->cbp(), dn->inv.second_default);
			}
			else {
				goto reset_secondary;
			}
			/* *** */

			/* Level 3 */
			if(player->items[ITEM_DENIAL].level >= 3) {
				if(*dn->inv.primary && !is_restricted(dn->inv.primary))
					CBasePlayer_GiveNamedItem(player->cbp(), dn->inv.primary);
			}
			else {
				goto reset_primary;
			}
			/* *** */

			continue;

		reset_equip:
			memset(&dn->inv.equip, 0, sizeof(dn->inv.equip));

		reset_secondary:
			memset(dn->inv.secondary, '\0', 24);
			memset(dn->inv.second_default, '\0', 24);

		reset_primary:
			memset(dn->inv.primary, '\0', 24);
		}
	}

	players_stripped = 0;
	players_spawned = 0;
	return ;
}

void CRPGI_Denial::NadeDetonate(CRPG_Player *player, char *nade) {
	CRPGI_Denial *dn;

	WARN_IF((player == NULL) || (strlen(nade) < 9), return)
	dn = IndextoDenial(player->index);
	WARN_IF(dn == NULL, return)

	if(!memcmp(nade, "hegrenade", 9))
		dn->inv.equip.hegrenade = 0;
	else if(!memcmp(nade, "flashbang", 9))
		dn->inv.equip.flashbang = 0;
	else if(!memcmp(nade, "smokegrenade", 12))
		dn->inv.equip.smokegrenade = 0;

	return ;
}
