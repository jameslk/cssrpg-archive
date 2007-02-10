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

#ifndef RPGI_IMPULSE_H
#define RPGI_IMPULSE_H

/**
 * @brief Speed increase for each level when player is damaged.
 */
#define IMPULSE_INC 0.2

/**
 * @brief Duration of Impulse's effect.
 */
#define IMPULSE_DURATION 0.8

/**
 * @brief Impulse Upgrade
 */
class CRPGI_Impulse: public CRPG_StaticLinkedList<CRPGI_Impulse> {
	/* Private Variables */
	float end_tm;
	int v_index; //victim's index
	
	static int *inv_entindex; //array of invisible entities

	/* Private Functions */
	CBaseEntity* new_inv_entity(const class Vector &origin);

	/* Friend Classes */
	friend class CRPGI_FPistol;

public:
	/* Public Functions */
	CRPGI_Impulse(): end_tm(0), v_index(0) {}

	static void Init(void);
	static void ShutDown(void);
	static bool BuyItem(class CRPG_Player *player);
	static bool SellItem(class CRPG_Player *player);

	static void RoundStart(void);
	static void GameFrame(void);
	static void PlayerDamage(CRPG_Player *attacker, CRPG_Player *victim, char *weapon);
};

#endif
