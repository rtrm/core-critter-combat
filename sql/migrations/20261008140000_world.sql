DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261008140000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261008140000');
-- Add your query below.

-- Critter Combat (ARCHITECTURE.md Milestone 1 step 4): rename the Capture spell's server-side
-- `spell_template` name to "Capture Critter", matching the client-side display rename
-- (`ui_action/synthetic_pet_battle.rs`) requested live after seeing it in the spellbook.

UPDATE `spell_template` SET `name` = 'Capture Critter' WHERE `entry` = 64001;

-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
