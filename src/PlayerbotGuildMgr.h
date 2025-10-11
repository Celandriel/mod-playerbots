#ifndef _PLAYERBOT_PLAYERBOTGUILDMGR_H
#define _PLAYERBOT_PLAYERBOTGUILDMGR_H

#include "Guild.h"
#include "Player.h"
#include "PlayerbotAI.h"

constexpr std::array<GuilderType, 6> GuilderMap = 
{
    GuilderType::SOLO,
    GuilderType::TINY,
    GuilderType::SMALL,
    GuilderType::MEDIUM,
    GuilderType::LARGE,
    GuilderType::VERY_LARGE
};

class PlayerbotAI;

class PlayerbotGuildMgr
{
public:
    static PlayerbotGuildMgr* instance()
    {
        static PlayerbotGuildMgr instance;
        return &instance;
    }

    void Init();
    int8 DetermineGuildType();
    void AssignToGuild(Player* player);
    void SaveDirtyGuilds();
    void ValidateGuildCache();
    void ResetGuildCache();

    void SetBotAvailability(uint32 guildId, ObjectGuid guid, BotAvailabilityStatus status);
    std::vector<ObjectGuid> GetAvailableGuildMembers(uint32 guildId);
    BotAvailabilityStatus GetBotAvailability(uint32 guildId, ObjectGuid guid);

private:
    PlayerbotGuildMgr();
    std::vector<uint32> guildTypeRatios;
    std::vector<uint32> guildNumPlayers;

    struct GuildCache
    {
        std::string name;
        uint8 type;
        uint8 status;
        uint32 guildID = 0;
        uint32 maxMembers = 0;
        uint32 memberCount = 0;
        uint8 faction = 0;
        bool dirty = false;
        Guild* guildPtr = nullptr;
    };
    std::unordered_map<std::string, GuildCache> guildCache;
    std::unordered_map<uint32, std::unordered_map<ObjectGuid, BotAvailabilityStatus>> guildBotStates;
};


void PlayerBotsGuildValidationScript();

#define sPlayerbotGuildMgr PlayerbotGuildMgr::instance()

#endif