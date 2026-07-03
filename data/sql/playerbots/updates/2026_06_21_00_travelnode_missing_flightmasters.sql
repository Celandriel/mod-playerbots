DELETE FROM `playerbots_travelnode`
WHERE `name` IN ('Runetog Wildhammer flightMaster', 'Grennik flightMaster',
                 'Munci flightMaster', 'Faldorf Bitterchill flightMaster',
                 'Kabarg Windtamer flightMaster', 'Dreadwind flightMaster');

SET @n1 := (SELECT IFNULL(MAX(`id`), 0) + 1 FROM `playerbots_travelnode`);
SET @n2 := @n1 + 1;
SET @n3 := @n1 + 2;
SET @n4 := @n1 + 3;
SET @n5 := @n1 + 4;
SET @n6 := @n1 + 5;

INSERT INTO `playerbots_travelnode` (`id`, `name`, `map_id`, `x`, `y`, `z`, `linked`) VALUES
(@n1, 'Grennik flightMaster',             530, 4160.1600, 2957.3000,  352.2980, 0),
(@n2, 'Munci flightMaster',               530,  210.4920, 6065.0900,  148.3960, 0),
(@n3, 'Runetog Wildhammer flightMaster',  530,  276.2000, 1486.9000,  -15.1000, 0),
(@n4, 'Faldorf Bitterchill flightMaster', 571, 6665.5900, -261.2580,  961.8200, 0),
(@n5, 'Kabarg Windtamer flightMaster',    571, 7855.9800, -732.3880, 1177.5600, 0),
(@n6, 'Dreadwind flightMaster',           571, 7429.6000, 4231.6400,  314.3670, 0);
