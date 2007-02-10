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

/* NOTICE: Some of the code featured here was provided by David "BAILOPAN"
   Anderson via his blog site.
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
#endif

/* Forward Declarations */
class CBaseAnimating;
class CBaseEntity;
class Vector;
class QAngle;
class CBaseCombatCharacter;
class CBaseCombatWeapon;
class SendTable;
class SendProp;
enum MoveType_t;

#ifdef WIN32
class CRPG_SigScan {
	/* Private Variables */
	static unsigned char *base_addr;
	static size_t base_len;

	unsigned char *sig_str;
	char *sig_mask;
	unsigned long sig_len;

	/* Private Functions */
	void parse_sig(char *sig);
	void* find_sig(void);

public:
	/* Public Variables */
	char sig_name[64];
	bool is_set; /* if the scan was successful or not */
	void *sig_addr;

	/* Public Functions */
	CRPG_SigScan(void): sig_str(NULL), sig_addr(NULL), sig_len(0) {}
	~CRPG_SigScan(void);

	static bool GetDllMemInfo(void);
	void Init(char *name, char *sig);
};

void init_sigs(void);
#endif

#ifndef WIN32 /* !WIN32 */
void init_lsym_funcs(void);
#endif

/**
 * @brief A class to meddle with network/data properties
 * @remarks Some of this code was adapted from KinkeyMunkey's CSS:RPG 1.0.4 patch.
 *			Where would have I been without him?
 */
class CRPG_ExternProps {
	/* Private Functions */
	static SendProp* scan_sendtable(SendTable *tbl, char *name);
	static void print_sendtable(FILE *fptr, SendTable *tbl, int depth);

public:
	/* Public Variables */
	/**
	 * @name Offset Structures
	 * @{
	 */
	static struct netp_s {
		long m_iHealth;
		long m_nRenderMode;
		long m_clrRender;
		long m_nRenderFX;
		long m_ArmorValue;
		long m_fFlags;
	} netp;

	static struct datap_s {
		long m_vecVelocity;
	} datap;
	/* @} */

	/* Public Functions */
	static void Init(IServerGameDLL *gamedll);

	static long FindNetProp(ServerClass *sc, char *path);
	static long FindDataProp(CBaseEntity *cbe, char *name);

	static void DumpNetProps(FILE *fptr, ServerClass *sc);
	static void DumpDataProps(FILE *fptr, CBaseEntity *cbe);
};

/* TempEnts Hacked Pointer */
extern class ITempEntsSystem* tempents;
void Initialize_TE_Pointer(class IEffects *effects);

/* Hacked Functions */
void CBaseAnimating_Ignite(CBaseAnimating *cba, float flFlameLifetime, bool bNPCOnly = false, float flSize = 0.0f, bool bCalledByLevelDesigner = false);
void CBaseEntity_Teleport(CBaseEntity *cbe, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity);
CBaseCombatWeapon* CBaseCombatCharacter_Weapon_GetSlot(CBaseCombatCharacter *cbcc, int slot);
int CBaseCombatCharacter_GiveAmmo(CBaseCombatCharacter *cbcc, int iCount, int iAmmoIndex, bool bSuppressSound = 0);
void CBaseEntity_SetMoveType(CBaseEntity *cbe, MoveType_t val, MoveCollide_t moveCollide);
CBaseEntity* CBasePlayer_GiveNamedItem(CBasePlayer *cbp, const char *pszName, int iSubType = 0);
CBaseEntity *CBaseEntity_CreateNoSpawn(const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner);
int DispatchSpawn(CBaseEntity *cbe);
void CBaseEntity_SetParent(CBaseEntity *cbe, CBaseEntity *parent, int iAttachment = -1); //If iAttachment is -1, it'll preserve the current m_iParentAttachment.

