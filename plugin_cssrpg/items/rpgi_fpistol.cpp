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
#include "rpgi_fpistol.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PISTOL_COUNT 6

struct {
	char *name;
	float speed;
} pistols[PISTOL_COUNT] = {
	{"glock", FPISTOL_GLOCK},
	{"usp", FPISTOL_USP},
	{"p228", FPISTOL_P228},
	{"deagle", FPISTOL_DEAGLE},
	{"elite", FPISTOL_ELITE},
	{"fiveseven", FPISTOL_FSEVEN}
};

template class CRPG_LinkedList<CRPGI_FPistol>;
template<> CRPGI_FPistol* CRPG_LinkedList<CRPGI_FPistol>::ll_first;
template<> CRPGI_FPistol* CRPG_LinkedList<CRPGI_FPistol>::ll_last;
template<> unsigned int CRPG_LinkedList<CRPGI_FPistol>::ll_count;

void CRPGI_FPistol::Init(void) {
	return ;
}

void CRPGI_FPistol::ShutDown(void) {
	CRPGI_FPistol *fp, *next;

	for(fp = ll_first;fp != NULL;fp = next) {
		next = fp->ll_next;
		fp->ll_del();
		delete fp;
	}

	return ;
}

void CRPGI_FPistol::BuyItem(void *ptr) {
	return ;
}

void CRPGI_FPistol::SellItem(void *ptr) {
	return ;
}

void CRPGI_FPistol::GameFrame(void) {
	register CRPG_Player *v_player;
	register CRPGI_FPistol *fp, *next;

	if(!ll_count)
		return ;

	for(fp = ll_first;fp != NULL;fp = next) {
		next = fp->ll_next;
		if(CRPG::s_globals()->curtime >= fp->end_tm) {
			v_player = IndextoRPGPlayer(fp->v_index);
			if(v_player != NULL) {
				CRPG::SetCheats(1);
				CRPG::s_helpers()->ClientCommand(v_player->e(), "ent_fire player_speedmod ModifySpeed 1.0\n");
				CRPG::SetCheats(0);
				fp->ll_del();
				delete fp;
			}
			else {
				fp->ll_del();
				delete fp;
			}
		}
	}

	return ;
}

void CRPGI_FPistol::PlayerDamage(CRPG_Player *attacker, CRPG_Player *victim, char *weapon) {
	CRPGI_FPistol *fp = NULL;
	unsigned int i = PISTOL_COUNT;
	char entfire_str[64];

	WARN_IF((attacker == NULL) || (victim == NULL), return)

	IF_ITEM_NENABLED(ITEM_FPISTOL)
		return ;

	IF_BOT_NENABLED(attacker)
		return ;

	if(!attacker->items[ITEM_FPISTOL].level)
		return ;

	while(i--) {
		if(CRPG::istrcmp(weapon, pistols[i].name)) {
			for(fp = ll_first;fp != NULL;fp = fp->ll_next) {
				if(fp->v_index == victim->index)
					break;
			}

			if(fp == NULL) {
				fp = new CRPGI_FPistol;
				fp->v_index = victim->index;
				fp->last_speed = pistols[i].speed;
				fp->ll_add();

				Q_snprintf(entfire_str, 64, "ent_fire player_speedmod ModifySpeed %f\n", fp->last_speed);
				CRPG::SetCheats(1);
				CRPG::s_helpers()->ClientCommand(victim->e(), "give player_speedmod\n");
				CRPG::s_helpers()->ClientCommand(victim->e(), entfire_str);
				CRPG::SetCheats(0);
			}
			else {
				if(fp->last_speed > pistols[i].speed) {
					fp->last_speed = pistols[i].speed;
					Q_snprintf(entfire_str, 64, "ent_fire player_speedmod ModifySpeed %f\n", fp->last_speed);
					CRPG::SetCheats(1);
					CRPG::s_helpers()->ClientCommand(victim->e(), entfire_str);
					CRPG::SetCheats(0);
				}
			}
			fp->end_tm = CRPG::s_globals()->curtime+((float)attacker->items[ITEM_FPISTOL].level*(float)FPISTOL_INC);
		}
	}

	return ;
}
