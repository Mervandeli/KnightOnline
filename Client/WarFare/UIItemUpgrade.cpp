// UIItemUpgrade.cpp: implementation of the CUIItemUpgrade class.
//Author : Monzantys(Mervan)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LocalInput.h"
#include "APISocket.h"
#include "GameProcMain.h"
#include "UIItemUpgrade.h"
#include "UIImageTooltipDlg.h"
#include "UIInventory.h"
#include "UIManager.h"
#include "PlayerMySelf.h"
#include "text_resources.h"
#include "UIHotKeyDlg.h"
#include "UISkillTreeDlg.h"
#include "resource.h"
#include <unordered_set>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUIItemUpgrade::CUIItemUpgrade()
{
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		m_pUpgradeScrollSlots[i] = nullptr;
	}
	for (int i = 0; i < 10; i++)
	{
		m_iUpgradeSlotInvPos[i] = -1;
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		m_pMyUpgradeInv[i] = nullptr;
		m_pBackupUpgradeInv[i] = nullptr;
	}

	m_pUpgradeResultSlot = nullptr;
	m_pUpgradeItemSlot = nullptr;
	m_pUITooltipDlg = nullptr;
	m_pStrMyGold = nullptr;

	m_eAnimationState = ANIM_NONE;
	m_fAnimationTimer = 0.0f;
	m_iCurrentFrame = 0;
	m_bUpgradeSucceeded = false;
	m_bUpgradeInProgress = false;
	m_iNpcID = 0;
}

CUIItemUpgrade::~CUIItemUpgrade()
{
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		DeleteIconItemSkill(m_pUpgradeScrollSlots[i]);
		m_iUpgradeSlotInvPos[i + 1] = -1; // 0 is UpgradeItemSlot [1,2..9] m_pUpgradeScrollSlots slot
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		DeleteIconItemSkill(m_pMyUpgradeInv[i]);
		DeleteIconItemSkill(m_pBackupUpgradeInv[i]);
	}
	DeleteIconItemSkill(m_pUpgradeItemSlot);
	m_iUpgradeSlotInvPos[0] = -1; //UpgradeItemSlot position

	m_pStrMyGold = nullptr;

	CN3UIWndBase::Release();
}

void CUIItemUpgrade::Release()
{
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		DeleteIconItemSkill(m_pUpgradeScrollSlots[i]);
		m_iUpgradeSlotInvPos[i+1] = -1; // 0 is UpgradeItemSlot [1,2..9] m_pUpgradeScrollSlots slot
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		DeleteIconItemSkill(m_pMyUpgradeInv[i]);
		DeleteIconItemSkill(m_pBackupUpgradeInv[i]);
	}
	DeleteIconItemSkill(m_pUpgradeItemSlot);
	m_iUpgradeSlotInvPos[0] = -1; //UpgradeItemSlot position

	m_pStrMyGold = nullptr;

	CN3UIWndBase::Release();
}

void CUIItemUpgrade::Tick()
{
	if (m_pImageCover1 != nullptr && m_pImageCover2 != nullptr)
	{
		switch (m_eAnimationState)
		{
			case ANIM_START:
				StartUpgradeAnim();
				break;
			case ANIM_FLIPFLOP:
				UpdateFlipFlopAnimation();
				break;
			case ANIM_RESULT:
				ShowResultUpgrade();
				break;
			case ANIM_COVER_OPENING:
				UpdateCoverAnimation();
				break;
			case ANIM_NONE:
				m_fAnimationTimer = 0.0f;
				m_iCurrentFrame = 0;
				break;
		}
	}
	CN3UIBase::Tick();
}

void CUIItemUpgrade::Render()
{
	if (!m_bVisible) 
		return;

	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();

	if (m_pUITooltipDlg != nullptr)
		m_pUITooltipDlg->DisplayTooltipsDisable();

	bool bTooltipRender = false;
	__IconItemSkill* spItem = nullptr;

	for (UIListReverseItor itor = m_Children.rbegin(); m_Children.rend() != itor; ++itor)
	{
		CN3UIBase* pChild = (*itor);
		if ((GetState() == UI_STATE_ICON_MOVING) && (pChild->UIType() == UI_TYPE_ICON) && (CN3UIWndBase::m_sSelectedIconInfo.pItemSelect) &&
			((CN3UIIcon*) pChild == CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon))
			continue;
		pChild->Render();

		if ((GetState() == UI_STATE_COMMON_NONE) &&
			(pChild->UIType() == UI_TYPE_ICON) && (pChild->GetStyle() & UISTYLE_ICON_HIGHLIGHT))
		{
			bTooltipRender = true;
			spItem = GetHighlightIconItem((CN3UIIcon*) pChild);
		}
	}

	// Display the count for items that should show a count.
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		__IconItemSkill* spItem = m_pMyUpgradeInv[i];
		if (spItem != nullptr && ((spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE) ||
			(spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL)))
		{
			std::string szID = fmt::format("s_count_{}", i);
			CN3UIString* pStr = (CN3UIString*)GetChildByID(szID);
			if (pStr != nullptr)
			{
				if ((GetState() == UI_STATE_ICON_MOVING) && (m_pMyUpgradeInv[i] == CN3UIWndBase::m_sSelectedIconInfo.pItemSelect))
				{
					pStr->SetVisible(false);
				}
				else
				{
					if (m_pMyUpgradeInv[i]->pUIIcon->IsVisible())
					{
						pStr->SetVisible(true);
						pStr->SetStringAsInt(m_pMyUpgradeInv[i]->iCount);
						pStr->Render();
					}
					else
					{
						pStr->SetVisible(false);
					}
				}
			}
		}
		else
		{
			std::string szID = fmt::format("s_count_{}", i);
			CN3UIString* pStr = (CN3UIString*)GetChildByID(szID);
			if (pStr != nullptr)
				pStr->SetVisible(false);
		}
	}

	if ((GetState() == UI_STATE_ICON_MOVING) && (CN3UIWndBase::m_sSelectedIconInfo.pItemSelect))
		CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon->Render();

	if (spItem != nullptr)
	{
		m_pUITooltipDlg->DisplayTooltipsEnable(ptCur.x, ptCur.y, spItem, false, false);
	}
}

