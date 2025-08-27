
#ifndef _PLAYERBOT_GUILDRPGTARGETVALUE_H
#define _PLAYERBOT_GUILDRPGTARGETVALUE_H

#include "NearestUnitsValue.h"
#include "PlayerbotAIConfig.h"

class GuildRpgTargetValue : public CalculatedValue<GuidVector>
{
public:
    GuildRpgTargetValue(PlayerbotAI* botAI, std::string name = "guild rpg targets") : CalculatedValue<GuidVector>(botAI, name) {}

    GuidVector Calculate() override;
};

#endif