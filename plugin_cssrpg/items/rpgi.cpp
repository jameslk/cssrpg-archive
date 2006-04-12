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

#include "rpgi_hbonus.h"
#include "rpgi_regen.h"
#include "rpgi_resup.h"
#include "rpgi_ljump.h"
#include "rpgi_stealth.h"
#include "rpgi_icestab.h"
#include "rpgi.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CRPGI::Init(void) {
	CRPGI_HBonus::Init();
	CRPGI_Regen::Init();
	CRPGI_Resup::Init();
	CRPGI_Stealth::Init();
	CRPGI_IceStab::Init();

	return ;
}

void CRPGI::ShutDown(void) {
	CRPGI_Regen::ShutDown();
	CRPGI_HBonus::ShutDown();
	CRPGI_Resup::ShutDown();
	CRPGI_LJump::ShutDown();
	CRPGI_Stealth::ShutDown();

	return ;
}

#define PREFIX_LEN 7 // length of "cssrpg_"
VAR_FUNC(CRPGI::CVARItemMaxLvl) {
	unsigned int lvl = abs(atoi(str)), i = ITEM_COUNT, index;
	struct item_type *type = NULL;
	char *name = setting->name+PREFIX_LEN;

	if(!lvl) {
		setting->var->SetValue(atoi(setting->defaultval));
		return ;
	}

	while(i--) {
		if(!memcmp(name, CRPG::item_types[i].shortname, strlen(CRPG::item_types[i].shortname))) {
			index = i;
			type = &CRPG::item_types[i];
		}
	}

	if(type == NULL) {
		setting->var->SetValue(atoi(setting->defaultval));
		return ;
	}

	if(lvl > type->maxlevelbarrier) {
		type->maxlevel = type->maxlevelbarrier;
		setting->var->SetValue((int)type->maxlevelbarrier);
	}
	else {
		type->maxlevel = lvl;
		setting->var->SetValue((int)lvl);
	}

	i = CRPG_Player::player_count;
	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			while(CRPG_Player::players[i]->items[index].level > type->maxlevel) {
				/* Give player their credit's back */
				CRPG_Player::players[i]->credits += CRPGI::GetItemCost(index, CRPG_Player::players[i]->items[index].level);
				CRPG_Player::players[i]->TakeItem(index);
			}
		}
	}

	return ;
}

void CRPGI::PlayerUpdate(CRPG_Player *player) {
	WARN_IF(player == NULL, return)

	CRPGI_HBonus::PlayerUpdate(player);

	return ;
}

unsigned int CRPGI::GetItemCost(unsigned int item_index, unsigned int lvl) {
	struct item_type *item;

	item = &CRPG::item_types[item_index];
	
	if(lvl <= 1)
		return item->start_cost;
	else
		return (item->inc_cost*(lvl-1))+item->start_cost;
}

unsigned int CRPGI::GetItemSale(unsigned int item_index, unsigned int lvl) {
	unsigned int cost, sale;
	struct item_type *item;

	item = &CRPG::item_types[item_index];
	cost = (item->inc_cost*(lvl-1))+item->start_cost;

	if(sale_percent == 1.0)
		return cost;
	
	if(lvl <= 1)
		return item->start_cost;

	sale = (item->inc_cost*(lvl-1))+item->start_cost;
	sale = (unsigned int)floor(((float)sale*(sale_percent > 1.0 ? (sale_percent/100.0) : sale_percent))+0.5);

	if(credits_inc <= 1)
		return sale;
	else
		sale = (sale + (unsigned int)floor((float)credits_inc/2.0)) / credits_inc * credits_inc;

	if(sale > cost)
		return cost;

	return sale;
}
