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

GuildRpgActivity GuildRpgBaseAction::RollRandomActivity(std::vector<uint32> weights, ActivityList activities)
{
    uint32 totalWeights = 0;
    for (uint weight : weights)
        totalWeights += weight;

    if (totalWeights == 0)
        return GuildRpgActivity::NONE;

    uint32 roll = urand(1, totalWeights);
    uint32 accumulate = 0;

    for (uint8 i = 0; i < weights.size(); ++i)
    {
        accumulate += weights[i];
        if (roll <= accumulate)
        {
            return activities[i];
        }
    }
    return GuildRpgActivity::NONE;
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
    auto it = ActivitiesByGuildType.find(guildType);
    if (it == ActivitiesByGuildType.end())
        return false;

    const ActivityList& activities = it->second;
    GuildRpgActivity objective = RollRandomActivity(weights, activities);
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
        LOG_ERROR("playerbots", "Bot {}: Required group composition not available", bot->GetName().c_str());
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

bool GuildRpgStatusUpdateAction::Execute(Event event)
{
    uint32 roll = urand(0, 100);
    if (roll < sPlayerbotAIConfig->guildRpgProbability)
        return ChooseRandomActivity();

    return false;
}
