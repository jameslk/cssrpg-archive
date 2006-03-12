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

#include <stdio.h>

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "engine/IEngineSound.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"

#include "cssrpg.h"
#include "cssrpg_hacks.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*	//////////////////////////////////////
	CRPG_SigScan Class
	Note: Plugin SigScanning on Linux is
	pretty much impossible when SourceMM
	is loaded.
	////////////////////////////////////// */
#ifdef WIN32
unsigned char* CRPG_SigScan::base_addr;
size_t CRPG_SigScan::base_len;

void CRPG_SigScan::Init(char *name, unsigned char *sig, char *mask, size_t len) {
	strncpy(sig_name, name, 63);
	sig_name[64] = '\0';
	is_set = 0;

	sig_len = len;
	sig_str = new unsigned char[sig_len];
	CRPG::ustrncpy(sig_str, sig, sig_len);

	sig_mask = new char[sig_len+1];
	strncpy(sig_mask, mask, sig_len);
	sig_mask[sig_len+1] = 0;

	if(!base_addr) {
		CRPG::ConsoleMsg("Failed to find the server module, CSS:RPG will not function properly.",
			MTYPE_ERROR);
		return ;
	}

	if((sig_addr = FindSignature()) == NULL) {
		CRPG::ConsoleMsg("Failed to find the signature \"%s\", CSS:RPG will not function properly.",
			MTYPE_ERROR, this->sig_name);
		return ;
	}

	is_set = 1;
	CRPG::DebugMsg("Found signature function: %s", this->sig_name);
	return ;
}

CRPG_SigScan::~CRPG_SigScan(void) {
	delete[] sig_str;
	delete[] sig_mask;
}

bool CRPG_SigScan::GetDllMemInfo(void) {
	char binpath[1024];
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 modent;

	base_addr = 0;
	base_len = 0;

	CRPG::s_engine()->GetGameDir(binpath, 512);
	sprintf(binpath, "%s\\bin\\server.dll", binpath);

	// Take a snapshot of all modules in the specified process.
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if(hModuleSnap == INVALID_HANDLE_VALUE) {
		CRPG::DebugMsg("CreateToolhelp32Snapshot failed.");
		return false;
	}

	// Set the size of the structure before using it.
	modent.dwSize = sizeof(MODULEENTRY32);

	// Retrieve information about the first module
	if(!Module32First(hModuleSnap, &modent)) {
		CRPG::DebugMsg("Module32First failed.");
		CloseHandle(hModuleSnap); // Must clean up the snapshot object! 
		return false;
	}

	/* Walk the module list of the process,
	   and display information about each module */
	do {
		if(CRPG::istrcmp(modent.szExePath, binpath)) {
			base_addr = modent.modBaseAddr;
			base_len = modent.modBaseSize;
			CloseHandle(hModuleSnap);
			return true;
		}
	} while(Module32Next(hModuleSnap, &modent));
 
	CloseHandle(hModuleSnap);
	CRPG::DebugMsg("Failed to find server module in module list.");
	return false;
}

void* CRPG_SigScan::FindSignature(void) {
	unsigned char *pBasePtr = base_addr;
	unsigned char *pEndPtr = base_addr+base_len;
	size_t i, height = 0;

	while(pBasePtr < pEndPtr) {
		for(i = 0;i < sig_len;i++) {
			if(i > height)
				height++;

			if((sig_mask[i] != '?') && (sig_str[i] != pBasePtr[i]))
				break;
		}

		// If 'i' reached the end, we know we have a match!
		if(i == sig_len)
			return (void*)pBasePtr;

		pBasePtr++;
	}

	CRPG::DebugMsg("Sig Failed at Height: %d", height);

	return NULL;
}

CRPG_SigScan CBaseAnimating_Ignite_Sig;
CRPG_SigScan CBaseEntity_Teleport_Sig;
CRPG_SigScan CBaseCombatCharacter_Weapon_GetSlot_Sig;
CRPG_SigScan CBaseCombatCharacter_GiveAmmo_Sig;
CRPG_SigScan CBaseEntity_SetRenderColor_Sig;
CRPG_SigScan CBaseEntity_SetRenderMode_Sig;

