
#include "GroupAction.h"
#include "PlayerbotAI.h"
#include "PlayerbotGroupMgr.h"
#include "Guild.h"
#include "Group.h"


bool CreateGroupAction::Execute(Event event)
{
    PlayerbotGroupMgr& groupMgr = *botAI->GetGroupMgr();

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
    return botAI->GetGroupMgr()->IsCompositionAvailable();
}

bool DisbandGroupAction::Execute(Event event)
{
    if (!botAI)
        return false;
    if (!botAI->GetBot()->GetGroup())
        return false;
    return botAI->GetGroupMgr()->DisbandGroup();
}
