#include "PlayerbotGuildMgr.h"
#include "PlayerbotAIConfig.h"
#include "DatabaseEnv.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "RandomPlayerbotMgr.h"
#include "ScriptMgr.h"

PlayerbotGuildMgr::PlayerbotGuildMgr()
{
    // Initialize guild type ratios and player counts
    _guildTypeRatios = sPlayerbotAIConfig->GuildTypeRatios;
    _maxIndex = static_cast<uint32>(std::distance(_guildTypeRatios.begin(), std::max_element(
        _guildTypeRatios.begin(),
        _guildTypeRatios.end()))) + 1;
    std::vector<uint32> guildSizes = sPlayerbotAIConfig->GuildSize;
    if (guildSizes.size()!= _nTypes)
    {
        LOG_ERROR("playerbots", "Config Error: GuildSize must have 4 numbers separated by commas.");
        guildSizes = {4,3,2,2};
    }

    for (int selection : guildSizes)
    {
        if (selection >=0 && selection <= 5)
        {
            _guildNumPlayers.push_back(static_cast<uint32>(GuilderMap[selection]));
        }
        else
        {
            LOG_ERROR("playerbots", "Invalid GuildSize set, defaulting to SOLO");
            _guildNumPlayers.push_back(static_cast<uint32>(GuilderMap[0]));
        }
    }
    if(sPlayerbotAIConfig->GuildTypeRatios.size() == _nTypes)
    {
        _guildTypeRatios = sPlayerbotAIConfig->GuildTypeRatios;
    }
    else
    {
        LOG_ERROR("playerbots", "Guild type ratios not defined correctly. Using default values.");
        _guildTypeRatios = {40, 20, 20, 10}; // Default values
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

        if (type == 0 || type > _guildTypeRatios.size())
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

        if (type > 0 && static_cast<size_t>(type) <= _guildNumPlayers.size())
        {
            entry.maxMembers = _guildNumPlayers[type - 1];
        }
        else
        {
            LOG_WARN("playerbots", "Guild type [{}] for guild [{}] has no max members defined", type, name);
            entry.maxMembers = 0; // Default to 0 if no max members defined
        }

        entry.dirty = false;
        } while (result->NextRow());
}

bool PlayerbotGuildMgr::CreateGuild(Player* player, std::string guildName)
{
    Guild* guild = new Guild();
    if (!guild->Create(player, guildName))
    {
        LOG_ERROR("playerbots", "Error creating guild [ {} ] with leader [ {} ]", guildName.c_str(),
            player->GetName().c_str());
        delete guild;
        return false;
    }
    sGuildMgr->AddGuild(guild);

    LOG_DEBUG("playerbots", "Guild created: id={} name='{}'", guild->GetId(), guildName.c_str());

    // create random emblem
    uint32 st, cl, br, bc, bg;
    bg = urand(0, 51);
    bc = urand(0, 17);
    cl = urand(0, 17);
    br = urand(0, 7);
    st = urand(0, 180);

    LOG_DEBUG("playerbots",
        "[TABARD] new guild id={} random -> style={}, color={}, borderStyle={}, borderColor={}, bgColor={}",
        guild->GetId(), st, cl, br, bc, bg);

    // populate guild table with a random tabard design
    CharacterDatabase.Execute(
        "UPDATE guild SET EmblemStyle={}, EmblemColor={}, BorderStyle={}, BorderColor={}, BackgroundColor={} "
        "WHERE guildid={}",
        st, cl, br, bc, bg, guild->GetId());
    LOG_DEBUG("playerbots", "[TABARD] UPDATE done for guild id={}", guild->GetId());

    // Immediate reading for log
    if (QueryResult qr = CharacterDatabase.Query(
            "SELECT EmblemStyle,EmblemColor,BorderStyle,BorderColor,BackgroundColor FROM guild WHERE guildid={}",
            guild->GetId()))
    {
        Field* f = qr->Fetch();
        LOG_DEBUG("playerbots",
            "[TABARD] DB check guild id={} => style={}, color={}, borderStyle={}, borderColor={}, bgColor={}",
            guild->GetId(), f[0].Get<uint8>(), f[1].Get<uint8>(), f[2].Get<uint8>(), f[3].Get<uint8>(), f[4].Get<uint8>());
    }

    GuildCache entry;
    entry.name = guildName;
    entry.guildID = guild->GetId();
    entry.memberCount = 1;
    entry.status = 1;
    entry.type = GetGuildTypeByName(guild->GetName());
    entry.maxMembers = _guildNumPlayers[entry.type - 1];
    entry.faction = player->GetTeamId();
    entry.dirty = true;

    guildCache[guildName] = entry;

    sPlayerbotAIConfig->randomBotGuilds.push_back(guild->GetId());
    return true;
}