void CUIItemUpgrade::InitIconWnd(e_UIWND eWnd)
{
	__TABLE_UI_RESRC* pTbl = CGameBase::s_pTbl_UI.Find(CGameBase::s_pPlayer->m_InfoBase.eNation);

	m_pUITooltipDlg = new CUIImageTooltipDlg();
	m_pUITooltipDlg->Init(this);
	m_pUITooltipDlg->LoadFromFile(pTbl->szItemInfo);
	m_pUITooltipDlg->InitPos();
	m_pUITooltipDlg->SetVisible(false);

	CN3UIWndBase::InitIconWnd(eWnd);

	N3_VERIFY_UI_COMPONENT(m_pStrMyGold, (CN3UIString*) GetChildByID("text_gold"));
	if (m_pStrMyGold != nullptr) 
		m_pStrMyGold->SetString("0");
}

__IconItemSkill* CUIItemUpgrade::GetHighlightIconItem(CN3UIIcon* pUIIcon)
{
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr && m_pUpgradeScrollSlots[i]->pUIIcon == pUIIcon)
			return m_pUpgradeScrollSlots[i];
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr && m_pMyUpgradeInv[i]->pUIIcon == pUIIcon)
			return m_pMyUpgradeInv[i];
	}

	if (m_pUpgradeItemSlot != nullptr && m_pUpgradeItemSlot->pUIIcon == pUIIcon)
		return m_pUpgradeItemSlot;

	return nullptr;
}

void CUIItemUpgrade::Open()
{
	SetVisibleWithNoSound(true,false,false);

	GoldUpdate();
}

void CUIItemUpgrade::SetNpcID(int iNpcID)
{
	m_iNpcID = iNpcID;
}

void CUIItemUpgrade::GoldUpdate()
{
	if (m_pStrMyGold != nullptr)
	{
		m_pStrMyGold->SetString(CGameBase::FormatNumber(CGameBase::s_pPlayer->m_InfoExt.iGold));
	}
}

void CUIItemUpgrade::ItemMoveFromInvToThis()
{
	CUIInventory* pInven = CGameProcedure::s_pProcMain->m_pUIInventory;
	if (pInven == nullptr)
		return;

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (pInven->m_pMyInvWnd[i] != nullptr)
		{
			__IconItemSkill* spItem = pInven->m_pMyInvWnd[i];
			spItem->pUIIcon->SetParent(this);

			pInven->m_pMyInvWnd[i] = nullptr;
			CN3UIArea* pArea;

			std::string szID = fmt::format("a_slot_{}", i);
			pArea = (CN3UIArea*) GetChildByID(szID);
			if (pArea != nullptr)
			{
				spItem->pUIIcon->SetRegion(pArea->GetRegion());
				spItem->pUIIcon->SetMoveRect(pArea->GetRegion());
			}

			m_pMyUpgradeInv[i] = spItem;

			m_pBackupUpgradeInv[i] = new __IconItemSkill(*m_pMyUpgradeInv[i]);// Backup the inventory state for restoration if needed.
		}
		else
		{
			m_pMyUpgradeInv[i] = nullptr;
			m_pBackupUpgradeInv[i] = nullptr;
		}

	}
}

void CUIItemUpgrade::Close()
{
	bool bwork = IsVisible();
	SetVisibleWithNoSound(false, bwork,false);

	if (GetState() == UI_STATE_ICON_MOVING)
		IconRestore();
	SetState(UI_STATE_COMMON_NONE);
	CN3UIWndBase::AllHighLightIconFree();
}

void CUIItemUpgrade::ItemMoveFromThisToInv()
{
	CUIInventory* pInven = CGameProcedure::s_pProcMain->m_pUIInventory;
	if (pInven == nullptr)
		return;

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		__IconItemSkill* spItem = m_pMyUpgradeInv[i];
		if (spItem != nullptr)
		{
			spItem->pUIIcon->SetParent(pInven);

			m_pMyUpgradeInv[i] = nullptr;

			CN3UIArea* pArea;

			pArea = pInven->GetChildAreaByiOrder(UI_AREA_TYPE_INV, i);
			if (pArea != nullptr)
			{
				spItem->pUIIcon->SetRegion(pArea->GetRegion());
				spItem->pUIIcon->SetMoveRect(pArea->GetRegion());
			}
			pInven->m_pMyInvWnd[i] = spItem;
		}
	}
}

