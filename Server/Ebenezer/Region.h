#pragma once

#include "Define.h"
#include "GameDefine.h"
#include <shared-server/STLMap.h>

typedef CSTLMap <_ZONE_ITEM>	ZoneItemArray;
typedef CSTLMap <int>			ZoneUserArray;
typedef CSTLMap <int>			ZoneNpcArray;

class CRegion
{
public:
	ZoneItemArray	m_RegionItemArray;
	ZoneUserArray	m_RegionUserArray;
	ZoneNpcArray	m_RegionNpcArray;
};
