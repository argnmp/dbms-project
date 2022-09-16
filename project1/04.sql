SELECT DISTINCT name FROM Evolution JOIN Pokemon ON id = before_id
WHERE before_id > after_id
ORDER BY name;
