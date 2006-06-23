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
	register float now = CRPG::s_globals()->curtime;

	if(!ll_count)
		return ;

	for(stab = ll_first;stab != NULL;stab = next) {
		next = stab->ll_next;
		v_player = IndextoRPGPlayer(stab->v_index);
		if(v_player != NULL) {
			if((stab->duration > 0) && (stab->duration < now)) {
				CBaseEntity_SetMoveType((CBaseEntity*)v_player->cbp(), MOVETYPE_WALK, MOVECOLLIDE_DEFAULT);
				engine->ClientCommand(v_player->e(), "exec cssrpg_config.cfg\n"); /* Set sensitivity back */
				stab->duration = 0;
			}
			else if(stab->duration && (now >= stab->scheck_time)) {
				engine->ClientCommand(v_player->e(), "sensitivity %f\n", ICESTAB_SENSITIVITY);
				stab->scheck_time = now+0.5;
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

void CRPGI_IceStab::PlayerDamage(CRPG_Player *attacker, CRPG_Player *victim, int dmg_health, int dmg_armor) {
	CRPGI_IceStab *stab;

	if((dmg_health+dmg_armor) < ICESTAB_DMG_MIN)
		return ;

	IF_ITEM_NENABLED(ITEM_ICESTAB)
		return ;

	if(!attacker->items[ITEM_ICESTAB].level)
		return ;

	IF_BOT_NENABLED(attacker)
		return ;

	if(attacker->css.team == victim->css.team)
		return ;

	for(stab = ll_first;stab != NULL;stab = stab->ll_next) {
		if(stab->v_index == attacker->index)
			return ; /* don't allow frozen attacker to icestab */
	}

	for(stab = ll_first;stab != NULL;stab = stab->ll_next) {
		if(stab->v_index == victim->index)
			break;
	}

	if(stab == NULL) {
		stab = new CRPGI_IceStab;
		stab->v_index = victim->index;
		stab->ll_add();

		/* make sure the old sensitivity isn't overwritten */
		engine->ClientCommand(victim->e(), "host_writeconfig cssrpg_config.cfg\n");
	}

	stab->duration = CRPG::s_globals()->curtime+(ICESTAB_INC*attacker->items[ITEM_ICESTAB].level);
	stab->fade = 0;

	CBaseEntity_SetMoveType((CBaseEntity*)victim->cbp(), MOVETYPE_NONE, MOVECOLLIDE_DEFAULT);
	engine->ClientCommand(victim->e(), "sensitivity %f\n", ICESTAB_SENSITIVITY);
	stab->scheck_time = CRPG::s_globals()->curtime+0.5;

	switch(rand()%3) {
		case 0:
			CRPG::EmitSound(0, "physics/glass/glass_impact_bullet1.wav", 1.0, victim);
			break;

		case 1:
			CRPG::EmitSound(0, "physics/glass/glass_impact_bullet2.wav", 1.0, victim);
			break;

		case 2:
			CRPG::EmitSound(0, "physics/glass/glass_impact_bullet3.wav", 1.0, victim);
			break;
	}

	victim->cbp()->SetRenderMode(kRenderTransColor);
	victim->cbp()->SetRenderColor(0, 0, 255);

	return ;
}

void CRPGI_IceStab::LimitDamage(CRPG_Player *victim, int *dmg_health, char *weapon) {
	CRPGI_IceStab *stab;
	int set_hp;

	if(!ll_count)
		return ;

	if(!CRPG_GlobalSettings::icestab_lmtdmg)
		return ;

	if(CRPG::istrcmp(weapon, "knife"))
		return ;

	for(stab = ll_first;stab != NULL;stab = stab->ll_next) {
		if(stab->v_index == victim->index) {
			if(1) {//!victim->isfake()) {
				/* CRPG::ChatAreaMsg(0, "damage = %d, prevhealth = %d, health = %d, gethealth = %d",
					*dmg_health, victim->cbp()->GetHealth()+(*dmg_health), health, victim->cbp()->GetHealth()); */

				set_hp = victim->cbp()->GetHealth()+(*dmg_health); /* The real handler uses the real damage */

				/* CRPG::ChatAreaMsg(0, "set_hp = %d+%d = %d", victim->cbp()->GetHealth(), *dmg_health, set_hp); */

				*dmg_health = (((unsigned int)*dmg_health) > CRPG_GlobalSettings::icestab_lmtdmg ? (int)CRPG_GlobalSettings::icestab_lmtdmg : *dmg_health); /* new dmg */
				
				/* CRPG::ChatAreaMsg(0, "new_hp = %d-%d = %d", set_hp, *dmg_health, set_hp-(*dmg_health));
				CRPG::ChatAreaMsg(0, "------------------------------------------------------------"); */

				victim->cbp()->SetHealth(set_hp-(*dmg_health));

				switch(rand()%3) {
					case 0:
						CRPG::EmitSound(0, "physics/glass/glass_sheet_impact_hard1.wav", 1.0, victim);
						break;

					case 1:
						CRPG::EmitSound(0, "physics/glass/glass_sheet_impact_hard2.wav", 1.0, victim);
						break;

					case 2:
						CRPG::EmitSound(0, "physics/glass/glass_sheet_impact_hard3.wav", 1.0, victim);
						break;
				}
			}
		}
	}

	return ;
}
