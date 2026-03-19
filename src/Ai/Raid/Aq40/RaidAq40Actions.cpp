#include "RaidAq40Actions.h"

#include "Playerbots.h"
#include "AttackAction.h"

bool Aq40UseResistanceBuffsAction::Execute(Event event)
{
    switch(bot->getClass())
    {
        case CLASS_HUNTER:
        {
            bool isNatureBoss = false;
            Unit* boss;

            if ((boss = AI_VALUE2(Unit*, "find target", "viscidus")) && boss->IsInCombat())
                isNatureBoss = true;

            else if ((boss = AI_VALUE2(Unit*, "find target", "princess huhuran")) && boss->IsInCombat())
                isNatureBoss = true;

            if (isNatureBoss)
            {
                if (!botAI->HasStrategy("rnature", BotState::BOT_STATE_COMBAT))
                {
                    botAI->ChangeStrategy("+rnature", BOT_STATE_NON_COMBAT);
                    botAI->ChangeStrategy("+rnature", BOT_STATE_COMBAT);
                    return true;
                }
            }
            else if (botAI->HasStrategy("rnature", BotState::BOT_STATE_COMBAT))
            {
                botAI->ChangeStrategy("-rnature", BOT_STATE_NON_COMBAT);
                botAI->ChangeStrategy("-rnature", BOT_STATE_COMBAT);
                return true;
            }
        }
        break;
        case CLASS_PRIEST:
        {
            // paladin aura seems like a waste when priests have buffs that don't have a radius limitation
            if (!botAI->HasStrategy("rshadow", BotState::BOT_STATE_NON_COMBAT))
            {
                botAI->ChangeStrategy("+rshadow", BOT_STATE_NON_COMBAT);
                botAI->ChangeStrategy("+rshadow", BOT_STATE_COMBAT);
                return true;
            }
        }
        break;
        case default;
        break;
    }

    return false;
}



// 88072: The Master's Eye for positioning maybe

bool Aq40MoveFromOtherEmperorAction::Execute(Event event)
{
    const float radius = 120.0f;  // emperors' heal range is 60

    if (Unit* boss1 = AI_VALUE2(Unit*, "find target", "emperor vek'lor"))
    if (Unit* boss2 = AI_VALUE2(Unit*, "find target", "emperor vek'nilash"))
    {
        ObjectGuid botGuid = bot->GetGUID();
        ObjectGuid petGuid = (ObjectGuid)0UL;
        if (Pet* pet = bot->GetPet())
            petGuid = pet->GetGUID();

        Unit* moveAwayFrom = NULL;

        if (boss1->GetTarget() == botGuid || boss1->GetTarget() == petGuid)
        {
            moveAwayFrom = boss2;
            if (petGuid)
                botAI->PetFollow();
        }
        else if (boss2->GetTarget() == botGuid || boss2->GetTarget() == petGuid)
        {
            moveAwayFrom = boss1;
            if (petGuid)
                botAI->PetFollow();
        }

        if (moveAwayFrom != NULL)
        {
            long distToTravel = radius - bot->GetDistance(moveAwayFrom);

            if (distToTravel > 0)
                return MoveAway(moveAwayFrom, distToTravel);
        }
    }

    return false;
}

bool Aq40MeleeViscidusAction::Execute(Event event)
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "viscidus"))
    {
        if (!bot->HasUnitState(UNIT_STATE_MELEE_ATTACKING))
        {
            // this might not work properly, didn't have problems with boss anyway
            ObjectGuid guid = boss->GetGUID();

            botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets")->Set({guid});
            bool result = Attack(boss);
            if (result)
            {
                bot->AddUnitState(UNIT_STATE_MELEE_ATTACKING);
                context->GetValue<ObjectGuid>("pull target")->Set(guid);
            }

            return result;
        }
    }

    return false;
}

bool Aq40AttackTargetByNameAction::Execute(Event event)
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", WhichEmperor()))
    {
        if (bot->GetTarget() != boss->GetGUID())
        {
            ObjectGuid guid = boss->GetGUID();

            botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets")->Set({guid});
            bool result = Attack(boss);
            if (result)
                context->GetValue<ObjectGuid>("pull target")->Set(guid);

            return result;
        }
    }
    return false;
}

