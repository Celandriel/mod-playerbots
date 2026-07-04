/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "QuestSpawnIndex.h"

#include "CreatureData.h"
#include "GameObjectData.h"
#include "Log.h"
#include "ObjectMgr.h"

QuestSpawnIndex* QuestSpawnIndex::instance()
{
    static QuestSpawnIndex inst;
    return &inst;
}

void QuestSpawnIndex::Init()
{
    if (_initialized)
        return;

    uint32 creatures = 0;
    uint32 gos = 0;

    for (auto const& kv : sObjectMgr->GetAllCreatureData())
    {
        CreatureData const& cd = kv.second;
        if (!cd.id)
            continue;
        Key const key{cd.mapid, static_cast<int32>(cd.id)};
        _index[key].emplace_back(cd.mapid, cd.posX, cd.posY, cd.posZ, cd.orientation);
        ++creatures;
    }

    for (auto const& kv : sObjectMgr->GetAllGOData())
    {
        GameObjectData const& gd = kv.second;
        if (!gd.id)
            continue;
        // Negative entry encodes GO (matches Quest::RequiredNpcOrGo
        // convention used by the do-quest action callers).
        Key const key{gd.mapid, -static_cast<int32>(gd.id)};
        _index[key].emplace_back(gd.mapid, gd.posX, gd.posY, gd.posZ, gd.orientation);
        ++gos;
    }

    _initialized = true;

    LOG_INFO("playerbots",
             ">> QuestSpawnIndex: indexed {} creature spawns + {} GO spawns ({} unique keys).",
             creatures, gos, static_cast<uint32>(_index.size()));
}

std::vector<WorldPosition> const& QuestSpawnIndex::GetSpawns(uint32 mapId, int32 entry) const
{
    auto it = _index.find(Key{mapId, entry});
    return (it != _index.end()) ? it->second : _empty;
}
