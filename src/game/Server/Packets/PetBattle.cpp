#include "PetBattle.h"

void WorldPackets::PetBattle::UseAbility::ReadFromWorldPacket(WorldPacket& recv_data)
{
    recv_data >> abilityId;
}

// --- Server Packets ---

static size_t EstimateAbilitySize(WorldPackets::PetBattle::AbilityInfo const& ability)
{
    return sizeof(ability.id) + sizeof(ability.effectType) +
           ability.name.size() + sizeof(char) + // null terminator
           ability.icon.size() + sizeof(char); // null terminator
}

static void AppendAbility(ByteBuffer& buffer, WorldPackets::PetBattle::AbilityInfo const& ability)
{
    buffer << ability.id;
    buffer << ability.name;
    buffer << ability.icon;
    buffer << ability.effectType;
}

size_t WorldPackets::PetBattle::BattleStart::EstimateFinalSize() const
{
    size_t size = playerPetName.size() + sizeof(char) + /*null terminator*/
                  sizeof(playerPetLevel) +
                  sizeof(playerPetMaxHp) +
                  sizeof(playerPetCurrentHp) +
                  enemyName.size() + sizeof(char) + /*null terminator*/
                  sizeof(enemyLevel) +
                  sizeof(enemyMaxHp) +
                  sizeof(enemyCurrentHp) +
                  sizeof(playerGoesFirst);
    for (AbilityInfo const& ability : playerAbilities)
        size += EstimateAbilitySize(ability);
    for (AbilityInfo const& ability : enemyAbilities)
        size += EstimateAbilitySize(ability);
    return size;
}

void WorldPackets::PetBattle::BattleStart::AppendBodyTo(ByteBuffer& buffer) const
{
    buffer << playerPetName;
    buffer << playerPetLevel;
    buffer << playerPetMaxHp;
    buffer << playerPetCurrentHp;
    for (AbilityInfo const& ability : playerAbilities)
        AppendAbility(buffer, ability);

    buffer << enemyName;
    buffer << enemyLevel;
    buffer << enemyMaxHp;
    buffer << enemyCurrentHp;
    for (AbilityInfo const& ability : enemyAbilities)
        AppendAbility(buffer, ability);

    buffer << playerGoesFirst;
}

size_t WorldPackets::PetBattle::BattleUpdate::EstimateFinalSize() const
{
    return sizeof(playerAbilityId) +
           sizeof(enemyAbilityId) +
           sizeof(playerActedFirst) +
           sizeof(playerPetCurrentHp) +
           sizeof(enemyCurrentHp);
}

void WorldPackets::PetBattle::BattleUpdate::AppendBodyTo(ByteBuffer& buffer) const
{
    buffer << playerAbilityId;
    buffer << enemyAbilityId;
    buffer << playerActedFirst;
    buffer << playerPetCurrentHp;
    buffer << enemyCurrentHp;
}

size_t WorldPackets::PetBattle::BattleEnd::EstimateFinalSize() const
{
    return sizeof(playerWon);
}

void WorldPackets::PetBattle::BattleEnd::AppendBodyTo(ByteBuffer& buffer) const
{
    buffer << playerWon;
}
