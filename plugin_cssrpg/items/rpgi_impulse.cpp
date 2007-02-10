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

#include "itempents.h"
#include "const.h"

#include "../cssrpg.h"
#include "../cssrpg_hacks.h"
#include "../MRecipientFilter.h"
#include "rpgi.h"
#include "rpgi_fpistol.h"
#include "rpgi_impulse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

template class CRPG_StaticLinkedList<CRPGI_Impulse>;
template<> CRPGI_Impulse* CRPG_StaticLinkedList<CRPGI_Impulse>::ll_first;
template<> CRPGI_Impulse* CRPG_StaticLinkedList<CRPGI_Impulse>::ll_last;
template<> unsigned int CRPG_StaticLinkedList<CRPGI_Impulse>::ll_count;
int *CRPGI_Impulse::inv_entindex = NULL;

extern int redtrail_sprite;

/**
 * @brief Creates a new invisible entity.
 */
CBaseEntity* CRPGI_Impulse::new_inv_entity(const Vector &origin) {
	CBaseEntity *invisible;

	inv_entindex[v_index] = -1;

	invisible = CBaseEntity_CreateNoSpawn("env_sprite", origin, QAngle(0, 0, 0), NULL);
	if(invisible != NULL) {
		CBaseEntity_SetRenderMode(invisible, kRenderNone);

		if(!DispatchSpawn(invisible))
			inv_entindex[v_index] = CRPG::EdicttoIndex(CRPG::BaseEntitytoEdict(invisible));
	}

	if(inv_entindex[v_index] == -1)
		return NULL;
	
	return invisible;
}

/**
 * @brief 
 */
void CRPGI_Impulse::Init(void) {
	unsigned int i = CRPG::maxClients();

	ll_init();

	inv_entindex = (int*)malloc(i*sizeof(int));
	while(i--)
		inv_entindex[i] = -1;

	return ;
}

/**
 * @brief 
 */
void CRPGI_Impulse::ShutDown(void) {
	CRPGI_Impulse *impulse, *next;

	for(impulse = ll_first;impulse != NULL;impulse = next) {
		next = impulse->ll_next;
		impulse->ll_del();
		delete impulse;
	}

	if(inv_entindex != NULL)
		free(inv_entindex);

	return ;
}

/**
 * @brief 
 */
bool CRPGI_Impulse::BuyItem(CRPG_Player *player) {
	return true;
}

/**
 * @brief 
 */
bool CRPGI_Impulse::SellItem(CRPG_Player *player) {
	return true;
}

/**
 * @brief Reset all invisible entity indexes since the previous round's entities
 *        were all deleted on round start.
 */
void CRPGI_Impulse::RoundStart(void) {
	unsigned int i = CRPG::maxClients();

	while(i--)
		inv_entindex[i] = -1;

	return ;
}

/**
 * @brief 
 */
void CRPGI_Impulse::GameFrame(void) {
	register CRPGI_Impulse *impulse, *next;
	CRPG_Player *v_player;
	CBaseEntity *invisible;

	if(!ll_count)
		return ;

	IF_ITEM_NENABLED(ITEM_IMPULSE)
		return ;

	for(impulse = ll_first;impulse != NULL;impulse = next) {
		next = impulse->ll_next;
		if(s_globals->curtime > impulse->end_tm) {
			v_player = IndextoRPGPlayer(impulse->v_index);
			if(v_player != NULL) {
				CRPG::SetCheats(1);
				s_helpers->ClientCommand(v_player->e(), "ent_fire player_speedmod ModifySpeed 1.0\n");
				CRPG::SetCheats(0);
				#pragma message("NOTICE: Offset")

				invisible = CRPG::EdicttoBaseEntity(CRPG::IndextoEdict(inv_entindex[impulse->v_index]));
				WARN_IF(invisible == NULL, impulse->ll_del(); delete impulse; continue);

				CBaseEntity_SetParent(invisible, NULL); //unset parent
			}

			impulse->ll_del();
			delete impulse;
		}
	}

	return ;
}

/**
 * @brief 
 */
void CRPGI_Impulse::PlayerDamage(CRPG_Player *attacker, CRPG_Player *victim, char *weapon) {
	CRPGI_FPistol *fp;
	CRPGI_Impulse *impulse;
	char entfire_str[64];
	MRecipientFilter filter;
	CBaseEntity *invisible;
	Vector vec_origin;

	WARN_IF((attacker == NULL) || (victim == NULL), return);

	IF_ITEM_NENABLED(ITEM_IMPULSE)
		return ;

	if(!victim->items[ITEM_IMPULSE].level)
		return ;

	for(fp = CRPGI_FPistol::ll_first;fp != NULL;fp = fp->ll_next) {
		if(fp->v_index == victim->index)
			return ; //Player has already been tagged with a FrostPistol
	}

	if(!(CBasePlayer_GetFlags(victim->cbp()) & FL_ONGROUND))
		return ; //Player is in midair

	if(attacker->css.team == victim->css.team)
		return ;

	IF_BOT_NENABLED(victim)
		return ;

	for(impulse = CRPGI_Impulse::ll_first;impulse != NULL;impulse = impulse->ll_next) {
		if(impulse->v_index == victim->index) {
			return ;
		}
	}

	impulse = new CRPGI_Impulse;
	impulse->v_index = victim->index;
	impulse->ll_add();

	/* Set player speed */
	CRPG::snprintf(entfire_str, 64, "ent_fire player_speedmod ModifySpeed %f\n", 1.0+((float)victim->items[ITEM_IMPULSE].level*(float)IMPULSE_INC));
	CRPG::SetCheats(1);
	s_helpers->ClientCommand(victim->e(), "give player_speedmod\n");
	s_helpers->ClientCommand(victim->e(), entfire_str);
	CRPG::SetCheats(0);

	impulse->end_tm = s_globals->curtime+(float)IMPULSE_DURATION;

	s_serverclients->ClientEarPosition(victim->e(), &vec_origin);
	vec_origin.z -= 40;

	if(inv_entindex[impulse->v_index] != -1) {
		invisible = CRPG::EdicttoBaseEntity(CRPG::IndextoEdict(inv_entindex[impulse->v_index]));
		if(invisible == NULL) { //this shouldn't happen but it may
			invisible = impulse->new_inv_entity(vec_origin);
			WARN_IF(invisible == NULL, return);
		}
	}
	else {
		invisible = impulse->new_inv_entity(vec_origin);
		WARN_IF(invisible == NULL, return);
	}

	CBaseEntity_SetParent(invisible, NULL); //Make sure this is unset before teleporting
	CBaseEntity_Teleport(invisible, &vec_origin, NULL, NULL);
	CBaseEntity_SetParent(invisible, victim->cbp());

	filter.AddAllPlayers();
	tempents->BeamFollow(filter,
		0.0, //delay
		inv_entindex[impulse->v_index], //entindex
		redtrail_sprite, //model
		redtrail_sprite, //halo
		(float)IMPULSE_DURATION, //life
		10.0, //width
		4.0, //end width
		2.0, //fade length
		255, 0, 0, 255); //rgba

	return ;
}
