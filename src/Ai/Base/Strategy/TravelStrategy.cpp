/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "TravelStrategy.h"
#include "Playerbots.h"

TravelStrategy::TravelStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

void TravelStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Above "new rpg status update" (11) so a trip outranks new rpg.
    triggers.push_back(
        new TriggerNode(
            "go travel finished",
            {
                NextAction("end go travel", ACTION_NORMAL + 3)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "go travel active",
            {
                NextAction("move to go travel destination", ACTION_NORMAL + 2)
            }
        )
    );
}
