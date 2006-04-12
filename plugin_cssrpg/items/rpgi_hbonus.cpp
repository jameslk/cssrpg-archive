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
#include "rpgi.h"
#include "rpgi_hbonus.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

template class CRPG_PlayerClass<CRPGI_HBonus>;
template<> CRPGI_HBonus** CRPG_PlayerClass<CRPGI_HBonus>::nodes;
CRPGI_HBonus** CRPGI_HBonus::health_array;
unsigned int CRPGI_HBonus::health_count;

void CRPGI_HBonus::Init(void) {
	unsigned int i;
	health_count = (unsigned int)CRPG::maxClients();
	i = health_count;

	health_array = new CRPGI_HBonus *[health_count];
	while(i--)
		health_array[i] = NULL;

	nodes = health_array;

	return ;
}

void CRPGI_HBonus::ShutDown(void) {
	free_nodes(health_count);

	return ;
}

void CRPGI_HBonus::BuyItem(void *ptr) {
	CRPG_Player *player = (CRPG_Player*)ptr;
	CRPGI_HBonus *hb;

	IF_ITEM_NENABLED(ITEM_HBONUS)
		return ;

	hb = IndextoHBonus(player->index);
	WARN_IF(hb == NULL, return)

	hb->health = 100+(HBONUS_INC*player->items[ITEM_HBONUS].level);

	return ;
}

void CRPGI_HBonus::SellItem(void *ptr) {
	CRPG_Player *player = (CRPG_Player*)ptr;
	CRPGI_HBonus *hb;

	IF_ITEM_NENABLED(ITEM_HBONUS)
		return ;

	hb = IndextoHBonus(player->index);
	WARN_IF(hb == NULL, return)

	hb->health = 100+(HBONUS_INC*player->items[ITEM_HBONUS].level);
	if(player->cbp()->GetHealth() > (int)hb->health)
		player->cbp()->SetHealth(hb->health);

	return ;
}

void CRPGI_HBonus::PlayerUpdate(CRPG_Player *player) {
	CRPGI_HBonus *hb;

	hb = IndextoHBonus(player->index);
	WARN_IF(hb == NULL, return)

	hb->health = 100+(HBONUS_INC*player->items[ITEM_HBONUS].level);
	player->cbp()->SetHealth(hb->health);

	return ;
}

CRPGI_HBonus* IndextoHBonus(int index) {
	return CRPGI_HBonus::IndextoHandle(index);
}

CRPGI_HBonus* EdicttoHBonus(edict_t *e) {
	return CRPGI_HBonus::EdicttoHandle(e);
}

void CRPGI_HBonus::AddPlayer(edict_t *e) {
	CRPGI_HBonus *hb;
	CRPG_Player *player;

	hb = new_node(e);
	if((player = IndextoRPGPlayer(hb->index)) != NULL)
		hb->health = 100+(HBONUS_INC*player->items[ITEM_HBONUS].level);
	else
		hb->health = 100;

	return ;
}

void CRPGI_HBonus::DelPlayer(void) {
	del_node(this);
	return ;
}

void CRPGI_HBonus::SetSpawnHealth(CRPG_Player *player) {
	CRPGI_HBonus *hb;

	WARN_IF(player == NULL, return)

	IF_ITEM_NENABLED(ITEM_HBONUS)
		return ;

	IF_BOT_NENABLED(player)
		return ;

	hb = IndextoHBonus(player->index);
	WARN_IF(hb == NULL, return)

	player->cbp()->SetHealth(hb->health);

	return ;
}
