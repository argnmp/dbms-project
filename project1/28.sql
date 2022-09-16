SELECT DISTINCT p.name, cp.level
FROM Gym AS g
JOIN Trainer AS t ON g.leader_id = t.id AND g.City = 'Sangnok City'
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
JOIN Pokemon AS p ON cp.pid = p.id
ORDER BY cp.level, p.name;
