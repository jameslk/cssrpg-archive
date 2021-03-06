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
#include <math.h>

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

#include "cssrpg.h"
#include "cssrpg_bot.h"
#include "cssrpg_database.h"
#include "cssrpg_textdb.h"
#include "cssrpg_stats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*	//////////////////////////////////////
	CRPG_StatsManager Class
	////////////////////////////////////// */
/* Calculate the experience needed for this level */
unsigned int CRPG_StatsManager::LvltoExp(unsigned int lvl) {
	unsigned int exp;

	if(lvl <= 1)
		exp =  exp_start;
	else
		exp = (lvl*exp_inc)+exp_start;

	return exp > exp_max ? exp_max : exp;
}

/* Calculate how many levels to increase by current level and experience */
unsigned int CRPG_StatsManager::calc_lvl_inc(unsigned int lvl, unsigned int exp) {
	unsigned int lvl_inc = 0, exp_req;

	while(1) {
		exp_req = LvltoExp(lvl+lvl_inc);
		if(exp > exp_req) {
			lvl_inc++;
			exp -= exp_req;
			continue;
		}
		else if(exp == exp_req) {
			lvl_inc++;
		}
		break;
	}

	return lvl_inc;
}

float CRPG_StatsManager::team_ratio(enum cssteam_t numerator) {
	float teamratio;

	if(numerator == team_none)
		return 0.0;

	if(numerator == team_t)
		teamratio = (float)CRPG_TeamBalance::teamt_count/(float)CRPG_TeamBalance::teamct_count;
	else
		teamratio = (float)CRPG_TeamBalance::teamct_count/(float)CRPG_TeamBalance::teamt_count;

	return teamratio;
}

void CRPG_StatsManager::player_new_lvl(CRPG_Player *player, unsigned int lvl_inc) {
	WARN_IF(!lvl_inc, return)

	if(player->isfake() && bot_maxlevel) {
		if((player->level+lvl_inc) > bot_maxlevel) {
			CRPG::DebugMsg("Bot %s has surpassed the maximum level of %d, resetting its stats",
				player->name(), bot_maxlevel);
			player->ResetStats();
			return ;
		}
	}

	player->level += lvl_inc;
	player->exp = 0;
	player->credits += lvl_inc*credits_inc;

	CRPG::DebugMsg(1, "%s is now level %d (%d level increase(s))", player->name(), player->level, lvl_inc);

	if(announce_newlvl)
		CRPG::ChatAreaMsg(0, TXTDB_ID(newlvl.msg1), player->name(), player->level);

	if(!player->info()->IsFakeClient()) {
		CRPG::EmitSound(player->index, "buttons/blip2.wav");
		if((player->level-lvl_inc) <= 1) {
			/* for newbies */
			CRPG::ChatAreaMsg(player->index, TXTDB(player, newbielvl.msg1));
			CRPG::ChatAreaMsg(player->index, TXTDB(player, newbielvl.msg2));
		}
		else {
			CRPG::ChatAreaMsg(player->index, TXTDB(player, newlvl.msg2), player->credits);
		}
	}
	else {
		CRPG_Bot::PickUpgrade(player);
	}

	return ;
}

void CRPG_StatsManager::add_exp(CRPG_Player *player, unsigned long exp, bool hidenotice) {
	unsigned int exp_req;

	WARN_IF(player == NULL, return)

	IF_BOT_NENABLED(player)
		return ;

	exp_req = LvltoExp(player->level);
	player->exp += exp;

	if(exp_notice && !hidenotice)
		CRPG::HintTextMsg(player->index, TXTDB(player, exphint.msg1), exp, player->exp, exp_req);

	if(player->exp >= exp_req)
		player_new_lvl(player, calc_lvl_inc(player->level, player->exp));

	return ;
}

void CRPG_StatsManager::PlayerDamage(CRPG_Player *attacker, CRPG_Player *victim, const char *weapon, int dmg_health, int dmg_armor) {
	unsigned int total_dmg = (unsigned int)(dmg_health+dmg_armor);
	unsigned int exp;

	if(!enable)
		return ;

	if(attacker->css.team == victim->css.team)
		return ;

	if(CRPG::istrcmp((char*)weapon, "knife"))
		exp = (unsigned int)ceil((float)total_dmg*(exp_knifedmg > exp_damage ? exp_knifedmg : exp_damage));
	else
		exp = (unsigned int)ceil((float)total_dmg*exp_damage);

	add_exp(attacker, exp, 1);
	return ;
}

