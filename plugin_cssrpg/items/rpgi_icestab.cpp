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
#include "rpgi_stealth.h"
#include "rpgi_icestab.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

template class CRPG_LinkedList<CRPGI_IceStab>;
template<> CRPGI_IceStab* CRPG_LinkedList<CRPGI_IceStab>::ll_first;
template<> CRPGI_IceStab* CRPG_LinkedList<CRPGI_IceStab>::ll_last;
template<> unsigned int CRPG_LinkedList<CRPGI_IceStab>::ll_count;

void CRPGI_IceStab::Init(void) {
	return ;
}

void CRPGI_IceStab::ShutDown(void) {
	CRPGI_IceStab *stab, *next;

	for(stab = ll_first;stab != NULL;stab = next) {
		next = stab->ll_next;
		stab->ll_del();
		delete stab;
	}

	return ;
}

void CRPGI_IceStab::BuyItem(void *ptr) {
	return ;
}

void CRPGI_IceStab::SellItem(void *ptr) {
	return ;
}

void CRPGI_IceStab::GameFrame(void) {
	register CRPG_Player *v_player;
	register CRPGI_IceStab *stab, *next;

	if(!ll_count)
		return ;

	for(stab = ll_first;stab != NULL;stab = next) {
		next = stab->ll_next;
		v_player = IndextoRPGPlayer(stab->v_index);
		if(v_player != NULL) {
			if((stab->duration > 0) && (stab->duration < CRPG::s_globals()->curtime)) {
				CBaseEntity_SetMoveType((CBaseEntity*)v_player->cbp(), MOVETYPE_WALK, MOVECOLLIDE_DEFAULT);
				stab->duration = 0;
			}

			if(stab->fade < 255) {
				stab->fade += ICESTAB_CLRFADE;
				v_player->cbp()->SetRenderMode(kRenderTransColor);
				v_player->cbp()->SetRenderColor(stab->fade, stab->fade, 255);
			}

			if(!stab->duration && (stab->fade >= 255)) {
				stab->ll_del();
				delete stab;
			}
		}
		else {
			stab->ll_del();
			delete stab;
		}
	}

	return ;
}

void CRPGI_IceStab::PlayerDamage(int attacker, int victim, int dmg_health, int dmg_armor) {
	CRPG_Player *a_player, *v_player;
	CRPGI_IceStab *stab;

	if((dmg_health+dmg_armor) < ICESTAB_DMG_MIN)
		return ;

	IF_ITEM_NENABLED(ITEM_ICESTAB)
		return ;

	a_player = UserIDtoRPGPlayer(attacker);
	if(a_player == NULL)
		return ;

	if(!a_player->items[ITEM_ICESTAB].level)
		return ;

	IF_BOT_NENABLED(a_player)
		return ;

	v_player = UserIDtoRPGPlayer(victim);
	if(v_player == NULL)
		return ;

	stab = new CRPGI_IceStab;
	stab->v_index = v_player->index;
	stab->duration = CRPG::s_globals()->curtime+(ICESTAB_INC*a_player->items[ITEM_ICESTAB].level);
	stab->fade = 0;
	stab->ll_add();

	CBaseEntity_SetMoveType((CBaseEntity*)v_player->cbp(), MOVETYPE_NONE, MOVECOLLIDE_DEFAULT);
	v_player->cbp()->SetRenderMode(kRenderTransColor);
	v_player->cbp()->SetRenderColor(0, 0, 255);

	return ;
}
