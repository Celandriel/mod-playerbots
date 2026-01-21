#include "GuildRpgBaseAction.h"
#include "NewRpgAction.h"
#include "FlightMasterCache.h"
#include "Guild.h"
#include "Group.h"
#include "PlayerbotGroupMgr.h"
#include "Playerbots.h"
#include "PlayerbotAI.h"

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
    {
        return false;
    }
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
            botAI->guildRpgInfo.ResetGuildActivity();
            return false;
        }

        // Create the group
        if (!groupMgr->CreateGroup())
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {}: Failed to create group", bot->GetName());
            botAI->guildRpgInfo.ResetGuildActivity();
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
            botAI->guildRpgInfo.ResetGuildActivity();
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

bool GuildRpgBaseAction::PreparationMovementToRpgLocation(Event event, uint32 mapId, uint32 targetZone, uint32 toNode)
{
    if (botAI->guildRpgInfo.phase != GuildRpgPhase::PREPARATION)
        return false;
        //logic to handle travel to location. 1 move if in same area, fly if in same map, teleport if different map. The action only goes to excution when bot is in the target area and then summons the bots.
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} has area {}, map {} checking against target area {}", bot->GetName(), bot->GetAreaId(), bot->GetMapId(), targetZone);
        if (bot->GetZoneId() == targetZone)
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} is in target area {}, summoning group", bot->GetName(), targetZone);
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::EXECUTING);
            botAI->SayToParty("summon");
            SyncGuildRpgStatus();
            return true;
        }
        if (bot->GetMapId() == mapId)
        {
            if (botAI->rpgInfo.status == RPG_TRAVEL_FLIGHT)
            {
                LOG_ERROR("playerbots", "[Guild RPG] Bot {} is currently in travel flight, waiting to arrive at destination", bot->GetName());
                return false;
            }
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} with faction {} is in target map {}, flying to target area {}", bot->GetName(), bot->GetTeamId(), mapId, targetZone);
            Creature* nearestFlightMaster = sFlightMasterCache->GetNearestFlightMaster(bot);
            if (!nearestFlightMaster)
            {
                LOG_ERROR("playerbots", "[Guild RPG] Bot {} could not find flight master or target position", bot->GetName());
                botAI->guildRpgInfo.ResetGuildActivity();
                return false;
            }
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} found nearest flight master {} and is {} yards away", bot->GetName(), nearestFlightMaster->GetName(), bot->GetDistance(nearestFlightMaster));
            uint32 fromNode = sObjectMgr->GetNearestTaxiNode(nearestFlightMaster->GetPositionX(), nearestFlightMaster->GetPositionY(),
                                              nearestFlightMaster->GetPositionZ(), nearestFlightMaster->GetMapId(),
                                              bot->GetTeamId());
            uint32 path, cost;
            sObjectMgr->GetTaxiPath(fromNode, toNode, path, cost);
            if (!path)
            {
                LOG_ERROR("playerbots", "[Guild RPG] Bot {} could not find taxi path from node {} to node {}", bot->GetName(), fromNode, toNode);
                botAI->guildRpgInfo.ResetGuildActivity();
                return false;
            }
            botAI->rpgInfo.ChangeToTravelFlight(nearestFlightMaster->GetGUID(), fromNode, toNode);
            NewRpgTravelFlightAction travelFlightAction(botAI);
            return travelFlightAction.Execute(event);
        }
        else
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} in different map teleporting to target area {}", bot->GetName(), targetZone);
            TaxiNodesEntry const* taxiNodeEntry = sTaxiNodesStore.LookupEntry(static_cast<uint32>(toNode));
            WorldPosition targetPos = WorldPosition(mapId, taxiNodeEntry->x, taxiNodeEntry->y, taxiNodeEntry->z);
            bot->TeleportTo(targetPos);
            return true;
        }
}
bool GuildRpgStatusUpdateAction::Execute(Event event)
{
    if (bot->GetLevel() < 10)
        return false;
    if (botAI->guildRpgInfo.GetActivityName()=="NONE")
    {
        uint32 roll = urand(0, 100);
        if (roll < sPlayerbotAIConfig->guildRpgProbability)
            return ChooseRandomActivity();
    }
    return false;
}