void CRPG_StatsManager::PlayerKill(int attacker, int victim, bool headshot) {
	CRPG_Player *a_player, *v_player;
	unsigned int exp;

	if(!enable)
		return ;

	if((attacker < 1) || (victim < 1))
		return ;

	if(victim == attacker)
		return ;

	a_player = UserIDtoRPGPlayer(attacker);
	v_player = UserIDtoRPGPlayer(victim);

	WARN_IF((a_player == NULL || v_player == NULL), return)

	if(a_player->css.team == v_player->css.team)
		return ;

	exp = (unsigned int)(v_player->level*exp_kill);
	if(headshot)
		exp += exp_headshot;

	add_exp(a_player, exp);
	return ;
}

void CRPG_StatsManager::BombPlanted(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	WARN_IF(player == NULL, return)

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil((float)LvltoExp(player->level)*exp_bombplanted*teamratio));

	return ;
}

void CRPG_StatsManager::BombDefused(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	WARN_IF(player == NULL, return)

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil((float)LvltoExp(player->level)*exp_bombdefused*teamratio));

	return ;
}

void CRPG_StatsManager::BombExploded(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	WARN_IF(player == NULL, return)

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil((float)LvltoExp(player->level)*exp_bombexploded*teamratio));

	return ;
}

void CRPG_StatsManager::HostageRescued(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	WARN_IF(player == NULL, return)

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil((float)LvltoExp(player->level)*exp_hostage*teamratio));

	return ;
}

void CRPG_StatsManager::VipEscaped(int userid) {
	CRPG_Player *player = UserIDtoRPGPlayer(userid);
	float teamratio;

	if(!enable)
		return ;

	WARN_IF(player == NULL, return)

	if(player->css.team == team_none)
		return ;

	teamratio = team_ratio(player->css.team == team_t ? team_ct : team_t);
	add_exp(player, (unsigned int)ceil((float)LvltoExp(player->level)*exp_vipescaped*teamratio));

	return ;
}

/* Experience given to the team for one of these reasons:
	1   Target Successfully Bombed!
	2   The VIP has escaped!
	3   VIP has been assassinated!
	7   The bomb has been defused!
	11   All Hostages have been rescued!
	12   Target has been saved!
	13   Hostages have not been rescued!
*/
void CRPG_StatsManager::WinningTeam(int team, int reason) {
	CRPG_Player *player;
	unsigned int i = CRPG_Player::player_count, exp = 0;
	cssteam_t winningteam;
	float teamratio;

	if(!enable)
		return ;

	switch(reason) {
		case 1:
		case 2:
		case 3:
		case 7:
		case 11:
		case 12:
		case 13:
			break;

		default:
			return ;
	}

	if(team == 2) {
		winningteam = team_t;
		teamratio = team_ratio(team_ct);
	}
	else if(team == 3) {
		winningteam = team_ct;
		teamratio = team_ratio(team_t);
	}
	else {
		return ;
	}

	while(i--) {
		player = CRPG_Player::players[i];
		if(player != NULL) {
			if(player->css.team == winningteam)
				add_exp(player, (unsigned int)ceil((float)LvltoExp(player->level)*exp_teamwin*teamratio));
		}
	}

	return ;
}

/*	//////////////////////////////////////
	CRPG_RankManager
	////////////////////////////////////// */
unsigned int CRPG_RankManager::GetPlayerRank(CRPG_Player *player) {
	struct tbl_result *result;
	int retval, rank;

	WARN_IF(player == NULL, return 0)

	retval = CRPG::db->Query(&result, "SELECT COUNT(*) AS c FROM %s WHERE level > '%d' OR (level == '%d' AND exp > '%d')",
		TBL_PLAYERS, player->level, player->level, player->exp);

	if(!retval || (result == NULL))
		return 0;

	rank = atoi(GetCell(result, "c"))+1; // +1 since the query returns the count, not the rank
	FreeResult(result);

	return rank;
}

