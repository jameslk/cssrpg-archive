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
#include "beam_flags.h"

#include "../cssrpg.h"
#include "../cssrpg_interface.h"
#include "../cssrpg_hacks.h"
#include "../MRecipientFilter.h"
#include "rpgi.h"
#include "rpgi_hbonus.h"
#include "rpgi_medic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern int beamring_sprite;

CRPG_Timer* CRPGI_Medic::medic_timer;
Vector* CRPGI_Medic::vec_array = NULL;

/**
 * @brief 
 */
void CRPGI_Medic::Init(void) {
	vec_array = new Vector[CRPG::maxClients()];

	IF_ITEM_ENABLED(ITEM_MEDIC)
		medic_timer = CRPG_Timer::AddTimer(MEDIC_DELAY, 0, CRPGI_Medic::MedicTimer, 0);
	else
		medic_timer = NULL;

	return ;
}

/**
 * @brief 
 */
void CRPGI_Medic::ShutDown(void) {
	if(vec_array != NULL) {
		delete[] vec_array;
		vec_array = NULL;
	}

	if(medic_timer != NULL) {
		medic_timer->DelTimer();
		medic_timer = NULL;
	}

	return ;
}

/**
 * @brief 
 */
bool CRPGI_Medic::BuyItem(CRPG_Player *player) {
	return true;
}

/**
 * @brief 
 */
bool CRPGI_Medic::SellItem(CRPG_Player *player) {
	return true;
}

/**
 * @brief Checks the distance of each player from a medic and assigns health
 *        to them accordingly.
 */
TIMER_FUNC(CRPGI_Medic::MedicTimer) {
	register unsigned int i = CRPG_Player::player_count, ii;
	unsigned int set_hp, set_armor;
	Vector ring_origin;
	CRPG_Player *medic, *player;
	MRecipientFilter *filter, filter_t, filter_ct;

	IF_ITEM_NENABLED(ITEM_MEDIC)
		return ;

	if(!i)
		return ;

	/* Buffer each player's origin into a Vector array */
	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			s_serverclients->ClientEarPosition(CRPG_Player::players[i]->e(), &vec_array[i]);
		}
	}

	filter_t.AddTeam(team_t);
	filter_t.AddTeam(team_none);

	filter_ct.AddTeam(team_ct);
	filter_ct.AddTeam(team_none);

	i = CRPG_Player::player_count;
	while(i--) {
		/* If player is a medic and player is not dead */
		if((CRPG_Player::players[i] != NULL)
			&& !CRPG_Player::players[i]->css.isdead
			&& CRPG_Player::players[i]->items[ITEM_MEDIC].level) {

			IF_BOT_NENABLED(CRPG_Player::players[i])
				continue;

			/* Medic found, now search for teammates */
			medic = CRPG_Player::players[i];
			ii = CRPG_Player::player_count;

			while(ii--) {
				/* If player is on the same team as medic, player is not dead,
				   and player is not the medic */
				if((CRPG_Player::players[ii] != NULL)
					&& (CRPG_Player::players[ii]->css.team == medic->css.team)
					&& !CRPG_Player::players[ii]->css.isdead
					&& (CRPG_Player::players[ii]->index != medic->index)) {

					IF_BOT_NENABLED(CRPG_Player::players[ii])
						continue;
					
					/* A suitable player has been found */
					player = CRPG_Player::players[ii];

					/* Check if player is in the medic's radius */
					if(vec_array[i].DistTo(vec_array[ii]) <= MEDIC_RADIUS) {
						#pragma message("NOTICE: Offset")
						set_hp = CBaseEntity_GetHealth(player->cbp());
						set_armor = CBasePlayer_GetArmor(player->cbp());

						/* If player is not at maximum health, heal him */
						if(set_hp < CRPGI_HBonus::GetMaxHealth(player)) {
							set_hp += medic->items[ITEM_MEDIC].level*MEDIC_INC;

							if(set_hp >= CRPGI_HBonus::GetMaxHealth(player))
								set_hp = CRPGI_HBonus::GetMaxHealth(player);

							#pragma message("NOTICE: Offset")
							CBaseEntity_SetHealth(player->cbp(), set_hp);

							/* Only teammates should see this ring */
							if(medic->css.team == team_t)
								filter = &filter_t;
							else
								filter = &filter_ct;

							ring_origin = vec_array[i];
							ring_origin.z -= 25;

							tempents->BeamRingPoint(*filter,
								0.0, //delay
								ring_origin, //origin
								8.0, //start radius
								(float)MEDIC_RADIUS+300.0, //end radius
								beamring_sprite, //texture
								beamring_sprite, //halo index
								0, //start frame
								1, //framerate
								1.5, //life
								10, //width
								0, //spread
								0, //amplitude
								5, 45, 255, 50, //rgba
								0, //speed
								FBEAM_FADEOUT);

							CRPG::EmitSound(0, "weapons/physcannon/physcannon_charge.wav", 0.2, player);
						}
						else if(set_armor < 100) {
							/* Else if player is not at maximum armor, repair him */

							if((medic->items[ITEM_MEDIC].level*MEDIC_INC) > 25)
								set_armor += 25;
							else
								set_armor += medic->items[ITEM_MEDIC].level*MEDIC_INC;

							if(set_armor >= 100)
								set_armor = 100;

							CBasePlayer_SetArmor(player->cbp(), set_armor);

							/* Only teammates should see this ring */
							if(medic->css.team == team_t)
								filter = &filter_t;
							else
								filter = &filter_ct;

							ring_origin = vec_array[i];
							ring_origin.z -= 25;

							tempents->BeamRingPoint(*filter,
								0.0, //delay
								ring_origin, //origin
								8.0, //start radius
								(float)MEDIC_RADIUS+300.0, //end radius
								beamring_sprite, //texture
								beamring_sprite, //halo index
								0, //start frame
								1, //framerate
								1.5, //life
								10, //width
								0, //spread
								0, //amplitude
								5, 255, 10, 50, //rgba
								0, //speed
								FBEAM_FADEOUT);

							CRPG::EmitSound(0, "weapons/physcannon/physcannon_charge.wav", 0.2, player);
						}
					}
				}
			} //weeeeeee!
		}
	}

	return ;
}
