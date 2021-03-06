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
#include "rpgi_ljump.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

template class CRPG_StaticLinkedList<CRPGI_LJump>;
template<> CRPGI_LJump* CRPG_StaticLinkedList<CRPGI_LJump>::ll_first;
template<> CRPGI_LJump* CRPG_StaticLinkedList<CRPGI_LJump>::ll_last;
template<> unsigned int CRPG_StaticLinkedList<CRPGI_LJump>::ll_count;

void CRPGI_LJump::Init(void) {
	ll_init();
	return ;
}

void CRPGI_LJump::ShutDown(void) {
	CRPGI_LJump *jump, *next;

	for(jump = ll_first;jump != NULL;jump = next) {
		next = jump->ll_next;
		jump->ll_del();
		delete jump;
	}

	return ;
}

bool CRPGI_LJump::BuyItem(CRPG_Player *player) {
	return true;
}

bool CRPGI_LJump::SellItem(CRPG_Player *player) {
	return true;
}

void CRPGI_LJump::PlayerJump(CRPG_Player *player) {
	CRPGI_LJump *jump;

	WARN_IF(player == NULL, return)

	jump = new CRPGI_LJump;
	jump->index = player->index;
	jump->userid = player->userid;
	#pragma message("NOTICE: Implement offset here")
	jump->prevel = player->cbp()->m_vecVelocity.Get(); //here

	jump->ll_add();
	return ;
}

void CRPGI_LJump::PlayerFootStep(int userid) {
	register CRPGI_LJump *jump;

	if(!ll_count)
		return ;

	for(jump = ll_first;jump != NULL;jump = jump->ll_next) {
		if(jump->userid == userid) { /* Player tried to sneak out some jump juice! */
			jump->ll_del();
			delete jump;
			break;
		}
	}

	return ;
}

void CRPGI_LJump::CheckAll(void) {
	register Vector vel;
	register CRPG_Player *player;
	register CRPGI_LJump *jump, *next;

	if(!ll_count)
		return ;

	for(jump = ll_first;jump != NULL;jump = next) {
		next = jump->ll_next;
		player = IndextoRPGPlayer(jump->index);
		if(player != NULL) {
			#pragma message("NOTICE: Implement offset here")
			vel = player->cbp()->m_vecVelocity.Get(); //here
			if(vel.z > jump->prevel.z) {
				jump->has_jumped(player, &vel);
				jump->ll_del();
				delete jump;
			}
		}
		else {
			jump->ll_del();
			delete jump;
		}
	}

	return ;
}

void CRPGI_LJump::has_jumped(CRPG_Player *player, Vector *vect) {
	float inc;

	IF_ITEM_NENABLED(ITEM_LJUMP)
		return ;

	if(!player->items[ITEM_LJUMP].level)
		return ;

	IF_BOT_NENABLED(player)
		return ;

	inc = ((LJUMP_INC*(float)player->items[ITEM_LJUMP].level)+1.0);
	vect->x *= inc;
	vect->y *= inc;

	CBaseEntity_Teleport((CBaseEntity*)player->cbp(), NULL, NULL, vect);
	return ;
}
