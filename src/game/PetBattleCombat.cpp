#include "PetBattleCombat.h"

#include "ObjectMgr.h"
#include "Utilities/Random.h"
#include "Objects/Creature.h"
#include "Objects/Pet.h"
#include "Objects/Player.h"
#include "Server/Packets/PetBattle.h"
#include "Server/WorldSession.h"

uint32 PetBattleCombat::MaxHp(uint32 level)
{
    return 20 + level * 5;
}

uint32 PetBattleCombat::CaptureChance(uint32 playerPetLevel, uint32 enemyLevel, uint32 enemyHpPercent)
{
    int32 chance;
    if (enemyHpPercent <= 20)
        chance = 70;
    else if (enemyHpPercent <= 50)
        chance = 15;
    else
        return 0; // too healthy to attempt at all

    int32 diff = static_cast<int32>(playerPetLevel) - static_cast<int32>(enemyLevel);
    chance += diff < 0 ? diff * 20 : diff * 5;

    chance = std::max(0, std::min(95, chance));
    return static_cast<uint32>(chance);
}

namespace
{
    // The side that acts first each round (ARCHITECTURE.md: "the higher-level pet acts first each
    // round; a tie on level is broken by current HP"). Recomputed every round, since current HP -
    // the tiebreak - changes as the fight goes on.
    bool PlayerActsFirst(PetBattleSession const& s)
    {
        if (s.playerPetLevel != s.enemyLevel)
            return s.playerPetLevel > s.enemyLevel;
        return s.playerPetCurrentHp >= s.enemyCurrentHp;
    }

    void ApplyAbility(PetBattleSession& s, uint32 abilityId, bool casterIsPlayer)
    {
        ObjectMgr::PetBattleAbility const* ability = sObjectMgr.GetPetBattleAbility(abilityId);
        if (!ability)
            return;

        uint32 casterLevel = casterIsPlayer ? s.playerPetLevel : s.enemyLevel;
        uint32 value = static_cast<uint32>(ability->baseValue + ability->valuePerLevel * casterLevel + 0.5f);

        switch (ability->effectType)
        {
            case 1: // DAMAGE
            {
                // A pending miss chance lives on the ATTACKER (set by an earlier HIT_CHANCE_DEBUFF
                // the opponent landed on them) - it guards this one attack attempt, hit or miss.
                uint32& casterMissChance = casterIsPlayer ? s.playerPendingMissChance : s.enemyPendingMissChance;
                if (casterMissChance > 0)
                {
                    bool missed = urand(1, 100) <= casterMissChance;
                    casterMissChance = 0;
                    if (missed)
                        return;
                }

                uint32& targetHp = casterIsPlayer ? s.enemyCurrentHp : s.playerPetCurrentHp;
                uint32& targetShield = casterIsPlayer ? s.enemyPendingShield : s.playerPendingShield;
                uint32 damage = value;
                if (targetShield > 0)
                {
                    damage = damage > targetShield ? damage - targetShield : 0;
                    targetShield = 0;
                }
                targetHp = damage >= targetHp ? 0 : targetHp - damage;
                return;
            }
            case 2: // HIT_CHANCE_DEBUFF: reduces the OPPONENT's chance to land their own next attack.
            {
                uint32& opponentMissChance = casterIsPlayer ? s.enemyPendingMissChance : s.playerPendingMissChance;
                opponentMissChance = std::min<uint32>(100, value);
                return;
            }
            case 3: // DAMAGE_TAKEN_SHIELD: absorbs the CASTER's own next incoming hit.
            {
                uint32& casterShield = casterIsPlayer ? s.playerPendingShield : s.enemyPendingShield;
                casterShield = value;
                return;
            }
            default:
                return;
        }
    }
}

PetBattleCombat::RoundOutcome PetBattleCombat::ResolveRound(PetBattleSession& s, uint32 playerAbilityId)
{
    RoundOutcome outcome;
    outcome.playerActedFirst = PlayerActsFirst(s);
    uint32 enemyAbilityId = s.enemyAbilityIds[urand(0, 2)];

    auto playerActs = [&]
    {
        outcome.playerAbilityUsed = playerAbilityId;
        ApplyAbility(s, playerAbilityId, true);
    };
    auto enemyActs = [&]
    {
        outcome.enemyAbilityUsed = enemyAbilityId;
        ApplyAbility(s, enemyAbilityId, false);
    };

    if (outcome.playerActedFirst)
    {
        playerActs();
        if (s.enemyCurrentHp == 0)
        {
            outcome.battleOver = true;
            outcome.playerWon = true;
            return outcome;
        }
        enemyActs();
        if (s.playerPetCurrentHp == 0)
        {
            outcome.battleOver = true;
            outcome.playerWon = false;
        }
    }
    else
    {
        enemyActs();
        if (s.playerPetCurrentHp == 0)
        {
            outcome.battleOver = true;
            outcome.playerWon = false;
            return outcome;
        }
        playerActs();
        if (s.enemyCurrentHp == 0)
        {
            outcome.battleOver = true;
            outcome.playerWon = true;
        }
    }
    return outcome;
}

