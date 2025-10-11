#ifndef _PLAYERBOT_GROUPACTION_H
#define _PLAYERBOT_GROUPACTION_H

#include "Action.h"
#include "PlayerbotAI.h"

class PlayerbotGroupMgr;

class CreateGroupAction : public Action
{
public:
    CreateGroupAction(PlayerbotAI* botAI) : Action(botAI, "create group") {}

    bool Execute(Event event) override;
    bool isUseful() override;
    bool isPossible() override;

private:
    bool ValidateTargetComposition(const TargetGroupComposition& comp);
};

class DisbandGroupAction : public Action
{
public:
    DisbandGroupAction(PlayerbotAI* botAI) : Action(botAI, "disband group") {}

    bool Execute(Event event) override;
};
#endif
