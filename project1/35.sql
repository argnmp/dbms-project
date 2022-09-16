SELECT DISTINCT t.name, SUM(cp.level)
FROM Trainer AS t
JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
WHERE t.hometown = 'Blue City'
AND cp.pid IN (
(SELECT before_id AS id FROM Evolution)
UNION
(SELECT after_id AS id FROM Evolution)
)
GROUP BY t.id
ORDER BY t.name;
