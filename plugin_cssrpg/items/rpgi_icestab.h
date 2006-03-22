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

#ifndef RPGI_ICESTAB_H
#define RPGI_ICESTAB_H

#define ICESTAB_INC 1.0 /* IceStab freeze duration increase for each level */
#define ICESTAB_DMG_MIN 50 /* Secondary knife attack is 50+ damage */
#define ICESTAB_CLRFADE 1 /* Blue color fade amount for each frame */

#include "../cssrpg_misc.h"
class CRPGI_IceStab: public CRPG_LinkedList<CRPGI_IceStab> {
private:
	/* Private Variables */
	int v_index;
	float duration;
	int fade;

public:
	/* Public Functions */
	static void Init(void);
	static void ShutDown(void);
	static void BuyItem(void *ptr);
	static void SellItem(void *ptr);

	static void GameFrame(void);
	static void PlayerDamage(CRPG_Player *attacker, CRPG_Player *victim, int dmg_health, int dmg_armor);
};

#endif
