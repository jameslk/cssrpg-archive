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

#ifndef _MRECIIENT_FILTER_H
#define _MRECIPIENT_FILTER_H
#include "irecipientfilter.h"
#include "bitvec.h"
#include "tier1/utlvector.h"
 
class MRecipientFilter: public IRecipientFilter {
public:
	MRecipientFilter(void);
	~MRecipientFilter(void);
	
	virtual bool IsReliable(void) const;
	virtual bool IsInitMessage(void) const;
	virtual int GetRecipientCount(void) const;
	virtual int GetRecipientIndex(int slot) const;
	void AddAllPlayers(int maxClients);
	void AddRecipient(int index);
	void RemoveAllRecipients(void);
	void RemoveRecipient(int index);

private:
	bool m_bReliable;
	bool m_bInitMessage;
	CUtlVector<int> m_Recipients;
};
 
#endif