bool CUIItemUpgrade::ReceiveIconDrop(__IconItemSkill* spItem, POINT ptCur)
{

	CN3UIArea* pArea;
	e_UIWND_DISTRICT eUIWnd = UIWND_DISTRICT_UNKNOWN;
	if (!m_bVisible)
		return false;

	//  Check if the selected window is correct and the drop is valid.
	if (CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWnd != m_eUIWnd)
		return false;
	if ((CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict != UIWND_DISTRICT_UPGRADE_SLOT) &&
		(CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict != UIWND_DISTRICT_UPGRADE_INV))
		return false;

	// Find which slot or area the item is being dropped onto.
	int iDestiOrder = -1;
	bool bFound = false;
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		std::string szID = fmt::format("a_m_{}", i);
		pArea = (CN3UIArea*)GetChildByID(szID);
		if (pArea != nullptr && pArea->IsIn(ptCur.x, ptCur.y))
		{
			bFound = true;
			eUIWnd = UIWND_DISTRICT_UPGRADE_SLOT;
			iDestiOrder = i;
			break;
		}
	}

	CN3UIWndBase::m_sSelectedIconInfo.pItemSelect = spItem;

	CN3UIWndBase::m_sRecoveryJobInfo.UIWndSourceEnd.UIWndDistrict = eUIWnd;

	switch (CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict)
	{
		case UIWND_DISTRICT_UPGRADE_INV:
			if (eUIWnd == UIWND_DISTRICT_UPGRADE_SLOT)
			{
				if (iDestiOrder != -1 && m_pUpgradeScrollSlots[iDestiOrder] == nullptr)
				{
					int iSourceOrder = GetItemiOrder(spItem, UIWND_DISTRICT_UPGRADE_INV);
					if (iSourceOrder != -1)
					{
						__IconItemSkill* pSrc = m_pMyUpgradeInv[iSourceOrder];
						if (!HandleSlotDrop(pSrc, iDestiOrder))
							return false;
					}
				}
				return false;
			}
			else if (m_pAreaUpgrade != nullptr && m_pAreaUpgrade->IsIn(ptCur.x, ptCur.y))
			{
				// Handle dropping item into the main upgrade area (a_upgrade)	
				if (HandleUpgradeAreaDrop(spItem))
				{
					CN3UIWndBase::AllHighLightIconFree();
					SetState(UI_STATE_COMMON_NONE);
					return true;
				}
			}
			break;
	}

	CN3UIWndBase::AllHighLightIconFree();
	SetState(UI_STATE_COMMON_NONE);

	return false;
}

void CUIItemUpgrade::CancelIconDrop(__IconItemSkill* spItem)
{
	CN3UIWndBase::AllHighLightIconFree();
	SetState(UI_STATE_COMMON_NONE);
}

void CUIItemUpgrade::AcceptIconDrop(__IconItemSkill* spItem)
{
	CN3UIWndBase::AllHighLightIconFree();
	SetState(UI_STATE_COMMON_NONE);
}

// Restores the icon's position to its original inventory area.
void CUIItemUpgrade::IconRestore()
{
	CN3UIArea* pArea;

	if (CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict == UIWND_DISTRICT_UPGRADE_INV)
	{
		if (m_pMyUpgradeInv[CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.iOrder] != nullptr)
		{
			std::string szID = fmt::format("a_slot_{}", CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.iOrder);
			pArea = (CN3UIArea*)GetChildByID(szID);
			if (pArea != nullptr)
			{
				m_pMyUpgradeInv[CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.iOrder]->pUIIcon->SetRegion(pArea->GetRegion());
				m_pMyUpgradeInv[CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.iOrder]->pUIIcon->SetMoveRect(pArea->GetRegion());
			}
		}
	}
}

uint32_t CUIItemUpgrade::MouseProc(uint32_t dwFlags, const POINT& ptCur, const POINT& ptOld)
{
	
	uint32_t dwRet = UI_MOUSEPROC_NONE;
	HandleInventoryIconRightClick(ptCur , dwFlags);
	if (!m_bVisible)
		return dwRet;

	if ((GetState() == UI_STATE_ICON_MOVING) &&
		(CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWnd == UIWND_UPGRADE))
	{
		CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon->SetRegion(GetSampleRect());
		CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon->SetMoveRect(GetSampleRect());
	}

	return CN3UIWndBase::MouseProc(dwFlags, ptCur, ptOld);
}

// Returns the index of the given item in the specified window district.
int CUIItemUpgrade::GetItemiOrder(__IconItemSkill* spItem, e_UIWND_DISTRICT eWndDist) const
{
	int iReturn = -1;

	switch (eWndDist)
	{
		case UIWND_DISTRICT_UPGRADE_SLOT:
			for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
			{
				if (m_pUpgradeScrollSlots[i] != nullptr && m_pUpgradeScrollSlots[i] == spItem)
					return i;
			}
			break;

		case UIWND_DISTRICT_UPGRADE_INV:
			for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
			{
				if ((m_pMyUpgradeInv[i] != nullptr) && (m_pMyUpgradeInv[i] == spItem))
					return i;
			}
			break;
	}

	return iReturn;
}

// Returns a rectangle centered at the mouse position, used for moving icons.
RECT CUIItemUpgrade::GetSampleRect()
{
	RECT rect;
	CN3UIArea* pArea;
	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	pArea = (CN3UIArea*)GetChildByID("a_slot_0");
	rect = pArea->GetRegion();
	float fWidth = (float) (rect.right - rect.left);
	float fHeight = (float) (rect.bottom - rect.top);
	fWidth *= 0.5f; fHeight *= 0.5f;
	rect.left = ptCur.x - (int) fWidth;  rect.right = ptCur.x + (int) fWidth;
	rect.top = ptCur.y - (int) fHeight; rect.bottom = ptCur.y + (int) fHeight;
	return rect;
}

// Determines which window district (slot or inventory) the given item belongs to.
e_UIWND_DISTRICT CUIItemUpgrade::GetWndDistrict(__IconItemSkill* spItem)
{
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr && m_pUpgradeScrollSlots[i] == spItem)
			return UIWND_DISTRICT_UPGRADE_SLOT;
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if ((m_pMyUpgradeInv[i] != nullptr) && (m_pMyUpgradeInv[i] == spItem))
			return UIWND_DISTRICT_UPGRADE_INV;
	}
	return UIWND_DISTRICT_UNKNOWN;
}

