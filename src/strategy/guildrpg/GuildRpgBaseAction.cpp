#include "GuildRpgBaseAction.h"
#include "PlayerbotAI.h"
#include "Guild.h"
#include "Group.h"

uint8 GuildRpgBaseAction::RollRandomAction(std::vector<uint32> weights)
{
    uint32 totalRatio = 0;
    for (float ratio : weights) 
        totalRatio += ratio;
    
    if (totalRatio == 0)
        return 0;

    uint32 roll = urand(1, totalRatio);
    uint32 accumulate = 0;
    uint8 objective;
    for (uint8 i = 0; i < weights.size(); ++i) {
        accumulate += weights[i];
        if (roll <= accumulate)
        {
            objective = i;
            break;
        }
    }    
    return objective;
}

Guild* GuildRpgBaseAction::GetGuild() const
{
    Player* player = botAI->GetBot();
    return player ? player->GetGuild() : nullptr;
}
