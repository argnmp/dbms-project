SELECT DISTINCT t.name FROM Trainer AS t JOIN CaughtPokemon AS cp ON t.id = cp.owner_id GROUP BY t.id, cp.pid HAVING COUNT(*) >= 2 ORDER BY t.name;
