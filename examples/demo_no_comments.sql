CREATE TABLE users (id int, name str, age int, department str);
CREATE TABLE orders (order_id int, user_id int, amount int, status str);

INSERT INTO users VALUES (1, "Alice", 25, "Engineering");
INSERT INTO users VALUES (2, "Bob", 30, "Marketing");
INSERT INTO users VALUES (3, "Charlie", 35, "Engineering");
INSERT INTO users VALUES (4, "Diana", 28, "Sales");

INSERT INTO orders VALUES (101, 1, 500, "Completed");
INSERT INTO orders VALUES (102, 2, 300, "Pending");
INSERT INTO orders VALUES (103, 1, 700, "Completed");
INSERT INTO orders VALUES (104, 3, 200, "Cancelled");

SELECT * FROM users;

SELECT name, department FROM users WHERE age > 25;

SELECT * FROM users WHERE age >= 30 AND department = "Engineering";

SELECT * FROM orders WHERE amount > 400 OR status = "Pending";

SELECT * FROM users WHERE (age > 30 OR department = "Sales") AND age < 35;

SELECT users.name, orders.amount, orders.status FROM users INNER JOIN orders ON users.id = orders.user_id;

UPDATE users SET department = "Technology" WHERE department = "Engineering";

DELETE FROM orders WHERE status = "Cancelled";

SELECT * FROM users;
SELECT * FROM orders;
