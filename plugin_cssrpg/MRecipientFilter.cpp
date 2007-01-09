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

#include "MRecipientFilter.h" 
#include "interface.h" 
#include "filesystem.h" 
#include "engine/iserverplugin.h" 
#include "dlls/iplayerinfo.h" 
#include "eiface.h" 
#include "igameevents.h" 
#include "convar.h" 
#include "Color.h" 
 
#include "shake.h" 
#include "IEffects.h" 
#include "engine/IEngineSound.h" 

#include "cssrpg.h"
#include "cssrpg_interface.h"
 
// memdbgon must be the last include file in a .cpp file!!! 
#include "tier0/memdbgon.h" 
 
MRecipientFilter::MRecipientFilter(void) { 
} 
 
MRecipientFilter::~MRecipientFilter(void) { 
} 
 
int MRecipientFilter::GetRecipientCount() const { 
   return m_Recipients.Size(); 
} 
 
int MRecipientFilter::GetRecipientIndex(int slot) const { 
   if (slot < 0 || slot >= GetRecipientCount()) 
      return -1; 
 
   return m_Recipients[slot]; 
} 
 
bool MRecipientFilter::IsInitMessage() const { 
   return false; 
} 
 
bool MRecipientFilter::IsReliable() const { 
   return false; 
} 
 
void MRecipientFilter::AddAllPlayers(int maxClients) {
	edict_t *pPlayer;

	m_Recipients.RemoveAll();
	int i; 
	for (i = 1; i <= maxClients; i++) { 
		pPlayer = s_engine->PEntityOfEntIndex(i);
		if(!pPlayer || pPlayer->IsFree())
			continue;

		if(s_engine->GetPlayerUserId(pPlayer) < 0)
			continue;

		m_Recipients.AddToTail(i); 
	}

	return ;
}

void MRecipientFilter::AddRecipient(int index) {
   // Already in list
   if (m_Recipients.Find(index) != m_Recipients.InvalidIndex())
      return;
 
   m_Recipients.AddToTail(index);
}

void MRecipientFilter::RemoveAllRecipients(void) {
	m_Recipients.RemoveAll();
}

void MRecipientFilter::RemoveRecipient(int index) {
	// Remove it if it's in the list
	m_Recipients.FindAndRemove(index);
}
