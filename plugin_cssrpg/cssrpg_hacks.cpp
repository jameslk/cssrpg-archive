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
   http://wiki.tcwonline.org/index.php/Signature_scanning
*/

#include <stdio.h>

#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "engine/IEngineSound.h"
#include "dlls/iplayerinfo.h"
#include "eiface.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"

#include "server_class.h"

#include "cssrpg.h"
#include "cssrpg_fvars.h"
#include "cssrpg_hacks.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*	//////////////////////////////////////
	CRPG_SigScan Class
	////////////////////////////////////// */

#ifdef WIN32
unsigned char* CRPG_SigScan::base_addr;
size_t CRPG_SigScan::base_len;

void CRPG_SigScan::parse_sig(char *sig) {
    int i, len, out_i = 0, mask_i = 0;
	char byte[3] = {0};
    
    len = strlen(sig);
    
    sig_str = (unsigned char*)calloc(len+1, sizeof(unsigned char));
    sig_mask = (char*)calloc(len+1, sizeof(char));
    
    for(i = 0;i < len;i++) {
        if(isalnum(sig[i]) && isalnum(sig[i+1])) {
            byte[0] = sig[i];
            byte[1] = sig[i+1];
			sig_str[out_i++] = CRPG::hextoul(byte);

            if(sig[i+2] == '?') {
                sig_mask[mask_i++] = '?';
                i += 2;
            }
            else {
                sig_mask[mask_i++] = 'x';
                i++;
            }
        }
    }

	sig_len = out_i;

    return ;
}

