CREATE TABLE users (id int, name str);
CREATE TABLE orders (id int, user_id int);
INSERT INTO users VALUES (1, "Alice");
INSERT INTO orders VALUES (10, 1);
SELECT users.name, orders.id FROM users INNER JOIN orders ON users.id = orders.user_id;