PetBattleCombat::CaptureOutcome PetBattleCombat::ResolveCaptureRound(PetBattleSession& s, uint32 captureChancePercent)
{
    CaptureOutcome outcome;
    outcome.playerActedFirst = PlayerActsFirst(s);

    auto enemyActs = [&]
    {
        uint32 enemyAbilityId = s.enemyAbilityIds[urand(0, 2)];
        outcome.enemyAbilityUsed = enemyAbilityId;
        ApplyAbility(s, enemyAbilityId, false);
    };
    auto attemptCapture = [&]
    {
        return captureChancePercent > 0 && urand(1, 100) <= captureChancePercent;
    };

    if (outcome.playerActedFirst)
    {
        if (attemptCapture())
        {
            outcome.captured = true;
            return outcome;
        }
        enemyActs();
        if (s.playerPetCurrentHp == 0)
        {
            outcome.battleOver = true;
            outcome.playerWon = false;
        }
    }
    else
    {
        enemyActs();
        if (s.playerPetCurrentHp == 0)
        {
            outcome.battleOver = true;
            outcome.playerWon = false;
            return outcome;
        }
        if (attemptCapture())
            outcome.captured = true;
    }
    return outcome;
}

static void FillAbility(WorldPackets::PetBattle::AbilityInfo& out, uint32 abilityId)
{
    ObjectMgr::PetBattleAbility const* ability = sObjectMgr.GetPetBattleAbility(abilityId);
    out.id = ability ? ability->id : 0;
    out.name = ability ? ability->name : "";
    out.icon = ability ? ability->icon : "";
    out.effectType = ability ? ability->effectType : 0;
}

