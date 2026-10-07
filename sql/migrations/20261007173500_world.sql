DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261007173500');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261007173500');
-- Add your query below.

-- Critter Combat Milestone 1 fix: Erma (entry 6749) only had UNIT_NPC_FLAG_STABLEMASTER (8192)
-- set, no UNIT_NPC_FLAG_GOSSIP (1). A pure-stablemaster-flagged NPC is vanilla's native Hunter
-- pet-stable interaction, which the client opens directly without ever sending CMSG_GOSSIP_HELLO
-- for non-Hunters - so our new "Learn Critter Combat" gossip option never had a chance to show.
-- Adding the GOSSIP bit makes the client always ask the server for a menu, regardless of class.
-- Relevant for the later bulk rollout to the other 90 stable masters too (ARCHITECTURE.md).

UPDATE `creature_template` SET `npc_flags` = `npc_flags` | 1 WHERE `entry` = 6749;

-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
