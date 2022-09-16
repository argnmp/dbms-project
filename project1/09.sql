SELECT DISTINCT t.name
FROM Trainer AS t, CaughtPokemon AS cp
WHERE t.id = cp.owner_id AND cp.pid IN (
SELECT a.after_id
FROM Evolution AS a
WHERE a.after_id NOT IN (
SELECT before_id
FROM Evolution
))
ORDER BY t.name;
