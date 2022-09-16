SELECT SUM(cp.level)
FROM CaughtPokemon AS cp
WHERE cp.pid NOT IN (
(SELECT before_id AS id FROM Evolution)
UNION
(SELECT after_id AS id FROM Evolution)
);
