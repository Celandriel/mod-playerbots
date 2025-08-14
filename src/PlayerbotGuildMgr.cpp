#include "PlayerbotGuildMgr.h"
#include "PlayerbotAIConfig.h"
#include "DatabaseEnv.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "RandomPlayerbotMgr.h"
#include "ScriptMgr.h"

using namespace ai;

PlayerbotGuildMgr::PlayerbotGuildMgr()
{
    // Initialize guild type ratios and player counts
    guildTypeRatios = sPlayerbotAIConfig->Guild_TypeRatios;
    guildNumPlayers = sPlayerbotAIConfig->Guild_Num_Bots;
    if(sPlayerbotAIConfig->Guild_TypeRatios.size() < 5)
    {
        LOG_ERROR("playerbots", "Guild type ratios not defined correctly. Using default values.");
        guildTypeRatios = {40, 20, 20, 10, 10}; // Default values
        guildNumPlayers = {100, 100, 50, 50, 50}; // Default values
    }
}

void PlayerbotGuildMgr::Init()
{
    guildCache.clear();
    LOG_INFO("playerbots", "Loading guilds from database");
    QueryResult result = CharacterDatabase.Query("SELECT name, guild_type, status, guildID, faction FROM playerbots_guild_names");
    if(!result)
    {
        LOG_ERROR("playerbots", "No guild names found in database");
        return;
    }
    do
    {
        Field* fields = result->Fetch();
        if (!fields)
        {
            LOG_WARN("playerbots", "Error fetching guild name fields");
            continue;
        }
        std::string name = fields[0].Get<std::string>();
        uint8 type = fields[1].Get<uint8>();
        uint8 status = fields[2].Get<uint8>();
        uint32 guildId = fields[3].Get<uint32>();
        uint8 faction = fields[4].Get<uint8>();

        if (type == 0 || type > guildTypeRatios.size())
        {
            LOG_WARN("playerbots", "Invalid guild type [{}] for guild [{}]", type, name);
            type = 0;
        }
        if (status > 2)
        {
            LOG_WARN("playerbots", "Invalid guild status [{}] for guild [{}]", status, name);
            status = 0; // Default to 0 if invalid
        }
        GuildCache& entry = guildCache[name];
        entry.name = name;
        entry.status = status;
        entry.type = type;
        entry.guildID = guildId;
        entry.faction = faction;

        if (entry.guildID != 0)
        {
            if (Guild* guild = sGuildMgr->GetGuildById(entry.guildID))
            {
                entry.memberCount = guild->GetMemberCount();
            }
            else
            {
                LOG_ERROR("playerbots", "Guild ID [{}] for guild [{}] not found in cache", entry.guildID, name);
                entry.memberCount = 0; // Reset if guild not found
            }
        }
        else
            entry.memberCount = 0; // No guild ID means no members

        if (type > 0 && static_cast<size_t>(type) <= guildNumPlayers.size())
        {
            entry.maxMembers = guildNumPlayers[type - 1];
        }
        else
        {
            LOG_WARN("playerbots", "Guild type [{}] for guild [{}] has no max members defined", type, name);
            entry.maxMembers = 0; // Default to 0 if no max members defined
        }
        
        entry.dirty = false;
        } while (result->NextRow());
}



int8 PlayerbotGuildMgr::DetermineGuildType()
{
    if (guildTypeRatios.empty())
    {
        LOG_ERROR("playerbots", "Guild type names not defined");
        return 1;
    }

    const size_t nTypes = guildTypeRatios.size();

    // Validate that config ratios vector length matches expected:
    if (sPlayerbotAIConfig->Guild_TypeRatios.size() < nTypes)
    {
        LOG_WARN("playerbots", "Guild_TypeRatios length ({}) < number of type names ({}). Missing values default to 0.", sPlayerbotAIConfig->Guild_TypeRatios.size(), nTypes);
    }

    // If no guilds loaded, default to 1:
    if (guildCache.empty())
    {
        LOG_INFO("playerbots", "No guilds loaded in cache; defaulting to type 1");
        return 1;
    }

    std::vector<int> currentGuildCounts(nTypes, 0);
    int totalGuilds = 0;
    for (const auto& kv : guildCache)
    {
        const GuildCache& gc = kv.second;
        if (gc.status > 0 && gc.type > 0 && static_cast<size_t>(gc.type) <= nTypes) // validate index
        {
            currentGuildCounts[gc.type - 1]++;
            totalGuilds++;
        }
    }

    if (totalGuilds == 0)
        return 1;

    std::vector<float> currentRatios(nTypes, 0.0f);
    for (size_t i = 0; i < nTypes; ++i)
        currentRatios[i] = (static_cast<float>(currentGuildCounts[i]) / static_cast<float>(totalGuilds)) * 100.0f;

    float maxDiff = -std::numeric_limits<float>::infinity();
    int8 bestType = 1; // default
    for (size_t i = 0; i < nTypes; ++i)
    {
        float desired = 0.0f;
        if (i < sPlayerbotAIConfig->Guild_TypeRatios.size())
            desired = sPlayerbotAIConfig->Guild_TypeRatios[i];

        float diff = desired - currentRatios[i];
        if (diff > maxDiff)
        {
            maxDiff = diff;
            bestType = static_cast<int8>(i + 1);
        }
    }
    return bestType;
}