#define CHECK_DATAPROP(NAME) \
	if(CRPG_ExternProps::datap.NAME == -1) { \
		CRPG_ExternProps::datap.NAME = CRPG_ExternProps::FindDataProp(cbe, #NAME); \
		if(CRPG_ExternProps::datap.NAME == -1) \
			return NULL; \
	}

/**
 * @brief Returns a Vector pointer to an entity's velocity.
 */
inline Vector* CBasePlayer_Velocity(CBaseEntity *cbe) {
	CHECK_DATAPROP(m_vecVelocity);

	return (Vector*)((char*)cbe + CRPG_ExternProps::datap.m_vecVelocity);
}

/**
 * @brief Sets an entity's current health.
 */
inline void CBaseEntity_SetHealth(CBaseEntity *cbe, unsigned int health) {
	if(CRPG_ExternProps::netp.m_iHealth == -1)
		return ;

	*(unsigned int*)((char*)cbe + CRPG_ExternProps::netp.m_iHealth) = health;
	return ;
}

/**
 * @brief Gets an entity's current health.
 */
inline unsigned int CBaseEntity_GetHealth(CBaseEntity *cbe) {
	if(CRPG_ExternProps::netp.m_iHealth == -1)
		return 0;

	return *(unsigned int*)((char*)cbe + CRPG_ExternProps::netp.m_iHealth);
}

/**
 * @brief Sets an entity's current health.
 */
inline void CBasePlayer_SetArmor(CBasePlayer *cbp, unsigned int armor) {
	if(CRPG_ExternProps::netp.m_iHealth == -1)
		return ;

	*(unsigned int*)((char*)cbp + CRPG_ExternProps::netp.m_ArmorValue) = armor;
	return ;
}

/**
 * @brief Gets an entity's current health.
 */
inline unsigned int CBasePlayer_GetArmor(CBasePlayer *cbp) {
	if(CRPG_ExternProps::netp.m_iHealth == -1)
		return 0;

	return *(unsigned int*)((char*)cbp + CRPG_ExternProps::netp.m_ArmorValue);
}

/**
 * @brief Gets a player's flags.
 */
inline int CBasePlayer_GetFlags(CBasePlayer *cbp) {
	if(CRPG_ExternProps::netp.m_fFlags == -1)
		return 0;

	return *(unsigned int*)((char*)cbp + CRPG_ExternProps::netp.m_fFlags);
}

/**
 * @brief Sets an entity's render mode.
 */
inline void CBaseEntity_SetRenderMode(CBaseEntity *cbe, RenderMode_t mode) {
	if(CRPG_ExternProps::netp.m_nRenderMode == -1)
		return ;

	unsigned char *m_nRenderMode = (unsigned char*)((char*)cbe + CRPG_ExternProps::netp.m_nRenderMode);
	*m_nRenderMode = static_cast<unsigned char>(mode);
}

/**
 * @brief Sets an entity's render fx.
 */
inline void CBaseEntity_SetRenderFX(CBaseEntity *cbe, RenderFx_t fx) {
	if(CRPG_ExternProps::netp.m_nRenderFX == -1)
		return ;

	unsigned char *m_nRenderFX = (unsigned char*)((char*)cbe + CRPG_ExternProps::netp.m_nRenderFX);
	*m_nRenderFX = static_cast<unsigned char>(fx);
}

/**
 * @brief Sets an entity's color.
 */
inline void CBaseEntity_SetRenderColor(CBaseEntity *cbe, byte r, byte g, byte b) {
	if(CRPG_ExternProps::netp.m_clrRender == -1)
		return ;

	color32 *clr = (color32*)((char*)cbe + CRPG_ExternProps::netp.m_clrRender);
	
	clr->r = r;
	clr->g = g;
	clr->b = b;

	return ;
}

/**
 * @brief Sets an entity's color and alpha.
 */
inline void CBaseEntity_SetRenderColor(CBaseEntity *cbe, byte r, byte g, byte b, byte a) {
	if(CRPG_ExternProps::netp.m_clrRender == -1)
		return ;

	color32 *clr = (color32*)((char*)cbe + CRPG_ExternProps::netp.m_clrRender);
	
	clr->r = r;
	clr->g = g;
	clr->b = b;
	clr->a = a;

	return ;
}

/**
 * @brief Sets the level of blue tint on an entity.
 */
inline void CBaseEntity_SetRenderColorR(CBaseEntity *cbe, byte r) {
	if(CRPG_ExternProps::netp.m_clrRender == -1)
		return ;

	color32 *clr = (color32*)((char*)cbe + CRPG_ExternProps::netp.m_clrRender);
	clr->r = r;
	return ;
}

/**
 * @brief Sets the level of green tint on an entity.
 */
inline void CBaseEntity_SetRenderColorG(CBaseEntity *cbe, byte g) {
	if(CRPG_ExternProps::netp.m_clrRender == -1)
		return ;

	color32 *clr = (color32*)((char*)cbe + CRPG_ExternProps::netp.m_clrRender);
	clr->g = g;
	return ;
}

/**
 * @brief Sets the level of red tint on an entity.
 */
inline void CBaseEntity_SetRenderColorB(CBaseEntity *cbe, byte b) {
	if(CRPG_ExternProps::netp.m_clrRender == -1)
		return ;

	color32 *clr = (color32*)((char*)cbe + CRPG_ExternProps::netp.m_clrRender);
	clr->b = b;
	return ;
}

/**
 * @brief Sets an entity's alpha color.
 */
inline void CBaseEntity_SetRenderColorA(CBaseEntity *cbe, byte a) {
	if(CRPG_ExternProps::netp.m_clrRender == -1)
		return ;

	color32 *clr = (color32*)((char*)cbe + CRPG_ExternProps::netp.m_clrRender);
	clr->a = a;
	return ;
}

#endif
