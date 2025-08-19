#include "GuildRPGBaseAction.h"
#include "PlayerbotGroupMgr.h"
#include "PlayerbotAI.h"
#include "Guild.h"
#include "Group.h"


GuildRPGBaseAction::~GuildRPGBaseAction()
{
    delete m_groupMgr;
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


bool CreateGroupAction::Execute(Event& event)
{
    if (!ValidateComposition())
        return false;
    
    if (!m_groupMgr)
        m_groupMgr = new PlayerbotGroupMgr(botAI);

    if (m_groupMgr->CreateGroup(m_targetComposition))
    {
        return true;
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

    return ValidateComposition();
}
bool CreateGroupAction::isPossible()
{
    return isUseful();
}

bool CreateGroupAction::ValidateComposition()
{
    if (m_targetComposition.groupSize == 0 || m_targetComposition.groupSize > 5)
        return false;
        
    // Validate role requirements don't exceed group size
    uint32 minRequired = m_targetComposition.tanks + m_targetComposition.minHealers + m_targetComposition.minDps;
    if (minRequired > m_targetComposition.groupSize)
        return false;
        
    // Validate max roles
    if (m_targetComposition.maxHealers < m_targetComposition.minHealers)
        return false;
    if (m_targetComposition.maxDps < m_targetComposition.minDps)
        return false;
        
    return true;
}


    