SELECT DISTINCT t.hometown, cp.nickname FROM Trainer AS t, CaughtPokemon AS cp,
(SELECT owner_id, MAX(level) as m FROM CaughtPokemon GROUP BY owner_id) AS p
WHERE t.id = cp.owner_id AND t.id = p.owner_id AND cp.level = p.m
ORDER BY t.hometown;
