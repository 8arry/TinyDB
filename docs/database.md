# In-Memory Database

Your task is to implement an in-memory database that supports a simplified SQL syntax for creating tables, inserting and modifying data and running queries. Your program should be able to parse a file containing multiple SQL statements and execute them starting from an empty database.

---

## Syntax

An SQL statement consists of a keyword `CREATE TABLE`, `INSERT`, `DELETE FROM` or `SELECT` followed by some arguments, and ends with a semicolon `;`.
We ignore any whitespace that doesn’t occur inside a string, and the parsing should be case-sensitive, i.e. it needs to be `SELECT` and not `select`.

* **CREATE TABLE**
  `CREATE TABLE table_name (column_name1 type, column_name2 type);`
  where type is either `int` or `str`: Creates a table with name `table_name` and columns `column_name1`, `column_name2` of the given types.

  * `int` stores an integer value
  * `str` stores a string of arbitrary size

* **INSERT INTO**
  `INSERT INTO table_name (column_name1, column_name2) VALUES ("a", 2), ("b", 14), ("c", -2);`
  Inserts three new rows into `table_name`, with columns being set to provided values.
  All other columns are set to default values (`""` for `str`, `0` for `int`).

* **DELETE FROM**

  * `DELETE FROM table_name;` → deletes all entries from the table.
  * `DELETE FROM table_name WHERE column_name2 = 2;` → deletes rows where `column_name2 = 2`.

* **UPDATE**
  `UPDATE table_name SET column_name1 = "b" WHERE column_name2 = 2;`
  Changes all rows matching the condition, setting `column_name1` to `"b"`.

* **SELECT**

  * `SELECT * FROM table_name;` → prints all columns for all rows.
  * `SELECT column_name2, column_name1 FROM table_name WHERE column_name1 = "b";` → prints selected columns in specified order.

  Example ASCII table output:

  ```
  +--------+------------+
  | Col1   | Column2    |
  +--------+------------+
  | text   | 42         |
  +--------+------------+
  | text2  |            |
  +--------+------------+
  ```

---

## WHERE Conditions

* Must support simple equality expressions like:

  * `column_name2 = 1`
  * `column_name != 1`
* Extensions: support inequality (`<`, `>`, `<=`, `>=`) and logical operations (`AND`, `OR`) with parentheses and precedence.

---

## Text Quoting

* Parser treats everything between the closest pair of quotes `"` as a string.
* Ignores semicolons or keywords inside quotes.
* No need to implement escaping like `\"`.

---

## Project Structure

* Library **core** implementing:

  * Parsing
  * Database representation
  * Statement execution
* Executable reads SQL from `std::cin`, executes, and:

  * Prints `SELECT` results to `std::cout`
  * Prints errors to `std::cerr`

---

## How to Approach This Project

1. Design representation of a database and different statement types.
2. Implement execution for each statement (return results or modify DB).
3. Implement text output in CSV and ASCII table format.
4. Build parser:

   * Single-statement parser → internal representation
   * Full-file parser → split input at semicolons `;` outside quotes `"`.
5. Implement error handling (syntax & semantic errors → throw exceptions).

---

## Extensions

Choose one (or more):

### Extension 1: JOIN Statements

* Support `INNER JOIN` for combining results from two tables.
* Example:
  `SELECT table1.col1, table2.col2 FROM table1 INNER JOIN table2 ON table1.col2 = table2.col1;`

### Extension 2: Unit Tests

* Use `Catch` or `GoogleTest`.
* Test parsing, valid/invalid inputs, and execution correctness.
* Cover at least one example for each statement type (`CREATE`, `INSERT`, `DELETE`, `SELECT`).

### Extension 3: Persistence

* Implement storage format (binary or text).
* Dump all tables to file and restore later.

---

## Report

* Describe software design (esp. library core).
* Justify design choices.
* Explain C++ features used.
* Aim for \~1 page / 250 words.

---

## Presentation

* Short demo of project (statement parsing + execution).
* Add 1–2 slides overview of design & implementation.
* Target: **10 min presentation**.

---

## Grading

1. Code compiles (ideally no warnings with `-Wall -Wextra`)
2. Works correctly for valid SQL queries
3. Well designed, appropriate C++ features
4. No crashes/segfaults for invalid inputs, report errors correctly
5. No additional failures under sanitizers
6. Efficient (avoid unnecessary copies, etc.)

---