void PlayerbotGuildMgr::AssignToGuild(Player* player)
{
    if (!player || !sPlayerbotAIConfig->enableGuildRPG)
        return;

    LOG_DEBUG("playerbots", "Assigning player [{}] to a guild", player->GetName());
    
    std::vector<GuildCache*> partiallyfilledguilds;
    partiallyfilledguilds.reserve(guildCache.size());
    for (auto& kv : guildCache)
    {
        GuildCache& cached = kv.second;
        if (cached.status == 1 && cached.memberCount < cached.maxMembers && cached.faction == player->GetTeamId())
        {
            partiallyfilledguilds.push_back(&cached);
        }
    }

    if (!partiallyfilledguilds.empty())
    {
        size_t idx = static_cast<size_t>(urand(0, static_cast<int>(partiallyfilledguilds.size()) - 1));
        GuildCache* chosen = partiallyfilledguilds[idx];

        uint32 chosenGuildId = chosen->guildID;
        Guild* guild = chosen->guildPtr ? chosen->guildPtr : sGuildMgr->GetGuildById(chosen->guildID);
        if (guild)
        {
            if (!chosen->guildPtr)
            {
                chosen->guildPtr = guild;
            }
            if (guild->AddMember(player->GetGUID()))
            {
                chosen->memberCount++;
                if (chosen->memberCount >= chosen->maxMembers)
                {
                    chosen->status = 2;
                    chosen->dirty = true;
                }
                LOG_DEBUG("playerbots", "Successfully added player [{}] to guild [{}] (ID: {})", 
                        player->GetName(), guild->GetName(), guild->GetId());
                return;
            }
            else
            {
                LOG_ERROR("playerbots", "Failed to add player [{}] to guild [{}]", 
                        player->GetName(), guild->GetName());
                return;
            }
        }
        else
        {
            LOG_ERROR("playerbots", "Guild ID {} not found in GuildMgr", chosen->guildID);
            // Mark this cache entry as invalid
            chosen->guildPtr = nullptr;
            chosen->status = 0; // or some error status
        }
    }

    // No partial guilds: determine type and pick an available one
    // Keep lock while we snapshot list of available guild names (store pointers to avoid copies)
    uint8 guildType = DetermineGuildType();

    std::vector<GuildCache*> availableGuilds;
    for (auto& kv : guildCache)
    {
        GuildCache& cached = kv.second;
        if (cached.status == 0 && cached.type == guildType) // NOTE: DetermineGuildType locks internally; could cause deadlock
        {
            availableGuilds.push_back(&cached);
        }
    }

    if (availableGuilds.empty())
    {
        LOG_ERROR("playerbots", "No available guilds of required type");
        return;
    }

    // choose an available guild
    size_t chosenIdx = static_cast<size_t>(urand(0, static_cast<int>(availableGuilds.size()) - 1));
    GuildCache* chosenCache = availableGuilds[chosenIdx];
    std::string chosenName = chosenCache->name;

    Guild* existing = sGuildMgr->GetGuildByName(chosenName);
    if (!existing)
    {
        Guild* newGuild = new Guild();
        newGuild->Create(player, chosenName);
        newGuild->AddMember(player->GetGUID());

        GuildCache& entry = guildCache[chosenName];
        entry.guildID = newGuild->GetId();
        entry.memberCount = 1;
        entry.maxMembers = guildNumPlayers[entry.type - 1];
        entry.status = 1;
        entry.faction = player->GetTeamId();
        entry.dirty = true;
        entry.guildPtr = newGuild;
        return;
    }
    LOG_ERROR("playerbots", "Unable to assign player [{}] to guild [{}].", 
            player->GetName(), chosenName);
    return;
}

