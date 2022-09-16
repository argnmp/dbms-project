SELECT SUM(level)
FROM CaughtPokemon AS cp
JOIN Pokemon AS p ON cp.pid = p.id
WHERE p.type NOT IN ('Fire', 'Grass', 'Water', 'Electric');
