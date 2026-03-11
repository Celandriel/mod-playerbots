#include "DungeonPathStrategy.h"
#include "DungeonPathMoveAction.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

DungeonPathStrategy::DungeonPathStrategy(PlayerbotAI* ai)
    : NonCombatStrategy(ai)
{
}

DungeonPathStrategy::~DungeonPathStrategy()
{
}

NextAction** DungeonPathStrategy::getDefaultActions()
{
    Player* bot = botAI->GetBot();

    Player* leadingTank = PlayerbotAI::FindGroupTankToFollow(bot, bot);
    bool isBotMainTank = (leadingTank == bot);

    if (isBotMainTank)
        return NextAction::array(0, new NextAction("dungeon path move", 1.0f), nullptr);
    else
        return NextAction::array(0, new NextAction("follow", 1.0f), nullptr);
}

void DungeonPathStrategy::InitTriggers(std::vector<TriggerNode*>& /*triggers*/) {}
