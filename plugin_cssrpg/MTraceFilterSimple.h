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

#include "vector.h"
#include "cmodel.h"
#include "utlvector.h"
#include "engine/IEngineTrace.h"
#include "engine/IStaticPropMgr.h"
#include "shared_classnames.h"

class MTraceFilterSimple : public CTraceFilter {
public:
	MTraceFilterSimple(const IHandleEntity *passentity, int collisionGroup);
	virtual bool ShouldHitEntity(IHandleEntity *pServerEntity, int contentsMask);

	virtual void SetPassEntity(const IHandleEntity *pPassEntity) {
		m_pPassEnt = pPassEntity;
	}

	const IHandleEntity *GetPassEntity(void){
		return m_pPassEnt;
	}

private:
	const IHandleEntity *m_pPassEnt;
	int m_collisionGroup;
};