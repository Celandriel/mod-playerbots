#include "AiObjectContext.h"
#include "AreaDefines.h"
#include "GuildRpgInfo.h"
#include "GuildRpgPveAction.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "PlayerbotAI.h"
#include "PlayerbotDungeonRepository.h"
#include "PlayerbotGroupMgr.h"
#include "Playerbots.h"
#include "TravelNode.h"
#include "TravelMgr.h"
#include "WorldPacket.h"

using FMN = FlightMasterNodes;

// Static mapping of dungeon name → entrance/taxi geographic data + waypoint name.
// Keyed by the full name from playerbots_dungeon_suggestion_definition.
//                                                    wpName  mapId                   entrMapId             entrZoneId                 taxiAlliance                 taxiHorde
static const std::unordered_map<std::string, DungeonEntranceInfo> s_entranceData =
{
    { "Ragefire Chasm",               { "RFC",        MAP_RAGEFIRE_CHASM,      MAP_KALIMDOR,         AREA_ORGRIMMAR,             0,                           (uint32)FMN::ORGRIMMAR          } },
    { "Wailing Caverns",              { "WC",         MAP_WAILING_CAVERNS,     MAP_KALIMDOR,         AREA_THE_BARRENS,           0,                           (uint32)FMN::CROSSROADS         } },
    { "Deadmines",                    { "DM",         MAP_DEADMINES,           MAP_EASTERN_KINGDOMS, AREA_WESTFALL,              (uint32)FMN::SENTINEL_HILL,  0                                } },
    { "Shadowfang Keep",              { "SFK",        MAP_SHADOWFANG_KEEP,     MAP_EASTERN_KINGDOMS, AREA_SILVERPINE_FOREST,     0,                           (uint32)FMN::THE_SEPULCHER      } },
    { "Blackfathom Deeps",            { "BFD",        MAP_BLACKFATHOM_DEEPS,   MAP_KALIMDOR,         AREA_ASHENVALE,             (uint32)FMN::AUBERDINE,      (uint32)FMN::ZORAMGAR_OUTPOST   } },
    { "Stormwind Stockade",           { "STOCKS",     MAP_STORMWIND_STOCKADE,  MAP_EASTERN_KINGDOMS, AREA_STORMWIND_CITY,        (uint32)FMN::STORMWIND,      0                                } },
    { "Gnomeregan",                   { "GNOMER",     MAP_GNOMEREGAN,          MAP_EASTERN_KINGDOMS, AREA_DUN_MOROGH,            (uint32)FMN::IRONFORGE,      0                                } },
    { "Scarlet Monastery: Graveyard", { "SM-GY",      MAP_SCARLET_MONASTERY,   MAP_EASTERN_KINGDOMS, AREA_TIRISFAL_GLADES,       0,                           (uint32)FMN::UNDERCITY          } },
    { "Scarlet Monastery: Library",   { "SM-LIB",     MAP_SCARLET_MONASTERY,   MAP_EASTERN_KINGDOMS, AREA_TIRISFAL_GLADES,       0,                           (uint32)FMN::UNDERCITY          } },
    { "Scarlet Monastery: Armory",    { "SM-ARMORY",  MAP_SCARLET_MONASTERY,   MAP_EASTERN_KINGDOMS, AREA_TIRISFAL_GLADES,       0,                           (uint32)FMN::UNDERCITY          } },
    { "Scarlet Monastery: Cathedral", { "SM-CATH",    MAP_SCARLET_MONASTERY,   MAP_EASTERN_KINGDOMS, AREA_TIRISFAL_GLADES,       0,                           (uint32)FMN::UNDERCITY          } },
    { "Razorfen Kraul",               { "RFK",        MAP_RAZORFEN_KRAUL,      MAP_KALIMDOR,         AREA_THE_BARRENS,           0,                           (uint32)FMN::CROSSROADS         } },
    { "Razorfen Downs",               { "RFD",        MAP_RAZORFEN_DOWNS,      MAP_KALIMDOR,         AREA_THE_BARRENS,           0,                           (uint32)FMN::CROSSROADS         } },
    { "Uldaman",                      { "ULDA",       MAP_ULDAMAN,             MAP_EASTERN_KINGDOMS, AREA_BADLANDS,              (uint32)FMN::THELSAMAR,      0                                } },
    { "Maraudon",                     { "MARA",       MAP_MARAUDON,            MAP_KALIMDOR,         AREA_DESOLACE,              0,                           (uint32)FMN::SHADOWPREY_VILLAGE } },
    { "Zul'Farrak",                   { "ZF",         MAP_ZUL_FARRAK,          MAP_KALIMDOR,         AREA_TANARIS,               (uint32)FMN::GADGETZAN,      (uint32)FMN::GADGETZAN          } },
    { "Temple of Atal'Hakkar",        { "ST",         MAP_SUNKEN_TEMPLE,       MAP_EASTERN_KINGDOMS, AREA_SWAMP_OF_SORROWS,      (uint32)FMN::STONARD,        (uint32)FMN::STONARD            } },
    { "Blackrock Depths",             { "BRD",        MAP_BLACKROCK_DEPTHS,    MAP_EASTERN_KINGDOMS, AREA_BURNING_STEPPES,       (uint32)FMN::THORIUM_POINT,  (uint32)FMN::THORIUM_POINT      } },
    { "Dire Maul: West",              { "DM-W",       MAP_DIRE_MAUL,           MAP_KALIMDOR,         AREA_FERALAS,               (uint32)FMN::FEATHERMOON,    (uint32)FMN::CAMP_MOJACHE       } },
    { "Dire Maul: East",              { "DM-E",       MAP_DIRE_MAUL,           MAP_KALIMDOR,         AREA_FERALAS,               (uint32)FMN::FEATHERMOON,    (uint32)FMN::CAMP_MOJACHE       } },
    { "Dire Maul: North",             { "DM-N",       MAP_DIRE_MAUL,           MAP_KALIMDOR,         AREA_FERALAS,               (uint32)FMN::FEATHERMOON,    (uint32)FMN::CAMP_MOJACHE       } },
    { "Lower Blackrock Spire",        { "LBRS",       MAP_BLACKROCK_SPIRE,     MAP_EASTERN_KINGDOMS, AREA_BURNING_STEPPES,       (uint32)FMN::THORIUM_POINT,  (uint32)FMN::THORIUM_POINT      } },
    { "Scholomance",                  { "SCHOLO",     MAP_SCHOLOMANCE,         MAP_EASTERN_KINGDOMS, AREA_WESTERN_PLAGUELANDS,   (uint32)FMN::CHILLWIND_CAMP, (uint32)FMN::CHILLWIND_CAMP     } },
    { "Stratholme",                   { "STRAT",      MAP_STRATHOLME,          MAP_EASTERN_KINGDOMS, AREA_EASTERN_PLAGUELANDS,   (uint32)FMN::LIGHTS_HOPE_CHAPEL_ALLIANCE, (uint32)FMN::LIGHTS_HOPE_CHAPEL_HORDE } },
};