void init_sigs(void) {
	CRPG_SigScan::GetDllMemInfo();

	/* virtual void CBaseAnimating::Ignite(float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner);
	Last Address: 0x220BC7A0
	Signature: 56 8B F1 8B? 86? BC? 00? 00? 00? C1? E8? 1B? A8? 01? 0F? 85? 9A? 00? 00? 00? 
	8B 16 FF 92? F0? 00? 00? 00? 80? 7C? 24? 0C? 00? 74? 08? 84 C0 0F? 84? 83? 00? 00? 00? 
	3C 01 75? 20? 80 7C 24 14 00 75? 19? 8B CE E8 83? 1A? 01? 00? 85? C0? 74? 0E? 8B 10 8B 
	C8 FF 92? 08? 05? 00? 00? 84 C0 74? 5F? 57 6A 01 56 E8 48? EA? 07? 00? 8B F8 83 C4 08 
	85 FF 74? 3D? 8B 44 24 0C 50 8B CF E8 83? E5? 07? 00? 68 00 00 00 08 8B CE */
CBaseAnimating_Ignite_Sig.Init("CBaseAnimating::Ignite",
(unsigned char*)"\x56\x8B\xF1\x8B\x86\xBC\x00\x00\x00\xC1\xE8\x1B\xA8\x01\x0F\x85\
\x9A\x00\x00\x00\x8B\x16\xFF\x92\xF0\x00\x00\x00\x80\x7C\x24\x0C\x00\x74\x08\x84\xC0\
\x0F\x84\x83\x00\x00\x00\x3C\x01\x75\x20\x80\x7C\x24\x14\x00\x75\x19\x8B\xCE\xE8\x83\
\x1A\x01\x00\x85\xC0\x74\x0E\x8B\x10\x8B\xC8\xFF\x92\x08\x05\x00\x00\x84\xC0\x74\x5F\
\x57\x6A\x01\x56\xE8\x48\xEA\x07\x00\x8B\xF8\x83\xC4\x08\x85\xFF\x74\x3D\x8B\x44\x24\
\x0C\x50\x8B\xCF\xE8\x83\xE5\x07\x00\x68\x00\x00\x00\x08\x8B\xCE",
"xxx?????????????????xxx????????????xx??????xx??xxxxx??xxx????????xxxxx?????xx??xxxxx\
????xxxxxxx??xxxxxxxx????xxxxxxx",
116);

	/* virtual void CBaseEntity::Teleport(const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity);
	Last Address: 0x220D9940
	Signature: 83 EC 18 53 56 8B D9 8B? 0D? 78? B2? 46? 22? 33 F6 33 C0 3B CE 7E? 21? 8B? 
	15? 6C? B2? 46? 22? EB? 03? 8D? 49? 00? 39 1C 82 74? 09? 83? C0? 01? 3B C1 7C? F4? EB? 
	08? 3B C6 0F? 8D? 17? 01? 00? 00? 55 57? 8D? 44? 24? 10? 50 51 B9? 6C? B2? 46? 22? 89 
	5C 24 18 E8? B4? 88? F9? FF? 8D? 4C? 24? 14? 51 53 89 44 24 18 89 74 24 1C 89 74 24 20 
	89 74 24 24 89 74 24 28 89 74 24 2C */
CBaseEntity_Teleport_Sig.Init("CBaseEntity::Teleport",
(unsigned char*)"\x83\xEC\x18\x53\x56\x8B\xD9\x8B\x0D\x78\xB2\x46\x22\x33\xF6\x33\xC0\
\x3B\xCE\x7E\x21\x8B\x15\x6C\xB2\x46\x22\xEB\x03\x8D\x49\x00\x39\x1C\x82\x74\x09\x83\xC0\
\x01\x3B\xC1\x7C\xF4\xEB\x08\x3B\xC6\x0F\x8D\x17\x01\x00\x00\x55\x57\x8D\x44\x24\x10\x50\
\x51\xB9\x6C\xB2\x46\x22\x89\x5C\x24\x18\xE8\xB4\x88\xF9\xFF\x8D\x4C\x24\x14\x51\x53\x89\
\x44\x24\x18\x89\x74\x24\x1C\x89\x74\x24\x20\x89\x74\x24\x24\x89\x74\x24\x28\x89\x74\x24\x2C",
"xxxxxxx??????xxxxxx?????????????xxx?????xx????xx??????x?????xx?????xxxx?????????xxxxxxxxxxxxxxxxxxxxxxxxxx",
106);

	/* CBaseCombatWeapon *CBaseCombatCharacter::Weapon_GetSlot(int slot) const;
	Last Address: 220C2960
	Signature: 53 55 8B 6C 24 0C 56 8B D9 57 33 F6 8D? BB? C4? 06? 00? 00? 8B 0F 83 
	F9 FF 74? 3A? 8B? 15? 10? 19? 43? 22? 8B C1 25 FF 0F 00 00 C1 E0 04 */
CBaseCombatCharacter_Weapon_GetSlot_Sig.Init("CBaseCombatCharacter::Weapon_GetSlot",
(unsigned char*)"\x53\x55\x8B\x6C\x24\x0C\x56\x8B\xD9\x57\x33\xF6\x8D\xBB\xC4\x06\x00\x00\
\x8B\x0F\x83\xF9\xFF\x74\x3A\x8B\x15\x10\x19\x43\x22\x8B\xC1\x25\xFF\x0F\x00\x00\xC1\xE0\x04",
"xxxxxxxxxxxx??????xxxxx????????xxxxxxxxxx",
41);

	/* int CBaseCombatCharacter::GiveAmmo(CBaseCombatCharacter *cbcc, int iCount, int iAmmoIndex, bool bSuppressSound);
	Last Address: 220B33C0
	Signature: 53 8B 5C 24 08 85 DB 57 8B F9 7F? 07? 5F 33 C0 5B C2? 0C? 00? 8B? 0D? 
	7C? EA? 58? 22? 8B 01 56 8B 74 24 14 56 57 */
CBaseCombatCharacter_GiveAmmo_Sig.Init("CBaseCombatCharacter::GiveAmmo",
(unsigned char*)"\x53\x8B\x5C\x24\x08\x85\xDB\x57\x8B\xF9\x7F\x07\x5F\x33\xC0\x5B\xC2\x0C\
\x00\x8B\x0D\x7C\xEA\x58\x22\x8B\x01\x56\x8B\x74\x24\x14\x56\x57",
"xxxxxxxxxx??xxxx?????????xxxxxxxxx",
34);

	/* void CBaseEntity::SetRenderColor(byte r, byte g, byte b);
	Last Address: 22053860
	Signature: 83 C1 24 E9 18? F9? FF? FF? */
CBaseEntity_SetRenderColor_Sig.Init("CBaseEntity::SetRenderColor",
(unsigned char*)"\x83\xC1\x24\xE9\x18\xF9\xFF\xFF",
"xxxx????",
8);

	/* void CBaseEntity::SetRenderMode(RenderMode_t nRenderMode);
	Last Address: 220F2220
	Signature: 8B 54 24 04 39 51 20 74? 26? 80 B9 F0 01 00 00 00 74? 0D? C6 81 00 02 00 00 01 89 51 20 C2 04 00 */
CBaseEntity_SetRenderMode_Sig.Init("CBaseEntity::SetRenderMode",
(unsigned char*)"\x8B\x54\x24\x04\x39\x51\x20\x74\x26\x80\xB9\xF0\x01\x00\x00\x00\x74\x0D\xC6\x81\
\x00\x02\x00\x00\x01\x89\x51\x20\xC2\x04\x00",
"xxxxxxx??xxxxxxx??xxxxxxxxxxxxx",
31);

	return ;
}
#endif /* if WIN32 */

