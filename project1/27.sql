SELECT DISTINCT t.name, COALESCE(SUM(cp.level), 0) AS level
FROM Gym AS g
LEFT JOIN Trainer AS t ON g.leader_id = t.id
LEFT JOIN CaughtPokemon AS cp ON t.id = cp.owner_id AND cp.level >= 50
GROUP BY t.id
ORDER BY t.name;
