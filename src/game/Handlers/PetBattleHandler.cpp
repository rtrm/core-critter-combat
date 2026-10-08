#include "Common.h"
#include "WorldSession.h"
#include "Player.h"
#include "Pet.h"
#include "Creature.h"
#include "Maps/Map.h"
#include "PetBattleCombat.h"
#include "Server/Packets/PetBattle.h"

// Critter Combat (see the critter-combat repo's ARCHITECTURE.md). One full round is resolved
// synchronously within this one call - see PetBattleCombat.h for why that keeps the engine simple.
void WorldSession::HandlePetBattleUseAbilityOpcode(WorldPackets::PetBattle::UseAbility const& packet)
{
    Player* player = GetPlayer();
    PetBattleSession* battle = player->m_petBattle;
    if (!battle)
        return; // not in a battle (stale click, or already ended)

    bool validAbility = false;
    for (uint32 id : battle->playerAbilityIds)
        validAbility = validAbility || id == packet.abilityId;
    if (!validAbility)
        return;

    PetBattleCombat::RoundOutcome outcome = PetBattleCombat::ResolveRound(*battle, packet.abilityId);

    // The battle's HP shows on each combatant's own real health bar (StartPetBattle set this up),
    // so every round's new values need to land on the live units too, not just the session.
    Pet* pet = player->GetMap()->GetPet(battle->playerPetGuid);
    Creature* wild = player->GetMap()->GetCreature(battle->enemyGuid);
    if (pet)
        pet->SetHealth(battle->playerPetCurrentHp);
    if (wild)
        wild->SetHealth(battle->enemyCurrentHp);

    auto update = std::make_unique<WorldPackets::PetBattle::BattleUpdate>();
    update->playerAbilityId = outcome.playerAbilityUsed;
    update->enemyAbilityId = outcome.enemyAbilityUsed;
    update->playerActedFirst = outcome.playerActedFirst;
    update->playerPetCurrentHp = battle->playerPetCurrentHp;
    update->enemyCurrentHp = battle->enemyCurrentHp;
    player->GetSession()->SendPacket(std::move(update));

    if (outcome.battleOver)
        player->EndPetBattle(outcome.playerWon ? PetBattleEndReason::PlayerWon : PetBattleEndReason::PlayerLost);
}
