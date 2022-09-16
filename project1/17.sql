SELECT AVG(level)
FROM CaughtPokemon AS cp JOIN Pokemon AS p ON cp.pid = p.id
WHERE p.type = 'Water';
