SELECT DISTINCT name
FROM Pokemon
WHERE LOWER(LEFT(name,1)) IN ('a','e','i','o','u')
ORDER BY name;
