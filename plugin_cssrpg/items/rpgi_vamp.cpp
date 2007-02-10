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
#include "rpgi_vamp.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CRPGI_Vamp::Init(void) {
	return ;
}

void CRPGI_Vamp::ShutDown(void) {
	return ;
}

bool CRPGI_Vamp::BuyItem(CRPG_Player *player) {
	return true;
}

bool CRPGI_Vamp::SellItem(CRPG_Player *player) {
	return true;
}

void CRPGI_Vamp::add_health(CRPG_Player *player, unsigned int hp) {
	unsigned int new_health, max_health;

	WARN_IF(player == NULL, return)

	IF_ITEM_ENABLED(ITEM_HBONUS) {
		#pragma message("NOTICE: Offset")
		if(CBaseEntity_GetHealth(player->cbp()) >= CRPGI_HBonus::GetMaxHealth(player))
			return ;
		max_health = CRPGI_HBonus::GetMaxHealth(player);
	}
	else {
		max_health = 100;
	}

	#pragma message("NOTICE: Offset")
	new_health = CBaseEntity_GetHealth(player->cbp())+hp;
	if(new_health <= max_health)
		CBaseEntity_SetHealth(player->cbp(), new_health);
	else
		CBaseEntity_SetHealth(player->cbp(), max_health);

	return ;
}

void CRPGI_Vamp::PlayerDamage(CRPG_Player *attacker, CRPG_Player *victim, int dmg_health, int dmg_armor) {
	unsigned int total_dmg = (unsigned int)(dmg_health+dmg_armor);
	float inc;

	WARN_IF((attacker == NULL) || (victim == NULL), return)

	IF_ITEM_NENABLED(ITEM_VAMP)
		return ;

	IF_BOT_NENABLED(attacker)
		return ;

	if(attacker->css.team == victim->css.team)
		return ;

	if(!attacker->items[ITEM_VAMP].level)
		return ;

	inc = (float)attacker->items[ITEM_VAMP].level*VAMP_INC;
	add_health(attacker, floor(((float)total_dmg*inc)+0.5f));

	return ;
}