// Handles UI messages such as button clicks and icon events.
bool CUIItemUpgrade::ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg)
{

	if (pSender == nullptr)
		return false;

	if (dwMsg == UIMSG_BUTTON_CLICK)
	{
		if (pSender == m_pBtnClose)
			Close();
		else if (pSender == m_pBtnCancel && !m_bUpgradeInProgress)
			RestoreInventoryFromBackup();
		else if (pSender == m_pBtnOk && !m_bUpgradeInProgress)
		{
			SendToServerUpgradeMsg();
		}
	}

	__IconItemSkill* spItem = nullptr;
	e_UIWND_DISTRICT eUIWnd;
	int iOrder;

	uint32_t dwBitMask = 0x000f0000;

	switch (dwMsg & dwBitMask)
	{
		case UIMSG_ICON_DOWN_FIRST:
			CN3UIWndBase::AllHighLightIconFree();

			// Get the item being interacted with.
			spItem = GetHighlightIconItem((CN3UIIcon*) pSender);

			// Save Select Info..
			CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWnd = UIWND_UPGRADE;
			eUIWnd = GetWndDistrict(spItem);
			if (eUIWnd == UIWND_DISTRICT_UPGRADE_SLOT)
				return false;
			if (eUIWnd == UIWND_DISTRICT_UNKNOWN)	
				return false;
			CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict = eUIWnd;
			iOrder = GetItemiOrder(spItem, eUIWnd);
			if (iOrder == -1)	
				return false;
			CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.iOrder = iOrder;
			CN3UIWndBase::m_sSelectedIconInfo.pItemSelect = spItem;
			// Set icon region for moving.
			((CN3UIIcon*) pSender)->SetRegion(GetSampleRect());
			((CN3UIIcon*) pSender)->SetMoveRect(GetSampleRect());
			// Play item sound.
			if (spItem != nullptr) PlayItemSound(spItem->pItemBasic);
			break;

		case UIMSG_ICON_UP:
			if (!CGameProcedure::s_pUIMgr->BroadcastIconDropMsg(CN3UIWndBase::m_sSelectedIconInfo.pItemSelect))

				// Restore the icon position to its original place if drop failed.
				IconRestore();
			break;

		case UIMSG_ICON_DOWN:
			if (GetState() == UI_STATE_ICON_MOVING)
			{
				CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon->SetRegion(GetSampleRect());
				CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon->SetMoveRect(GetSampleRect());
			}
			break;
	}

	return true;
}


void CUIItemUpgrade::SetVisibleWithNoSound(bool bVisible, bool bWork, bool bReFocus)
{
	CN3UIBase::SetVisibleWithNoSound(bVisible, bWork, bReFocus);

	if (bVisible)
	{
		CGameProcedure::s_pUIMgr->SetVisibleFocusedUI(this);
		ItemMoveFromInvToThis();
	}

	if (bWork && !bVisible)
	{

		if (GetState() == UI_STATE_ICON_MOVING)
			IconRestore();
		SetState(UI_STATE_COMMON_NONE);
		CN3UIWndBase::AllHighLightIconFree();

		//Move the items from this window's inventory area to the inventory area of this inventory window.
		RestoreInventoryFromBackup();
		ItemMoveFromThisToInv();
		AnimClose();
		
		if (m_pUITooltipDlg != nullptr) m_pUITooltipDlg->DisplayTooltipsDisable();
	}
}

// Loads the UI from file and initializes all required UI components.
bool CUIItemUpgrade::Load(HANDLE hFile)
{
	if (CN3UIBase::Load(hFile) == false)
		return false;

	N3_VERIFY_UI_COMPONENT(m_pBtnClose, (CN3UIButton*) this->GetChildByID("btn_close"));
	N3_VERIFY_UI_COMPONENT(m_pBtnOk, (CN3UIButton*) this->GetChildByID("btn_ok"));
	N3_VERIFY_UI_COMPONENT(m_pBtnCancel, (CN3UIButton*) this->GetChildByID("btn_cancel"));
	N3_VERIFY_UI_COMPONENT(m_pBtnConversation, (CN3UIButton*) this->GetChildByID("btn_conversation"));
	N3_VERIFY_UI_COMPONENT(m_pAreaUpgrade, (CN3UIArea*) this->GetChildByID("a_upgrade"));
	N3_VERIFY_UI_COMPONENT(m_pAreaResult, (CN3UIArea*) this->GetChildByID("a_result"));

	this->GetChildByID("img_cover_01")->SetVisible(false);
	this->GetChildByID("img_cover_02")->SetVisible(false);
	
	for (int i = 0; i < 20; ++i)
	{
		std::string szID;
		szID = fmt::format("img_s_load_{}", i);
		if (CN3UIImage* pImg = (CN3UIImage*) GetChildByID(szID))
			pImg->SetVisible(false);
		szID = fmt::format("img_f_load_{}", i);
		if (CN3UIImage* pImg = (CN3UIImage*) GetChildByID(szID))
			pImg->SetVisible(false);
	}
	return true;
}

// Handles key press events, such as closing the UI with ESC.
bool CUIItemUpgrade::OnKeyPress(int iKey)
{
	switch (iKey)
	{
		case DIK_ESCAPE:
			ReceiveMessage(m_pBtnClose, UIMSG_BUTTON_CLICK);
			return true;
	}

	return CN3UIBase::OnKeyPress(iKey);
}