int8 PlayerbotGuildMgr::DetermineGuildType()
{
    // If no guilds loaded, find max ratio:
    if (guildCache.empty())
    {
        LOG_INFO("playerbots", "No guilds loaded in cache; defaulting to max");
        return _maxIndex;
    }

    std::vector<int> currentGuildCounts(_nTypes, 0);
    int totalGuilds = 0;
    for (const auto& keyValue : guildCache)
    {
        const GuildCache& gc = keyValue.second;
        if (gc.status > 0 && gc.type > 0 && static_cast<size_t>(gc.type) <= _nTypes) // validate index
        {
            currentGuildCounts[gc.type - 1]++;
            totalGuilds++;
        }
    }

    if (totalGuilds == 0)
        return _maxIndex;

    std::vector<float> currentRatios(_nTypes, 0.0f);
    for (size_t i = 0; i < _nTypes; ++i)
        currentRatios[i] = (static_cast<float>(currentGuildCounts[i]) / static_cast<float>(totalGuilds)) * 100.0f;

    float maxDiff = -std::numeric_limits<float>::infinity();
    int8 bestType = _maxIndex; // default
    for (size_t i = 0; i < _nTypes; ++i)
    {
        float desired = 0.0f;
        if (i < sPlayerbotAIConfig->GuildTypeRatios.size())
            desired = sPlayerbotAIConfig->GuildTypeRatios[i];

        float diff = desired - currentRatios[i];
        if (diff > maxDiff)
        {
            maxDiff = diff;
            bestType = static_cast<int8>(i + 1);
        }
    }
    return bestType;
}

std::string PlayerbotGuildMgr::AssignToGuild(Player* player)
{
    if (!player || !sPlayerbotAIConfig->enableGuildRpgStrategy)
        return "";

    LOG_DEBUG("playerbots", "Assigning player [{}] to a guild", player->GetName());

    int playerFaction = player->GetTeamId();
    std::vector<GuildCache*> partiallyfilledguilds;
    partiallyfilledguilds.reserve(guildCache.size());

    for (auto& keyValue : guildCache)
    {
        GuildCache& cached = keyValue.second;
        if (cached.status == 1 && cached.faction == playerFaction)
            partiallyfilledguilds.push_back(&cached);
    }

    if (!partiallyfilledguilds.empty())
    {
        size_t idx = static_cast<size_t>(urand(0, static_cast<int>(partiallyfilledguilds.size()) - 1));
        return (partiallyfilledguilds[idx]->name);
    }
    // No partial guilds: determine type and pick an available one
    uint8 guildType = DetermineGuildType();
    std::vector<GuildCache*> availableGuilds;
    for (auto& keyValue : guildCache)
    {
        GuildCache& cached = keyValue.second;
        if (cached.status == 0 && cached.type == guildType)
        {
            availableGuilds.push_back(&cached);
        }
    }

    if (availableGuilds.empty())
    {
        LOG_ERROR("playerbots", "No available guilds of required type");
        return "";
    }

    // choose an available guild
    size_t chosenIdx = static_cast<size_t>(urand(0, static_cast<int>(availableGuilds.size()) - 1));
    GuildCache* chosenCache = availableGuilds[chosenIdx];
    LOG_ERROR("playerbots","Assigning player [{}] to guild [{}]", player->GetName(), chosenCache->name);
    return chosenCache->name;
}

