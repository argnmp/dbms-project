SELECT DISTINCT t.name, COUNT(*)
FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
JOIN Pokemon AS p ON cp.pid = p.id
GROUP BY t.id
HAVING COUNT(DISTINCT p.type) = 2
ORDER BY t.name;