void CUIItemUpgrade::UpdateBackupUpgradeInv()
{
	// Clear existing backup
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		m_pBackupUpgradeInv[i] = nullptr;
	}

	// Create new backup from current inventory
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr)
		{
			m_pBackupUpgradeInv[i] = new __IconItemSkill(*m_pMyUpgradeInv[i]);
		}
	}
}

// Restores the inventory and slots from the backup, recreating icons as needed.
void CUIItemUpgrade::RestoreInventoryFromBackup()
{

	//Clear existing slots first
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		DeleteIconItemSkill(m_pMyUpgradeInv[i]);
	}

	DeleteIconItemSkill(m_pUpgradeItemSlot);

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr)
			DeleteIconItemSkill(m_pUpgradeScrollSlots[i]);

	}

	// Restore items from backup
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pBackupUpgradeInv[i] != nullptr)
		{
			m_pMyUpgradeInv[i] = new __IconItemSkill(*m_pBackupUpgradeInv[i]);

			//If the icon file name is not empty, create a new UI icon
			if (m_pMyUpgradeInv[i]->pUIIcon != nullptr)
			{
				CreateUIIconForItem(m_pMyUpgradeInv[i]);

				//Set the UI position based on the inventory area
				std::string szID = fmt::format("a_slot_{}", i);
				CN3UIArea* pArea = (CN3UIArea*)GetChildByID(szID);
				SetupIconArea(m_pMyUpgradeInv[i], pArea);
			}
		}
	}
}

// Checks if the given item ID is an upgrade scroll.
bool CUIItemUpgrade::IsUpgradeScroll(uint32_t dwEffectID2) const
{
	if (dwEffectID2 == 255)
		return true;

	return false;
}

bool CUIItemUpgrade::IsTrina(uint32_t dwID) const
{
	static const std::unordered_set<int> upgradeItemIDs = {
	// Trina
	379256000, 379257000, 379258000, 700002000
	};

	return upgradeItemIDs.contains(dwID);
}

// Checks if the given item is allowed to be upgraded (unique or upgrade type).
bool CUIItemUpgrade::IsAllowedUpgradeItem(__IconItemSkill* spItem) const
{
	if (m_pUpgradeItemSlot != nullptr)
		return false;
	if (spItem != nullptr  && spItem->pItemBasic != nullptr)
	{

		if (spItem->pItemBasic->byAttachPoint == ITEM_POS_FINGER // Ring
			|| spItem->pItemBasic->byAttachPoint == ITEM_POS_NECK // Necklace
			|| spItem->pItemBasic->byAttachPoint == ITEM_POS_BELT // Belt
			|| spItem->pItemBasic->byAttachPoint == ITEM_POS_EAR) // Earring
		{
			return false;
		}
	}
	e_ItemAttrib eTA = (e_ItemAttrib) (spItem->pItemExt->byMagicOrRare);
	return (eTA == ITEM_ATTRIB_UNIQUE || eTA == ITEM_ATTRIB_UPGRADE || eTA == ITEM_ATTRIB_UNIQUE_REVERSE || eTA == ITEM_ATTRIB_UPGRADE);
}

// Deletes the given icon item skill and its UI icon
void CUIItemUpgrade::DeleteIconItemSkill(__IconItemSkill*& pItem)
{
	if (pItem != nullptr)
	{
		if (pItem->pUIIcon != nullptr)
		{
			delete pItem->pUIIcon;
			pItem->pUIIcon = nullptr;
		}
		delete pItem;
		pItem = nullptr;
	}
}

void CUIItemUpgrade::SendToServerUpgradeMsg()
{
	if (m_pUpgradeItemSlot == nullptr || m_pUpgradeScrollSlots == nullptr)
		return;
	m_bUpgradeInProgress = true;
	uint8_t byBuff[512];
	int iOffset = 0;
	int itotalSent = 0;

	CAPISocket::MP_AddByte(byBuff, iOffset, WIZ_ITEM_UPGRADE);
	CAPISocket::MP_AddByte(byBuff, iOffset, ITEM_UPGRADE_PROCESS);
	CAPISocket::MP_AddByte(byBuff, iOffset, 1);
	CAPISocket::MP_AddByte(byBuff, iOffset, m_iNpcID);

	int32_t nItemID;
	int8_t bPos;

	// Add Upgrade Item
	if (m_pUpgradeItemSlot != nullptr)
	{
		nItemID= m_pUpgradeItemSlot->pItemBasic->dwID + m_pUpgradeItemSlot->pItemExt->dwID;
		bPos= m_iUpgradeSlotInvPos[0]; // m_pUpgradeItemSlot position

		CAPISocket::MP_AddDword(byBuff, iOffset, nItemID);
		CAPISocket::MP_AddByte(byBuff, iOffset, bPos);
		++itotalSent;
	}

	// Add Upgrade Slots
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr)
		{
			nItemID= m_pUpgradeScrollSlots[i]->pItemBasic->dwID +
				m_pUpgradeScrollSlots[i]->pItemExt->dwID;
			bPos = m_iUpgradeSlotInvPos[i+1];
			CAPISocket::MP_AddDword(byBuff, iOffset, nItemID);
			CAPISocket::MP_AddByte(byBuff, iOffset, bPos);
			++itotalSent;
		}
	}

	nItemID = 0;
	bPos = -1;
	while (itotalSent<10)
	{
		CAPISocket::MP_AddDword(byBuff, iOffset, nItemID);
		CAPISocket::MP_AddByte(byBuff, iOffset, bPos);
		++itotalSent;
	}

	CGameProcedure::s_pSocket->Send(byBuff, iOffset);
}