/*	//////////////////////////////////////
	Linux Symbol Table Functions
	////////////////////////////////////// */
#ifndef WIN32 /* !WIN32 */
void *CBaseAnimating_Ignite_Addr;
void *CBaseEntity_Teleport_Addr;
void *CBaseCombatCharacter_Weapon_GetSlot_Addr;
void *CBaseCombatCharacter_GiveAmmo_Addr;

void* find_sym_addr(void *dl_handle, char *name, char *symbol) {
	void *addr;

	if(dl_handle == NULL)
		return NULL;

	addr = dlsym(dl_handle, symbol);
	if(addr == NULL) {
		CRPG::ConsoleMsg("dlsym() for function \"%s\" with error: %s",
			MTYPE_ERROR, name, dlerror());
		return NULL;
	}

	CRPG::DebugMsg("Found symbol function: %s", name);
	return addr;
}

void init_lsym_funcs(void) {
	char binpath[1024];
	void *handle;

	CRPG::s_engine()->GetGameDir(binpath, 512);
	sprintf(binpath, "%s/bin/server_i486.so", binpath);

	handle = dlopen(binpath, RTLD_NOW);

	CBaseAnimating_Ignite_Addr = find_sym_addr(handle,
		"CBaseAnimating::Ignite", "_ZN14CBaseAnimating6IgniteEfbfb");

	CBaseEntity_Teleport_Addr = find_sym_addr(handle,
		"CBaseEntity::Teleport", "_ZN11CBaseEntity8TeleportEPK6VectorPK6QAngleS2_");

	CBaseCombatCharacter_Weapon_GetSlot_Addr = find_sym_addr(handle,
		"CBaseCombatCharacter::Weapon_GetSlot", "_ZNK20CBaseCombatCharacter14Weapon_GetSlotEi");

	CBaseCombatCharacter_GiveAmmo_Addr = find_sym_addr(handle,
		"CBaseCombatCharacter::GiveAmmo", "_ZN20CBaseCombatCharacter8GiveAmmoEiPKcb");

	dlclose(handle);
}
#endif /* endif !WIN32 */

/*	//////////////////////////////////////
	Hacked Functions
	////////////////////////////////////// */