bool Aq40AttackEmperorPestsAction::Execute(Event event)
{
    Unit* current = AI_VALUE(Unit*, "current target");

    if (current && (current->GetName() == "qiraji scarab" || current->GetName() == "qiraji scorpion"))
        return false;


    Unit* pest1 = AI_VALUE2(Unit*, "find target", "qiraji scarab");
    Unit* pest2 = AI_VALUE2(Unit*, "find target", "qiraji scorpion");
    Unit* pest;

    if (pest1 && pest2)
    {
        if (pest1->GetDistance(bot) < pest2->GetDistance(bot))
            pest = pest1;

        else
            pest = pest2;
    }
    else if (pest1)
        pest = pest1;

    else if (pest2)
        pest = pest2;

    else
        return false;


    ObjectGuid guid = pest->GetGUID();
    botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets")->Set({guid});
    bool result = Attack(pest);
    if (result)
        context->GetValue<ObjectGuid>("pull target")->Set(guid);

    return result;
}

bool Aq40MoveTowardsEmperorAction::Execute(Event event)
{
    const float radius = 20.0f; // assume general healing range of 40, try to keep healers near the tanks/victims

    if (Unit* boss = AI_VALUE2(Unit*, "find target", WhichEmperor()))
    {
        if (ObjectGuid bossTarget = boss->GetTarget())
        {
            Unit* target = botAI->GetUnit(bossTarget);

            long travelTarger = -1;//radius - bot->GetDistance(target);
            long travelBoss = radius - bot->GetDistance(boss);

            if (travelTarger > travelBoss)
                return MoveTo(target, travelTarger);

            else
                return MoveTo(boss, travelBoss);
        }
    }

    return false;
}

bool Aq40OuroBurrowedFleeAction::Execute(Event event)
{
    bool doFlee = false;

    if (Unit* boss = AI_VALUE2(Unit*, "find target", "ouro"))
    {
        if (boss->IsInCombat() && (boss->GetUnitFlags() & UNIT_FLAG_NOT_SELECTABLE) == UNIT_FLAG_NOT_SELECTABLE)
        {
            doFlee = true;

            // todo: if ankle-biter scarab is near, doFlee = false
        }

    }
    /*
    else if (Unit* boss = AI_VALUE2(Unit*, "find target", "eye of c'thun"))
    {
        if (boss->IsInCombat())
        {
            float dist = bot->GetDistance(boss);

            //printf("eyedist %f\n",dist);

            doFlee = dist < 13.0;

            if (bot->GetName() == "Snusnu")
            {
                printf("eyedist %f doFlee %d\n",dist,doFlee);
            }
        }
    }
    */


    if (doFlee)
    {
        if (!botAI->HasStrategy("move from group", BotState::BOT_STATE_COMBAT))
        {
            // add/remove from both for now as it will make it more obvious to
            // player if this strat remains on after fight somehow
            // (which it will in this case, todo: remove after relevant, or just issue 'follow' command to bots)
            botAI->ChangeStrategy("+move from group", BOT_STATE_NON_COMBAT);
            botAI->ChangeStrategy("+move from group", BOT_STATE_COMBAT);
        }
    }
    else if (botAI->HasStrategy("move from group", BotState::BOT_STATE_COMBAT))
    {
        // add/remove from both for now as it will make it more obvious to
        // player if this strat remains on after fight somehow
        // (which it will in this case, todo: remove after relevant, or just issue 'follow' command to bots)
        botAI->ChangeStrategy("-move from group", BOT_STATE_NON_COMBAT);
        botAI->ChangeStrategy("-move from group", BOT_STATE_COMBAT);
    }

    return true;
}

int Aq40Cthun1PositionAction::WrappingDistanceBetween(int first, int second, int scope)
{
    int retVal = first - second;
    if (abs(retVal) > scope / 2)
    {
        if (retVal > scope / 2)
            retVal = retVal - scope;

        else
            retVal = scope + retVal;
    }
    return retVal;
}

