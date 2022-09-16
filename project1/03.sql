SELECT DISTINCT t.name, cp.nickname FROM Trainer AS t JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
WHERE cp.level IN (
SELECT MAX(b.level)
FROM Trainer AS a JOIN CaughtPokemon AS b ON a.id = b.owner_id
WHERE a.id = t.id
GROUP BY a.id HAVING COUNT(*) >= 3
)
ORDER BY t.name, cp.nickname;
