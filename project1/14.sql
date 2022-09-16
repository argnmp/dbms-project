SELECT DISTINCT p.name FROM Pokemon AS p
WHERE p.id NOT IN (
SELECT pid FROM CaughtPokemon
)
ORDER BY p.name;