static const std::vector<uint32> s_hordeCityNodes    = { (uint32)FMN::ORGRIMMAR, (uint32)FMN::UNDERCITY, (uint32)FMN::THUNDER_BLUFF };
static const std::vector<uint32> s_allianceCityNodes = { (uint32)FMN::STORMWIND, (uint32)FMN::IRONFORGE, (uint32)FMN::DARNASSUS };

const DungeonEntranceInfo* GuildRpgPveAction::GetEntranceInfo(const std::string& dungeonName)
{
    auto it = s_entranceData.find(dungeonName);
    if (it != s_entranceData.end())
        return &it->second;
    return nullptr;
}

bool GuildRpgPveAction::MoveGroupThroughPortal(const DungeonEntranceInfo* entrance)
{
    // Find the area trigger that teleports into this dungeon map
    uint32 triggerId = 0;
    const auto& allTeleports = sObjectMgr->GetAllAreaTriggerTeleports();
    for (const auto& [id, teleport] : allTeleports)
    {
        if (teleport.target_mapId == entrance->mapId)
        {
            // Verify the trigger is on the correct continent
            AreaTrigger const* at = sObjectMgr->GetAreaTrigger(id);
            if (at && at->map == entrance->entranceMapId)
            {
                triggerId = id;
                break;
            }
        }
    }

    if (triggerId == 0)
    {
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} could not find area trigger for dungeon map {}", bot->GetName(), entrance->mapId);
        return false;
    }

    AreaTrigger const* at = sObjectMgr->GetAreaTrigger(triggerId);
    if (!at)
        return false;

    // Move bot to the portal location on the continent, then fire the area trigger
    float distance = bot->GetDistance(at->x, at->y, at->z);
    if (distance > 5.0f)
    {
        // Not close enough yet — walk to the portal
        bot->GetMotionMaster()->MovePoint(at->map, at->x, at->y, at->z, FORCED_MOVEMENT_NONE, 0.0f, 0.0f, true, false);
        return true;
    }

    // Close enough — fire the area trigger for all group members
    Group* group = bot->GetGroup();
    if (group)
    {
        for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->GetSource();
            if (!member || member->GetMapId() != entrance->entranceMapId)
                continue;

            // Teleport member to the trigger position first, then fire the trigger
            member->NearTeleportTo(at->x, at->y, at->z, at->orientation);

            WorldPacket p(CMSG_AREATRIGGER);
            p << triggerId;
            p.rpos(0);
            member->GetSession()->HandleAreaTriggerOpcode(p);

            PlayerbotAI* memberBotAI = GET_PLAYERBOT_AI(member);
            if (memberBotAI)
                memberBotAI->rpgInfo.ChangeToDungeonPve(entrance->mapId, entrance->waypointName);
        }
    }
    else
    {
        bot->NearTeleportTo(at->x, at->y, at->z, at->orientation);

        WorldPacket p(CMSG_AREATRIGGER);
        p << triggerId;
        p.rpos(0);
        bot->GetSession()->HandleAreaTriggerOpcode(p);

        botAI->rpgInfo.ChangeToDungeonPve(entrance->mapId, entrance->waypointName);
    }

    LOG_DEBUG("playerbots", "[Guild RPG] Bot {} group entering portal (trigger {}) for dungeon map {}", bot->GetName(), triggerId, entrance->mapId);
    return true;
}

