SELECT DISTINCT t.name, COUNT(*) FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
GROUP BY t.id HAVING COUNT(*) >= 3
ORDER BY COUNT(*);
