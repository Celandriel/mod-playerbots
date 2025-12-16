#ifndef _PLAYERBOT_PLAYERBOTGUILDMGR_H
#define _PLAYERBOT_PLAYERBOTGUILDMGR_H

#include "Guild.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "GuildRpgInfo.h"

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
    GuildType DetermineGuildType();
    std::string AssignToGuild(Player* player);
    void LoadGuildNames();
    void ValidateGuildCache();
    void ResetGuildCache();
    bool CreateGuild(Player* player, std::string guildName);
    void OnGuildUpdate  (Guild* guild);
    GuildType GetGuildTypeByName(std::string guildName);
    GuildType GetGuildTypeById(uint32 guildId);
    bool SetGuildEmblem(uint32 guildId);
    void DeleteBotGuilds();
    bool IsRealGuild(uint32 guildId);
    bool IsRealGuild(Player* bot);

    void SetBotAvailability(uint32 guildId, ObjectGuid guid, BotAvailabilityStatus status);
    std::vector<ObjectGuid> GetAvailableGuildMembers(uint32 guildId);
    BotAvailabilityStatus GetBotAvailability(uint32 guildId, ObjectGuid guid);

private:
    PlayerbotGuildMgr();
    GuildType _defaultGuildType;
    struct GuildInfo
    {
        bool isAvailable;
        GuildType type;
    };
    std::unordered_map<std::string, GuildInfo> _guildData;
    std::vector<uint32> _guildTypeRatios;
    std::vector<uint32> _guildNumPlayers;
    uint8_t const _nTypes = 4;

    struct GuildCache
    {
        std::string name;
        GuildType type = GuildType::NONE;
        uint8 status;
        uint32 maxMembers = 0;
        uint32 memberCount = 0;
        uint8 faction = 0;
        bool hasRealPlayer = false;
    };
    std::unordered_map<uint32 , GuildCache> _guildCache;
//TODO
    std::unordered_map<uint32, std::unordered_map<ObjectGuid, BotAvailabilityStatus>> guildBotStates;
    std::vector<std::string> _shuffled_guild_keys;

};

void PlayerBotsGuildValidationScript();

#define sPlayerbotGuildMgr PlayerbotGuildMgr::instance()

#endif
