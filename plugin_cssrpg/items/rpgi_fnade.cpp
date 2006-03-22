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

#define GAME_DLL 1
#include "cbase.h"
#define GAME_DLL 1

#include "../cssrpg.h"
#include "../cssrpg_hacks.h"
#include "rpgi.h"
#include "rpgi_fnade.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CRPGI_FNade::Init(void) {
	return ;
}

void CRPGI_FNade::ShutDown(void) {
	return ;
}

void CRPGI_FNade::BuyItem(void *ptr) {
	return ;
}

void CRPGI_FNade::SellItem(void *ptr) {
	return ;
}

void CRPGI_FNade::PlayerDamage(CRPG_Player *attacker, CRPG_Player *victim, int dmg_health, int dmg_armor) {
	float duration;

	if((dmg_health+dmg_armor) < FNADE_DMG_MIN)
		return ;

	IF_ITEM_NENABLED(ITEM_FNADE)
		return ;

	if(!attacker->items[ITEM_FNADE].level)
		return ;

	IF_BOT_NENABLED(attacker)
		return ;

	if(attacker->css.team == victim->css.team)
		return ;

	duration = (float)(attacker->items[ITEM_FNADE].level*FNADE_INC);
	CBaseAnimating_Ignite(dynamic_cast<CBaseAnimating*>(victim->cbp()), duration);

	return ;
}
