#ifndef _PLAYERBOT_DUNGEONPATHSTRATEGY_H
#define _PLAYERBOT_DUNGEONPATHSTRATEGY_H

#include "NonCombatStrategy.h"

class DungeonPathStrategy : public NonCombatStrategy
{
public:
    DungeonPathStrategy(PlayerbotAI* ai);
    ~DungeonPathStrategy();
    std::string const getName() override { return "dungeon path"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    NextAction** getDefaultActions() override;
};

#endif
