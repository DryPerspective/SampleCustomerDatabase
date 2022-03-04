# CustomerTracker

## Introduction

This project acts as a wrapper around data manipulation inside a SQL database, using [SQLite](https://sqlite.org/index.html). It manages a sample set of customer data following the below schema:

![DB Schema](https://i.imgur.com/uphouSB.png)

Where Customer_ID is the primary key of the Customers Table, Address_ID is the primary key of the CustomerAddress table, and Customer_Short_Name is a unique text identifier for that particular customer.

When run, the code will read (or generate if it doesn't exist) a database file called Customers.db, which if empty will then be filled with sample data. It will prompt the user to decide which operation they wish to perform, and step them through the process. It makes use of prepared statements where necessary to prevent injection.


## Features

There are five main features supported by the code:
1. **View Data** - View the customer and address data which exists in the database, either all of it or data for a specific customer. Use of the SELECT statement.
2. **Add Data** - Add new customer(s) or address(es) to the database. Use of the INSERT.
3. **Update Data** - Update existing data in the database. Use of the UPDATE statement.
4. **Remove Data** - Remove a customer and their addresses, or remove a specific address. Use of the DELETE statement.
5. **Run Custom SQL** - Run a custom-entered SQL statement against the database. In real world code this would of course not be included as it would prevent a security risk.

The user can perform as many of the above features as they please per run of the program.

## Notes on the Code

This was compiled in the C++17 standard using Visual Studio for Windows 10, but to my knowledge does not use any platform-specific code. Other than standard library includes, it requires [SQLite](https://sqlite.org/index.html) to compile. SQLite is not included with the source code and must be downloaded separately, however I will include a pre-compiled version of the project under releases.