void CBaseAnimating_Ignite(CBaseAnimating *cba, float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner) {
	#ifdef WIN32
	if(!CBaseAnimating_Ignite_Sig.is_set)
		return ;

	typedef void (__fastcall *func)(CBaseAnimating*, void*, float, bool, float, bool);
	func thisfunc = (func)CBaseAnimating_Ignite_Sig.sig_addr;
	thisfunc(cba, 0, flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner);

	#else

	if(CBaseAnimating_Ignite_Addr == NULL)
		return ;

	typedef void (*func)(CBaseAnimating*, float, bool, float, bool);
	func thisfunc = (func)CBaseAnimating_Ignite_Addr;
	thisfunc(cba, flFlameLifetime, (bool)bNPCOnly, flSize, (bool)bCalledByLevelDesigner);
	#endif

	return ;
}

void CBaseEntity_Teleport(CBaseEntity *cbe, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity) {
	#ifdef WIN32
	if(!CBaseEntity_Teleport_Sig.is_set)
		return ;

	typedef void (__fastcall *func)(CBaseEntity*, void*, const Vector*, const QAngle*, const Vector*);
	func thisfunc = (func)CBaseEntity_Teleport_Sig.sig_addr;
	thisfunc(cbe, 0, newPosition, newAngles, newVelocity);

	#else

	if(CBaseEntity_Teleport_Addr == NULL)
		return ;

	typedef void (*func)(CBaseEntity*, const Vector*, const QAngle*, const Vector*);
	func thisfunc = (func)CBaseEntity_Teleport_Addr;
	thisfunc(cbe, newPosition, newAngles, newVelocity);
	#endif

	return ;
}

CBaseCombatWeapon* CBaseCombatCharacter_Weapon_GetSlot(CBaseCombatCharacter *cbcc, int slot) {
	CBaseCombatWeapon *ret;

	#ifdef WIN32
	if(!CBaseCombatCharacter_Weapon_GetSlot_Sig.is_set)
		return NULL;

	typedef CBaseCombatWeapon* (__fastcall *func)(CBaseCombatCharacter*, void*, int);
	func thisfunc = (func)CBaseCombatCharacter_Weapon_GetSlot_Sig.sig_addr;
	ret = thisfunc(cbcc, 0, slot);

	#else

	if(CBaseCombatCharacter_Weapon_GetSlot_Addr == NULL)
		return ;

	typedef CBaseCombatWeapon* (*func)(CBaseCombatCharacter*, int);
	func thisfunc = (func)CBaseCombatCharacter_Weapon_GetSlot_Addr;
	ret = thisfunc(cbcc, slot);
	#endif

	return ret;
}

int CBaseCombatCharacter_GiveAmmo(CBaseCombatCharacter *cbcc, int iCount, int iAmmoIndex, bool bSuppressSound) {
	int ret;

	#ifdef WIN32
	if(!CBaseCombatCharacter_GiveAmmo_Sig.is_set)
		return 0;

	typedef int (__fastcall *func)(CBaseCombatCharacter*, void*, int, int, bool);
	func thisfunc = (func)CBaseCombatCharacter_GiveAmmo_Sig.sig_addr;
	ret = thisfunc(cbcc, 0, iCount, iAmmoIndex, bSuppressSound);

	#else

	if(CBaseCombatCharacter_GiveAmmo_Addr == NULL)
		return ;

	typedef int (*func)(CBaseCombatCharacter*, int, int, bool);
	func thisfunc = (func)CBaseCombatCharacter_GiveAmmo_Addr;
	ret = thisfunc(cbcc, iCount, iAmmoIndex, bSuppressSound);
	#endif

	return ret;
}

void CBaseEntity_SetRenderColor(CBaseEntity *cbe, byte r, byte g, byte b, byte a) {
	#ifdef WIN32
	if(!CBaseEntity_SetRenderColor_Sig.is_set)
		return ;

	typedef void (__fastcall *func)(CBaseEntity*, void*, byte, byte, byte, byte);
	func thisfunc = (func)CBaseEntity_SetRenderColor_Sig.sig_addr;
	thisfunc(cbe, 0, r, b, g, a);
	#endif

	return ;
}

void CBaseEntity_SetRenderMode(CBaseEntity *cbe, RenderMode_t nRenderMode) {
	#ifdef WIN32
	if(!CBaseEntity_SetRenderMode_Sig.is_set)
		return ;

	typedef void (__fastcall *func)(CBaseEntity*, void*, RenderMode_t);
	func thisfunc = (func)CBaseEntity_SetRenderMode_Sig.sig_addr;
	thisfunc(cbe, 0, nRenderMode);
	#endif

	return ;
}