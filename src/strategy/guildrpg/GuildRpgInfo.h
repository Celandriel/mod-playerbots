#ifndef _PLAYERBOT_GUILDRPGTASKS_H
#define _PLAYERBOT_GUILDRPGTASKS_H


enum class GuildRpgPhase
{
    IDLE,
    SELECTION,
    GROUPING,
    PREPARATION,
    EXECUTING,
    COMPLETED,
};

GuildRpgActivityKey = std::pair<GuildType, int>;
GuildRpgActivityMap = std::map<ActivityKey, std::string>;

GuildRpgActivityMap Activities = {
    // PVP
    {{GuildType::PVP, 1}, "QUEUE_FOR_BG"},
    {{GuildType::PVP, 2}, "PATROL_AREA"},
    {{GuildType::PVP, 3}, "ATTACK_CITY"},
    {{GuildType::PVP, 4}, "DEFEND_BASE"},
    {{GuildType::PVP, 5}, "WORLD_PVP"},
    
    // PvE
    {{GuildType::PVE, 1}, "RUN_DUNGEON"},
    {{GuildType::PVE, 2}, "RUN_RAID"},
    {{GuildType::PVE, 3}, "WORLD_EVENT"},
    
    // Profession
    {{GuildType::PROFESSION, 1}, "GATHER_NODES"},
    {{GuildType::PROFESSION, 2}, "FARM_MOBS"},
    {{GuildType::PROFESSION, 3}, "MASS_CRAFT"},
    {{GuildType::PROFESSION, 4}, "FISHING"},
    
    // Roleplay
    {{GuildType::ROLEPLAY, 1}, "INNS_MEETUP"},
    {{GuildType::ROLEPLAY, 2}, "EMOTE_EVENT"}
};


struct GuildRpgInfo
{
    GuildRpgActivityKey activity;
    void SetGuildRpgActivity(uint8 activity);
    void ResetGuildActivity();
}

#endif