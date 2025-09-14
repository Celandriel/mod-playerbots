
#include "GroupAction.h"
#include "PlayerbotAI.h"
#include "PlayerbotGroupMgr.h"
#include "Guild.h"
#include "Group.h"


bool CreateGroupAction::Execute(Event event)
{
    PlayerbotGroupMgr& groupMgr = *botAI->m_groupMgr;
    TargetGroupComposition comp = botAI->GetTargetGroupComposition();

    // Set composition for the manager
    groupMgr.SetTargetComposition(comp);

    // If the group is incomplete, try to create/fill it
    if (!groupMgr.CheckGroupComposition())
    {
        groupMgr.CreateGroup();
        return false; // still building
    }

    // Group is ready
    return true;
}

bool CreateGroupAction::isUseful()
{
    if (!botAI)
        return false;

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

bool CreateGroupAction::isPossible()
{
    TargetGroupComposition comp = botAI->GetTargetGroupComposition();
    if (!ValidateTargetComposition(comp))
        return false;
    return botAI->m_groupMgr->IsCompositionAvailable(comp);
}

bool DisbandGroupAction::Execute(Event event)
{
    if (!botAI)
        return false;
    if (!botAI->GetBot()->GetGroup())
        return false;
    return botAI->m_groupMgr->DisbandGroup();
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
