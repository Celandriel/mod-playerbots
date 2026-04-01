#ifndef _PLAYERBOT_RAIDAQ40ACTIONS_H
#define _PLAYERBOT_RAIDAQ40ACTIONS_H

#include "MovementActions.h"
#include "GenericSpellActions.h"
#include "AttackAction.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

class Aq40UseResistanceBuffsAction : public Action
{
public:
    Aq40UseResistanceBuffsAction(PlayerbotAI* botAI, std::string const name = "aq40 use resistance buffs")
        : Action(botAI, name) {}
    bool Execute(Event event) override;
};

class Aq40MoveFromOtherEmperorAction : public MovementAction
{
public:
    Aq40MoveFromOtherEmperorAction(PlayerbotAI* botAI, std::string const name = "aq40 move from other emperor")
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;
};

BEGIN_RANGED_SPELL_ACTION(Aq40WarlockCastSearingPainAction, "searing pain")
END_SPELL_ACTION()

BEGIN_RANGED_SPELL_ACTION(Aq40MageCastFrostboltAction, "frostbolt")
END_SPELL_ACTION()

class Aq40MeleeViscidusAction : public AttackAction
{
public:
    Aq40MeleeViscidusAction(PlayerbotAI* botAI) : AttackAction(botAI, "aq40 melee viscidus") {}
    bool Execute(Event event) override;
};

class Aq40AttackTargetByNameAction : public AttackAction
{
public:
    Aq40AttackTargetByNameAction(PlayerbotAI* botAI, std::string const name) : AttackAction(botAI, name) {}
    bool Execute(Event event) override;

    virtual std::string const WhichEmperor() { return ""; }
};

class Aq40AttackEmperorVekLorAction : public Aq40AttackTargetByNameAction
{
public:
    Aq40AttackEmperorVekLorAction(PlayerbotAI* botAI) : Aq40AttackTargetByNameAction(botAI, "aq40 attack emperor vek'lor") {}

    std::string const WhichEmperor() override { return "emperor vek'lor"; }
};

class Aq40AttackEmperorVekNilashAction : public Aq40AttackTargetByNameAction
{
public:
    Aq40AttackEmperorVekNilashAction(PlayerbotAI* botAI) : Aq40AttackTargetByNameAction(botAI, "aq40 attack emperor vek'nilash") {}

    std::string const WhichEmperor() override { return "emperor vek'nilash"; }
};

class Aq40AttackEmperorPestsAction : public AttackAction
{
public:
    Aq40AttackEmperorPestsAction(PlayerbotAI* botAI) : AttackAction(botAI, "aq40 attack emperor pests") {}
    bool Execute(Event event) override;
};

class Aq40MoveTowardsEmperorAction : public MovementAction
{
public:
    Aq40MoveTowardsEmperorAction(PlayerbotAI* botAI, std::string const name)
        : MovementAction(botAI, name) {}
    bool Execute(Event event) override;

    virtual std::string const WhichEmperor() { return ""; }
};

class Aq40MoveTowardsEmperorVekLorAction : public Aq40MoveTowardsEmperorAction
{
public:
    Aq40MoveTowardsEmperorVekLorAction(PlayerbotAI* botAI)
        : Aq40MoveTowardsEmperorAction(botAI, "aq40 move towards emperor vek'lor") {}
    
    std::string const WhichEmperor() override { return "emperor vek'lor"; }
};

class Aq40MoveTowardsEmperorVekNilashAction : public Aq40MoveTowardsEmperorAction
{
public:
    Aq40MoveTowardsEmperorVekNilashAction(PlayerbotAI* botAI)
        : Aq40MoveTowardsEmperorAction(botAI, "aq40 move towards emperor vek'nilash") {}
    
    std::string const WhichEmperor() override { return "emperor vek'nilash"; }
};

class Aq40TankAnchorPositionAction : public MovementAction
{
public:
    Aq40TankAnchorPositionAction(PlayerbotAI* botAI)
        : MovementAction(botAI, "aq40 tank anchor position") {}
    bool Execute(Event event) override;
};

class Aq40MoveToRoomCenterAction : public MovementAction
{
public:
    Aq40MoveToRoomCenterAction(PlayerbotAI* botAI)
        : MovementAction(botAI, "aq40 move to room center") {}
    bool Execute(Event event) override;
};

class Aq40MoveFromVeklorAction : public MovementAction
{
public:
    Aq40MoveFromVeklorAction(PlayerbotAI* botAI)
        : MovementAction(botAI, "aq40 move from veklor") {}
    bool Execute(Event event) override;
};

class Aq40OuroBurrowedFleeAction : public Action
{
public:
    Aq40OuroBurrowedFleeAction(PlayerbotAI* botAI, std::string const name = "aq40 ouro burrowed flee")
        : Action(botAI, name) {}
    bool Execute(Event event) override;
};

class Aq40Cthun1PositionAction : public MovementAction
{
public:
    Aq40Cthun1PositionAction(PlayerbotAI* botAI, std::string const name = "aq40 cthun1 get positioned")
        : MovementAction(botAI, name)
    {
        /*
            x=[-8620.924,-8534.281]
            y=[1943.8806,2029.8264]

            -8578.6025,1986.8535 radius 43 = circumference 270, or room for about 20 bots spread 13.5 units apart
                so.. two circles, one at radius 50, one at radius 36?
                    center at -8579.0,1987.0
                    rings 14 units apart
                    outer ring 23 bots about 13.7 units apart
                    inner ring 17 bots about 13.3 units apart
        */
        for (int n = 0; n < outerPointsCount; n++)
        {
            Position* what = new Position(-8579.0F, 1987.0F, 101.0F);
            // 43.0: outside of 25 yard range, inside of 30 yard range
            what->RelocatePolarOffset(n * M_PI * 2.0 / outerPointsCount, 43.0);
            outerPoints[n] = what;
        }

        for (int n = 0; n < innerPointsCount; n++)
        {
            Position* what = new Position(-8579.0F, 1987.0F, 101.0F);
            // 31.0: inner ring radius
            what->RelocatePolarOffset(n * M_PI * 2.0 / innerPointsCount, 31.0);
            innerPoints[n] = what;
        }
    }
    bool Execute(Event event) override;

protected:
    static int WrappingDistanceBetween(int src, int dst, int scope);
    int GetNearestPoint(int excludeouter = -1, bool doinner = true);

    static const int outerPointsCount = 23;
    static const int innerPointsCount = 17;

    Position* outerPoints[outerPointsCount];
    Position* innerPoints[innerPointsCount];
};

class Aq40Cthun2PositionAction : public Aq40Cthun1PositionAction
{
public:
    Aq40Cthun2PositionAction(PlayerbotAI* botAI, std::string const name = "aq40 cthun2 get positioned")
        : Aq40Cthun1PositionAction(botAI, name)
    {
        // oldworld trigger 87646 entry 15384
        getOut = new Position(-8546.2, 1987.2, -96.52);
        insideAttack = new Position(-8550.7, 2000.4, -97.495);
    }
    bool Execute(Event event) override;

protected:
    Position* getOut;
    Position* insideAttack;
    bool outTriggered = false;
    bool attackPositioned = false;
};

#endif