void Player::StartPetBattle(Creature* wild)
{
    if (m_petBattle)
        return; // already mid-battle

    if (!wild || !wild->IsAlive() || !sObjectMgr.IsPetBattleWild(wild->GetEntry()))
        return;

    Pet* pet = GetMiniPet();
    if (!pet || !pet->IsAlive())
    {
        GetSession()->SendNotification("You need a living companion pet out to engage in Critter Combat.");
        return;
    }

    std::vector<uint32> const* playerAbilityIds = sObjectMgr.GetPetBattleAbilities(pet->GetEntry());
    std::vector<uint32> const* enemyAbilityIds = sObjectMgr.GetPetBattleAbilities(wild->GetEntry());
    if (!playerAbilityIds || playerAbilityIds->size() != 3 || !enemyAbilityIds || enemyAbilityIds->size() != 3)
    {
        // This species (on either side) has no authored abilities yet - ability content is its
        // own later pass per ARCHITECTURE.md, so this is an expected gap outside the pilot.
        GetSession()->SendNotification("Neither of you knows how to fight yet.");
        return;
    }

    // "No camera change... the two pets simply position themselves facing each other" (ARCHITECTURE.md).
    // Clear(true, true)+MoveIdle() on both: the wild critter's own flee-on-hostile-spell reaction
    // is handled separately (CritterAI::SpellHit special-cases spell 64000), but its default
    // ambient wander still needs stopping, same as the player's pet otherwise following its owner.
    // Clear()'s own default args (`all=false`) leave the *bottom* movement generator on the stack
    // untouched - only one layer below MoveIdle's push, so the creature's base wander could still
    // resurface (e.g. if anything else calls a plain Clear() on it mid-battle and pops idle back
    // off); `all=true` wipes the whole stack so there is nothing left for idle to sit on top of.
    pet->GetMotionMaster()->Clear(true, true);
    pet->GetMotionMaster()->MoveIdle();
    wild->GetMotionMaster()->Clear(true, true);
    wild->GetMotionMaster()->MoveIdle();
    pet->SetFacingToObject(wild);
    wild->SetFacingToObject(pet);

    // Both units' real Health is about to show the battle's HP (below) - their own passive regen
    // would otherwise silently heal them back up between rounds, undermining the fight.
    pet->ClearCreatureState(CSTATE_REGEN_HEALTH);
    wild->ClearCreatureState(CSTATE_REGEN_HEALTH);

    m_petBattle = new PetBattleSession();
    m_petBattle->playerPetGuid = pet->GetObjectGuid();
    m_petBattle->playerPetLevel = pet->GetLevel();
    m_petBattle->playerPetMaxHp = PetBattleCombat::MaxHp(m_petBattle->playerPetLevel);
    // Persistent pet HP (ARCHITECTURE.md step 6) isn't wired up yet - every battle starts at full
    // HP for this pilot increment.
    m_petBattle->playerPetCurrentHp = m_petBattle->playerPetMaxHp;
    for (uint8 i = 0; i < 3; ++i)
        m_petBattle->playerAbilityIds[i] = (*playerAbilityIds)[i];

    m_petBattle->enemyGuid = wild->GetObjectGuid();
    m_petBattle->enemyLevel = wild->GetLevel();
    m_petBattle->enemyMaxHp = PetBattleCombat::MaxHp(m_petBattle->enemyLevel);
    m_petBattle->enemyCurrentHp = m_petBattle->enemyMaxHp;
    for (uint8 i = 0; i < 3; ++i)
        m_petBattle->enemyAbilityIds[i] = (*enemyAbilityIds)[i];

    // The battle's HP lives on each combatant's own real health bar, not a custom UI - save their
    // actual Health/MaxHealth to restore exactly at battle end, then overwrite with battle values.
    m_petBattle->playerPetOriginalMaxHp = pet->GetMaxHealth();
    m_petBattle->playerPetOriginalHp = pet->GetHealth();
    m_petBattle->enemyOriginalMaxHp = wild->GetMaxHealth();
    m_petBattle->enemyOriginalHp = wild->GetHealth();
    pet->SetMaxHealth(m_petBattle->playerPetMaxHp);
    pet->SetHealth(m_petBattle->playerPetCurrentHp);
    wild->SetMaxHealth(m_petBattle->enemyMaxHp);
    wild->SetHealth(m_petBattle->enemyCurrentHp);

    auto packet = std::make_unique<WorldPackets::PetBattle::BattleStart>();
    packet->playerPetName = pet->GetName();
    packet->playerPetLevel = m_petBattle->playerPetLevel;
    packet->playerPetMaxHp = m_petBattle->playerPetMaxHp;
    packet->playerPetCurrentHp = m_petBattle->playerPetCurrentHp;
    for (uint8 i = 0; i < 3; ++i)
        FillAbility(packet->playerAbilities[i], m_petBattle->playerAbilityIds[i]);

    packet->enemyName = wild->GetName();
    packet->enemyLevel = m_petBattle->enemyLevel;
    packet->enemyMaxHp = m_petBattle->enemyMaxHp;
    packet->enemyCurrentHp = m_petBattle->enemyCurrentHp;
    for (uint8 i = 0; i < 3; ++i)
        FillAbility(packet->enemyAbilities[i], m_petBattle->enemyAbilityIds[i]);

    packet->playerGoesFirst = m_petBattle->playerPetLevel != m_petBattle->enemyLevel
        ? m_petBattle->playerPetLevel > m_petBattle->enemyLevel
        : m_petBattle->playerPetCurrentHp >= m_petBattle->enemyCurrentHp;

    GetSession()->SendPacket(std::move(packet));
}

