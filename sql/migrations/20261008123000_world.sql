DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261008123000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261008123000');
-- Add your query below.

-- Critter Combat (ARCHITECTURE.md Milestone 1 step 4): ability icons for the battle action bar,
-- live-requested after seeing the plain-text buttons in game. Bare `Interface\Icons\` basenames,
-- same convention as every other icon column in this core (spell_template has none of its own,
-- but item/talent/etc. icon columns all store it this way) - the client prepends the folder.
-- Verified present in the real 1.12.1 client data before picking them (`interface.MPQ`).

ALTER TABLE `pet_battle_ability` ADD COLUMN `icon` VARCHAR(100) NOT NULL DEFAULT '' AFTER `name`;

UPDATE `pet_battle_ability` SET `icon` = 'INV_Misc_MonsterClaw_04' WHERE `id` = 63000; -- Nibble
UPDATE `pet_battle_ability` SET `icon` = 'Spell_Nature_Cyclone' WHERE `id` = 63001; -- Dust Cloud
UPDATE `pet_battle_ability` SET `icon` = 'Spell_Nature_StoneClawTotem' WHERE `id` = 63002; -- Burrow

-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
