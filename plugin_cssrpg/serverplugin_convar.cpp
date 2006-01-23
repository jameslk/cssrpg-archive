//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

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

#include "icvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ICvar *s_pCVar;

class CPluginConVarAccessor : public IConCommandBaseAccessor {
public:
	virtual bool RegisterConCommandBase(ConCommandBase *pCommand) {
		pCommand->AddFlags(FCVAR_PLUGIN);

		// Unlink from plugin only list
		pCommand->SetNext(0);

		// Link to engine's list instead
		s_pCVar->RegisterConCommandBase(pCommand);
		return true;
	}
};

CPluginConVarAccessor g_ConVarAccessor;

void InitCVars(CreateInterfaceFn cvarFactory) {
	s_pCVar = (ICvar*)cvarFactory(VENGINE_CVAR_INTERFACE_VERSION, NULL);
	if (s_pCVar) {
		ConCommandBaseMgr::OneTimeInit(&g_ConVarAccessor);
	}
}

