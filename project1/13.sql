SELECT DISTINCT p1.id, p1.name, p2.name, p3.name FROM Evolution AS e1
JOIN Evolution AS e2 ON e1.after_id = e2.before_id
JOIN Pokemon AS p1 ON e1.before_id = p1.id
JOIN Pokemon AS p2 ON e1.after_id = p2.id
JOIN Pokemon AS p3 ON e2.after_id = p3.id
ORDER BY p1.id;
