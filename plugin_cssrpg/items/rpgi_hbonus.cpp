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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CRPGI_HBonus::Init(void) {
	return ;
}

void CRPGI_HBonus::ShutDown(void) {
	return ;
}

bool CRPGI_HBonus::BuyItem(CRPG_Player *player) {
	unsigned int health;
	#pragma message("NOTICE: Offset")

	IF_ITEM_NENABLED(ITEM_HBONUS)
		return false;

	health = CBaseEntity_GetHealth(player->cbp());
	#pragma message("NOTICE: Offset")
	if(health >= (100 + HBONUS_INC*(player->items[ITEM_HBONUS].level-1)))
		CBaseEntity_SetHealth(player->cbp(), GetMaxHealth(player));

	return true;
}

bool CRPGI_HBonus::SellItem(CRPG_Player *player) {
	IF_ITEM_NENABLED(ITEM_HBONUS)
		return false;

	#pragma message("NOTICE: Offset")
	if(CBaseEntity_GetHealth(player->cbp()) > GetMaxHealth(player))
		CBaseEntity_SetHealth(player->cbp(), GetMaxHealth(player));

	return true;
}

void CRPGI_HBonus::SetSpawnHealth(CRPG_Player *player) {
	WARN_IF(player == NULL, return)

	IF_ITEM_NENABLED(ITEM_HBONUS)
		return ;

	IF_BOT_NENABLED(player)
		return ;

	#pragma message("NOTICE: Offset")
	CBaseEntity_SetHealth(player->cbp(), GetMaxHealth(player));

	return ;
}
