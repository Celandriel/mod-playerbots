#include "GuildRPGBaseAction.h"
#include "PlayerbotGroupMgr.h"
#include "PlayerbotAI.h"
#include "Guild.h"
#include "Group.h"


GuildRPGBaseAction::~GuildRPGBaseAction()
{
    delete m_groupMgr;
    m_groupMgr = nullptr;
}

PlayerbotGroupMgr* GuildRPGBaseAction::GetGroupMgr()
{
    if (!m_groupMgr)
        m_groupMgr = new PlayerbotGroupMgr(botAI);
    return m_groupMgr;
}

Guild* GuildRPGBaseAction::GetGuild() const
{
    Player* player = botAI->GetBot();
    return player ? player->GetGuild() : nullptr;
}


bool CreateGroupAction::Execute(Event event)
{  
    if (!m_groupMgr)
    {    
        m_groupMgr = GetGroupMgr();
        TargetGroupComposition comp = botAI->GetTargetGroupComposition();
        if (!ValidateTargetComposition(comp))
            return false;
        
        m_groupMgr->SetTargetComposition(comp);
        m_groupMgr->CreateGroup();
        return false;
    }
    else if (m_groupMgr)
    {
        if (m_groupMgr->WaitingforResponse())
            return false;
        if (m_groupMgr->CheckGroupComposition())
            return true;
        m_groupMgr->CleanGroup();
        return false;
    }
    return false;
}
bool CreateGroupAction::isUseful()
{
    Player* player = botAI->GetBot();
    if (!player)
        return false;
    
    Guild* guild = player->GetGuild();
    if (!guild)
        return false;

    if (player->GetGroup())
        return false;
        
    return true;
}

bool CreateGroupAction::ValidateTargetComposition(const TargetGroupComposition& comp)
{
    if (comp.groupSize == 0)
        return false;
        
    // Validate role requirements don't exceed group size
    uint32 minRequired = comp.tanks + comp.minHealers + comp.minDps;
    if (minRequired > comp.groupSize)
        return false;
        
    // Validate max roles
    if (comp.maxHealers < comp.minHealers)
        return false;
    if (comp.maxDps < comp.minDps)
        return false;
    //Check level limits. 
    if (comp.lowerLevelLimit > comp.upperLevelLimit)
        return false;
        
    return true;
}    

bool CheckGroupAction::Execute(Event event)
{
    if (!m_groupMgr)
    {
        return false;
    }
    if (m_groupMgr->WaitingforResponse())
        return false;
    if (m_groupMgr->CheckGroupComposition())
        return true;
    m_groupMgr->CleanGroup();
    return false;
}
    


bool CheckGroupAction::isUseful()
{
    if (!m_groupMgr)
    {
        return false;
    }
    return true;
}


bool DisbandGroupAction::isPossible()
{
    if (!m_groupMgr) 
    {
        return false;
    }
    return true;
}

bool DisbandGroupAction::isUseful()
{
    //TODO Change to check on GuildRPG State. if the state is reset to 0 then disband group.
    return isPossible(); 
}

bool DisbandGroupAction::Execute(Event event)
{
    PlayerbotGroupMgr* groupMgr = GetGroupMgr();
    if (!groupMgr) return false;

    bool success = groupMgr->DisbandGroup();
    
    if (success)
    {
        delete m_groupMgr;
        m_groupMgr = nullptr;
    }   
    return success;
}