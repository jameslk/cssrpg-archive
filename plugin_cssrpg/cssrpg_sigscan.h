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

/* NOTICE: Some of the code featured here was provided by David "BAILOPAN" Anderson via
   his blog site.
   LINKS:
   http://www.sourcemod.net/devlog/?p=55
   http://www.sourcemod.net/devlog/?p=56
   http://www.sourcemod.net/devlog/?p=57
*/

#ifndef CSSRPG_SIGSCAN_H
#define CSSRPG_SIGSCAN_H

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <tlhelp32.h>
#else
	#include <dlfcn.h>
	#include <sys/types.h>
	#include <sys/stat.h> 
#endif

class CRPG_SigScan {
private:
	/* Private Variables */
	static unsigned char *base_addr;
	static size_t base_len;

	unsigned char *sig_str;
	char *sig_mask;
	unsigned long sig_len;

	/* Private Functions */
	static unsigned int parse_maps(char *maps, char *matchpath, long *base, long *len);
	void* FindSignature(void);

public:
	/* Public Variables */
	char sig_name[64];
	char is_set; /* if the scan was successful or not */
	void *sig_addr;

	/* Public Functions */
	CRPG_SigScan(void): sig_str(NULL), sig_addr(NULL), sig_len(0) {}
	~CRPG_SigScan(void);

	static bool GetDllMemInfo(void);
	void Init(char *name, unsigned char *sig, char *mask, size_t len);
};

void init_sigs(void);

class CBaseAnimating;
class CBaseEntity;
class Vector;
class QAngle;
class CBaseCombatCharacter;
class CBaseCombatWeapon;
/* Sig Functions */
void CBaseAnimating_Ignite(CBaseAnimating *cba, float flFlameLifetime, bool bNPCOnly = false, float flSize = 0.0f, bool bCalledByLevelDesigner = false);
void CBaseEntity_Teleport(CBaseEntity *cbe, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity);
CBaseCombatWeapon* CBaseCombatCharacter_Weapon_GetSlot(CBaseCombatCharacter *cbcc, int slot);
int CBaseCombatCharacter_GiveAmmo(CBaseCombatCharacter *cbcc, int iCount, int iAmmoIndex, bool bSuppressSound = 0);
void CBaseEntity_SetRenderColor(CBaseEntity *cbe, byte r, byte g, byte b, byte a);
void CBaseEntity_SetRenderMode(CBaseEntity *cbe, RenderMode_t nRenderMode);

#endif
