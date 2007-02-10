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

#ifndef RPGI_FPISTOL_H
#define RPGI_FPISTOL_H

#define FPISTOL_GLOCK	0.45
#define FPISTOL_USP		0.45
#define FPISTOL_P228	0.3
#define FPISTOL_DEAGLE	0.50
#define FPISTOL_ELITE	0.8
#define FPISTOL_FSEVEN	0.3

#define FPISTOL_INC 0.1 /* FrostPistol speed time increase for each level */

class CRPGI_FPistol: public CRPG_StaticLinkedList<CRPGI_FPistol> {
	/* Private Variables */
	float end_tm;
	float last_speed;
	int v_index;

	friend class CRPGI_Impulse;

public:
	/* Public Functions */
	static void Init(void);
	static void ShutDown(void);
	static bool BuyItem(class CRPG_Player *player);
	static bool SellItem(class CRPG_Player *player);

	static void GameFrame(void);
	static void PlayerDamage(CRPG_Player *attacker, CRPG_Player *victim, char *weapon);
};

#endif
