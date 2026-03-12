#include "TravelMgr.h"
#include "TravelNode.h"
#include "Group.h"
#include "Guild.h"
#include "GuildRpgBaseAction.h"
#include "PlayerbotGroupMgr.h"
#include "Playerbots.h"
#include "PlayerbotAI.h"
#include "NewRpgAction.h"

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
    GuildType guildType = botAI->guildRpgInfo.type;
    switch (guildType)
    {
        case GuildType::PVP:
            {
                weights = sPlayerbotAIConfig.GuildRpgPvpWeights;
                break;
            }
        case GuildType::PVE:
            {
                weights = sPlayerbotAIConfig.GuildRpgPveWeights;
                break;
            }
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
    LOG_DEBUG("playerbots","[Guild RPG] Bot {} has guild type {} and has selected activity {}", bot->GetName(), guildType, objective);
    botAI->guildRpgInfo.SetGuildRpgActivity(botAI, objective);
    return true;
}

bool GuildRpgBaseAction::isUseful()
{
    Guild* guild = bot->GetGuild();
    if (!guild)
        return false;

    if (botAI->guildRpgInfo.type == GuildType::NONE)
        return false;

    if (bot->GetGroup() && bot->GetGroup()->GetLeaderGUID() != bot->GetGUID())
        return false;

    if (botAI->guildRpgInfo.IsSleep())
        return false;

    return true;
}

bool GuildRpgBaseAction::Execute(Event event)
{
    GuildRpgInfo rpgInfo = botAI->guildRpgInfo;

    switch (rpgInfo.phase)
    {
        case GuildRpgPhase::IDLE:
            return HandleSelection(event);
        case GuildRpgPhase::GROUPING:
            return HandleGrouping(event);
        case GuildRpgPhase::PREPARATION:
            return HandlePreparation(event);
        case GuildRpgPhase::EXECUTING:
            return HandleExecution(event);
        case GuildRpgPhase::COMPLETED:
            return HandleCompletion(event);
        default:
            break;
    }
    return false;
}

void GuildRpgBaseAction::SyncGuildRpgStatus()
{
    Group* group = bot->GetGroup();
    if (!group)
        return;

    GuildRpgInfo rpgInfo = botAI->guildRpgInfo;
    GuildRpgPhase phase = rpgInfo.phase;
    GuildRpgActivity activity = rpgInfo.activity;

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        PlayerbotAI* memberBotAI = GET_PLAYERBOT_AI(member);
        if (!memberBotAI)
            continue;
        GuildRpgInfo& memberRpgInfo = memberBotAI->guildRpgInfo;
        memberRpgInfo.phase = phase;
        memberRpgInfo.activity = activity;
    }
    return;
}


void GuildRpgBaseAction::EndGuildRpgActivity()
{
    Group* group = bot->GetGroup();
    if (!group)
        return;

    for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
    {
        Player* member = gref->GetSource();
        PlayerbotAI* memberBotAI = GET_PLAYERBOT_AI(member);
        if (!memberBotAI)
            continue;
        memberBotAI->guildRpgInfo.ResetGuildActivity();
        memberBotAI->rpgInfo.ChangeToIdle();
    }
    return;
}

bool GuildRpgBaseAction::HandleSelection(Event event)
{
    return false;
}

bool GuildRpgBaseAction::HandleGrouping(Event event)
{
    Player* bot = botAI->GetBot();
    if (!bot)
        return false;

    PlayerbotGroupMgr* groupMgr = botAI->GetGroupMgr();
    if (!groupMgr)
    {
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} has no group manager available", bot->GetName());
        return false;
    }
    Group* group = bot->GetGroup();
    if (!group)
    {
        // Check if composition is available
        if (!groupMgr->IsCompositionAvailable())
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {}: Required group composition not available", bot->GetName());
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false;
        }

        // Create the group
        if (!groupMgr->CreateGroup())
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {}: Failed to create group", bot->GetName());
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false;
        }
        LOG_DEBUG("playerbots", "[Guild RPG] Bot {} created group for activity", bot->GetName());
        return true;
    }


    // Wait for group formation to complete
    if (groupMgr->WaitingforResponse())
    {
        LOG_DEBUG("playerbots", "[Guild RPG] Bot {} waiting for group invites to be accepted", bot->GetName());
        return true;
    }

    // Check if group is complete
    if (!groupMgr->CheckGroupComposition())
    {
        LOG_DEBUG("playerbots", "[Guild RPG] Bot {}: Group composition not yet complete for activity", bot->GetName());
        if (!groupMgr->CreateGroup())
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {}: Failed to send re-invites to complete group. Reset activity.", bot->GetName());
            botAI->guildRpgInfo.ResetGuildActivity(true);
            groupMgr->DisbandGroup();
            groupMgr->Reset();
            return false;
        }
        return true; // Continue waiting for complete group
    }

    // Group is ready. Transition to PREPARATION phase.
    LOG_DEBUG("playerbots", "[Guild RPG] Bot {} group is ready for activity. Updating task phase to PREPARATION.", bot->GetName());
    botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::PREPARATION);
    SyncGuildRpgStatus();

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

bool GuildRpgBaseAction::PreparationMovementToRpgLocation(Event event, WorldPosition targetPos, uint32 targetZone)
{
    if (botAI->guildRpgInfo.phase != GuildRpgPhase::PREPARATION)
        return false;

    // Optional zone arrival check — used by callers that want zone-based completion
    if (targetZone && bot->GetZoneId() == targetZone)
    {
        LOG_DEBUG("playerbots", "[Guild RPG] Bot {} arrived in target zone {}, transitioning to EXECUTING", bot->GetName(), targetZone);
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::EXECUTING);
        botAI->SayToParty("summon");
        SyncGuildRpgStatus();
        return true;
    }

    // NewRpg is still walking/flying the travel path — wait
    if (botAI->rpgInfo.GetStatus() == RPG_MOVE_FAR || botAI->rpgInfo.GetStatus() == RPG_TRAVEL_FLIGHT)
        return false;

    // Compute full travel path using the travel node system
    WorldPosition currentPos(bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ());
    TravelPath travelPath = TravelNodeMap::getFullPath(currentPos, targetPos, bot);

    if (travelPath.empty())
    {
        LOG_DEBUG("playerbots", "[Guild RPG] Bot {} could not find travel path, falling back to teleport", bot->GetName());
        bot->TeleportTo(targetPos);
        return true;
    }

    LOG_DEBUG("playerbots", "[Guild RPG] Bot {} starting travel path ({} waypoints)", bot->GetName(), travelPath.getPath().size());
    botAI->rpgInfo.travelPath = std::move(travelPath);
    botAI->rpgInfo.ChangeToMoveFar();
    return true;
}

bool GuildRpgStatusUpdateAction::Execute(Event event)
{
    if (bot->GetLevel() < 10)
        return false;
    if (botAI->guildRpgInfo.GetActivityName()=="NONE")
    {
        uint32 roll = urand(0, 100);
        if (roll < sPlayerbotAIConfig.guildRpgProbability)
            return ChooseRandomActivity();
    }
    return false;
}
