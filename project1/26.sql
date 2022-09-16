SELECT DISTINCT t.name, p.name, COUNT(*)
FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
JOIN Pokemon AS p ON cp.pid = p.id
WHERE t.id IN (
SELECT t.id FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
JOIN Pokemon AS p ON cp.pid = p.id
GROUP BY t.id HAVING COUNT(DISTINCT p.type) = 1
)
GROUP BY t.id, p.id
ORDER BY t.name, p.name;
