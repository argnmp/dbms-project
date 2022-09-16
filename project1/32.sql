SELECT DISTINCT t.name, t.hometown
FROM Trainer AS t
JOIN Gym AS g ON t.id = g.leader_id
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
WHERE cp.pid NOT IN (
SELECT before_id FROM Evolution
)
ORDER BY t.name;
