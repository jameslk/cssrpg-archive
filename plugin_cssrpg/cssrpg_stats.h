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

#ifndef CSSRPG_STATS_H
#define CSSRPG_STATS_H

class CRPG_StatsManager: private CRPG_GlobalSettings {
private:
	/* Private Functions */
	/* Calculate how many levels to increase by experience and current level */
	static unsigned int calc_lvl_inc(unsigned int lvl, unsigned int exp);

public:
	/* Public Functions */
	/* Calculate the experience needed for this level */
	static void player_new_lvl(CRPG_Player *player, unsigned int lvl_inc);
	static void add_exp(CRPG_Player *player, unsigned long exp);

	static unsigned int LvltoExp(unsigned int lvl);

	static void PlayerDamage(int attacker, int dmg_health, int dmg_armor);
	static void PlayerKill(int attacker, int victim, bool headshot);
};

class CRPG_RankManager {
public:
	static unsigned int GetPlayerRank(CRPG_Player *player);
	static unsigned int GetRankCount(void);
	//static unsigned int GetTopNPlayers(char ***players, unsigned int n);
};

#endif
