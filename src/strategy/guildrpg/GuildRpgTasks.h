#ifndef _PLAYERBOT_GUILDRPGTASKS_H
#define _PLAYERBOT_GUILDRPGTASKS_H


namespace BaseActivity
{
    enum Objectives
    {
        MOVE_TO_POSITION = 1, // Uses 'location'
        FOLLOW_LEADER = 2,    // Follow the group leader
        DISBAND_GROUP = 3     // End the activity
    };
}

namespace PvpActivity
{
    enum Objectives
    {
        QUEUE_FOR_BG = 1,   // Uses 'params'="arathi_basin"
        PATROL_AREA = 2,     // Uses 'location' and 'params'="ashenvale"
        ATTACK_CITY = 3,     // Uses 'params'="stormwind"
        DEFEND_BASE = 4,     // Uses 'location'
        WORLD_PVP = 5        // Roam a contested zone for fights
        // RPG_PVP_DUEL is removed. This is an individual action, not a guild objective.
    };
}

namespace PveActivity
{
    enum Objectives
    {
        RUN_DUNGEON = 1,    // Uses 'params'="deadmines"
        RUN_RAID = 2,       // Uses 'params'="molten_core"
        WORLD_EVENT = 3     // Uses 'params'="darkmoon_faire"
    };
}

namespace ProfessionActivity
{
    enum Objectives
    {
        GATHER_NODES = 1,   // Uses 'location' and 'params'="rich_thorium_vein"
        FARM_MOBS = 2,      // Uses 'location' and 'params'="elemental_water"
        MASS_CRAFT = 3,     // Uses 'params'="mithril_bar|50"
        FISHING = 4         // Uses 'location' (fishing pool)
    };
}

namespace RoleplayActivity
{
    enum Objectives
    {
        INNS_MEETUP = 1, // Uses 'location' (tavern)
        EMOTE_EVENT = 2        // Uses 'params'="dance,wave,cheer"
    };
}

struct GuildTask
{
    GuildActivityType type;    // How to interpret the objectiveId
    uint32 objectiveId;        // The specific goal (e.g., for ACTIVITY_PVP, 2 = "Attack City")
    uint32 priority;           // Task priority
    time_t receivedTime;       // When the task was received
    time_t timeOut;            // When the task should be abandoned

    // Universal targets for the task
    WorldPosition location;
    ObjectGuid target;
    std::string params;        // "stormwind", "deadmines", "mithril_ore", "/dance"

    GuildTask() : type(ACTIVITY_NONE), objectiveId(0), priority(0), receivedTime(0), timeOut(0) {}
    bool IsValid() const;
    bool IsExpired() const;
};

namespace GuildTasksPvp
{
    bool ExecuteAttackCity(PlayerbotAI* botAI, const GuildTask& task);
    bool ExecuteDefendBase(PlayerbotAI* botAI, const GuildTask& task);
    bool ExecuteQueueForBg(PlayerbotAI* botAI, const GuildTask& task);
    bool ExecutePatrol(PlayerbotAI* botAI, const GuildTask& task);
}

namespace GuildTasksPve
{
    bool ExecuteDungeonRun(PlayerbotAI* botAI, const GuildTask& task);
    bool ExecuteRaidRun(PlayerbotAI* botAI, const GuildTask& task);
    bool ExecuteWorldEvent(PlayerbotAI* botAI, const GuildTask& task);
}

namespace GuildTasksProfession
{
    bool ExecuteGatherNodes(PlayerbotAI* botAI, const GuildTask& task);
    bool ExecuteMassCraft(PlayerbotAI* botAI, const GuildTask& task);
}

namespace GuildTasksRoleplay
{
    bool ExecuteTavernGathering(PlayerbotAI* botAI, const GuildTask& task);
    bool ExecuteCeremony(PlayerbotAI* botAI, const GuildTask& task);
}

namespace GuildTasksBase
{
    bool ExecuteMoveToPosition(PlayerbotAI* botAI, const GuildTask& task);
    bool ExecuteFollowLeader(PlayerbotAI* botAI, const GuildTask& task);
}

// 5. Utility function to create common tasks (optional but very useful for leaders)
namespace GuildTaskFactory
{
    GuildTask CreatePvpAttackCityTask(const std::string& cityName, uint32 priority = 100);
    GuildTask CreatePveDungeonRunTask(const std::string& dungeonName, uint32 priority = 100);
    GuildTask CreateProfessionGatherTask(const std::string& resourceName, const WorldPosition& location, uint32 priority = 80);
    GuildTask CreateMoveToTask(const WorldPosition& location, uint32 priority = 50, uint32 timeoutSeconds = 300);
    GuildTask CreateAttackTargetTask(const ObjectGuid& targetGuid, uint32 priority = 150);
}

#endif