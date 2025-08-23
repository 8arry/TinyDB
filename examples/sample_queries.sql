-- TinyDB Sample SQL Queries
-- This file contains example SQL statements that demonstrate TinyDB's capabilities

-- Create a users table
CREATE TABLE users (id int, name str, age int);

-- Insert some sample data
INSERT INTO users (id, name, age) VALUES (1, "Alice", 25), (2, "Bob", 30), (3, "Charlie", 22);

-- Select all users
SELECT * FROM users;

-- Select specific columns
SELECT name, age FROM users;

-- Select with WHERE condition
SELECT name FROM users WHERE age > 25;

-- Update user data
UPDATE users SET age = 31 WHERE name = "Bob";

-- Delete a user
DELETE FROM users WHERE id = 3;

-- Select remaining users
SELECT * FROM users;

-- Create another table for products
CREATE TABLE products (id int, name str, price int);

-- Insert product data
INSERT INTO products (id, name, price) VALUES 
    (1, "Laptop", 1000), 
    (2, "Mouse", 25), 
    (3, "Keyboard", 75);

-- Select expensive products
SELECT name, price FROM products WHERE price > 50;

-- Delete all users (demonstrate DELETE without WHERE)
DELETE FROM users;