int Aq40Cthun1PositionAction::GetNearestPoint(int excludeouter, bool doinner)
{
    int retVal = -1;
    float retValDist = 0.0F;

    for (int n = 0; n < outerPointsCount; n++)
    {
        float dist = bot->GetDistance(outerPoints[n]->GetPositionX(), outerPoints[n]->GetPositionY(), outerPoints[n]->GetPositionZ());

        if (retVal < 0 || retValDist > dist)
        {
            if (excludeouter != n)
            {
                retVal = n;
                retValDist = dist;
            }
        }
    }

    if (doinner)
    {
        for (int n = 0; n < innerPointsCount; n++)
        {
            float dist = bot->GetDistance(innerPoints[n]->GetPositionX(), innerPoints[n]->GetPositionY(), innerPoints[n]->GetPositionZ());

            if (retVal < 0 || retValDist > dist)
            {
                retVal = n + outerPointsCount;
                retValDist = dist;
            }
        }
    }

    return retVal;
}

bool Aq40Cthun1PositionAction::Execute(Event event)
{
    // boss_cthun.cpp
    const int SPELL_GREEN_BEAM                            = 26134;
    const int SPELL_RED_COLORATION                        = 22518;        //Probably not the right spell but looks similar


    ObjectGuid botGuid = bot->GetGUID();

    if (Unit* boss = AI_VALUE2(Unit*, "find target", "eye of c'thun"))
    {
        Position* point = NULL;

        ObjectGuid bossTargetguid = boss->GetTarget();
        if (boss->HasAura(SPELL_RED_COLORATION))
        {
            int bossd = (int)(boss->GetOrientation() * outerPointsCount / (M_PI * 2.0));
            if (bossd < 0)
            {
                bossd = bossd + outerPointsCount * ((-bossd) / outerPointsCount + 1);
            }
            bossd %= outerPointsCount;

            int nearest = GetNearestPoint(bossd, false);
            int dist = WrappingDistanceBetween(bossd, nearest, outerPointsCount);

            if (abs(dist) <= outerPointsCount / 6)
            {
                nearest = (nearest + outerPointsCount - dist * (outerPointsCount / 6) / abs(dist)) % outerPointsCount;
            }

            point = outerPoints[nearest];

            if (bot->GetDistance(*point) < 0.5)
            {
                return false;
            }
        }
        else
        {
            if (bossTargetguid == botGuid)
            {
                bot->StopMoving();
                return true;
            }
            else
            {
                Spell* spell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);

                if (spell and spell->m_spellInfo->Id == SPELL_GREEN_BEAM)
                {
                    Unit* bossTarget = botAI->GetUnit(bossTargetguid);
                    float bdist = bot->GetDistance(bossTarget);

                    if (bdist < 20.0)
                    {
                        if (bdist <= 10.0)
                        {
                            bot->AttackStop();
                            //printf("%s: still %f away from %s\n",bot->GetName().c_str(),bdist,bossTarget->GetName().c_str());
                        }

                        return MoveAway(bossTarget, 20.0);
                    }
                }
            }
        }

        if (!point)
        {
            Group* group = bot->GetGroup();
            if (bot->GetDistance(boss) < 60.0F && group)
            {
                Player* closest = NULL;
                float closestDist = 0.0F;

                for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                {
                    Player* member = itr->GetSource();

                    if (member->GetGUID() != botGuid && member->IsAlive())
                    {
                        float dist = bot->GetDistance(member);

                        if (dist < 10.0 && (closest == NULL || closestDist < dist))
                        {
                            closest = member;
                            closestDist = dist;
                        }
                    }
                }

                if (closest)
                {
                    return MoveAway(closest, 10.0);
                }
            }

            int nearest = GetNearestPoint();

            if (nearest >= outerPointsCount)
            {
                point = innerPoints[nearest - outerPointsCount];
            }
            else
            {
                point = outerPoints[nearest];
            }

            if (bot->GetDistance(*point) < 0.5)
            {
                return false;
            }
        }

        if (point)
        {
            return MoveTo(bot->GetMapId(), point->GetPositionX(), point->GetPositionY(), point->GetPositionZ(),
                false, false, false, true, MovementPriority::MOVEMENT_COMBAT);
        }
    }

    return false;
}

