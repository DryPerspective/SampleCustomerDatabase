#ifndef CUSTOMERTRACKER_H
#define CUSTOMERTRACKER_H
#pragma once

//Standard library includes
#include <iostream>
#include <stdio.h>
#include <string> //For easy string handling
#include <limits> //To ignore bad input through the console
#include <vector> //To store IDs in certain circumstances.


//Third party includes
#include<sqlite3.h>

//Our callback function for pre-made SQLite queries. This particular function will print the result of the SQL query to the console.
int callback(void* NotUsed, int argc, char** argv, char** azColName);

//A wrapper around the usual sqlite3_exec function to execute pre-made statements, check that they are executed, and if not print the error message to the console.
//If the showMessages parameter is set to false, then messages will not be printed to the console.
//NB: This function does not use prepared SQL statements so should NOT be used with statements which have taken user input if we want to prevent injection.
void executeStatement(std::string& inStmt, sqlite3* inDB, bool showMessages = true);

//Same as above function but for direct c-style string input.
//NB: This function does not use prepared SQL statements so should NOT be used with statements which have taken user input if we want to prevent injection.
void executeStatement(const char* inStmt, sqlite3* inDB, bool showMessages = true);

//A simple function to take an int through the std::cin input stream, with input validation.
int getInt();

//Uses the getInt() function to get an int inside a range between inMin and inMax inclusive.
int getIntBetween(int inMin, int inMax);

//A simple function to read a yes or no answer in through the console. Before using this function the user should be prompted with [y/n]
bool getYesNo();

//A function which trims all leading and trailing whitespace from the input string. Used on inputs as we don't want whitespace there.
void trimWhiteSpace(std::string& inString);

//This function returns the result of SELECT COUNT(colName) FROM tableName. It returns -1 in the event of failed execution.
//NB: Due to limitations in binding values with sqlite, this function does not protect against injection and must not be run on user-input data.
int selectCount(sqlite3* db, const std::string& colName, const std::string& tableName);

//This function is similar to the above, except it allows a condition to be checked. It returns the result of "SELECT COUNT(colName) FROM tableName WHERE conditionColName = conditionValue;"
//Due to the same constraints with sqlite as above, only the parameter conditionValue is injection-safe so only conditionValue can be user-input data.
int selectCount(sqlite3* db, const std::string& colName, const std::string& tableName, const std::string& conditionColName, const std::string& conditionValue);

//This function is a wrapper for binding a single value into a prepared statement.
void bindValueOrNull(sqlite3* inDB, sqlite3_stmt* inStmt, int bindNumber, const char* inLabel);

//This function reads in a customer short name identifier through the console. It only proceeds if the short name matches one in the DB.
std::string getShortName(sqlite3* db);

//This function finds the customer_ID value associated with the input short name. It returns -1 in the event of a failed extraction.
int getCustomerID(sqlite3* db, const std::string& inShortName);

//This function prints all addresses associated with a particular customer to the console, and prompts the user to select a valid address_ID of an address associated with that customer.
//Returns the ID if all went well, returns -1 if there was a SQL error, and returns -2 if the customer is not associated with any addresses.
int getAddressID(sqlite3* inDB, const std::string& inShortName);

//Inserts sample data into the DB, intended to be used exactly once if you are starting with an empty DB or no DB at all (DB and table creation is handled automatically in that case).
void insertSampleData(sqlite3* inDB);



#endif
