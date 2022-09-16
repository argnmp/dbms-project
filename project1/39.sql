SELECT DISTINCT t.name
FROM Trainer AS t
JOIN CaughtPokemon AS cp1 ON t.id = cp1.owner_id
JOIN CaughtPokemon AS cp2 ON t.id = cp2.owner_id AND cp1.pid < cp2.pid
LEFT JOIN (
SELECT e1.before_id AS p1, e2.after_id AS p2
FROM Evolution AS e1
JOIN Evolution AS e2 ON (e1.before_id = e2.before_id AND e1.after_id = e2.after_id)
OR (e1.after_id = e2.before_id)
) AS pair ON cp1.pid = pair.p1 AND cp2.pid = pair.p2
WHERE (pair.p1 IS NOT NULL) AND (pair.p2 IS NOT NULL)
ORDER BY t.name;
