SELECT DISTINCT t.name, SUM(cp.level)
FROM Trainer AS t, CaughtPokemon AS cp, Gym AS g
WHERE t.id = g.leader_id AND t.id = cp.owner_id
GROUP BY t.id
ORDER BY SUM(cp.level) desc, t.name;
