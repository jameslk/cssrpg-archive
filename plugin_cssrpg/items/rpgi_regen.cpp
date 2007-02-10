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
#include "rpgi_hbonus.h"
#include "rpgi_regen.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CRPG_Timer* CRPGI_Regen::inc_health;

void CRPGI_Regen::Init(void) {
	IF_ITEM_ENABLED(ITEM_REGEN)
		inc_health = CRPG_Timer::AddTimer(1, 0, CRPGI_Regen::IncreaseHealth, 0);
	else
		inc_health = NULL;

	return ;
}

void CRPGI_Regen::ShutDown(void) {
	if(inc_health != NULL) {
		inc_health->DelTimer();
		inc_health = NULL;
	}

	return ;
}

bool CRPGI_Regen::BuyItem(CRPG_Player *player) {
	return true;
}

bool CRPGI_Regen::SellItem(CRPG_Player *player) {
	return true;
}

TIMER_FUNC(CRPGI_Regen::IncreaseHealth) {
	CRPG_Player *player;
	unsigned int lvl, i = CRPG_Player::player_count;

	IF_ITEM_NENABLED(ITEM_REGEN)
		return ;

	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			player = CRPG_Player::players[i];
			IF_BOT_NENABLED(player)
				continue;

			lvl = player->items[ITEM_REGEN].level;
			if(lvl) {
				if(/* player->cbp()->GetHealth() < (int)hb->health*/ 1) {
					#pragma message("NOTICE: Offset")
					IF_ITEM_ENABLED(ITEM_HBONUS)
						CBaseEntity_SetHealth(player->cbp(), CBaseEntity_GetHealth(player->cbp())+lvl > CRPGI_HBonus::GetMaxHealth(player) ?
							CRPGI_HBonus::GetMaxHealth(player) : CBaseEntity_GetHealth(player->cbp())+lvl);
					else
						CBaseEntity_SetHealth(player->cbp(), CBaseEntity_GetHealth(player->cbp())+lvl > 100 ?
							100 : CBaseEntity_GetHealth(player->cbp())+lvl);
				}
			}
		}
	}
	
	return ;
}
