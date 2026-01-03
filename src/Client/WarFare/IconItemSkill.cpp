#include "StdAfx.h"
#include "IconItemSkill.h"
#include "GameDef.h"
#include "N3UIWndBase.h"

uint32_t __IconItemSkill::GetItemID() const
{
	if (pItemBasic == nullptr)
		return 0;

	uint32_t nItemID = pItemBasic->dwID;
	if (pItemExt != nullptr)
		nItemID += pItemExt->dwID;

	return nItemID;
}

int __IconItemSkill::GetBuyPrice() const
{
	if (pItemBasic == nullptr || pItemExt == nullptr)
		return 0;

	return pItemBasic->iPrice * pItemExt->siPriceMultiply;
}

int __IconItemSkill::GetSellPrice(bool bHasPremium /*= false*/) const
{
	if (pItemBasic == nullptr || pItemExt == nullptr)
		return 0;

	constexpr int PREMIUM_RATIO = 4;
	constexpr int NORMAL_RATIO  = 6;

	int iSellPrice              = pItemBasic->iPrice * pItemExt->siPriceMultiply;

	if (pItemBasic->iSaleType != SALE_TYPE_FULL)
	{
		if (bHasPremium)
			iSellPrice /= PREMIUM_RATIO;
		else
			iSellPrice /= NORMAL_RATIO;
	}

	if (iSellPrice < 1)
		iSellPrice = 1;

	return iSellPrice;
}

bool __IconItemSkill::IsStackable() const
{
	if (pItemBasic == nullptr)
		return false;

	return pItemBasic->byContable == UIITEM_TYPE_COUNTABLE || pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL;
}