void CUIItemUpgrade::MsgRecv_ItemUpgrade(Packet& pkt)
{
	int8_t result = pkt.read<uint8_t>();
	uint32_t nItemID[10];
	uint8_t bPos[10];
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT + 1; i++)
	{
		pkt >> nItemID[i];
		pkt >> bPos[i];
	}

	__TABLE_ITEM_EXT* itemExt = nullptr;
	__TABLE_ITEM_BASIC* itemBasic = nullptr;
	e_PartPosition ePart;
	e_PlugPosition ePlug;
	std::string szIconFN;
	float fUVAspect = (float) 45.0f / (float) 64.0f;
	std::string szMsg;
		// Clean upgrade scroll slots
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
	{
		DeleteIconItemSkill(m_pUpgradeScrollSlots[i]);
	}

	// Clean  item upgrade slot
	DeleteIconItemSkill(m_pUpgradeItemSlot);

	if (result == 0 || result == 1)
	{
		m_eAnimationState = ANIM_START;

		if (result == 0)
		{
			m_bUpgradeSucceeded = false;
			szMsg = fmt::format_text_resource(6701);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(255, 0, 255, 255));
		}
		else if (result == 1)
		{
			m_bUpgradeSucceeded = true;
			szMsg = fmt::format_text_resource(6700);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(128, 128, 255, 255));

			itemBasic = CGameBase::s_pTbl_Items_Basic.Find(nItemID[0] / 1000 * 1000);
			if (itemBasic && itemBasic->byExtIndex >= 0 && itemBasic->byExtIndex <= MAX_ITEM_EXTENSION)
				itemExt = CGameBase::s_pTbl_Items_Exts[itemBasic->byExtIndex].Find(nItemID[0] % 1000);
			else
				itemExt = nullptr;
			e_ItemType eType = CGameBase::MakeResrcFileNameForUPC(itemBasic, itemExt, nullptr, &szIconFN, ePart, ePlug, CGameBase::s_pPlayer->m_InfoBase.eRace);
			if (ITEM_TYPE_UNKNOWN == eType)
				return;
			__IconItemSkill* spItemNew;
			spItemNew = new __IconItemSkill;
			spItemNew->pItemBasic = itemBasic;
			spItemNew->pItemExt = itemExt;
			spItemNew->szIconFN = szIconFN;
			spItemNew->iCount = 1;
			CreateUIIconForItem(spItemNew, szIconFN);

			if (m_pAreaResult != nullptr)
			{
				m_pUpgradeResultSlot = spItemNew;
				m_pMyUpgradeInv[bPos[0]] = m_pUpgradeResultSlot;
			}
		}
	}
	else
	{
		m_bUpgradeInProgress = false;
		RestoreInventoryFromBackup();
		if (result == 2)
		{
			//cannot perform item upgrade
			szMsg = fmt::format_text_resource(6702);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(255, 0, 255, 255));

		}
		else if (result == 3)
		{
			//Upgrade Need coin
			szMsg = fmt::format_text_resource(6703);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(255, 0, 255, 255));
		}
		else if (result == 4)
		{
			//Upgrade Not Match or other
			szMsg = fmt::format_text_resource(6704);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(255, 0, 255, 255));
		}
	}

	UpdateBackupUpgradeInv();
	GoldUpdate();
	CN3UIWndBase::AllHighLightIconFree();
	SetState(UI_STATE_COMMON_NONE);

}

void CUIItemUpgrade::UpdateCoverAnimation()
{
    m_fAnimationTimer += CN3Base::s_fSecPerFrm;
	static const float COVER_ANIMATION_DURATION = 0.8f;
    float t = m_fAnimationTimer / COVER_ANIMATION_DURATION;
    if (t > 1.0f) t = 1.0f;

    // Only handle opening (covers move outward and hide)
    float ease = 1.0f - ((1.0f - t) * (1.0f - t));
	int m_iCoverShift = m_rcCover1Original.bottom - m_rcCover1Original.top;
    // Calculate new Y positions for opening
    int y1 = m_rcCover1Original.top + (int)((m_rcCover1Original.top - m_iCoverShift - m_rcCover1Original.top) * ease);
    int y2 = m_rcCover2Original.top + (int)((m_rcCover2Original.top + m_iCoverShift - m_rcCover2Original.top) * ease);

    // Animation completed - hide covers and reset positions
    if (t >= 1.0f)
    {
        m_eAnimationState = ANIM_NONE;
        m_pImageCover1->SetVisible(false);
        m_pImageCover2->SetVisible(false);
        m_pImageCover1->SetRegion(m_rcCover1Original);
        m_pImageCover2->SetRegion(m_rcCover2Original);
		m_bUpgradeInProgress = false;
        return;
    }

    // Update cover regions
    RECT rc1 = m_rcCover1Original;
    RECT rc2 = m_rcCover2Original;
    int height1 = rc1.bottom - rc1.top;
    int height2 = rc2.bottom - rc2.top;
    rc1.top = y1;
    rc1.bottom = y1 + height1;
    rc2.top = y2;
    rc2.bottom = y2 + height2;
    m_pImageCover1->SetRegion(rc1);
    m_pImageCover2->SetRegion(rc2);
}

void CUIItemUpgrade::UpdateFlipFlopAnimation()
{
	m_fAnimationTimer += CN3Base::s_fSecPerFrm;
	static const float FLIPFLOP_FRAME_DELAY = 0.1f;
	static const int FLIPFLOP_MAX_FRAMES = 20;

	if (m_fAnimationTimer >= FLIPFLOP_FRAME_DELAY)
	{
		m_fAnimationTimer -= FLIPFLOP_FRAME_DELAY;
		m_iCurrentFrame++;

		if (m_iCurrentFrame >= FLIPFLOP_MAX_FRAMES)
		{
			HideAllAnimationFrames();
			
			m_eAnimationState = ANIM_RESULT;
			m_fAnimationTimer = 0.0f;
		}
		else
		{
			FlipFlopAnim();
		}
	}
}

