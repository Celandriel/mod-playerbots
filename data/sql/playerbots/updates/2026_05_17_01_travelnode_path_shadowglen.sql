-- Manual travelnode coverage for the Aldrassil ramp in Shadowglen
-- (Teldrassil, map 1, zone 141). Adds 9 anchor nodes along the spiral
-- ramp (base -> intermediate ramp waypoints -> top platform near
-- Tenaron Stormgrip). All nodes are `linked = 0` so
-- `.playerbots travel generatenode` will iterate them and let mmap
-- compute the actual walk paths between consecutive nodes. Splitting
-- the climb into short segments (~30y each) gives mmap a much better
-- chance of resolving each piece than a single 300y end-to-end probe.
--
-- TEMPORARILY DISABLED: isolating generatenode behaviour on the OG
-- (cmangos-imported) graph. Re-enable by removing the comment prefix
-- once the regen flow is verified stable.

-- SET @n1 := (SELECT IFNULL(MAX(id), 0) + 1 FROM playerbots_travelnode);
-- SET @n2 := @n1 + 1;
-- SET @n3 := @n1 + 2;
-- SET @n4 := @n1 + 3;
-- SET @n5 := @n1 + 4;
-- SET @n6 := @n1 + 5;
-- SET @n7 := @n1 + 6;
-- SET @n8 := @n1 + 7;
-- SET @n9 := @n1 + 8;

-- INSERT INTO playerbots_travelnode (id, name, map_id, x, y, z, linked) VALUES
-- (@n1, 'Aldrassil Ramp 1 (base)', 1, 10413.756, 887.97363, 1319.3668, 0),
-- (@n2, 'Aldrassil Ramp 2',        1, 10440.520, 870.32320, 1328.9324, 0),
-- (@n3, 'Aldrassil Ramp 3',        1, 10497.001, 854.46014, 1345.1770, 0),
-- (@n4, 'Aldrassil Ramp 4',        1, 10517.199, 821.48640, 1354.7914, 0),
-- (@n5, 'Aldrassil Ramp 5',        1, 10477.926, 847.88855, 1372.1685, 0),
-- (@n6, 'Aldrassil Ramp 6',        1, 10455.358, 831.34240, 1380.9377, 0),
-- (@n7, 'Aldrassil Ramp 7',        1, 10460.220, 800.71716, 1388.3368, 0),
-- (@n8, 'Aldrassil Ramp 8',        1, 10507.434, 793.30420, 1397.2166, 0),
-- (@n9, 'Aldrassil Ramp 9 (top)',  1, 10495.496, 804.67700, 1397.2662, 0);

SELECT 1;  -- no-op so the file is still valid SQL for the updater
