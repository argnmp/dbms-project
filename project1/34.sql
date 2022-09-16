SELECT DISTINCT t.name, c.description
FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
JOIN Pokemon AS p ON cp.pid = p.id AND p.type = 'Fire'
JOIN City AS c ON t.hometown = c.name
ORDER BY t.name;
