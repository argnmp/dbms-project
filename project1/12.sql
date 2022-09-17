SELECT DISTINCT t.hometown, cp.nickname FROM Trainer AS t JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
JOIN (
SELECT t.hometown, MAX(cp.level) AS level
FROM Trainer AS t JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
GROUP BY t.hometown
) AS m ON t.hometown = m.hometown AND cp.level = m.level
ORDER BY t.hometown;
