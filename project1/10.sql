SELECT DISTINCT p.type FROM Pokemon AS p 
JOIN (
(SELECT before_id AS eid FROM Evolution) UNION (SELECT after_id AS eid FROM Evolution)
) AS e ON p.id = e.eid 
GROUP BY p.type
HAVING COUNT(*) >= 3
ORDER BY p.type;