void* CRPG_SigScan::find_sig(void) {
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

void CRPG_SigScan::Init(char *name, char *sig) {
	strncpy(sig_name, name, 63);
	sig_name[64] = '\0';
	is_set = 0;

	if(!base_addr) {
		CRPG::ConsoleMsg("Failed to find the server module, CSS:RPG will not function properly.",
			MTYPE_ERROR);
		return ;
	}

	parse_sig(sig);

	if((sig_addr = find_sig()) == NULL) {
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

/**
 * @brief Finds the start and end address of the game dll in memory.
 */
bool CRPG_SigScan::GetDllMemInfo(void) {
	char binpath[1024];
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 modent;

	base_addr = 0;
	base_len = 0;

	s_engine->GetGameDir(binpath, 512);
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

/**
 * @brief Windows Signature Scanner File Variables
 *
 * @{
 */
/* virtual void CBaseAnimating::Ignite(float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner);
   Last Address: 0x220BC7A0 */
CRPG_FileVar CBaseAnimating_Ignite_Sig("CBaseAnimating_Ignite_Sig",
	"56 8B F1 8B? 86? BC? 00? 00? 00? C1? E8? 1B? A8? 01? 0F? 85? 9A?"
	"00? 00? 00? 8B 16 FF 92? F0? 00? 00? 00? 80? 7C? 24? 0C? 00? 74? 08? 84 C0"
	"0F? 84? 83? 00? 00? 00? 3C 01 75? 20? 80 7C 24 14 00 75? 19? 8B CE E8 83?"
	"1A? 01? 00? 85? C0? 74? 0E? 8B 10 8B  C8 FF 92? 08? 05? 00? 00? 84 C0 74?"
	"5F? 57 6A 01 56 E8 48? EA? 07? 00? 8B F8 83 C4 08 85 FF 74? 3D? 8B 44 24"
	"0C 50 8B CF E8 83? E5? 07? 00? 68 00 00 00 08 8B CE",
	"sigscanner/windows/CBaseAnimating_Ignite");

/* virtual void CBaseEntity::Teleport(const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity);
   Last Address: 0x220D9940 */
CRPG_FileVar CBaseEntity_Teleport_Sig("CBaseEntity_Teleport_Sig",
	"83 EC 18 53 56 8B D9 8B? 0D? 78? B2? 46? 22? 33 F6 33 C0 3B CE 7E? 21? 8B?"
	"15? 6C? B2? 46? 22? EB? 03? 8D? 49? 00? 39 1C 82 74? 09? 83? C0? 01? 3B C1"
	"7C? F4? EB? 08? 3B C6 0F? 8D? 17? 01? 00? 00? 55 57? 8D? 44? 24? 10? 50 51"
	"B9? 6C? B2? 46? 22? 89 5C 24 18 E8? B4? 88? F9? FF? 8D? 4C? 24? 14? 51 53"
	"89 44 24 18 89 74 24 1C 89 74 24 20 89 74 24 24 89 74 24 28 89 74 24 2C",
   "sigscanner/windows/CBaseEntity_Teleport");

/* CBaseCombatWeapon *CBaseCombatCharacter::Weapon_GetSlot(int slot) const;
   Last Address: 220C2960 */
CRPG_FileVar CBaseCombatCharacter_Weapon_GetSlot_Sig("CBaseCombatCharacter_Weapon_GetSlot_Sig",
   "53 55 8B 6C 24 0C 56 8B D9 57 33 F6 8D? BB? C4? 06? 00? 00? 8B 0F 83 F9 FF"
   "74? 3A? 8B? 15? 10? 19? 43? 22? 8B C1 25 FF 0F 00 00 C1 E0 04",
   "sigscanner/windows/CBaseCombatCharacter_Weapon_GetSlot");

/* int CBaseCombatCharacter::GiveAmmo(CBaseCombatCharacter *cbcc, int iCount, int iAmmoIndex, bool bSuppressSound);
   Last Address: 220B33C0 */
CRPG_FileVar CBaseCombatCharacter_GiveAmmo_Sig("CBaseCombatCharacter_GiveAmmo_Sig",
   "53 8B 5C 24 08 85 DB 57 8B F9 7F? 07? 5F 33 C0 5B C2? 0C? 00? 8B? 0D? 7C?"
   "EA? 58? 22? 8B 01 56 8B 74 24 14 56 57",
   "sigscanner/windows/CBaseCombatCharacter_GiveAmmo");

/* void CBaseEntity::SetMoveType(MoveType_t val, MoveCollide_t moveCollide);
   Last Address: 220F0890 */
CRPG_FileVar CBaseEntity_SetMoveType_Sig("CBaseEntity_SetMoveType_Sig",
   "56 8B F1 0F? B6? 86? DE? 00? 00? 00? 3B 44 24 08 57 8D? BE? DE? 00? 00? 00?"
   "75 15 8D 4C 24 10 51",
   "sigscanner/windows/CBaseEntity_SetMoveType");

/* CBaseEntity* CBasePlayer::GiveNamedItem(const char *pszName, int iSubType);
   Last Address: 221FD7B0 */
CRPG_FileVar CBasePlayer_GiveNamedItem_Sig("CBasePlayer_GiveNamedItem_Sig",
   "53 8B 5C 24 0C 55 56 8B 74 24 10 53 56 8B E9 E8? 6C? 89? EC? FF? 85 C0 74?"
   "08? 5E 5D 33 C0 5B C2 08 00",
   "sigscanner/windows/CBasePlayer_GiveNamedItem");

/* CBaseEntity *CBaseEntity::CreateNoSpawn(const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner);
   Last Address: 220F3B60 */
CRPG_FileVar CBaseEntity_CreateNoSpawn_Sig("CBaseEntity_CreateNoSpawn_Sig",
	"8B 44 24 04 56 6A FF 50 E8 A3? 09? 0B? 00? 8B F0 83 C4 08 85 F6 75 02 5E"
	"C3 8B 4C 24 0C 53 57 51 8B CE",
	"sigscanner/windows/CBaseEntity_CreateNoSpawn_Sig");

/* int DispatchSpawn(CBaseEntity *pEntity);
   Last Address: 2229C1C0 */
CRPG_FileVar DispatchSpawn_Sig("DispatchSpawn_Sig",
   "53 55 56 8B 74 24 10 85 F6 57 0F? 84? 3A? 01? 00? 00? 8B 1D 0C 65 57 22 8B"
   "03 8B CB FF 50? 60? 8B 16 8B CE",
   "sigscanner/windows/DispatchSpawn_Sig");

/* void CBaseEntity::SetParent(CBaseEntity *pParentEntity, int iAttachment);
   Last Address: 220F1A40 */
CRPG_FileVar CBaseEntity_SetParent_Sig("CBaseEntity_SetParent_Sig",
   "81 EC DC 00 00 00 83 BC 24 E4 00 00 00 FF 55 56 8B F1 75? 0E? 0F B6 86 DD"
   "00 00 00 89 84 24 EC 00 00 00",
   "sigscanner/windows/CBaseEntity_SetParent_Sig");
/** @} */

CRPG_SigScan CBaseAnimating_Ignite_SigScan;
CRPG_SigScan CBaseEntity_Teleport_SigScan;
CRPG_SigScan CBaseCombatCharacter_Weapon_GetSlot_SigScan;
CRPG_SigScan CBaseCombatCharacter_GiveAmmo_SigScan;
CRPG_SigScan CBaseEntity_SetMoveType_SigScan;
CRPG_SigScan CBasePlayer_GiveNamedItem_SigScan;
CRPG_SigScan CBaseEntity_CreateNoSpawn_SigScan;
CRPG_SigScan DispatchSpawn_SigScan;
CRPG_SigScan CBaseEntity_SetParent_SigScan;

void init_sigs(void) {
	CRPG_SigScan::GetDllMemInfo();

	CBaseAnimating_Ignite_SigScan.Init("CBaseAnimating::Ignite", CBaseAnimating_Ignite_Sig.String());
	CBaseEntity_Teleport_SigScan.Init("CBaseEntity::Teleport", CBaseEntity_Teleport_Sig.String());
	CBaseCombatCharacter_Weapon_GetSlot_SigScan.Init("CBaseCombatCharacter::Weapon_GetSlot", CBaseCombatCharacter_Weapon_GetSlot_Sig.String());
	CBaseCombatCharacter_GiveAmmo_SigScan.Init("CBaseCombatCharacter::GiveAmmo", CBaseCombatCharacter_GiveAmmo_Sig.String());
	CBaseEntity_SetMoveType_SigScan.Init("CBaseEntity::SetMoveType", CBaseEntity_SetMoveType_Sig.String());
	CBasePlayer_GiveNamedItem_SigScan.Init("CBasePlayer::GiveNamedItem", CBasePlayer_GiveNamedItem_Sig.String());
	CBaseEntity_CreateNoSpawn_SigScan.Init("CBaseEntity::CreateNoSpawn", CBaseEntity_CreateNoSpawn_Sig.String());
	DispatchSpawn_SigScan.Init("DispatchSpawn", DispatchSpawn_Sig.String());
	CBaseEntity_SetParent_SigScan.Init("CBaseEntity::SetParent", CBaseEntity_SetParent_Sig.String());

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
void *CBaseEntity_SetMoveType_Addr;
void *CBasePlayer_GiveNamedItem_Addr;
void *CBaseEntity_CreateNoSpawn_Addr;
void *DispatchSpawn_Addr;
void *CBaseEntity_SetParent_Addr;

ITempEntsSystem **TE_Pointer_Linux;

/**
 * @brief Search for a function symbol in the game dll.
 */
void* find_sym_addr(void *dl_handle, char *name, char *symbol) {
	void *addr;

	if(dl_handle == NULL)
		return NULL;

	addr = dlsym(dl_handle, symbol);
	if(addr == NULL) {
		CRPG::ConsoleMsg("dlsym() failed for function \"%s\" with error: \"%s\", CSS:RPG may not function properly.",
			MTYPE_ERROR, name, dlerror());
		return NULL;
	}

	CRPG::DebugMsg("Found symbol function: %s, address: %X", name, addr);
	return addr;
}

void init_lsym_funcs(void) {
	char binpath[1024];
	void *handle;
	int error = 0;

	s_engine->GetGameDir(binpath, 512);
	sprintf(binpath, "%s/bin/server_i486.so", binpath);

	handle = dlopen(binpath, RTLD_NOW);
	if(handle == NULL) {
		CRPG::ConsoleMsg("dlopen() failed with error: \"%s\", CSS:RPG will not function properly.",
			MTYPE_ERROR, dlerror());
		return ;
	}

	CBaseAnimating_Ignite_Addr = find_sym_addr(handle,
		"CBaseAnimating::Ignite", "_ZN14CBaseAnimating6IgniteEfbfb");

	CBaseEntity_Teleport_Addr = find_sym_addr(handle,
		"CBaseEntity::Teleport", "_ZN11CBaseEntity8TeleportEPK6VectorPK6QAngleS2_");

	CBaseCombatCharacter_Weapon_GetSlot_Addr = find_sym_addr(handle,
		"CBaseCombatCharacter::Weapon_GetSlot", "_ZNK20CBaseCombatCharacter14Weapon_GetSlotEi");

	CBaseCombatCharacter_GiveAmmo_Addr = find_sym_addr(handle,
		"CBaseCombatCharacter::GiveAmmo", "_ZN20CBaseCombatCharacter8GiveAmmoEiib");

	CBaseEntity_SetMoveType_Addr = find_sym_addr(handle,
		"CBaseEntity::SetMoveType", "_ZN11CBaseEntity11SetMoveTypeE10MoveType_t13MoveCollide_t");

	CBasePlayer_GiveNamedItem_Addr = find_sym_addr(handle,
		"CBasePlayer::GiveNamedItem", "_ZN11CBasePlayer13GiveNamedItemEPKci");

	CBaseEntity_CreateNoSpawn_Addr = find_sym_addr(handle,
		"CBaseEntity::CreateNoSpawn", "_ZN11CBaseEntity13CreateNoSpawnEPKcRK6VectorRK6QAnglePS_");

	DispatchSpawn_Addr = find_sym_addr(handle,
		"DispatchSpawn", "_Z13DispatchSpawnP11CBaseEntity");

	CBaseEntity_SetParent_Addr = find_sym_addr(handle,
		"CBaseEntity::SetParent", "_ZN11CBaseEntity9SetParentEPS_i");

	/* Get the tempents pointer */
	TE_Pointer_Linux = find_sym_addr(handle, "TempEnts Pointer", "te");

	dlclose(handle);
	return ;
}
#endif /* endif !WIN32 */

/*	//////////////////////////////////////
	Offsets and Stuff
	////////////////////////////////////// */
CRPG_FileVar CBE_DataDescMap_Offset("CBE_DataDescMap_Offset", "123", "offsets/CBE_DataDescMap");

/**
 * @brief ITempEntsSystem Hacked Pointer
 *
 * The ITempEntsSystem allows for cool effects to be used like explosions,
 * light beams, light rings, etc.
 * Credits go to Lance VOrgin/Mani for the code and offsets.
 *
 * @{
 */
CRPG_FileVar Offset_TE_VFunc("Offset_TE_VFunc", "12", "offsets/te_vfunc");
CRPG_FileVar Offset_TE_Code("Offset_TE_Code", "107", "offsets/te_code");

ITempEntsSystem* tempents = NULL;

void Initialize_TE_Pointer(class IEffects *effects) {
#ifdef WIN32
	tempents = **(ITempEntsSystem***)(*(unsigned long*)((*(unsigned long*)effects) + Offset_TE_VFunc.ULong()) + (Offset_TE_Code.ULong()));
#else
	tempents = *TE_Pointer_Linux;
#endif
	return ;
}
/** @} */

/*	//////////////////////////////////////
	CRPG_ExternProps Class
	////////////////////////////////////// */
struct CRPG_ExternProps::netp_s CRPG_ExternProps::netp = {-1};
struct CRPG_ExternProps::datap_s CRPG_ExternProps::datap = {-1};

/**
 * @brief 
 */
SendProp* CRPG_ExternProps::scan_sendtable(SendTable *tbl, char *name) {
	int i, count;
	SendProp *prop;

	count = tbl->GetNumProps();
	for(i = 0;i < count;i++) {
		prop = tbl->GetProp(i);
		if(!strcmp(prop->GetName(), name))
			return prop;
	}

	return NULL;
}

/**
 * @brief 
 */
void CRPG_ExternProps::print_sendtable(FILE *fptr, SendTable *tbl, int depth) {
	int i, count;
	int depth_inc;

	for(depth_inc = 0;depth_inc < depth;depth_inc++)
			fprintf(fptr, " ");

	fprintf(fptr, "Sub-Class Table (%d Deep): %s\n", depth+1, tbl->GetName());

	count = tbl->GetNumProps();
	for(i = 0;i < count;i++) {
		if(!strcmp(tbl->GetProp(i)->GetName(), "baseclass"))
			print_sendtable(fptr, tbl->GetProp(i)->GetDataTable(), depth+1);

		for(depth_inc = 0;depth_inc < depth;depth_inc++)
			fprintf(fptr, " ");

		fprintf(fptr, "- Member: %s\n", tbl->GetProp(i)->GetName());
	}

	return ;
}

/**
 * @brief External Property Paths
 *
 * @{
 */
CRPG_FileVar NP_m_iHealth("NP_m_iHealth", "CBasePlayer/m_iHealth", "extern_props/NP_m_iHealth");
CRPG_FileVar NP_m_nRenderMode("NP_m_nRenderMode", "CBaseEntity/m_nRenderMode", "extern_props/NP_m_nRenderMode");
CRPG_FileVar NP_m_clrRender("NP_m_clrRender", "CBaseEntity/m_clrRender", "extern_props/NP_m_clrRender");
CRPG_FileVar NP_m_nRenderFX("NP_m_nRenderFX", "CBaseEntity/m_nRenderFX", "extern_props/NP_m_nRenderFX");
CRPG_FileVar NP_m_ArmorValue("NP_m_ArmorValue", "CCSPlayer/m_ArmorValue", "extern_props/NP_m_ArmorValue");
CRPG_FileVar NP_m_fFlags("NP_m_fFlags", "CBasePlayer/m_fFlags", "extern_props/NP_m_fFlags");
/** @} */

#define CHECK_NETPROP(NAME) \
	if(netp.NAME < 0) \
		CRPG::ConsoleMsg("Unable to find %s network property", MTYPE_ERROR, #NAME);

/**
 * @brief 
 */
void CRPG_ExternProps::Init(IServerGameDLL *gamedll) {
	ServerClass *sc;

	WARN_IF(gamedll == NULL, return);

	sc = gamedll->GetAllServerClasses();

	netp.m_iHealth = FindNetProp(sc, NP_m_iHealth.String());
	CHECK_NETPROP(m_iHealth);

	netp.m_nRenderMode = FindNetProp(sc, NP_m_nRenderMode.String());
	CHECK_NETPROP(m_nRenderMode);

	netp.m_clrRender = FindNetProp(sc, NP_m_clrRender.String());
	CHECK_NETPROP(m_clrRender);

	netp.m_nRenderFX = FindNetProp(sc, NP_m_nRenderFX.String());
	CHECK_NETPROP(m_nRenderFX);

	netp.m_ArmorValue = FindNetProp(sc, NP_m_ArmorValue.String());
	CHECK_NETPROP(m_ArmorValue);

	netp.m_fFlags = FindNetProp(sc, NP_m_fFlags.String());
	CHECK_NETPROP(m_fFlags);

	return ;
}

/**
 * @brief 
 */
long CRPG_ExternProps::FindNetProp(ServerClass *sc, char *path) {
	unsigned int i, pathlen = strlen(path);
	char *name = (char*)calloc(pathlen+1, sizeof(char));
	SendTable *tbl;
	SendProp *prop;

	WARN_IF(name == NULL, return -1);

	strcpy(name, path);
	for(i = 0;i < pathlen;i++) {
		if(name[i] == '/')
			name[i] = '\0';
	}
    
	while(sc) {
		tbl = sc->m_pTable;

		if(strcmp(sc->GetName(), name)) {
			sc = sc->m_pNext;
			continue;
		}

		for(i = 0;(name[i] != '\0') && (i < pathlen);i++);

		i++; //increment passed the '\0' character
		if(i >= pathlen)
			return -1; //Invalid network property path

		while(tbl != NULL) {
			prop = scan_sendtable(tbl, name+i);
			if(prop == NULL)
				break;

			while((name[i] != '\0') && (i < pathlen))
				i++;

			i++; //increment passed the '\0' character
			if(i >= pathlen) {
				CRPG::DebugMsg("Found network property: %s", path);
				free(name);
				return prop->GetOffset();
			}

			tbl = prop->GetDataTable();
		}

		sc = sc->m_pNext;
	}

	free(name);
	return -1;
}

/**
 * @brief 
 */
long CRPG_ExternProps::FindDataProp(CBaseEntity *cbe, char *name) {
	return -1;
}

/**
 * @brief 
 */
void CRPG_ExternProps::DumpNetProps(FILE *fptr, ServerClass *sc) {
	while(sc) {
		fprintf(fptr, "Class Table: %s\n", sc->GetName());
		print_sendtable(fptr, sc->m_pTable, 0);
		sc = sc->m_pNext;
	}

	return ;
}

/**
 * @brief 
 */
void CRPG_ExternProps::DumpDataProps(FILE *fptr, CBaseEntity *cbe) {
}

/*	//////////////////////////////////////
	Hacked Functions
	////////////////////////////////////// */
void CBaseAnimating_Ignite(CBaseAnimating *cba, float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner) {
	#ifdef WIN32
	WARN_IF(!CBaseAnimating_Ignite_SigScan.is_set, return);

	typedef void (__fastcall *func)(CBaseAnimating*, void*, float, bool, float, bool);
	func thisfunc = (func)CBaseAnimating_Ignite_SigScan.sig_addr;
	thisfunc(cba, 0, flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner);

	#else

	WARN_IF(CBaseAnimating_Ignite_Addr == NULL, return);

	typedef void (*func)(CBaseAnimating*, float, bool, float, bool);
	func thisfunc = (func)CBaseAnimating_Ignite_Addr;
	thisfunc(cba, flFlameLifetime, (bool)bNPCOnly, flSize, (bool)bCalledByLevelDesigner);
	#endif

	return ;
}

void CBaseEntity_Teleport(CBaseEntity *cbe, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity) {
	#ifdef WIN32
	WARN_IF(!CBaseEntity_Teleport_SigScan.is_set, return);

	typedef void (__fastcall *func)(CBaseEntity*, void*, const Vector*, const QAngle*, const Vector*);
	func thisfunc = (func)CBaseEntity_Teleport_SigScan.sig_addr;
	thisfunc(cbe, 0, newPosition, newAngles, newVelocity);

	#else

	WARN_IF(CBaseEntity_Teleport_Addr == NULL, return);

	typedef void (*func)(CBaseEntity*, const Vector*, const QAngle*, const Vector*);
	func thisfunc = (func)CBaseEntity_Teleport_Addr;
	thisfunc(cbe, newPosition, newAngles, newVelocity);
	#endif

	return ;
}

CBaseCombatWeapon* CBaseCombatCharacter_Weapon_GetSlot(CBaseCombatCharacter *cbcc, int slot) {
	CBaseCombatWeapon *ret;

	#ifdef WIN32
	WARN_IF(!CBaseCombatCharacter_Weapon_GetSlot_SigScan.is_set, return NULL);

	typedef CBaseCombatWeapon* (__fastcall *func)(CBaseCombatCharacter*, void*, int);
	func thisfunc = (func)CBaseCombatCharacter_Weapon_GetSlot_SigScan.sig_addr;
	ret = thisfunc(cbcc, 0, slot);

	#else

	WARN_IF(CBaseCombatCharacter_Weapon_GetSlot_Addr == NULL, return NULL);

	typedef CBaseCombatWeapon* (*func)(CBaseCombatCharacter*, int);
	func thisfunc = (func)CBaseCombatCharacter_Weapon_GetSlot_Addr;
	ret = thisfunc(cbcc, slot);
	#endif

	return ret;
}

int CBaseCombatCharacter_GiveAmmo(CBaseCombatCharacter *cbcc, int iCount, int iAmmoIndex, bool bSuppressSound) {
	int ret;

	#ifdef WIN32
	WARN_IF(!CBaseCombatCharacter_GiveAmmo_SigScan.is_set, return 0);

	typedef int (__fastcall *func)(CBaseCombatCharacter*, void*, int, int, bool);
	func thisfunc = (func)CBaseCombatCharacter_GiveAmmo_SigScan.sig_addr;
	ret = thisfunc(cbcc, 0, iCount, iAmmoIndex, bSuppressSound);

	#else

	WARN_IF(CBaseCombatCharacter_GiveAmmo_Addr == NULL, return 0);

	typedef int (*func)(CBaseCombatCharacter*, int, int, bool);
	func thisfunc = (func)CBaseCombatCharacter_GiveAmmo_Addr;
	ret = thisfunc(cbcc, iCount, iAmmoIndex, bSuppressSound);
	#endif

	return ret;
}

void CBaseEntity_SetMoveType(CBaseEntity *cbe, MoveType_t val, MoveCollide_t moveCollide) {
	#ifdef WIN32
	WARN_IF(!CBaseEntity_SetMoveType_SigScan.is_set, return);

	typedef void (__fastcall *func)(CBaseEntity*, void*, MoveType_t val, MoveCollide_t moveCollide);
	func thisfunc = (func)CBaseEntity_SetMoveType_SigScan.sig_addr;
	thisfunc(cbe, 0, val, moveCollide);

	#else

	WARN_IF(CBaseEntity_SetMoveType_Addr == NULL, return);

	typedef void (*func)(CBaseEntity*, MoveType_t val, MoveCollide_t moveCollide);
	func thisfunc = (func)CBaseEntity_SetMoveType_Addr;
	thisfunc(cbe, val, moveCollide);
	#endif

	return ;
}

CBaseEntity* CBasePlayer_GiveNamedItem(CBasePlayer *cbp, const char *pszName, int iSubType) {
	CBaseEntity *ret;

	#ifdef WIN32
	WARN_IF(!CBasePlayer_GiveNamedItem_SigScan.is_set, return NULL);

	typedef CBaseEntity* (__fastcall *func)(CBasePlayer*, void*, const char*, int);
	func thisfunc = (func)CBasePlayer_GiveNamedItem_SigScan.sig_addr;
	ret = thisfunc(cbp, 0, pszName, iSubType);

	#else

	WARN_IF(CBasePlayer_GiveNamedItem_Addr == NULL, return NULL);

	typedef CBaseEntity* (*func)(CBasePlayer*, const char*, int);
	func thisfunc = (func)CBasePlayer_GiveNamedItem_Addr;
	ret = thisfunc(cbp, pszName, iSubType);
	#endif

	return ret;
}

CBaseEntity *CBaseEntity_CreateNoSpawn(const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner) {
	typedef CBaseEntity* (*func)(const char *, const Vector&, const QAngle&, CBaseEntity*);

	#ifdef WIN32
	WARN_IF(!CBaseEntity_CreateNoSpawn_SigScan.is_set, return NULL);
	func thisfunc = (func)CBaseEntity_CreateNoSpawn_SigScan.sig_addr;
	#else
	WARN_IF(CBaseEntity_CreateNoSpawn_Addr == NULL, return NULL);
	func thisfunc = (func)CBaseEntity_CreateNoSpawn_Addr;
	#endif

	return thisfunc(szName, vecOrigin, vecAngles, pOwner);
}

int DispatchSpawn(CBaseEntity *cbe) {
	typedef int (*func)(CBaseEntity*);

	#ifdef WIN32
	WARN_IF(!DispatchSpawn_SigScan.is_set, return -1);
	func thisfunc = (func)DispatchSpawn_SigScan.sig_addr;
	#else
	WARN_IF(DispatchSpawn_Addr == NULL, return -1);
	func thisfunc = (func)DispatchSpawn_Addr;
	#endif

	return thisfunc(cbe);
}

void CBaseEntity_SetParent(CBaseEntity *cbe, CBaseEntity *parent, int iAttachment) {
	#ifdef WIN32
	WARN_IF(!CBaseEntity_SetParent_SigScan.is_set, return);

	typedef void (__fastcall *func)(CBaseEntity*, void*, CBaseEntity*, int);
	func thisfunc = (func)CBaseEntity_SetParent_SigScan.sig_addr;
	thisfunc(cbe, 0, parent, iAttachment);

	#else

	WARN_IF(CBaseEntity_SetParent_Addr == NULL, return);

	typedef void (*func)(CBaseEntity*, CBaseEntity*, int);
	func thisfunc = (func)CBaseEntity_SetParent_Addr;
	thisfunc(cbe, parent, iAttachment);
	#endif

	return ;
}