const DungeonSuggestion* GuildRpgPveAction::SelectDungeonForLevel(uint8 botLevel) const
{
    auto candidates = PlayerbotDungeonRepository::instance().GetDungeonsForLevel(botLevel);
    std::vector<const DungeonSuggestion*> eligible;

    uint8 faction = bot->GetTeamId();

    for (const DungeonSuggestion* suggestion : candidates)
    {
        const DungeonEntranceInfo* entrance = GetEntranceInfo(suggestion->name);
        if (!entrance)
            continue;

        // Skip dungeons the faction has no taxi access to
        uint32 taxiNode = (faction == TEAM_HORDE) ? entrance->taxiNodeHorde : entrance->taxiNodeAlliance;
        if (taxiNode == 0)
            continue;

        // Verify waypoint data exists
        const DungeonPath* path = AiObjectContext::s_dungeonWaypointMgr.GetPath(entrance->mapId, entrance->waypointName);
        if (!path || path->empty())
            continue;

        eligible.push_back(suggestion);
    }

    if (eligible.empty())
        return nullptr;

    return eligible[urand(0, eligible.size() - 1)];
}

TargetGroupComposition GuildRpgPveAction::CreatePveGroupComposition(const DungeonSuggestion& dungeon) const
{
    TargetGroupComposition composition = {};
    composition.groupSize = DEFAULT_GROUP_SIZE;
    composition.lowerLevelLimit = dungeon.min_level;
    composition.upperLevelLimit = dungeon.max_level;

    composition.tanks = 1;
    composition.minHealers = 1;
    composition.maxHealers = 1;
    composition.minDps = 3;
    composition.maxDps = 3;

    composition.allowPartial = false;
    return composition;
}

bool GuildRpgPveAction::HandleSelection(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;
    uint8 botLevel = bot->GetLevel();

    if (activity == GuildRpgActivity::RUN_DUNGEON)
    {
        const DungeonSuggestion* dungeon = SelectDungeonForLevel(botLevel);
        if (!dungeon)
        {
            LOG_DEBUG("playerbots", "[Guild RPG] Bot {} could not find suitable dungeon for level {}", bot->GetName(), botLevel);
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false;
        }

        botAI->guildRpgInfo.activityTarget = dungeon->name;

        TargetGroupComposition groupComp = CreatePveGroupComposition(*dungeon);
        if (!botAI->SetTargetGroupComposition(groupComp))
        {
            LOG_DEBUG("playerbots", "[Guild RPG] Bot {} could not set group composition for {}", bot->GetName(), dungeon->name);
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false;
        }

        LOG_DEBUG("playerbots", "[Guild RPG] Bot {} selected dungeon {}", bot->GetName(), dungeon->name);
    }
    else if (activity == GuildRpgActivity::RUN_RAID)
    {
        // Not yet implemented
        botAI->guildRpgInfo.ResetGuildActivity(true);
        return false;
    }
    else if (activity == GuildRpgActivity::WORLD_EVENT)
    {
        // Not yet implemented
        botAI->guildRpgInfo.ResetGuildActivity(true);
        return false;
    }
    else
        return false;

    botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::GROUPING);
    SyncGuildRpgStatus();
    return true;
}

