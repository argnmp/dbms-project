SELECT DISTINCT t.name
FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
WHERE cp.level IN ((SELECT MAX(level) AS level FROM CaughtPokemon) UNION (SELECT MIN(level) AS level FROM CaughtPokemon))
ORDER BY t.name;