bool Aq40Cthun2PositionAction::Execute(Event event)
{
    // boss_cthun.cpp
    const int NPC_TRIGGER                                 = 15384;
    const int NPC_EXIT_TRIGGER                            = 15800;

    const int SPELL_DIGESTIVE_ACID                        = 26476;

    // Areatriggers
    const int SPELL_SPIT_OUT                              = 25383;
    const int SPELL_EXIT_STOMACH                          = 26221;
    const int SPELL_RUBBLE_ROCKY                          = 26271;

    const int SPELL_CARAPACE_CTHUN                        = 26156;     // Server-side


    // temple_of_ahn_quiraj.h
    const int NPC_CLAW_TENTACLE       = 15725;
    const int NPC_EYE_TENTACLE        = 15726;
    const int NPC_GIANT_CLAW_TENTACLE = 15728;
    const int NPC_GIANT_EYE_TENTACLE  = 15334;
    const int NPC_FLESH_TENTACLE      = 15802;

    ObjectGuid botGuid = bot->GetGUID();

    if (Unit* boss = AI_VALUE2(Unit*, "find target", "c'thun"))
    {
        Position* point = NULL;

        if (bot->GetPositionZ() > 0.0)
        {
            Creature* tentacle1 = bot->FindNearestCreature(NPC_GIANT_EYE_TENTACLE, 100.0f, true);

            if (tentacle1)
            {
                Group* group = bot->GetGroup();
                if (group)
                {
                    Player* closest = NULL;
                    float closestDist = 0.0F;

                    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
                    {
                        Player* member = itr->GetSource();

                        if (member->GetGUID() != botGuid && member->IsAlive())
                        {
                            float dist = bot->GetDistance(member);

                            if (dist < 11.0 && (closest == NULL || closestDist < dist))
                            {
                                closest = member;
                                closestDist = dist;
                            }
                        }
                    }

                    if (closest)
                    {
                        return MoveAway(closest, 11.0);
                    }
                }
            }

            Unit* attackTarget = NULL;

            if (!boss->HasAura(SPELL_CARAPACE_CTHUN))
            {
                attackTarget = boss;
            }
            else if (tentacle1)
            {
                attackTarget = tentacle1;
            }
            else if (Creature* tentacle3 = bot->FindNearestCreature(NPC_EYE_TENTACLE, 100.0f, true))
            {
                attackTarget = tentacle3;
            }
            else if (Creature* tentacle2 = bot->FindNearestCreature(NPC_GIANT_CLAW_TENTACLE, 100.0f, true))
            {
                attackTarget = tentacle2;
            }
            else if (Creature* tentacle4 = bot->FindNearestCreature(NPC_CLAW_TENTACLE, 100.0f, true))
            {
                attackTarget = tentacle4;
            }

            if (attackTarget)
            {
                ObjectGuid guid = attackTarget->GetGUID();

                botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets")->Set({guid});
                bot->Attack(attackTarget, botAI->IsMelee(bot));

                return false;
            }

            outTriggered = false;
            attackPositioned = false;
        }
        else
        {
            if (bot->GetPositionZ() > -50.0)
            {
                // flying around out of bounds where they shouldn't be...
                // teleport them to splashdown at -8562.1 2037.0 -99.58
                bot->TeleportTo(bot->GetMapId(), -8562.1, 2037.0, -99.58, 5.03);
            }

            bool doGetOut = false;

            if (!attackPositioned)
            {
                point = insideattack;

                if (bot->GetDistance(*point) < 1.0)
                {
                    attackPositioned = true;
                }
            }
            else
            {
                int auraStack = bot->GetAuraCount(SPELL_DIGESTIVE_ACID);
                int auraDamage = (auraStack + (auraStack + 1) + (auraStack + 2)) * 150; // 150 per stack, 5 second tick

                if (auraDamage >= bot->GetHealth())
                {
                    point = getOut;
                    doGetOut = true;
                    //printf("%s: low health, get out\n",bot->GetName().c_str());
                }
                else if (auraStack < 1) // bugged state check
                {
                    // immediately leave
                    point = getOut;
                    doGetOut = true;
                    //printf("%s: bugged state, get out\n",bot->GetName().c_str());
                }
                else if (botAI->IsTank(bot, true))
                {
                    // immediately leave
                    point = getOut;
                    doGetOut = true;
                    //printf("%s: tank, get out\n",bot->GetName().c_str());
                }
                else if (botAI->IsHeal(bot, true))
                {
                    // stack to 5 and leave, lower tolerance for healers
                    if (bot->GetAuraCount(SPELL_DIGESTIVE_ACID) >= 5 || bot->GetHealthPct() <= 50.0)
                    {
                        point = getOut;
                        doGetOut = true;
                    }
                }
                else
                {
                    // search completely failed with AI_VALUE2 here, but the above AI_VALUE2 started
                    // working after the below statement was used here instead?
                    if (Creature* tboss = bot->FindNearestCreature(NPC_FLESH_TENTACLE, 100.0f, true))
                    {
                        bool hasTarget = false;

                        ObjectGuid targetguid = bot->GetTarget();
                        if (Unit* target = botAI->GetUnit(targetguid))
                        {
                            if (target->GetEntry() == NPC_FLESH_TENTACLE)
                            {
                                hasTarget = true;
                            }
                        }

                        if (!hasTarget)
                        {
                            ObjectGuid guid = tboss->GetGUID();

                            botAI->GetAiObjectContext()->GetValue<GuidVector>("prioritized targets")->Set({guid});
                            bot->SetTarget(guid);

                            //printf("%s: attack flesh tentacle\n",bot->GetName().c_str());

                            if (botAI->IsMelee(bot))
                            {
                                // bots not picking up on these tentacles very effectively
                                return MoveTo(tboss, -1.0);
                            }

                            return false;
                        }
                        else
                        {
                            //printf("%s: tentacle already target\n",bot->GetName().c_str());
                        }
                    }
                    else
                    {
                        //printf("%s: no flesh tentacle, get out\n",bot->GetName().c_str());
                        point = getOut;
                        doGetOut = true;
                    }
                }
            }

            if (doGetOut && !outTriggered)
            {
                if (bot->GetDistance(*point) < 10.0)
                {
                    // at_cthun_stomach_exit::OnTrigger from boss_cthun.cpp
                    Player* player = bot;
                    Unit* cthun = boss;

                    if (Creature* trigger = player->FindNearestCreature(NPC_TRIGGER, 15.0f))
                    {
                        if (!trigger->GetCurrentSpell(CurrentSpellTypes::CURRENT_GENERIC_SPELL))
                        {
                            trigger->CastSpell(player, SPELL_EXIT_STOMACH, true);

                            if (Creature* exittrigger = player->FindNearestCreature(NPC_EXIT_TRIGGER, 15.0f))
                            {
                                exittrigger->CastSpell(player, SPELL_RUBBLE_ROCKY, true);
                            }

                            outTriggered = true;
                        }
                    }

                    if (outTriggered)
                    {
                        player->m_Events.AddEventAtOffset([player, cthun]()
                        {
                            if (player->FindNearestCreature(NPC_EXIT_TRIGGER, 10.0f))
                            {
                                player->JumpTo(0.0f, 80.0f, false);

                                player->m_Events.AddEventAtOffset([player, cthun]()
                                {
                                    if (cthun)
                                        player->NearTeleportTo(cthun->GetPositionX(), cthun->GetPositionY(), cthun->GetPositionZ() + 10, float(rand32() % 6));

                                    player->RemoveAurasDueToSpell(SPELL_DIGESTIVE_ACID);
                                }, 1s);
                            }
                            else
                            {
                                player->m_Events.KillAllEvents(false);
                            }
                        }, 3s);
                    }
                }
            }
        }

        if (point)
        {
            return MoveTo(bot->GetMapId(), point->GetPositionX(), point->GetPositionY(), point->GetPositionZ(),
                false, false, false, true, MovementPriority::MOVEMENT_COMBAT);
        }
    }

    return false;
}
