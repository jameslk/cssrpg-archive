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

#ifndef RPGI_HBONUS_H
#define RPGI_HBONUS_H

/* Health max increase for each level */
#define HBONUS_INC 25

#include "../cssrpg_misc.h"
class CRPGI_HBonus: public CRPG_PlayerClass<CRPGI_HBonus> {
public:
	/* Public Variables */
	static CRPGI_HBonus **health_array;
	static unsigned int health_count;

	unsigned int health;

	/* Public Functions */
	CRPGI_HBonus(): health(100) {
		index = 0;
		userid = 0;
	}

	static void Init(void);
	static void ShutDown(void);
	static void BuyItem(void *ptr);
	static void SellItem(void *ptr);
	static void PlayerUpdate(CRPG_Player *player);

	static void AddPlayer(edict_t *e);
	void DelPlayer(void);
	static void SetSpawnHealth(CRPG_Player *player);
};

CRPGI_HBonus* IndextoHBonus(int index);
CRPGI_HBonus* EdicttoHBonus(edict_t *e);

#endif