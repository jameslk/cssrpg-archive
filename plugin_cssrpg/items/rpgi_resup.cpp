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
#include "rpgi_resup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CRPG_Timer* CRPGI_Resup::inc_ammo;

void CRPGI_Resup::Init(void) {
	IF_ITEM_ENABLED(ITEM_REGEN)
		inc_ammo = CRPG_Timer::AddTimer(3, 0, CRPGI_Resup::IncreaseAmmo, 0);
	else
		inc_ammo = NULL;

	return ;
}

void CRPGI_Resup::ShutDown(void) {
	if(inc_ammo != NULL) {
		inc_ammo->DelTimer();
		inc_ammo = NULL;
	}

	return ;
}

bool CRPGI_Resup::BuyItem(CRPG_Player *player) {
	return true;
}

bool CRPGI_Resup::SellItem(CRPG_Player *player) {
	return true;
}

TIMER_FUNC(CRPGI_Resup::IncreaseAmmo) {
	CRPG_Player *player;
	unsigned int lvl, i = CRPG_Player::player_count, ii;
	CBaseCombatCharacter *cbcc;

	IF_ITEM_NENABLED(ITEM_RESUP)
		return ;

	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			player = CRPG_Player::players[i];
			IF_BOT_NENABLED(player)
				continue;

			if((lvl = player->items[ITEM_RESUP].level)) {
				cbcc = static_cast<CBaseCombatCharacter*>(player->cbp());
				ii = 15;
				while(ii--) {
					if(ii != 12) /* 12 = flashbang */
						CBaseCombatCharacter_GiveAmmo(cbcc, lvl, ii, 1);
				}
			}
		}
	}
	
	return ;
}
