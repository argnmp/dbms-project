SELECT DISTINCT t.name, AVG(cp.level)
FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
JOIN Pokemon AS p ON cp.pid = p.id AND p.type IN ('Normal', 'Electric')
GROUP BY t.id
ORDER BY AVG(cp.level), t.name;
