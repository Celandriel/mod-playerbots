#ifndef _PLAYERBOT_PLAYERBOTSGUILDMGR_H
#define _PLAYERBOT_PLAYERBOTSGUILDMGR_H

#include "Player.h"
#include <vector>
#include <string>

class PlayerbotAI;

namespace ai
{
    struct GuildInfo
    {
        std::string name;
        uint32 guild_type;
        uint32 status;
    };

    class PlayerbotsGuildMgr
    {
    public:
        static PlayerbotsGuildMgr* instance()
        {
            static PlayerbotsGuildMgr instance;
            return &instance;
        }

        void LoadGuilds();
        std::string DetermineGuildType();
        bool IsGuildFull(uint32 guildId);
        void CheckGuildFull(uint32 guildId);
        void SaveGuildStatus(const std::string& guildName, uint32 status);
        void AssignToGuild(Player* player);

    private:
        PlayerbotsGuildMgr();
        std::vector<int> guildTypeRatios;
        std::vector<int> guildPlayers;
        std::vector<std::string> guildTypeNames;
        std::vector<GuildInfo> guilds;
        int GetGuildTypeIndex(const std::string& guildType);
    };
}

#define sPlayerbotGuildMgr ai::PlayerbotsGuildMgr::instance()

#endif