/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "PlayerbotRpgStateRepository.h"

#include "DatabaseEnv.h"
#include "Field.h"
#include "Log.h"
#include "QueryResult.h"
#include "Timer.h"

void PlayerbotRpgStateRepository::LoadAll()
{
    std::unique_lock<std::shared_mutex> lock(_mutex);

    // Idempotence guard: a runtime "playerbots reload" must not re-run this
    // and overwrite newer in-RAM state or block the world thread a second time.
    // Follows the RandomPlayerbotMgr formatted-Execute precedent (raw formatted
    // SQL rather than prepared statements — acceptable for startup-only bulk load).
    if (_loaded)
        return;
    _loaded = true;

    _cache.clear();

    QueryResult result = PlayerbotsDatabase.Query(
        "SELECT `guid`, `goal`, `zone_id`, `hub_map`, `hub_x`, `hub_y`, `hub_z` "
        "FROM `playerbots_rpg_state`");

    uint32 count = 0;
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 const guidLow = fields[0].Get<uint32>();
            RpgPersistedState state;
            state.goal     = fields[1].Get<uint8>();
            state.zoneId   = fields[2].Get<uint32>();
            state.hubMapId = fields[3].Get<uint32>();
            state.hubX     = fields[4].Get<float>();
            state.hubY     = fields[5].Get<float>();
            state.hubZ     = fields[6].Get<float>();

            _cache[guidLow] = state;
            ++count;
        } while (result->NextRow());
    }

    LOG_INFO("playerbots", "Loaded {} bot RPG state rows from playerbots_rpg_state", count);
}

bool PlayerbotRpgStateRepository::TryGet(uint32 guidLow, RpgPersistedState& out) const
{
    std::shared_lock<std::shared_mutex> lock(_mutex);
    auto it = _cache.find(guidLow);
    if (it == _cache.end())
        return false;
    out = it->second;
    return true;
}

void PlayerbotRpgStateRepository::Save(uint32 guidLow, RpgPersistedState const& state)
{
    {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        auto it = _cache.find(guidLow);
        if (it != _cache.end())
        {
            RpgPersistedState const& cached = it->second;
            if (cached.goal     == state.goal     &&
                cached.zoneId   == state.zoneId   &&
                cached.hubMapId == state.hubMapId  &&
                cached.hubX     == state.hubX      &&
                cached.hubY     == state.hubY      &&
                cached.hubZ     == state.hubZ)
            {
                return;
            }
        }
        _cache[guidLow] = state;
    }

    PlayerbotsDatabase.Execute(
        "INSERT INTO `playerbots_rpg_state` (`guid`, `goal`, `zone_id`, `hub_map`, `hub_x`, `hub_y`, `hub_z`) "
        "VALUES ({}, {}, {}, {}, {}, {}, {}) "
        "ON DUPLICATE KEY UPDATE "
        "`goal` = VALUES(`goal`), `zone_id` = VALUES(`zone_id`), `hub_map` = VALUES(`hub_map`), "
        "`hub_x` = VALUES(`hub_x`), `hub_y` = VALUES(`hub_y`), `hub_z` = VALUES(`hub_z`)",
        guidLow, state.goal, state.zoneId, state.hubMapId, state.hubX, state.hubY, state.hubZ);
}
