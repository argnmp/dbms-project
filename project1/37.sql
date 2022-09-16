SELECT DISTINCT cp.id, cp.nickname
FROM CaughtPokemon AS cp
JOIN Trainer AS t ON cp.owner_id = t.id AND t.hometown = 'Blue City'
JOIN Pokemon AS p ON cp.pid = p.id AND p.type = 'Water'
ORDER BY cp.id, cp.nickname;
