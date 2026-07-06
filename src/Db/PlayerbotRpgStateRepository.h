/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_PLAYERBOTRPGSTATEREPOSITORY_H
#define PLAYERBOTS_PLAYERBOTRPGSTATEREPOSITORY_H

#include <shared_mutex>
#include <unordered_map>

#include "Define.h"

struct RpgPersistedState
{
    uint8  goal{0};
    uint32 zoneId{0};
    uint32 hubMapId{0};
    float  hubX{0.0f};
    float  hubY{0.0f};
    float  hubZ{0.0f};
};

class PlayerbotRpgStateRepository
{
public:
    static PlayerbotRpgStateRepository& instance()
    {
        static PlayerbotRpgStateRepository instance;

        return instance;
    }

    // Load all rows from DB into the cache. Call once at world startup
    // before any bots exist (i.e. after sTravelMgr.Init()).
    void LoadAll();

    // Thread-safe read from the cache.
    bool TryGet(uint32 guidLow, RpgPersistedState& out) const;

    // Update the cache and queue an async DB write if the state changed.
    // Safe to call from map threads (async Execute, never blocks).
    void Save(uint32 guidLow, RpgPersistedState const& state);

private:
    PlayerbotRpgStateRepository() = default;
    ~PlayerbotRpgStateRepository() = default;

    PlayerbotRpgStateRepository(PlayerbotRpgStateRepository const&) = delete;
    PlayerbotRpgStateRepository& operator=(PlayerbotRpgStateRepository const&) = delete;

    PlayerbotRpgStateRepository(PlayerbotRpgStateRepository&&) = delete;
    PlayerbotRpgStateRepository& operator=(PlayerbotRpgStateRepository&&) = delete;

    mutable std::shared_mutex _mutex;
    bool _loaded{false};
    std::unordered_map<uint32, RpgPersistedState> _cache;
};

#define sPlayerbotRpgStateRepository PlayerbotRpgStateRepository::instance()

#endif