uint32 PlayerbotGuildMgr::GetGuildTypeByName(std::string guildName)
{
    if (guildName.empty())
        return 0;
    for (auto& keyValue : guildCache)
    {
        if (keyValue.first == guildName)
        {
            return keyValue.second.type;
        }
    }
    return 0;
}

uint32 PlayerbotGuildMgr::GetGuildTypeById(uint32 guildID)
{
    for (auto& keyValue : guildCache)
    {
        if (keyValue.second.guildID == guildID)
        {
            return keyValue.second.type;
        }
    }
    return 0;
}

void PlayerbotGuildMgr::OnGuildUpdate(Guild* guild)
{
    auto it = guildCache.find(guild->GetName());
    if (it == guildCache.end())
        return;

    GuildCache& entry = it->second;
    entry.memberCount++;
    entry.dirty = true;

    if (entry.memberCount >= entry.maxMembers)
        entry.status = 2; // Full
}

void PlayerbotGuildMgr::ResetGuildCache()
{
    for (auto it = guildCache.begin(); it != guildCache.end();)
        {
            GuildCache& cached = it->second;
            cached.guildID = 0;
            cached.memberCount = 0;
            cached.faction = 2;
            cached.status = 0;
            cached.dirty = true;
        }
}

void PlayerbotGuildMgr::LoadGuildCache()
{
    LOG_INFO("playerbots", "Loading guild cache from playerbots_guild_names...");

    QueryResult result = CharacterDatabase.Query("SELECT name_id, name, guild_type, status, guildID, faction FROM playerbots_guild_names"
    );

    if (!result)
    {
        LOG_INFO("playerbots", "No entries found in playerbots_guild_names. Initial cache is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        std::string name = fields[1].Get<std::string>();
        GuildCache& entry = guildCache[name];

        entry.name = name;
        entry.type = fields[2].Get<uint8>();
        entry.status = fields[3].Get<uint8>();
        entry.guildID = fields[4].Get<uint32>();
        entry.faction = fields[5].Get<uint8>();

        // Initialize other runtime fields
        entry.memberCount = 0; // Will be validated in the next step
        entry.maxMembers = _guildNumPlayers[entry.type - 1];
        entry.dirty = false; // Start clean

        // Use the guild name as the key for the map
        guildCache[entry.name] = entry;

    } while (result->NextRow());

    LOG_INFO("playerbots", "Loaded {} guild cache entries from custom table.", guildCache.size());
}

void PlayerbotGuildMgr::ValidateGuildCache()
{
    bool saveRequired = false;
    QueryResult result = CharacterDatabase.Query("SELECT g.guildid, g.name, COUNT(gm.guid) AS memberCount "
         "FROM guild g LEFT JOIN guild_member gm ON g.guildid = gm.guildid "
         "GROUP BY g.guildid, g.name ORDER BY memberCount DESC;");
    if (!result)
    {
        LOG_ERROR("playerbots", "No guilds found in database, resetting guild cache");
        ResetGuildCache();
        return;
    }
    struct DbGuildInfo {
        uint32 guildID;
        uint32 memberCount;
    };
    std::unordered_map<std::string, DbGuildInfo> dbGuilds;
    do
    {
        Field* fields = result->Fetch();
        uint32 guildId = fields[0].Get<uint32>();
        std::string guildName = fields[1].Get<std::string>();
        uint32 memberCount = fields[2].Get<uint32>();
        dbGuilds[guildName] = {guildId, memberCount};
    } while (result->NextRow());

    for (auto it = guildCache.begin(); it != guildCache.end(); )
    {
        std::string guildName = it->first;
        GuildCache& cached = it->second;
        bool cacheUpdated = false;

        auto dbGuildIt = dbGuilds.find(guildName); // Matched Name

        if (dbGuildIt == dbGuilds.end())
        {
            if (cached.status != 0)
            {
                cached.status = 0;
                cached.guildID = 0;
                cached.memberCount = 0;
                cached.faction = 2; // Neutral faction
                cached.dirty = true;
                LOG_INFO("playerbots", "Cached guild [{}] not found in DB, resetting status to 0", guildName);
            }
        }
        else
        {
            DbGuildInfo& guildInfo = dbGuildIt->second;
            if (cached.guildID != guildInfo.guildID)
            {
                cached.guildID = guildInfo.guildID;
                cached.dirty = true;
                cacheUpdated = true;
            }

            if (cached.memberCount != guildInfo.memberCount)
            {
                cached.memberCount = guildInfo.memberCount;
                cached.dirty = true;
                cacheUpdated = true;
            }

            uint8 expectedStatus = 0;
            if (guildInfo.memberCount == 0)
                expectedStatus = 0; // empty
            else if (guildInfo.memberCount < cached.maxMembers)
                expectedStatus = 1; // partially filled
            else
                expectedStatus = 2; // full

            if (cached.status != expectedStatus)
            {
                cached.status = expectedStatus;
                cached.dirty = true;
                cacheUpdated = true;
            }
        }

        if (cacheUpdated)
        {
            saveRequired = true;
        }
        it++;
    }
    if (saveRequired)
        SaveDirtyGuilds();
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

void PlayerbotGuildMgr::SetBotAvailability(uint32 guildId, ObjectGuid guid, BotAvailabilityStatus status)
{
    guildBotStates[guildId][guid] = status;
}

BotAvailabilityStatus PlayerbotGuildMgr::GetBotAvailability(uint32 guildId, ObjectGuid guid)
{
    auto botguild = guildBotStates.find(guildId);
    if (botguild == guildBotStates.end())
        return BOT_STATUS_OFFLINE;

    auto member = botguild->second.find(guid);
    if (member == botguild->second.end())
        return BOT_STATUS_OFFLINE;

    return member->second;
}
std::vector<ObjectGuid> PlayerbotGuildMgr::GetAvailableGuildMembers(uint32 guildId)
{
    std::vector<ObjectGuid> availableMembers;
    auto botguild = guildBotStates.find(guildId);
    if (botguild == guildBotStates.end())
        return availableMembers;

    for (const auto& [guid, state] : botguild->second)
    {
        if (state == BOT_STATUS_ONLINE)
            availableMembers.push_back(guid);
    }
    return availableMembers;
}


class BotGuildCacheWorldScript : public WorldScript
{
    public:

        BotGuildCacheWorldScript() : WorldScript("BotGuildCacheWorldScript"), _validateTimer(0), _timer(0){}

        void OnStartup() override
        {
            if (sPlayerbotAIConfig->enableGuildRpgStrategy)
            {
                sPlayerbotGuildMgr->LoadGuildCache();
                LOG_INFO("server.loading", "Bot guild cache initialized");
                sPlayerbotGuildMgr->ValidateGuildCache();
                LOG_INFO("server.loading", "Bot guild cache validated");
            }
        }

        void OnUpdate(uint32 diff) override
        {
            if (!sPlayerbotAIConfig->enableGuildRpgStrategy)
                return;
            _timer += diff;
            _validateTimer += diff;
            if (_timer >= _saveInterval)
            {
                _timer = 0;
                sPlayerbotGuildMgr->SaveDirtyGuilds();
                LOG_INFO("playerbots", "Bot guild cache saved");
            }
            if (_validateTimer >= _saveInterval * 4) // Validate every hour
            {
                _validateTimer = 0;
                sPlayerbotGuildMgr->ValidateGuildCache();
                LOG_INFO("playerbots", "Bot guild cache validated");
            }
        }

        void OnShutdown() override
        {
            if (sPlayerbotAIConfig->enableGuildRpgStrategy)
            {
                sPlayerbotGuildMgr->SaveDirtyGuilds();
                LOG_INFO("playerbots", "Bot guild cache saved on shutdown");
            }
        }
    private:
        uint32 _saveInterval = 900000; // 15 minutes
        uint32 _validateTimer;
        uint32 _timer;

};

void PlayerBotsGuildValidationScript()
{
    new BotGuildCacheWorldScript();
}