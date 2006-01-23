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

void CRPGI_Vamp::BuyItem(void *ptr) {
	return ;
}

void CRPGI_Vamp::SellItem(void *ptr) {
	return ;
}

void CRPGI_Vamp::add_health(CRPG_Player *player, unsigned int hp) {
	CRPGI_HBonus *hb;
	unsigned int new_health;

	if(player == NULL)
		return ;

	hb = IndextoHBonus(player->index);
	if((unsigned int)player->cbp()->GetHealth() >= hb->health)
		return ;

	new_health = player->cbp()->GetHealth()+hp;
	if(new_health <= hb->health)
		player->cbp()->SetHealth(new_health);
	else
		player->cbp()->SetHealth(hb->health);

	return ;
}

void CRPGI_Vamp::PlayerDamage(int attacker, int dmg_health, int dmg_armor) {
	unsigned int total_dmg = (unsigned int)(dmg_health+dmg_armor);
	float inc;
	CRPG_Player *player;

	IF_ITEM_NENABLED(ITEM_VAMP)
		return ;

	player = UserIDtoRPGPlayer(attacker);
	if(player == NULL)
		return ;

	if(!player->items[ITEM_VAMP].level)
		return ;

	if((player->isfake()) && (!CRPG_GlobalSettings::bot_enable))
		return ;

	inc = (float)player->items[ITEM_VAMP].level*VAMP_INC;
	add_health(player, floor(((float)total_dmg*inc)+0.5f));

	return ;
}