void PlayerbotGuildMgr::ResetGuildCache()
{
    for (auto it = guildCache.begin(); it != guildCache.end();)
        {
            GuildCache& cached = it->second;
            cached.guildID = 0;
            cached.guildPtr = nullptr;
            cached.memberCount = 0;
            cached.faction = 2;
            cached.status = 0;
            cached.dirty = true;
        }
}

void PlayerbotGuildMgr::ValidateGuildCache()
{
    QueryResult guild_members = CharacterDatabase.Query("SELECT guildID, guid FROM guild_member");
    if (!guild_members)
    {
        LOG_ERROR("playerbots", "No guilds found in database, resetting guild cache");
        ResetGuildCache();
        return;
    }

    std::unordered_map<uint32, uint32> playerCounts;
    do
    {
        Field* fields = guild_members->Fetch();
        uint32 guildId = fields[0].Get<uint32>();
        playerCounts[guildId]++;
    } while (guild_members->NextRow());
    
    QueryResult guild_table = CharacterDatabase.Query("SELECT guildid, name FROM guild");
    if (!guild_table)
    {
        LOG_ERROR("playerbots", "No guilds found in database, resetting guild cache");
        ResetGuildCache();
        return;
    }

    std::unordered_map<std::string, uint32> guildNameToId;
    do
    {
        Field* fields = guild_table->Fetch();
        std::string guildName = fields[1].Get<std::string>();
        uint32 guildId = fields[0].Get<uint32>();
    } while (guild_table->NextRow());

    for (auto& [guildName, cached] : guildCache)
    {
        auto dbGuildIt = guildNameToId.find(guildName);

        if (dbGuildIt == guildNameToId.end())
        {
            if (cached.status != 0)
            {
                cached.status = 0;
                cached.guildID = 0;
                cached.memberCount = 0;
                cached.faction = 2; // Neutral faction
                cached.guildPtr = nullptr;
                cached.dirty = true;
                LOG_INFO("playerbots", "Cached guild [{}] not found in DB, resetting status to 0", guildName);
            }
            continue;
        }

        uint32 guildId = dbGuildIt->second;
        uint32 dbMemberCount = playerCounts[guildId];

        if (cached.memberCount != dbMemberCount)
        {
            cached.memberCount = dbMemberCount;
            cached.dirty = true;
        }
            
        uint8 expectedStatus = 0;
        if (dbMemberCount == 0)
            expectedStatus = 0; // empty
        else if (dbMemberCount < cached.maxMembers)
            expectedStatus = 1; // partially filled
        else
            expectedStatus = 2; // full

        if (cached.status != expectedStatus)
        {
            cached.status = expectedStatus;
            cached.dirty = true;
        }
    }
}


void PlayerbotGuildMgr::SaveDirtyGuilds()
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    for (auto& [name, cached] : guildCache)
    {
        if (cached.dirty)
        {
            trans->Append("UPDATE playerbots_guild_names SET status = {}, guildID = {}, faction = {} WHERE name = '{}'",
                          cached.status, cached.guildID, cached.faction, name);
            cached.dirty = false;
        }
    }
    CharacterDatabase.CommitTransaction(trans);
}

class BotGuildCacheWorldScript : public WorldScript
{
    public:

        BotGuildCacheWorldScript() : WorldScript("BotGuildCacheWorldScript"){}

        void OnStartup() override
        {
            if (sPlayerbotAIConfig->enableGuildRPG)
            {
                sPlayerbotGuildMgr->ValidateGuildCache();
                LOG_INFO("server.loading", "Bot guild cache initialized and validated");
            }
        }

        void OnUpdate(uint32 diff) override
        {
            if (!sPlayerbotAIConfig->enableGuildRPG)
                return;
            m_timer += diff;
            m_validateTimer += diff;
            if (m_timer >= m_saveInterval)
            {
                m_timer = 0;
                sPlayerbotGuildMgr->SaveDirtyGuilds();
                LOG_INFO("playerbots", "Bot guild cache saved");
            }
            if (m_validateTimer >= m_saveInterval * 4) // Validate every hour
            {
                m_validateTimer = 0;
                sPlayerbotGuildMgr->ValidateGuildCache();
                LOG_INFO("playerbots", "Bot guild cache validated");
            }
        }
    private:
        uint32 m_saveInterval = 900000; // 15 minutes
        uint32 m_validateTimer;
        uint32 m_timer;

};

void PlayerBotsGuildValidationScript()
{
    new BotGuildCacheWorldScript();
}