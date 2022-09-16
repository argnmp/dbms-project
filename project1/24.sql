SELECT DISTINCT p.name FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
JOIN Pokemon AS p ON cp.pid = p.id
WHERE t.hometown IN ('Sangnok City', 'Brown City')
ORDER BY p.name;
