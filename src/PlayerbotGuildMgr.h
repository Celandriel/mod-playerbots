#ifndef _PLAYERBOT_PLAYERBOTGUILDMGR_H
#define _PLAYERBOT_PLAYERBOTGUILDMGR_H

#include "Player.h"
#include <vector>
#include <string>

class PlayerbotAI;

namespace ai
{
    class PlayerbotGuildMgr
    {
    public:
        static PlayerbotGuildMgr* instance()
        {
            static PlayerbotGuildMgr instance;
            return &instance;
        }

        void LoadGuilds();
        int8 DetermineGuildType();
        void AssignToGuild(Player* player);
        void SaveDirtyGuilds();
        void CreateRPGGuilds();

    private:
        PlayerbotGuildMgr();
        std::vector<uint32> guildTypeRatios;
        std::vector<uint32> guildNumPlayers;

        // Guild cache structure
        struct GuildCache {
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
    };
}

#define sPlayerbotGuildMgr ai::PlayerbotGuildMgr::instance()

#endif