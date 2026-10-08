#ifndef MANGOS_PACKETS_PET_BATTLE_H
#define MANGOS_PACKETS_PET_BATTLE_H

#include "Packet.h"
#include <string>

// Critter Combat (see the critter-combat repo's ARCHITECTURE.md): this fork's own protocol, not
// vmangos/vanilla - the battle abilities are deliberately not real spells (no cast time/GCD/mana
// /LOS, which is most of what spell_template's columns exist for), so there is no vanilla opcode
// to piggyback the turn exchange on.
namespace WorldPackets { namespace PetBattle
{
    // One ability as the client needs to know it to draw the battle action bar: see
    // ObjectMgr::PetBattleAbility for what each field means server-side.
    struct AbilityInfo
    {
        uint32 id = 0;
        std::string name;
        /// Bare `Interface\Icons\` basename; the client prepends the folder.
        std::string icon;
        uint8 effectType = 0; // 1 DAMAGE, 2 HIT_CHANCE_DEBUFF (enemy), 3 DAMAGE_TAKEN_SHIELD (self)
    };

    // --- Client Packets ---

    class UseAbility final : public ClientPacket
    {
    public:
        uint32 abilityId = 0;

        explicit UseAbility() : ClientPacket(CMSG_PET_BATTLE_USE_ABILITY) {}
        void ReadFromWorldPacket(WorldPacket& recv_data) override;
    };

    // --- Server Packets ---

    // Sent once, when Engage Critter Combat starts a battle: both sides' full combatant info, so
    // the client can raise the battle action bar and draw both health bars without a round trip.
    class BattleStart final : public ServerPacket
    {
    public:
        std::string playerPetName;
        uint32 playerPetLevel = 0;
        uint32 playerPetMaxHp = 0;
        uint32 playerPetCurrentHp = 0;
        AbilityInfo playerAbilities[3];

        std::string enemyName;
        uint32 enemyLevel = 0;
        uint32 enemyMaxHp = 0;
        uint32 enemyCurrentHp = 0;
        AbilityInfo enemyAbilities[3];

        bool playerGoesFirst = true;

        explicit BattleStart() : ServerPacket(SMSG_PET_BATTLE_START) {}
        size_t EstimateFinalSize() const override;
        void AppendBodyTo(ByteBuffer& buffer) const override;
    };

    // Sent once per round, after both sides' abilities (or just the survivor's, if the other side's
    // pet died to the first actor) have resolved.
    class BattleUpdate final : public ServerPacket
    {
    public:
        uint32 playerAbilityId = 0; // 0 if the player's pet didn't act this round (already dead)
        uint32 enemyAbilityId = 0;  // 0 if the enemy didn't act this round (already dead)
        bool playerActedFirst = true;
        uint32 playerPetCurrentHp = 0;
        uint32 enemyCurrentHp = 0;

        explicit BattleUpdate() : ServerPacket(SMSG_PET_BATTLE_UPDATE) {}
        size_t EstimateFinalSize() const override;
        void AppendBodyTo(ByteBuffer& buffer) const override;
    };

    // Sent once, when either side's HP reaches 0; the client closes the battle action bar on
    // receipt.
    class BattleEnd final : public ServerPacket
    {
    public:
        bool playerWon = false;

        explicit BattleEnd() : ServerPacket(SMSG_PET_BATTLE_END) {}
        size_t EstimateFinalSize() const override;
        void AppendBodyTo(ByteBuffer& buffer) const override;
    };
}} // namespace WorldPackets::PetBattle

#endif // MANGOS_PACKETS_PET_BATTLE_H
