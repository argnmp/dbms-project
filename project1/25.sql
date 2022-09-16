SELECT DISTINCT t.name, p.type, COUNT(*)
FROM Trainer AS t
JOIN CaughtPokemon AS cp on t.id = cp.owner_id
JOIN Pokemon AS p ON cp.pid = p.id
GROUP BY t.id, p.type
ORDER BY t.name, p.type;