void CUIItemUpgrade::HideAllAnimationFrames()
{
    // Hide all img_s_load_X frames
    for (int i = 0;; ++i)
    {
		std::string szID = fmt::format("img_s_load_{}", i);
        CN3UIImage* pImg = (CN3UIImage*)GetChildByID(szID);
        if (pImg == nullptr) break;
        pImg->SetVisible(false);
    }
    // Hide all img_f_load_X frames
    for (int i = 0;; ++i)
    {
		std::string szID = fmt::format("img_f_load_{}", i);
        CN3UIImage* pImg = (CN3UIImage*)GetChildByID(szID);
        if (pImg == nullptr) break;
        pImg->SetVisible(false);
    }
}

void CUIItemUpgrade::CreateUIIconForItem(__IconItemSkill* pItem, const std::string& szIconFN)
{
	if (pItem == nullptr)
		return;

	pItem->pUIIcon = new CN3UIIcon;
	pItem->pUIIcon->Init(this);

	static const float UV_ASPECT_RATIO = 45.0f / 64.0f;
	std::string iconFile = szIconFN.empty() ? pItem->szIconFN : szIconFN;
	pItem->pUIIcon->SetTex(iconFile);
	pItem->pUIIcon->SetUVRect(0, 0, UV_ASPECT_RATIO, UV_ASPECT_RATIO);
	pItem->pUIIcon->SetUIType(UI_TYPE_ICON);
	pItem->pUIIcon->SetStyle(UISTYLE_ICON_ITEM | UISTYLE_ICON_CERTIFICATION_NEED);
	pItem->pUIIcon->SetVisible(true);
}

__IconItemSkill* CUIItemUpgrade::CreateIconFromSource(const __IconItemSkill* pSrc, int count)
{
	if (pSrc == nullptr)
		return nullptr;	

	__IconItemSkill* pNew = new __IconItemSkill(*pSrc);
	pNew->iCount = count;
	CreateUIIconForItem(pNew);
	return pNew;
}

void CUIItemUpgrade::SetupIconArea(__IconItemSkill* pItem, CN3UIArea* pArea)
{
	if (pItem == nullptr || pItem->pUIIcon == nullptr || pArea == nullptr)
		return;

	pItem->pUIIcon->SetRegion(pArea->GetRegion());
	pItem->pUIIcon->SetMoveRect(pArea->GetRegion());
}

bool CUIItemUpgrade::HandleUpgradeAreaDrop(__IconItemSkill* spItem)
{
	// Only Upgrade and Unique items can be dropped here
	if (!IsAllowedUpgradeItem(spItem))
		return false;

	// Move the item to the upgrade slot
	m_pUpgradeItemSlot = spItem;

	// Remove the item from inventory
	for (int i = 0; i < MAX_ITEM_INVENTORY; ++i)
	{
		if (m_pMyUpgradeInv[i] == spItem)
		{
			m_pMyUpgradeInv[i] = nullptr;
			m_iUpgradeSlotInvPos[0] = i; // m_pUpgradeItemSlot position
			break;
		}
	}

	// Update the item's UI position
	if (m_pAreaUpgrade != nullptr)
	{
		spItem->pUIIcon->SetRegion(m_pAreaUpgrade->GetRegion());
		spItem->pUIIcon->SetMoveRect(m_pAreaUpgrade->GetRegion());
		spItem->pUIIcon->SetParent(this);
	}

	return true;
}

bool CUIItemUpgrade::IsSlotCompatible(__IconItemSkill* pSrc, int iDestiOrder) const
{
	if (!IsUpgradeScroll(pSrc->pItemBasic->dwEffectID2))
		return false;

	bool m_bhasTrina = false;
	bool m_bhasScroll = false;

	for (int k = 0; k < MAX_ITEM_UPGRADE_SLOT; ++k)
	{
		if (m_pUpgradeScrollSlots[k] != nullptr)
		{
			if (IsTrina(m_pUpgradeScrollSlots[k]->pItemBasic->dwID))
			{
				m_bhasTrina = true;
			}
			else 
				m_bhasScroll = true;
		}
	}
	if (m_bhasTrina && IsTrina(pSrc->pItemBasic->dwID))
		return false;
	if (m_bhasScroll && !IsTrina(pSrc->pItemBasic->dwID))
		return false;
	if (m_bhasTrina && m_bhasScroll)
		return false;
	return true;
}

