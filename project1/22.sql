SELECT DISTINCT t.name
FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
JOIN Pokemon AS p ON cp.pid = p.id AND p.type = 'Water'
GROUP BY t.id
HAVING COUNT(*) >= 2
ORDER BY t.name;
