#include "GuildRpgBaseAction.h"
#include "PlayerbotAI.h"
#include "Guild.h"
#include "Group.h"
#include "PlayerbotGroupMgr.h"

bool TellGuildRpgStatusAction::Execute(Event event)
{
    Player* owner = event.getOwner();
    if (!owner)
        return false;
    GuildRpgInfo rpgInfo = botAI->guildRpgInfo;
    std::string out = rpgInfo.ToString();
    bot->Whisper(out.c_str(), LANG_UNIVERSAL, owner);
    return true;
}

uint8 GuildRpgBaseAction::RollRandomActivity(std::vector<uint32> weights)
{
    uint32 totalRatio = 0;
    for (float ratio : weights)
        totalRatio += ratio;

    if (totalRatio == 0)
        return 0;

    uint32 roll = urand(1, totalRatio);
    uint32 accumulate = 0;
    uint8 activity;
    for (uint8 i = 0; i < weights.size(); ++i) {
        accumulate += weights[i];
        if (roll <= accumulate)
        {
            activity = i;
            break;
        }
    }
    return activity;
}

bool GuildRpgBaseAction::ChooseRandomActivity()
{
    std::vector<uint32> weights;
    GuildType guildType = botAI->GetGuildType();
    switch (guildType)
    {
        case GuildType::PVP:
            {
                weights = sPlayerbotAIConfig->GuildRpgPvpWeights;
                break;
            }
        case GuildType::PVE:
        case GuildType::PROFESSION:
        case GuildType::ROLEPLAY:
        case GuildType::NONE:
        default:
            return false;
    }
    uint8 objective = RollRandomActivity(weights);
    botAI->guildRpgInfo.SetGuildRpgActivity(botAI, objective);
    return true;
}

bool GuildRpgBaseAction::isUseful()
{
    Guild* guild = bot->GetGuild();
    if (!guild)
        return false;

    if (botAI->GetGuildType() == GuildType::NONE)
        return false;

    if (bot->GetGroup() && bot->GetGroup()->GetLeaderGUID() != bot->GetGUID())
            return false;
    return true;
}

bool GuildRpgBaseAction::Execute(Event event)
{
    GuildRpgInfo rpgInfo = botAI->guildRpgInfo;

    switch (rpgInfo.phase)
    {
        case GuildRpgPhase::IDLE:
            HandleGrouping(event);
            break;
        case GuildRpgPhase::GROUPING:
            HandleGrouping(event);
            break;
        case GuildRpgPhase::PREPARATION:
            HandlePreparation(event);
            break;
        case GuildRpgPhase::EXECUTING:
            HandleExecution(event);
            break;
        case GuildRpgPhase::COMPLETED:
            HandleCompletion(event);
            break;
        default:
            break;
    }
    return true;
}

bool GuildRpgBaseAction::HandleSelection(Event event)
{
    return false;
}

bool GuildRpgBaseAction::HandleGrouping(Event event)
{
    Player* bot = botAI->GetBot();
    PlayerbotGroupMgr* groupMgr = botAI->GetGroupMgr();
    if (!groupMgr)
    {
        LOG_ERROR("playerbots", "Bot {} has no group manager available", bot->GetName().c_str());
        return false;
    }

    // Check if composition is available
    if (!groupMgr->IsCompositionAvailable())
    {
        LOG_INFO("playerbots", "Bot {}: Required group composition not available", bot->GetName().c_str());
        return false;
    }

    // Create the group
    if (!groupMgr->CreateGroup())
    {
        LOG_ERROR("playerbots", "Bot {}: Failed to create group", bot->GetName().c_str());
        return false;
    }

    LOG_INFO("playerbots", "Bot {} created group for PVP activity", bot->GetName().c_str());

    // Wait for group formation to complete
    if (groupMgr->WaitingforResponse())
    {
        LOG_INFO("playerbots", "Bot {} waiting for group invites to be accepted", bot->GetName().c_str());
        return true; // Return true to continue waiting
    }

    // Check if group is complete
    if (!groupMgr->CheckGroupComposition())
    {
        LOG_DEBUG("playerbots", "Bot {}: Group composition not yet complete for PVP activity", bot->GetName().c_str());
        return true; // Continue waiting for complete group
    }

    // Group is ready. Transition to PREPARATION phase.
    LOG_INFO("playerbots", "Bot {} group is ready for PVP activity. Updating task phase to PREPARATION.", bot->GetName().c_str());
    botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::PREPARATION);

    return true;
}

bool GuildRpgBaseAction::HandlePreparation(Event event)
{
    return false;
}

bool GuildRpgBaseAction::HandleExecution(Event event)
{
    return false;
}

bool GuildRpgBaseAction::HandleCompletion(Event event)
{
    return false;
}



bool GuildRpgActivityUpdateAction::Execute(Event event)
{
    //random float 0-1 and check against guildrpgprobability
    float roll = (float)rand() / RAND_MAX;
    if (roll < sPlayerbotAIConfig->guildRpgProbability)
    {
        ChooseRandomActivity();
        LOG_INFO("playerbots", "Bot {} selected new guild RPG activity: {}", botAI->GetBot()->GetName().c_str(), botAI->guildRpgInfo.activityNumber);
        return true;
    }
}