bool CUIItemUpgrade::HandleSlotDrop(__IconItemSkill* spItem, int iDestiOrder)
{
	if (iDestiOrder == -1 || m_pUpgradeScrollSlots[iDestiOrder] != nullptr)
		return false;

	int iSourceOrder = GetItemiOrder(spItem, UIWND_DISTRICT_UPGRADE_INV);
	if (iSourceOrder == -1)
		return false;

	__IconItemSkill* pSrc = m_pMyUpgradeInv[iSourceOrder];
	if (!IsSlotCompatible(pSrc, iDestiOrder))
		return false;

	// Handle countable items
	if (pSrc->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE ||
		pSrc->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL)
	{
		if (pSrc->iCount > 1)
		{
			// Create a new icon, put it in 1 slot, reduce the number in the inventory
			__IconItemSkill* pNew = CreateIconFromSource(pSrc, 1);
			std::string szID = fmt::format("a_m_{}", iDestiOrder);
			CN3UIArea* pSlotArea = (CN3UIArea*)GetChildByID(szID);
			SetupIconArea(pNew, pSlotArea);
			m_pUpgradeScrollSlots[iDestiOrder] = pNew;
			pSrc->iCount -= 1;
		}
		else
		{
			// If the last one, move directly
			m_pUpgradeScrollSlots[iDestiOrder] = pSrc;
			m_pMyUpgradeInv[iSourceOrder] = nullptr;
			std::string szID = fmt::format("a_m_{}", iDestiOrder);
			CN3UIArea* pSlotArea = (CN3UIArea*)GetChildByID(szID);
			SetupIconArea(pSrc, pSlotArea);
		}
	}
	else
	{
		// If is not countable item, just move it
		m_pUpgradeScrollSlots[iDestiOrder] = pSrc;
		m_pMyUpgradeInv[iSourceOrder] = nullptr;
		std::string szID = fmt::format("a_m_{}", iDestiOrder);
		CN3UIArea* pSlotArea = (CN3UIArea*)GetChildByID(szID);
		SetupIconArea(pSrc, pSlotArea);
	}
	m_iUpgradeSlotInvPos[iDestiOrder+1] = iSourceOrder;
	return true;
}

void CUIItemUpgrade::FlipFlopAnim()
{
	if (m_eAnimationState != ANIM_FLIPFLOP)
		return;
	std::string szID;

	// Hide before frame
	if (m_iCurrentFrame > 0)
	{
		szID = fmt::format(fmt::runtime(m_bUpgradeSucceeded ? "img_s_load_{}" : "img_f_load_{}"), m_iCurrentFrame - 1);
		if (CN3UIImage* pImg = (CN3UIImage*)GetChildByID(szID))
			pImg->SetVisible(false);
	}

	// Show current frame
	szID = fmt::format(fmt::runtime(m_bUpgradeSucceeded ? "img_s_load_{}" : "img_f_load_{}"), m_iCurrentFrame);
	if (CN3UIImage* pImg = (CN3UIImage*) GetChildByID(szID))
	{
		pImg->SetVisible(true);
		pImg->SetParent(this);
	}
}
void CUIItemUpgrade::AnimClose()
{
	if (m_eAnimationState != ANIM_NONE)
	{
		float m_fAnimationTimer = 0.0f;
		int m_iCurrentFrame = 0;
		m_pImageCover1->SetVisible(false);
		m_pImageCover2->SetVisible(false);
	}

	m_eAnimationState = ANIM_NONE;
	m_bUpgradeInProgress = false;
	HideAllAnimationFrames();
}

void CUIItemUpgrade::ShowResultUpgrade()
{
	// Show result item in result area if upgrade was successful
	if (m_bUpgradeSucceeded && m_pUpgradeResultSlot->pUIIcon != nullptr && m_pAreaResult != nullptr)
	{
		// Set icon position to result area
		m_pUpgradeResultSlot->pUIIcon->SetRegion(m_pAreaResult->GetRegion());
		m_pUpgradeResultSlot->pUIIcon->SetMoveRect(m_pAreaResult->GetRegion());
		m_pUpgradeResultSlot->pUIIcon->SetParent(this);
		m_pUpgradeResultSlot->pUIIcon->SetVisible(true);
	}

	m_eAnimationState = ANIM_COVER_OPENING;
}

void CUIItemUpgrade::StartUpgradeAnim()
{
	if (m_bUpgradeInProgress)
	{
		if (m_pImageCover1 == nullptr || m_pImageCover2 == nullptr)
			return;
		m_fAnimationTimer = 0.0f;
		m_iCurrentFrame = 0;

		N3_VERIFY_UI_COMPONENT(m_pImageCover1, (CN3UIImage*) this->GetChildByID("img_cover_01"));
		N3_VERIFY_UI_COMPONENT(m_pImageCover2, (CN3UIImage*) this->GetChildByID("img_cover_02"));

		// Make covers visible during animation
		m_pImageCover1->SetVisible(true);
		m_pImageCover2->SetVisible(true);
		m_pImageCover1->SetParent(this);
		m_pImageCover2->SetParent(this);

		// save original positions
		m_rcCover1Original = m_pImageCover1->GetRegion();
		m_rcCover2Original = m_pImageCover2->GetRegion();

		m_eAnimationState = ANIM_FLIPFLOP;
	}
}

void CUIItemUpgrade::HandleInventoryIconRightClick(POINT ptCur, uint32_t dwMouseFlags)
{

    // Check for right mouse button click using MOUSE_RBCLICKED
    if (dwMouseFlags & MOUSE_RBCLICKED)
    {
        // Find the icon under the mouse
        for (int i = 0; i < MAX_ITEM_INVENTORY; ++i)
        {
            __IconItemSkill* pIconSkill = m_pMyUpgradeInv[i];
            if (pIconSkill == nullptr || pIconSkill->pUIIcon == nullptr)
				continue;

            CN3UIIcon* pIcon = pIconSkill->pUIIcon;
            if (pIcon->IsVisible() && pIcon->IsIn(ptCur.x, ptCur.y))
            {
                // Call HandleSlotDrop and HandleUpgradeAreaDrop for the icon under the mouse
				if (HandleUpgradeAreaDrop(pIconSkill))
				{
					pIconSkill->pUIIcon->SetRegion(m_pAreaUpgrade->GetRegion());
					pIconSkill->pUIIcon->SetMoveRect(m_pAreaUpgrade->GetRegion());
				}

				for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
				{
					if (HandleSlotDrop(pIconSkill, i)) 
						return;
				}
				
                return; // Only handle one icon per click
            }
        }
    }
}

