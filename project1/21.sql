SELECT MAX(t.co) AS largest, round(MAX(t.co) * 100 / SUM(t.co), 2)
FROM (SELECT type, COUNT(*) AS co FROM Pokemon GROUP BY type) AS t;
