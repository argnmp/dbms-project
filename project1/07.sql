SELECT DISTINCT a.name, a.s AS total FROM (SELECT t.id, t.name, SUM(cp.level) AS s FROM Trainer AS t JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
GROUP BY t.id) AS a LEFT JOIN (SELECT t.id, t.name, SUM(cp.level) AS s FROM Trainer AS t JOIN CaughtPokemon AS cp ON t.id = cp.owner_id
GROUP BY t.id) AS b ON a.s < b.s
WHERE b.id IS NULL
ORDER BY a.name;