unsigned int CRPG_RankManager::GetRankCount(void) {
	unsigned int i = CRPG_Player::player_count;
	int count = CRPG::db->RowCount(TBL_PLAYERS);

	while(i--) {
		if(CRPG_Player::players[i] != NULL) {
			if(CRPG_Player::players[i]->dbinfo.player_id < 0) 
				count++; /* accounts for players not saved in the db */
		}
	}

	if(count > 0)
		return count;

	return 0;
}

void CRPG_RankManager::GetTop10Players(struct ranklist ***ranks) {
	struct tbl_result *result;
	unsigned int i;
	int retval;

	*ranks = NULL;

	retval = CRPG::db->Query(&result,
		"SELECT name, level, exp, credits FROM %s ORDER BY level DESC LIMIT 10", TBL_PLAYERS);

	if(!retval || (result == NULL))
		return ;

	if((result->col_count < 4) || (result->row_count < 11)) {
		FreeResult(result);
		return ;
	}

	*ranks = (struct ranklist**)malloc(10*sizeof(struct ranklist*));
	WARN_IF(*ranks == NULL, FreeResult(result); return)

	for(i = 0;i < 10;i++)
		(*ranks)[i] = (struct ranklist*)malloc(sizeof(struct ranklist));

	for(i = 1;i < 11;i++) {
		memset((*ranks)[i-1]->name, 0, 32);
		strncpy((*ranks)[i-1]->name, result->array[i][0], 31);
		(*ranks)[i-1]->level = atoi(result->array[i][1]);
		(*ranks)[i-1]->exp = atoi(result->array[i][2]);
		(*ranks)[i-1]->credits = atol(result->array[i][3]);
	}

	FreeResult(result);
	return ;
}

void CRPG_RankManager::FreeRanksList(struct ranklist **ranks) {
	int i = 10;

	while(i--)
		free(ranks[i]);

	free(ranks);
	return ;
}

void CRPG_RankManager::ChatAreaRank(CRPG_Player *player, int sendto) {
	WARN_IF(player == NULL, return)

	if(sendto == -1)
		CRPG::ChatAreaMsg(0, TXTDB_ID(rpgrank.msg1), player->name(), player->level, CRPG_RankManager::GetPlayerRank(player), CRPG_RankManager::GetRankCount(),
			player->exp, CRPG_StatsManager::LvltoExp(player->level), player->credits);
	else
		CRPG::ChatAreaMsg(sendto, TXTDB_ID(rpgrank.msg1), player->name(), player->level, CRPG_RankManager::GetPlayerRank(player), CRPG_RankManager::GetRankCount(),
			player->exp, CRPG_StatsManager::LvltoExp(player->level), player->credits);

	return ;
}

/*	//////////////////////////////////////
	CRPG_TeamBalance
	////////////////////////////////////// */
unsigned int CRPG_TeamBalance::teamt_total = 0;
unsigned int CRPG_TeamBalance::teamct_total = 0;
unsigned int CRPG_TeamBalance::teamt_count = 0;
unsigned int CRPG_TeamBalance::teamct_count = 0;
bool CRPG_TeamBalance::roundend_check = 0;

void CRPG_TeamBalance::recalc_teams(void) {
	unsigned int i = CRPG_Player::player_count;
	CRPG_Player *player;

	while(i--) {
		player = CRPG_Player::players[i];
		if(player != NULL) {
			if(player->css.team == team_t)
				teamt_total += player->level;
			else if(player->css.team == team_ct)
				teamct_total += player->level;
		}
	}

	return ;
}

CRPG_Player* CRPG_TeamBalance::find_lvl(unsigned int lvl, enum cssteam_t team) {
	/*unsigned int i = CRPG_Player::player_count, candlvl = 0;
	int candidate;
	CRPG_Player *player;

	while(i--) {
		player = CRPG_Player::players[i];
		if(player != NULL) {
			if((player->css.team == team) && (player->level) */

	return NULL;
}

void CRPG_TeamBalance::RoundEnd(void) {
	int lvldiff;

	if(roundend_check)
		roundend_check = 0;
	else
		return ;

	recalc_teams();

	if(!enable || !teamt_total || !teamct_total || (teamt_total == teamct_total))
		return ;

	lvldiff = abs((int)teamt_total-(int)teamct_total);
}
