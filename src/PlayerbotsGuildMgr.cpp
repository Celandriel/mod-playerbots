#include "PlayerbotsGuildMgr.h"
#include "PlayerbotAIConfig.h"
#include "DatabaseEnv.h"
#include "Guild.h"
#include "GuildMgr.h"
#include <vector>
#include <string>
#include <sstream>

using namespace ai;

PlayerbotsGuildMgr::PlayerbotsGuildMgr()
{
    // Initialize guild type ratios and player counts
    std::vector<uint32> guildTypeRatios = sPlayerbotAIConfig->Guild_TypeRatios;
    std::vector<uint32> guildPlayers = sPlayerbotAIConfig->Guild_Num_Bots;

    // No need to parse strings anymore, using vectors directly

    guildTypeNames = { "PvP", "PvE", "Gathering/Crafter", "Roleplaying", "Adventurer/Explorer" };
    LoadGuilds();
}

void PlayerbotsGuildMgr::LoadGuilds()
{
    guilds.clear();
    QueryResult result = CharacterDatabase.Query("SELECT name, guild_type, status FROM playerbots_guild_names");
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            GuildInfo guild;
            guild.name = fields[0].Get<std::string>();
            guild.guild_type = fields[1].Get<uint32>();
            guild.status = fields[2].Get<uint32>();
            guilds.push_back(guild);
        } while (result->NextRow());
    }
}

std::string PlayerbotsGuildMgr::DetermineGuildType()
{
    // Check for incomplete guilds
    std::vector<GuildInfo*> incompleteGuilds;
    for (auto& guild : guilds)
    {
        if (guild.status == 1)
        {
            incompleteGuilds.push_back(&guild);
        }
    }

    if (!incompleteGuilds.empty())
    {
        GuildInfo* chosenGuild = incompleteGuilds[urand(0, incompleteGuilds.size() - 1)];
        return guildTypeNames[chosenGuild->guild_type - 1];
    }

    // Calculate current ratios of full guilds
    std::vector<int> currentGuildCounts(guildTypeNames.size(), 0);
    int totalFullGuilds = 0;
    for (const auto& guild : guilds)
    {
        if (guild.status == 2)
        {
            currentGuildCounts[guild.guild_type - 1]++;
            totalFullGuilds++;
        }
    }

    // Determine desired guild type
    if (totalFullGuilds > 0)
    {
        std::vector<float> currentRatios(guildTypeNames.size(), 0.0f);
        for (size_t i = 0; i < guildTypeNames.size(); ++i)
        {
            currentRatios[i] = (float)currentGuildCounts[i] / totalFullGuilds * 100.0f;
        }

        int bestType = -1;
        float maxDiff = -1000.0f;
        for (size_t i = 0; i < guildTypeNames.size(); ++i)
        {
            float diff = (float)guildTypeRatios[i] - currentRatios[i];
            if (diff > maxDiff)
            {
                maxDiff = diff;
                bestType = i;
            }
        }
        if (bestType != -1)
        {
            return guildTypeNames[bestType];
        }
    }

    // Default to highest ratio if no other criteria met
    int highestRatioIndex = 0;
    for (size_t i = 1; i < guildTypeRatios.size(); ++i)
    {
        if (guildTypeRatios[i] > guildTypeRatios[highestRatioIndex])
        {
            highestRatioIndex = i;
        }
    }

    // Find an available guild of the chosen type
    std::vector<GuildInfo*> availableGuilds;
    for (auto& guild : guilds)
    {
        if (guild.status == 0 && (guild.guild_type - 1) == highestRatioIndex)
        {
            availableGuilds.push_back(&guild);
        }
    }

    if (!availableGuilds.empty())
    {
        GuildInfo* chosenGuild = availableGuilds[urand(0, availableGuilds.size() - 1)];
        chosenGuild->status = 1;
        SaveGuildStatus(chosenGuild->name, 1);
    }

    return guildTypeNames[highestRatioIndex];
}

bool PlayerbotsGuildMgr::IsGuildFull(uint32 guildId)
{
    Guild* guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return false;

    for (const auto& guildInfo : guilds)
    {
        if (guildInfo.name == guild->GetName())
        {
            int typeIndex = guildInfo.guild_type - 1;
            if (typeIndex < 0 || typeIndex >= (int)guildPlayers.size())
                return false;
            return guild->GetMemberCount() >= (uint32)guildPlayers[typeIndex];
        }
    }

    return false;
}

void PlayerbotsGuildMgr::CheckGuildFull(uint32 guildId)
{
    if (IsGuildFull(guildId))
    {
        Guild* guild = sGuildMgr->GetGuildById(guildId);
        if (guild)
        {
            for (auto& guildInfo : guilds)
            {
                if (guildInfo.name == guild->GetName())
                {
                    guildInfo.status = 2;
                    SaveGuildStatus(guildInfo.name, 2);
                    break;
                }
            }
        }
    }
}

void PlayerbotsGuildMgr::SaveGuildStatus(const std::string& guildName, uint32 status)
{
    CharacterDatabase.Execute("UPDATE playerbots_guild_names SET status = {} WHERE name = {}", status, guildName.c_str());
}

int PlayerbotsGuildMgr::GetGuildTypeIndex(const std::string& guildType)
{
    for (size_t i = 0; i < guildTypeNames.size(); ++i)
    {
        if (guildTypeNames[i] == guildType)
        {
            return i;
        }
    }
    return -1;
}
void PlayerbotsGuildMgr::AssignToGuild(Player* player)
{
    std::string guildTypeName = DetermineGuildType();
    int typeIndex = GetGuildTypeIndex(guildTypeName);

    if (typeIndex == -1)
        return;

    std::vector<GuildInfo*> availableGuilds;
    for (auto& guild : guilds)
    {
        if (guild.guild_type == (uint32)(typeIndex + 1) && guild.status != 2)
        {
            availableGuilds.push_back(&guild);
        }
    }

    if (availableGuilds.empty())
        return;

    GuildInfo* chosenGuildInfo = availableGuilds[urand(0, availableGuilds.size() - 1)];
    Guild* guild = sGuildMgr->GetGuildByName(chosenGuildInfo->name);

    if (!guild)
    {
        // Create guild if it doesn't exist
        guild = new Guild();
        if (!guild->Create(player, chosenGuildInfo->name))
        {
            delete guild;
            return;
        }
        sGuildMgr->AddGuild(guild);
        chosenGuildInfo->status = 1; // Set to incomplete
        SaveGuildStatus(chosenGuildInfo->name, 1);
    }

    if (guild->GetMemberCount() < guildPlayers[typeIndex])
    {
        guild->AddMember(player->GetGUID());
        CheckGuildFull(guild->GetId());
    }
}
