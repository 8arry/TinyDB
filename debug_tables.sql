CREATE TABLE users (id int, name str);
CREATE TABLE orders (id int, user_id int);
INSERT INTO users VALUES (1, "Alice");
INSERT INTO orders VALUES (10, 1);
SELECT * FROM users;
SELECT * FROM orders;