bool GuildRpgPveAction::HandlePreparation(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;

    if (activity == GuildRpgActivity::RUN_DUNGEON)
    {
        const std::string& targetName = botAI->guildRpgInfo.activityTarget;
        const DungeonEntranceInfo* entrance = GetEntranceInfo(targetName);
        if (!entrance)
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} no entrance data for dungeon {}", bot->GetName(), targetName);
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false;
        }

        // Already inside the dungeon instance — skip straight to execution
        if (bot->GetMapId() == entrance->mapId)
        {
            LOG_DEBUG("playerbots", "[Guild RPG] Bot {} already in dungeon map {}, transitioning to EXECUTING", bot->GetName(), entrance->mapId);
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::EXECUTING);
            SyncGuildRpgStatus();
            return true;
        }

        // Travel path was consumed — bot has arrived on the entrance map
        if (botAI->rpgInfo.GetStatus() != RPG_MOVE_FAR && botAI->rpgInfo.GetStatus() != RPG_TRAVEL_FLIGHT
            && bot->GetMapId() == entrance->entranceMapId)
        {
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::EXECUTING);
            SyncGuildRpgStatus();
            return true;
        }

        // Find the area trigger to get the dungeon entrance position on the continent
        WorldPosition entrancePos;
        const auto& allTeleports = sObjectMgr->GetAllAreaTriggerTeleports();
        for (const auto& [id, teleport] : allTeleports)
        {
            if (teleport.target_mapId == entrance->mapId)
            {
                AreaTrigger const* at = sObjectMgr->GetAreaTrigger(id);
                if (at && at->map == entrance->entranceMapId)
                {
                    entrancePos = WorldPosition(at->map, at->x, at->y, at->z);
                    break;
                }
            }
        }

        if (entrancePos == WorldPosition())
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} could not find entrance position for {}", bot->GetName(), targetName);
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false;
        }

        return PreparationMovementToRpgLocation(event, entrancePos);
    }
    else if (activity == GuildRpgActivity::RUN_RAID)
    {
        // Not yet implemented
        botAI->guildRpgInfo.ResetGuildActivity(true);
        return false;
    }
    else if (activity == GuildRpgActivity::WORLD_EVENT)
    {
        // Not yet implemented
        botAI->guildRpgInfo.ResetGuildActivity(true);
        return false;
    }

    botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
    return false;
}

bool GuildRpgPveAction::HandleExecution(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;

    if (activity == GuildRpgActivity::RUN_DUNGEON)
    {
        const std::string& targetName = botAI->guildRpgInfo.activityTarget;
        const DungeonEntranceInfo* entrance = GetEntranceInfo(targetName);
        if (!entrance)
        {
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false;
        }

        // If bot is not yet inside the dungeon, walk group through the portal
        if (bot->GetMapId() != entrance->mapId)
            return MoveGroupThroughPortal(entrance);

        // Already inside - NewRpgDungeonPveAction handles waypoint movement via RPG_DUNGEON_PVE status

        // Check if the tank has gone idle (reached end of path)
        if (botAI->rpgInfo.GetStatus() != RPG_DUNGEON_PVE)
        {
            LOG_DEBUG("playerbots", "[Guild RPG] Bot {} dungeon run ended (status no longer DUNGEON_PVE)", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            SyncGuildRpgStatus();
            return true;
        }

        // Timeout safety
        if (botAI->rpgInfo.HasStatusPersisted(dungeonRunDuration))
        {
            LOG_DEBUG("playerbots", "[Guild RPG] Bot {} dungeon run timed out", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            SyncGuildRpgStatus();
            return true;
        }

        return false;
    }
    else if (activity == GuildRpgActivity::RUN_RAID)
    {
        // Not yet implemented
        botAI->guildRpgInfo.ResetGuildActivity(true);
        return false;
    }
    else if (activity == GuildRpgActivity::WORLD_EVENT)
    {
        // Not yet implemented
        botAI->guildRpgInfo.ResetGuildActivity(true);
        return false;
    }

    return false;
}

bool GuildRpgPveAction::HandleCompletion(Event event)
{
    LOG_DEBUG("playerbots", "[Guild RPG] Bot {} PVE activity completed, cleaning up", bot->GetName());

    // Teleport group to a random major city
    uint8 faction = bot->GetTeamId();
    const auto& cityNodes = (faction == TEAM_HORDE) ? s_hordeCityNodes : s_allianceCityNodes;
    uint32 cityNode = cityNodes[urand(0, cityNodes.size() - 1)];

    TaxiNodesEntry const* taxiNodeEntry = sTaxiNodesStore.LookupEntry(cityNode);
    if (taxiNodeEntry)
    {
        uint32 cityMapId = taxiNodeEntry->map_id;

        Group* group = bot->GetGroup();
        if (group)
        {
            for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
            {
                Player* member = gref->GetSource();
                if (!member)
                    continue;

                member->TeleportTo(cityMapId, taxiNodeEntry->x, taxiNodeEntry->y, taxiNodeEntry->z, 0.0f);
            }
        }
    }

    botAI->guildRpgInfo.ResetGuildActivity();
    botAI->rpgInfo.ChangeToIdle();
    EndGuildRpgActivity();
    botAI->LeaveOrDisbandGroup();
    return true;
}
