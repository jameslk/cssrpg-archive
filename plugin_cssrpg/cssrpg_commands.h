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

#ifndef CSSRPG_COMMANDS_H
#define CSSRPG_COMMANDS_H

typedef int (rpgcmd_func)(int argc, char *argv[], char *args, char *thiscmd);

#include "../cssrpg_misc.h"
class CRPG_Commands: public CRPG_LinkedList<CRPG_Commands> {
public:
	char name[64];
	char info[128];
	unsigned int req_argc;
	char arg_template[128];
	int access;

	rpgcmd_func *call;

	CRPG_Commands(char *reg, char *desc, unsigned int req_argu, char *argu_template, rpgcmd_func *func);
	~CRPG_Commands();

	static void ExecCmdFromPlayer(CRPG_Player *player) {}
	static void ExecCmd(void);
};

#define RPGCMD_PREFIX "rpg_"
#define RPGCMD_PREFIX_LEN 4

#define RPG_CMD(name, description, req_argu, argu_template) \
	int cmdfunc_##name(int argc, char *argv[], char *args, char *thiscmd); \
	CRPG_Commands cmd_##name(#name, description, req_argu, argu_template, cmdfunc_##name); \
	ConCommand concmd_##name(RPGCMD_PREFIX #name, CRPG_Commands::ExecCmd, description); \
	int cmdfunc_##name(int argc, char *argv[], char *args, char *thiscmd)

#endif
