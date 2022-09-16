SELECT DISTINCT name FROM Evolution AS e JOIN Pokemon AS p ON e.after_id = p.id
WHERE before_id NOT IN ( SELECT after_id FROM Evolution ) ORDER BY name;