// Capture (ARCHITECTURE.md step 5): usable mid-battle via its own targeted spell cast
// (SpellEffects.cpp case 64001), not one of the three battle-ability slots - but still costs a
// full round via ResolveCaptureRound, same as choosing a weak ability would, so it can't be
// spammed for free (live feedback: a miss needs a meaningful penalty). Above 50% enemy HP there's
// no real attempt to roll at all, so that rejection alone doesn't cost a round either.
void Player::TryCapturePet(Creature* wild)
{
    if (!m_petBattle || !wild || wild->GetObjectGuid() != m_petBattle->enemyGuid)
        return; // only the pet you're actually fighting can be captured

    uint32 captureSpellId = sObjectMgr.GetPetBattleCaptureSpell(wild->GetEntry());
    if (!captureSpellId)
    {
        GetSession()->SendNotification("This critter can't be captured yet.");
        return;
    }

    PetBattleSession& battle = *m_petBattle;
    uint32 enemyHpPercent = battle.enemyMaxHp ? battle.enemyCurrentHp * 100 / battle.enemyMaxHp : 0;
    if (enemyHpPercent > 50)
    {
        GetSession()->SendNotification("%s is still too healthy to capture - wear it down first.", wild->GetName());
        return;
    }

    uint32 chance = PetBattleCombat::CaptureChance(battle.playerPetLevel, battle.enemyLevel, enemyHpPercent);
    PetBattleCombat::CaptureOutcome outcome = PetBattleCombat::ResolveCaptureRound(battle, chance);

    // Only the player's pet can take a hit from this round (the enemy's own HP never changes from
    // a catch attempt), but sync it to its real health bar same as every other round does.
    if (Pet* pet = GetMiniPet())
        pet->SetHealth(battle.playerPetCurrentHp);

    auto update = std::make_unique<WorldPackets::PetBattle::BattleUpdate>();
    update->playerAbilityId = 0; // no battle ability was used - this round's "move" was the catch attempt
    update->enemyAbilityId = outcome.enemyAbilityUsed;
    update->playerActedFirst = outcome.playerActedFirst;
    update->playerPetCurrentHp = battle.playerPetCurrentHp;
    update->enemyCurrentHp = battle.enemyCurrentHp;
    GetSession()->SendPacket(std::move(update));

    if (outcome.captured)
    {
        if (!HasSpell(captureSpellId))
            LearnSpell(captureSpellId, false);
        GetSession()->SendNotification("You captured %s!", wild->GetName());
        EndPetBattle(PetBattleEndReason::Captured);
        return;
    }

    if (outcome.battleOver)
    {
        GetSession()->SendNotification("The critter broke free, and your pet couldn't take another hit!");
        EndPetBattle(PetBattleEndReason::PlayerLost);
        return;
    }

    GetSession()->SendNotification("The critter broke free!");
}

// Shared teardown for every way a battle can end (round-resolved win/loss, or a successful
// Capture) - factored out of what was originally only WorldSession::HandlePetBattleUseAbilityOpcode
// so Capture's own end-of-battle path (above) doesn't duplicate it.
void Player::EndPetBattle(PetBattleEndReason reason)
{
    PetBattleSession* battle = m_petBattle;
    if (!battle)
        return;

    auto end = std::make_unique<WorldPackets::PetBattle::BattleEnd>();
    end->playerWon = reason != PetBattleEndReason::PlayerLost;
    GetSession()->SendPacket(std::move(end));

    // Hand movement and regen back to the player's pet (always alive) - StartPetBattle froze both
    // to stop it following its owner and borrowed its real Health/MaxHealth to show the battle's
    // HP on its own health bar, both restored here regardless of how the battle ended.
    Pet* pet = GetMap()->GetPet(battle->playerPetGuid);
    if (pet)
    {
        pet->GetMotionMaster()->Initialize();
        pet->AddCreatureState(CSTATE_REGEN_HEALTH);
        pet->SetMaxHealth(battle->playerPetOriginalMaxHp);
        pet->SetHealth(battle->playerPetOriginalHp);
    }

    Creature* wild = GetMap()->GetCreature(battle->enemyGuid);
    if (wild)
    {
        switch (reason)
        {
            case PetBattleEndReason::PlayerWon:
                // A real death, not a restore: Kill() handles the death animation, despawn/respawn
                // timer and whatever else a normal kill does, none of which "set HP back to what it
                // was" would - that path is only correct for the wild critter winning, below.
                wild->Kill(wild, nullptr);
                break;
            case PetBattleEndReason::Captured:
                // No death - the critter simply vanishes into its new owner's pocket. ForcedDespawn
                // briefly flips through the engine's normal death state to remove it cleanly (same
                // call plenty of other "just disappear" scripted despawns already use), then lets
                // it respawn on its usual timer, same as any other wild critter that wanders off.
                wild->ForcedDespawn();
                break;
            case PetBattleEndReason::PlayerLost:
                wild->GetMotionMaster()->Initialize();
                wild->AddCreatureState(CSTATE_REGEN_HEALTH);
                wild->SetMaxHealth(battle->enemyOriginalMaxHp);
                wild->SetHealth(battle->enemyOriginalHp);
                break;
        }
    }

    delete m_petBattle;
    m_petBattle = nullptr;
}
