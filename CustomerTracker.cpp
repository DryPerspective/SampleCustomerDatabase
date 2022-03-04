#include "CustomerTracker.h"


// Here we create a callback function to handle our sql commands. Parameters are as follows:
// NotUsed - not used but can be passed directly from the sql_exec() function
// argc - Number of arguments
// argv - Array of values
// azColName - Array of column names
int callback(void* NotUsed, int argc, char** argv, char** azColName) {

	//Print out the values called (e.g. in a SELECT statement)
	for (int i = 0; i < argc; ++i) {
		std::cout << azColName[i] << " : " << std::string(argv[i] ? argv[i] : "NULL") << '\n';	//?: used as a NULL value will throw an exception if unchecked.
	}
	std::cout << '\n';

	//Return success code.
	return 0;
}


//This function wraps around executing pre-made SQL statements, while also providing confirmation printed to the console that they executed properly, or an error message if they did not.
//NB: As this does not use prepared statements, it should only be used for SQL statements with no user input, to prevent injection.
void executeStatement(std::string& inStmt, sqlite3* inDB, bool showMessages) {
	char* zErrorMsg;
	int status{ sqlite3_exec(inDB, inStmt.c_str(), callback, 0, &zErrorMsg) };
	if (status!=SQLITE_OK && showMessages) {
		std::cerr << "Error executing statement: " << sqlite3_errmsg(inDB) << '\n';
	}
	else if(status == SQLITE_OK && showMessages){
		std::cout << "Statement executed successfully.\n";
	}
}

//Alias of the above function to handle c-style string input.
void executeStatement(const char* inStmt, sqlite3* inDB, bool showMessages) {
	std::string newString{ inStmt };
	executeStatement(newString, inDB, showMessages);
}

//A function which gets an int value through the console, with input validation.
int getInt(){
	int input;
	while (true){
		std::cin >> input;
		if (std::cin.fail()){	//If extraction fails
			std::cin.clear();	//Reset our input stream flag
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');	//And ignore anything left in the buffer.
			std::cout << "Error. Please enter a valid integer value. \n";
		}
		else{
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');	//Clear any extraneous input
			return input;
		}
	}
}

//This function builds on the getInt() function and is used for cases where you need an int in a particular range. e.g. for menu options selection.
//NB: Range is inclusive.
int getIntBetween(int inMin, int inMax) {
	int input;
	//Loop because we need to input validate the entered int to ensure it is inside the desired range.
	while (true) {
		input = getInt();
		if (input >= inMin && input <= inMax) return input;
		else {
			std::cout << "Please enter a number which corresponds to one of the options. \n";
		}
	}
}

//A function to read whether the user has entered a y/n yes/no into the console.
bool getYesNo() {
	//We have to loop here for inpute validation
	while (true) {
		char inputChar{};
		std::cin >> inputChar; //Read in our char
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');	//And ignore anything left in the buffer.

		//Check if it's a valid input to be interpreted as yes or no.
		switch (inputChar) {
		case 'y':
		case 'Y':
			return true;
		case 'n':
		case 'N':
			return false;
		default:
			std::cout << "Error. Please enter a valid answer. [y/n] \n";
		}
	}
}

//This function removes leading and trailing whitespace from a string - used on inputs as we don't want leading or trailing whitespace there.
//Returns void as it passes the string in by reference and trims it in place.
void trimWhiteSpace(std::string& inString) {
	size_t startOfWord{ inString.find_first_not_of(" \n\r\t\f\v") };
	size_t endOfWord{ inString.find_last_not_of(" \n\r\t\f\v") };

	if (startOfWord != std::string::npos && endOfWord != std::string::npos)inString = std::move(inString.substr(startOfWord, endOfWord - startOfWord + 1));
	else std::cerr << "Error: Trimming of whitespace failed.\n";
}


//A function to run a SELECT COUNT statement on the db and return the result. If an error occurs, it returns -1.
//NB: This function does not protect from injection. DO NOT CALL IT with user-entered data.
int selectCount(sqlite3* db, const std::string& colName, const std::string& tableName) {
	std::string statement{ "SELECT COUNT(" + colName + ") FROM " + tableName + ";" };	//We can't use prepared statements and bindings on table names and column names, so we have to do this.
	sqlite3_stmt* statementHandle;
	int countResult{ -1 };
	int prepStatus{ sqlite3_prepare_v2(db, statement.c_str(), -1, &statementHandle, NULL) };
	if (prepStatus != SQLITE_OK)std::cerr << "Error preparing SELECT COUNT statement: " << sqlite3_errmsg(db) << '\n';
	else {
		int stepStatus{ sqlite3_step(statementHandle) };
		if (stepStatus != SQLITE_ROW) std::cerr << "Error stepping into SELECT COUNT table: " << sqlite3_errmsg(db) << '\n';		//We expect a result of SQLITE_ROW, meaning that we can process the row in the result table.
		else {
			countResult = sqlite3_column_int(statementHandle, 0);			//Get the result of the statement.
		}
		
	}


	//If all goes well, countResult now holds the result of the SELECT COUNT statement. In any case we need to properly close off our connection.
	sqlite3_finalize(statementHandle);
	return countResult;
	
}


//Similar to the above function, however it allows a condition check. Effectively returns the result of "SELECT COUNT(colName) FROM tableName WHERE conditionColName = conditionValue;
// NB: The final parameter is the ONLY one which protects against injection. Only it should be used with user input.
//As with the above function, returns -1 when the command fails.
int selectCount(sqlite3* db, const std::string& colName, const std::string& tableName, const std::string& conditionColName, const std::string& conditionValue) {
	std::string statement{ "SELECT COUNT(" + colName + ") FROM " + tableName +" WHERE "+conditionColName+" = ? ;" }; //We can't use prepared statements and bindings on table names and column names, so we have to do this.
	sqlite3_stmt* statementHandle;
	int countResult{ -1 };
	int prepStatus{ sqlite3_prepare_v2(db, statement.c_str(), -1, &statementHandle, NULL) };
	if (prepStatus != SQLITE_OK)std::cerr << "Error preparing SELECT COUNT statement: " << sqlite3_errmsg(db) << '\n';
	else {
		int bindStatus{ sqlite3_bind_text(statementHandle,1,conditionValue.c_str(),-1,SQLITE_TRANSIENT) };
		if (bindStatus != SQLITE_OK)std::cerr << "Error binding column name to SELECT COUNT statement: " << sqlite3_errmsg(db) << '\n';
		else {
			int stepStatus{ sqlite3_step(statementHandle) };
			if (stepStatus != SQLITE_ROW) std::cerr << "Error stepping into SELECT COUNT table: " << sqlite3_errmsg(db) << '\n';		//We expect a result of SQLITE_ROW, meaning that we can process the row in the result table.
			else {
				countResult = sqlite3_column_int(statementHandle, 0);			//Get the result of the statement.
			}
		}
	}

	//If all goes well, countResult now holds the result of the SELECT COUNT statement. In any case we need to properly close off our connection.
	sqlite3_finalize(statementHandle);
	return countResult;
}

//This function binds text to a prepared statement. It reads input from the user, and binds the text entered to the statement. If the user does not enter anything, it binds NULL.
//The parameters are as follows:
// inDB and inStmt - the database object and statement being prepared.
// bindNumber - the unbound value in the prepared statement we are binding to.
// inLabel - a label describing the field we're binding into, to be printed to the console in the event of an error.
void bindValueOrNull(sqlite3* inDB, sqlite3_stmt* inStmt, int bindNumber, const char* inLabel) {
	int bindStatus{ 0 };
	std::string inputLine;
	std::getline(std::cin, inputLine);
	if (inputLine.empty()) {
		bindStatus = sqlite3_bind_null(inStmt, bindNumber);
	}
	else {
		trimWhiteSpace(inputLine);
		bindStatus = sqlite3_bind_text(inStmt, bindNumber, inputLine.c_str(), -1, SQLITE_TRANSIENT);
	}
	if (bindStatus != SQLITE_OK)std::cerr << "Error binding " << inLabel << " to statement : " << sqlite3_errmsg(inDB) << '\n';
}

//This function reads in a customer short name identifier entered by the user, and checks if it is in the database.
std::string getShortName(sqlite3* db){
	std::string shortName;
	while (true) {						
		std::getline(std::cin >> std::ws, shortName);		//Read in our short name
		trimWhiteSpace(shortName);						//And trim off any trailing whitespace.

		int shortNameCount{ selectCount(db,"*","Customers","Customer_Short_Name",shortName) };
		if (shortNameCount == 0)std::cout << "Error: Customer short name not found in the database.\nPlease try again\n";
		else if (shortNameCount == -1)std::cout << "An error occurred searching for that name in the database.\nPlease try again.\n";
		else {
			std::cout << "Customer identified. Proceeding.\n";
			break;
		}
	}
	return shortName;
}

//This function finds the customer_ID value associated with the input short name.
int getCustomerID(sqlite3* db, const std::string& inShortName) {
	int customerID{ -1 };
	sqlite3_stmt* statementHandle;

	std::string selectID{ "SELECT Customer_ID FROM Customers WHERE Customer_Short_Name = ?;" };
	int prepStatus = sqlite3_prepare_v2(db, selectID.c_str(), -1, &statementHandle, NULL);
	if (prepStatus != SQLITE_OK)std::cerr << "Error preparing SELECT ID statement: " << sqlite3_errmsg(db) << '\n';
	else {
		int bindStatus{ sqlite3_bind_text(statementHandle,1,inShortName.c_str(),-1,SQLITE_TRANSIENT) };
		if (bindStatus != SQLITE_OK)std::cerr << "Error binding to SELECT ID statement: " << sqlite3_errmsg(db) << '\n';
		else {
			int stepStatus{ sqlite3_step(statementHandle) };
			if (stepStatus != SQLITE_ROW)std::cerr << "Error stepping into SELECT ID table: " << sqlite3_errmsg(db) << '\n';
			else {
				customerID = sqlite3_column_int(statementHandle, 0);
			}
		}
	}

	//Whether it works or not we want to finalise our connection.
	sqlite3_finalize(statementHandle);

	return customerID;
}

//This function prints all of the addresses associated with a particular customer short name, and has the user pick a valid address ID from that list.
int getAddressID(sqlite3* inDB, const std::string& inShortName) {
	int customerID{ getCustomerID(inDB,inShortName) };	//Get the customer's ID.
	//If there was an error fetching the ID of that customer
	if (customerID == -1) {	
		std::cout << "Error fetching customer ID: " << sqlite3_errmsg(inDB) << '\n';
		return -1;
	}

	//First we calculate how many addresses the customer is associated with.
	int customerAddresses{ selectCount(inDB,"*","CustomerAddress","Customer_ID",std::to_string(customerID)) };	

	//If the customer is associated with 0 addresses, we can't pick a valid address ID anyway, so we return early.
	if (customerAddresses == 0) return -2;

	//Or if an error occurred fetching the number of addresses, we also return early.
	else if (customerAddresses < 0) return -1;



	//First we print how many addresses the customer is associated with.
	std::cout << "Customer " << inShortName << " is associated with " << customerAddresses << " addresses:\n";


	//Next, we want to do two things. First, we want to show all the addresses which are associated with the customer in contention.
	//Secondly, we want to make an internal list of these addresses to ensure that the customer only selects an address associated with that customer.
	//To do this we need to iterate over the table returned by a SELECT statement, print its values, and store the address IDs.
	std::string selectStatement{ "SELECT * FROM CustomerAddress WHERE Customer_ID = ?" };
	std::vector<int> addressIDs;
	std::vector<std::string> columnHeaders{ "Address_ID","Customer_ID","Address_Type","Contact_Name","Address_Line_1","Address_Line_2","Address_Line_3","Address_Line_4","Address_Line_5","Created_On","Updated_On" };
	sqlite3_stmt* statementHandle;



	int prepStatus{ sqlite3_prepare_v2(inDB,selectStatement.c_str(),-1,&statementHandle,NULL) };
	if (prepStatus != SQLITE_OK)std::cerr << "Error preparing statement: " << sqlite3_errmsg(inDB) << '\n';
	else {
		int bindStatus{ sqlite3_bind_int(statementHandle,1,customerID) };
		if (bindStatus != SQLITE_OK)std::cerr << "Error binding ID to statement: " << sqlite3_errmsg(inDB) << '\n';
		else {
			while (sqlite3_step(statementHandle) == SQLITE_ROW) {													//Iterate over every row in the result set.
				for (int i = 0; i < 11; ++i) {																		//Iterate over all 11 columns in the table.
					std::cout << columnHeaders[i] << " : " << (sqlite3_column_text(statementHandle, i) ? reinterpret_cast<const char*>(sqlite3_column_text(statementHandle, i)) : "NULL") << '\n';	//And print the result to the console.
																//We need the above ?: statement as if the value coming out is NULL, cout will throw an access violation exception. We also need to cast the result to match types.
				}
				addressIDs.push_back(sqlite3_column_int(statementHandle, 0));		//Store the address ID in the vector.
				std::cout << '\n';				//And put out a newline for nice formatting.
			}
		}
	}
	sqlite3_finalize(statementHandle);	//And don't forget to close off this statement.

	//As each customer might be associated with multiple addresses, we need to pick one.
	std::cout << "Please enter the address ID of the address you would like to process:\n";
	int addressID{ -1 };
	bool addressOnList{ false };
	while (!addressOnList) {
		addressID = getInt();
		for (int i = 0; i < static_cast<int>(addressIDs.size()); ++i) {	//Iterate over our list of valid IDs... (Casting to prevent signed/unsigned mismatch)
			if (addressID == addressIDs[i]) {							//And try to match the user input to one of them.
				addressOnList = true;
				break;
			}
		}
		if (addressOnList)std::cout << "Address identified. Proceeding.\n";
		else std::cout << "Error: Please enter an address ID which corresponds with customer " << inShortName << "\n";
	}
	std::cout << '\n';

	return addressID;
}


int main(){
	//Declare our db
	sqlite3* db;
	
	//Declare the varables we're going to use as we go along.
	std::string stmt;							//Our (non-prepared) sql statement. For hard-coded SQL statements (e.g. opening the db, setting up tables, etc) we don't need to worry about injection.
	sqlite3_stmt* preparedStatement;			//Our prepared statement handle used for when we need to go through the sqlite_prepare_v2() process. 
	std::string inputLine{ "DEFAULT VALUE SHOULD NEVER BE USED" };	//An all-purpose string to store input from the user.

	//Below are the integers used to handle sqlite errors. Sqlite functions return an int corresponding to the success or failure of the function, which can be evaluated to find the error code.
	//As these are going to be used and reused as we go along, they are declared upfront. This should prevent potential issues from name shadowing and other potential scoping pitfalls.
	int prepStatus;
	int bindStatus;
	int stepStatus;
	int openStatus;
	int closeStatus;

	//Use an int to ensure we could make the connection properly
	openStatus = sqlite3_open("Customers.db", &db);

	//If there was an error.
	if (openStatus != SQLITE_OK) {
		std::cerr << "Error opening DB: " << sqlite3_errmsg(db) << '\n';
		sqlite3_close(db);
		return -1;			//If we can't open the DB then we can't do anything remotely useful with it, so in this one case we return early.
	}
	else std::cout << "Database opened successfully." << '\n';

	//Debug lines
	//stmt = "DROP TABLE Customers; DROP TABLE CustomerAddress;";
	//executeStatement(stmt,db);


	//Basic startup - we need tables to manipulate so these lines ensures we always have tables to our particular spec.
	//First, our customer table. Nothing particularly exciting here - just customer details and information.
	stmt = "CREATE TABLE IF NOT EXISTS Customers( \
										Customer_ID INTEGER PRIMARY KEY AUTOINCREMENT, \
										Customer_Short_Name varchar(20) NOT NULL UNIQUE,\
										First_Name varchar(20), \
										Last_Name varchar(20), \
										Group_Name varchar(20),\
										Credit_Limit number(15,2),\
										Outstanding_Credit number(15,2),\
										Created_On date,\
										Updated_On date);";

	std::cout << "Creating Customers Table:\n";
	executeStatement(stmt, db);



	//And because it is possible for a customer to have multiple different addresses, addresses get their own table, keyed to the ID of the customer table.
	stmt = "CREATE TABLE IF NOT EXISTS CustomerAddress( \
										Address_ID INTEGER PRIMARY KEY AUTOINCREMENT, \
										Customer_ID int NOT NULL, \
										Address_Type varchar(10),\
										Contact_Name varchar(50),\
										Address_Line_1 varchar(50) NOT NULL,\
										Address_Line_2 varchar(50), \
										Address_Line_3 varchar(50), \
										Address_Line_4 varchar(50), \
										Address_Line_5 varchar(50), \
										Created_On date, \
										Updated_On date, \
										FOREIGN KEY(Customer_ID) REFERENCES Customers(Customer_ID));";

	std::cout << "Creating Addresses Table:\n";
	executeStatement(stmt, db);
	std::cout << '\n';

	//Now to see if the table is empty, and populate it with sample data if so.
	int customerCount{ -1 };	//Initialise to -1 as it will never be given as an answer for the number of rows in a table.
	//We use the sql statement in the below line as it is slightly more efficient than a simple SELECT COUNT(*) FROM TABLE.
	prepStatus = sqlite3_prepare_v2(db, "SELECT CASE WHEN EXISTS (SELECT * FROM Customers) THEN 1 ELSE 0 END", -1, &preparedStatement, NULL);
	if (prepStatus != SQLITE_OK) std::cerr << "Error reading table size. Error code:" << sqlite3_errmsg(db) << '\n';				//If we don't get a result of SQLITE_OK then something went wrong with this statement.
	else {
		stepStatus = sqlite3_step(preparedStatement);
		if(stepStatus != SQLITE_ROW) std::cerr<< "Error stepping into count table:" << sqlite3_errmsg(db) << '\n';		//We expect a result of SQLITE_ROW, meaning that we can process the row in the result table.
		else {
			customerCount = sqlite3_column_int(preparedStatement, 0);			//Get the result of the statement.
			if (customerCount == 0) { //And if the db is empty, add sample data.
				std::cout << "Customer table is empty. Adding sample data...\n";
				insertSampleData(db);
			}
		}
	}
	sqlite3_finalize(preparedStatement);										//Don't forget to finalise and close this statement's connection.

	if (customerCount == -1)std::cerr << "Error adding sample data to table.\n";
	std::cout << '\n';
	
	//And now that setup is out of the way, we can get on to our main user input.
	std::cout << "Welcome to the Customer Manager. ";
	bool exitProgram{ false };	//This bool is used to keep track of whether to exit the main loop.



	//A big loop which allows us to perform however many operations we like as the program is run.
	while (!exitProgram) {

		//First we need to display options and ask for input.
		std::cout << "Please select your option by entering the correct number : \n" 
		"1. View data in the database. \n"
		"2. Add new data to the database. \n"
		"3. Update existing data in the database. \n"
		"4. Remove customer(s) from the database. \n"
		"5. Run custom SQL on the database. \n"
		"0. Exit \n";

		std::cout << "\n";
		int selection{ getIntBetween(0,5) };
		

		//Now to go over the input.
		switch (selection) {

		case 0:				//-------EXIT--------//
			exitProgram = true;
			break;


		case 1: 				//-------VIEW DATA------//
		{
			//This case is put into its own scope to prevent errors when initialising the below two variables.
			int customerCount{ selectCount(db,"*","Customers") };
			int addressCount{ selectCount(db,"*","CustomerAddress") };
			if (customerCount != -1 && addressCount != -1) {		//If we got the values correctly.
				std::cout << "Currently storing " << customerCount << " customers and " << addressCount << " addresses.\n";
			}
			else std::cerr << "Error: Could not count number of customers and addresses in database.\n";


			//Loop as we may want to perform multiple operations.
			bool breakSelectLoop{ false };
			while (!breakSelectLoop) {
				std::cout << "Please select action:\n"
					"1: View all Customer data.\n"
					"2: View all Address data.\n"
					"3: View all Customer and Address joint data.\n"
					"4: Search for data on a specific customer.\n"
					"0: Exit.\n";
				int userSelection{ getIntBetween(0,4) };

				switch (userSelection) {
				case 0:
					breakSelectLoop = true;
					break;
				case 1:
					executeStatement("SELECT * FROM Customers;", db);
					break;
				case 2:
					executeStatement("SELECT * FROM CustomerAddress;", db);
					break;
				case 3:
					executeStatement("SELECT * FROM Customers INNER JOIN CustomerAddress WHERE Customers.Customer_ID = CustomerAddress.Customer_ID ORDER BY Customers.Customer_ID;", db);
					break;
				case 4:
					std::cout << "Please enter the short name identifier of the customer you would like to search.\n";
					inputLine = getShortName(db);							
				
					
					//We want to print customer data and then address data.
					//Because Customers.Customer_ID is a foreign key in CustomerAddress, we need to get the Customer_ID for that customer.
					//We could also do this with a simple join, but I prefer this approach as it does not repeat customer data in every returned value.
					int customerID{ getCustomerID(db,inputLine) };


					//If the customerID extraction went wrong,
					if (customerID == -1) {
						std::cout << "Error fetching customer data.\n";
					}					
					else { //Otherwise...
						//We print customer data first.
						std::cout << "Customer Data:\n";
						std::string selectStatement{ "SELECT * FROM Customers WHERE Customer_ID = '" + std::to_string(customerID) + "';" };				//As customer ID is an internal (integer) variable and never entered by the user, we don't need to prepare
						executeStatement(selectStatement, db, false);
						//Then count how many addresses the Customer has.
						int numberOfAddresses{ selectCount(db,"*","CustomerAddress","Customer_ID",std::to_string(customerID)) };
						std::cout << "Customer " << inputLine << " is associated with " << numberOfAddresses << " addresses:\n";
						//And print all the addresses, if any.
						selectStatement = "SELECT * FROM CustomerAddress WHERE Customer_ID = '" + std::to_string(customerID) + "';";
						executeStatement(selectStatement, db, false);
					}
				}				
			}
			break;

		}
		case 2:				//-------ADD DATA-------//
		{
			bool breakInsertLoop{ false };
			while (!breakInsertLoop) {
				std::cout << "Would you like to add a new customer or new address to the database?\n"
					"1: Customer\n"
					"2: Address\n"
					"0: Exit\n";
				int addDataSelection{ getIntBetween(0,2) };

				//A note on the code. I'm aware that some might consider it best practice to switch-case over our user selection rather than use an if-else block.
				//Ordinarily I would do this, however we are already inside a switch-case block and I figure nesting switch-cases would cause more problems for readability than it would solve.
				if (addDataSelection == 0)breakInsertLoop = true;
				else if (addDataSelection == 1) {

					//First we take the customer short name. Because this must be unique, we take this first and check to make sure that the name isn't already in the DB.
					//We do this with another loop, which hopefully we'll only neeed to go around once.
					std::string insertShortName;	//Declared outside the loop as we'll need it later.
					while (true) {
						std::cout << "Please enter a unique customer short name, which can be used as an identifier. Typical format: John Smith -> JSMITH \n";
						std::getline(std::cin >> std::ws, insertShortName);

						int shortNameCount{ selectCount(db,"Customer_Short_Name","Customers","Customer_Short_Name",insertShortName) };

						if (shortNameCount == 0)break;
						else std::cout << "Error: Short name already in table. Please use new name or amend existing record.\n \n";
					}

					//Once we have a unique shortname, we can then prepare our other statements and take user input.
					//A note on this: I'm aware that the safest and most foolproof approach is to nest every step inside an if/else so that each step will only get executed if everything previously has gone well.
					//I also know that 6+ layers of nested if/else with repetitive code on every layer won't look pretty.
					// For the purpose of this exercise as a fun personal project, I'm just throwing up errors as they appear and continuing regardless.
					// Input should be fairly foolproof, and errors should only come up from non-user issues.
					// 
					//First we prep our statement. String stored separately for easier reading.
					std::string insertStatement{ "INSERT INTO Customers(Customer_Short_Name, First_Name, Last_Name, Group_Name, Credit_Limit, Outstanding_Credit, Created_On, Updated_On) VALUES (?,?,?,?,?,?,DATE('now'),DATE('now'));" };
					prepStatus = sqlite3_prepare_v2(db, insertStatement.c_str(), -1, &preparedStatement, NULL);
					if (prepStatus != SQLITE_OK)std::cerr << "Error preparing INSERT statement:" << sqlite3_errmsg(db) << '\n';

					//And while we're here, we can bind our unique shortname into the statement.
					bindStatus = sqlite3_bind_text(preparedStatement, 1, insertShortName.c_str(), -1, NULL);
					if (bindStatus != SQLITE_OK)std::cerr << "Error binding short name to statement:" << sqlite3_errmsg(db) << '\n';


					//And now we take our next field, the customer's first name.
					std::cout << "Please enter the new customer's first name:\nLeave blank for NULL\n";
					bindValueOrNull(db, preparedStatement, 2, "Customer First Name");

					//And our next field, the surname.
					std::cout << "Please enter the new customer's surname:\nLeave blank for NULL\n";
					bindValueOrNull(db, preparedStatement, 3, "Customer Surname");

					//Our next field is the group name
					std::cout << "Please enter the new customer's group name:\nLeave blank for NULL\n";
					bindValueOrNull(db, preparedStatement, 4, "Customer Group Name");

					//Next their credit limit credit. This one should be easy to keep non-NULL.
					std::cout << "Please enter the new customer's credit limit:\n";
					int inputInt{ getInt() };
					bindStatus = sqlite3_bind_int(preparedStatement, 5, inputInt);
					if (bindStatus != SQLITE_OK)std::cerr << "Error binding credit limit to statement:" << sqlite3_errmsg(db) << '\n';

					//And finally, their outstanding credit.
					std::cout << "Please enter the new customer's outstanding credit:\n";
					inputInt = getInt();
					bindStatus = sqlite3_bind_int(preparedStatement, 6, inputInt);
					if (bindStatus != SQLITE_OK)std::cerr << "Error binding outstanding balance to statement:" << sqlite3_errmsg(db) << '\n';


					//We can now try to evaluate the prepared statement.
					stepStatus = sqlite3_step(preparedStatement);
					if (stepStatus == SQLITE_DONE)std::cout << "Record added successfully.\n \n";
					else std::cerr << "Error executing statement:" << sqlite3_errmsg(db) << '\n';

					//Whatever happens, we should destruct our statement object.
					sqlite3_finalize(preparedStatement);

				}
				else {
					std::cout << "To add a new address, the corresponding customer must first be specified. Please enter the Customer's Short Name identifier:\n";

					std::cout << "Please enter Customer Short Name:\n";
					std::string inputShortName{ getShortName(db) };

					//So we set up our INSERT statement.
					//As with inserting new customers, the cleanest approach is nested if-else statements which effectively stop all execution if a single operation fails.
					//I have made the decision to forgo that for the sake of easy-to-read code, particularly during the writing/debugging stage.
					std::string insertStatement{ "INSERT INTO CustomerAddress(Customer_ID, Address_Type, Contact_Name, Address_Line_1, Address_Line_2, Address_Line_3, Address_Line_4, Address_Line_5, Created_On, Updated_On) VALUES ((SELECT Customer_ID FROM Customers WHERE Customer_Short_Name = ?),?,?,?,?,?,?,?,DATE('now'),DATE('now'));" };
					prepStatus = sqlite3_prepare_v2(db, insertStatement.c_str(), -1, &preparedStatement, NULL);
					if (prepStatus != SQLITE_OK)std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << '\n';

					//Next we can bind our Customer_ID by binding the SELECT statement inside the INSERT values.
					bindStatus = sqlite3_bind_text(preparedStatement, 1, inputShortName.c_str(), -1, SQLITE_TRANSIENT);
					if (bindStatus != SQLITE_OK)std::cerr << "Error binding short name to statement: " << sqlite3_errmsg(db) << '\n';

					//Next we need to fill our Address_Type field.
					std::cout << "Please enter the address type for the new address:\nLeave blank for NULL.\n";
					bindValueOrNull(db, preparedStatement, 2, "Address Type");


					//Next, the Contact_Name field.
					std::cout << "Please enter the contact name for this address:\nLeave blank for NULL.\n";
					bindValueOrNull(db, preparedStatement, 3, "Contact Name");

					//Next, the first line of the address. This cannot be NULL.
					std::cout << "Please enter the first line of the new address:\n";
					std::getline(std::cin >> std::ws, inputLine);
					trimWhiteSpace(inputLine);
					bindStatus = sqlite3_bind_text(preparedStatement, 4, inputLine.c_str(), -1, SQLITE_TRANSIENT);
					if (bindStatus != SQLITE_OK)std::cerr << "Error binding address line 1 to statement:" << sqlite3_errmsg(db) << '\n';

					//Next line 2.
					std::cout << "Please enter the second line of the new address:\nLeave blank for NULL.\n";
					bindValueOrNull(db, preparedStatement, 5, "Address Line 2");

					//Line 3
					std::cout << "Please enter the third line of the new address:\nLeave blank for NULL.\n";
					bindValueOrNull(db, preparedStatement, 6, "Address Line 3");

					//Line 4
					std::cout << "Please enter the fourth line of the new address:\nLeave blank for NULL.\n";
					bindValueOrNull(db, preparedStatement, 7, "Address Line 4");

					//And finally line 5
					std::cout << "Please enter the fifth line of the new address:\nLeave blank for NULL.\n";
					bindValueOrNull(db, preparedStatement, 8, "Address Line 5");

					//Execute the statement.
					stepStatus = sqlite3_step(preparedStatement);
					if (stepStatus == SQLITE_DONE)std::cout << "Record added successfully.\n";
					else std::cerr << "Error adding record: " << sqlite3_errmsg(db) << '\n';

					//And destruct our connection.
					sqlite3_finalize(preparedStatement);

				}

			}
			break;
		}

		case 3:				//-------UPDATE DATA-------//
		{
			while (true) {
				std::cout << "Which type of data would you like to update?\n"
					"1. Customer\n"
					"2. Address\n"
					"0. Exit\n";
				int userSelection{ getIntBetween(0,2) };

				if (userSelection == 0)break;		//Put the exit up here to prevent unnecessary processing.
				//Also a note on the code - I'm using a series of if statements here for easier readability - remember this occurs inside a switch-case and there is a switch-case used when updating customer data.
				//I figured for short lists of possible cases, alternating if and switch-case would be more readable than three nested switch-cases.

				//Whether updating a customer or an address, we need to know which customer's data we are updating. To prevent repeating ourselves, we put the code up here.
				std::cout << "Please enter the Short Name identifier of the customer you would like to update:\n";
				std::string updateShortName{ getShortName(db) };

				//Next, we want to show the data for that customer. But we don't want to run a non-prepared select statement for a user entered value.
				//Instead we swap the short name out for the customer ID. As this value is never entered by the user (and is an integer), we can safely use it for non-prepared statements.
				int customerID{ getCustomerID(db,updateShortName) };


				if (userSelection == 1) {		//-----UPDATE CUSTOMER----///


					//Now we have our customer ID, we can use it freely in statements with no risk of injection.
					std::cout << "Showing data for customer: " << updateShortName << '\n';
					std::string selectStatement{ "SELECT * FROM Customers WHERE Customer_ID = " + std::to_string(customerID) + ";" };
					executeStatement(selectStatement, db, false);

					std::cout << "Which data would you like to update for this customer?\n"
						"1. Customer Name and Group Name.\n"
						"2. Customer Credit Limit and Outstanding Credit.\n";
					//I would reuse the customer selection int here, but we're inside an if statement for that int's value, so I figured changing it here would be bad practice.
					int updateSelection{ getIntBetween(1,2) };

					std::string updateStatement;	//Declare this here as it's used in both cases.

					switch (updateSelection) {
					case 1:							//-------UPDATE NAME------//
						updateStatement = "UPDATE Customers SET First_Name = ?,Last_Name = ?, Group_Name = ?, Updated_On = DATE('now') WHERE Customer_ID = ?;";
						//Prepare our statement
						prepStatus = sqlite3_prepare_v2(db, updateStatement.c_str(), -1, &preparedStatement, NULL);
						if (prepStatus != SQLITE_OK)std::cerr << "Error preparing update statement: " << sqlite3_errmsg(db) << '\n';

						//Bind our values
						std::cout << "Please enter the customer's updated first name:\nLeave blank for NULL.\n";
						bindValueOrNull(db, preparedStatement, 1, "Customer First Name");

						std::cout << "Please enter the customer's updated surname:\nLeave blank for NULL.\n";
						bindValueOrNull(db, preparedStatement, 2, "Customer Surname");

						std::cout << "Please enter the customer's updated group name:\nLeave blank for NULL.\n";
						bindValueOrNull(db, preparedStatement, 3, "Customer Group Name");

						//Now we bind the customer_ID.
						bindStatus = sqlite3_bind_int(preparedStatement, 4, customerID);
						if (bindStatus != SQLITE_OK)std::cerr << "Error binding customer ID to statement: " << sqlite3_errmsg(db) << '\n';

						//Now we have our bindings, we execute the statement.
						stepStatus = sqlite3_step(preparedStatement);
						if (stepStatus != SQLITE_DONE)std::cerr << "Error executing UPDATE statement: " << sqlite3_errmsg(db) << '\n';	//As this is an insert of one line, we expect a result of SQLITE_OK
						else {
							std::cout << "Record updated successfully.\n";
						}

						//And then we destruct this statement's connection to the DB.
						sqlite3_finalize(preparedStatement);
						break;


					case 2:					//---------UPDATE CREDIT--------//
						updateStatement = "UPDATE Customers SET Credit_Limit = ?, Outstanding_Credit = ?,Updated_On = DATE('now') WHERE Customer_ID = ?;";
						//Prepare our statement
						prepStatus = sqlite3_prepare_v2(db, updateStatement.c_str(), -1, &preparedStatement, NULL);
						if (prepStatus != SQLITE_OK)std::cerr << "Error preparing update statement: " << sqlite3_errmsg(db) << '\n';

						//Bind our values
						std::cout << "Please enter the customer's updated credit limit:\n";
						int creditValue{ getInt() };
						bindStatus = sqlite3_bind_int(preparedStatement, 1, creditValue);
						if (bindStatus != SQLITE_OK)std::cerr << "Error binding new credit limit: " << sqlite3_errmsg(db) << '\n';

						std::cout << "Please enter the customer's updated outstanding credit:\n";
						creditValue = getInt();
						bindStatus = sqlite3_bind_int(preparedStatement, 2, creditValue);
						if (bindStatus != SQLITE_OK)std::cerr << "Error binding new outstanding credit: " << sqlite3_errmsg(db) << '\n';

						bindStatus = sqlite3_bind_int(preparedStatement, 3, customerID);
						if (bindStatus != SQLITE_OK)std::cerr << "Error binding customer ID to INSERT statement: " << sqlite3_errmsg(db) << '\n';

						//Execute our statement
						stepStatus = sqlite3_step(preparedStatement);
						if (stepStatus != SQLITE_DONE)std::cerr << "Error executing UPDATE statement: " << sqlite3_errmsg(db) << '\n';	//As this is an insert of one line, we expect a result of SQLITE_OK
						else {
							std::cout << "Record updated successfully.\n";
						}

						//And destruct our connection
						sqlite3_finalize(preparedStatement);
						break;
					}



				}
				else {		//-----UPDATE ADDRESS-----//

					//First list the addresses and get the ID of the address we're going to update.
					int addressToChange{ getAddressID(db, updateShortName) };

					//First deal with cases where things went wrong.
					if (addressToChange == -2)std::cout << "Customer " << updateShortName << " is not associated with any addresses in the database.\n";
					else if (addressToChange == -1)std::cout << "Error fetching addresses associated with customer " << updateShortName << ": " << sqlite3_errmsg(db) << '\n';

					//Then proceed as planned
					else {
						std::string updateStatement{ "UPDATE CustomerAddress SET Address_Type = ?,Contact_Name = ?,Address_Line_1 = ?,Address_Line_2 = ?,Address_Line_3 = ?, Address_Line_4 = ?,Address_Line_5 = ?,Updated_On = DATE('now') WHERE Address_ID = ?;" };
						prepStatus = sqlite3_prepare_v2(db, updateStatement.c_str(), -1, &preparedStatement, NULL);
						if (prepStatus != SQLITE_OK)std::cerr << "Error preparing UPDATE statement: " << sqlite3_errmsg(db) << '\n';

						//Prompt the user to enter data...
						std::cout << "Please enter the updated Address Type:\nLeave blank for NULL.\n";
						bindValueOrNull(db, preparedStatement, 1, "Address Type");	//...and bind it to the statement.

						std::cout << "Please enter the updated Contact Name:\nLeave blank for NULL.\n";
						bindValueOrNull(db, preparedStatement, 2, "Contact Name");

						//This one can't be NULL so we don't give them that option.
						std::cout << "Please enter the updated first line of the address.\n";
						std::getline(std::cin >> std::ws, inputLine);
						bindStatus = sqlite3_bind_text(preparedStatement, 3, inputLine.c_str(), -1, SQLITE_TRANSIENT);
						if (bindStatus != SQLITE_OK)std::cerr << "Error binding address first line to statement:" << sqlite3_errmsg(db) << '\n';

						//And we're back to values which can be NULL.
						std::cout << "Please enter the updated second line of the address:\nLeave blank for NULL.\n";
						bindValueOrNull(db, preparedStatement, 4, "Address Second Line");

						std::cout << "Please enter the updated third line of the address:\nLeave blank for NULL.\n";
						bindValueOrNull(db, preparedStatement, 5, "Address Third Line");

						std::cout << "Please enter the updated fourth line of the address:\nLeave blank for NULL.\n";
						bindValueOrNull(db, preparedStatement, 6, "Address Fourth Line");

						std::cout << "Please enter the updated fifth line of the address:\nLeave blank for NULL.\n";
						bindValueOrNull(db, preparedStatement, 7, "Address Fifth Line");

						bindStatus = sqlite3_bind_int(preparedStatement, 8, addressToChange);
						if (bindStatus != SQLITE_OK)std::cerr << "Error binding address ID to statement:" << sqlite3_errmsg(db) << '\n';

						//Now we've bound our values, we need to execute our statement
						stepStatus = sqlite3_step(preparedStatement);
						if (stepStatus != SQLITE_DONE)std::cerr << "Error executing UPDATE statement: " << sqlite3_errmsg(db) << '\n';
						else std::cout << "Address updated successfully.\n \n";

						//And we want to close off our statement.
						sqlite3_finalize(preparedStatement);
					}

				}
			}
			break;
		}

		case 4:				//-------REMOVE DATA-------//
			while (true){
				std::cout << "Please select action:\n"
					"1. Delete customer and all associated addresses.\n"
					"2. Delete a single address associated with a particular customer.\n"
					"0. Exit\n";
				int userSelection{ getIntBetween(0,2) };

				if (userSelection == 0)break;
				
				//In either case we need to know which customer we are dealing with.
				std::cout << "Please enter the short name identifier of the customer:\n";
				std::string deleteShortName{ getShortName(db) };

				//Since the customer_ID is shared between both tables, we grab that now.
				int customerID{ getCustomerID(db,deleteShortName) };

				//If fetching it fails we are in trouble, so we don't go any further and loop back to the beginning.
				if (customerID == -1) {
					std::cerr << "Error fetching customer ID for Customer " << deleteShortName << ": " << sqlite3_errmsg(db) << '\n';
				}
				else {
					if (userSelection == 1) {
						//If they picked this option, the process is very simple. All we need to do is run two DELETE statements - one for addresses and one for the customer.
						//We will add confirmation here:
						std::cout << "This command will delete all customer and address data associated with customer " << deleteShortName << ". Are you sure you would like to proceed? [y/n]\n";
						bool proceedWithDelete{ getYesNo() };
						if (proceedWithDelete) {
							//First we delete from the address table.
							std::string deleteAddress{ "DELETE FROM CustomerAddress WHERE Customer_ID = ?;" };
							prepStatus = sqlite3_prepare_v2(db, deleteAddress.c_str(), -1, &preparedStatement, NULL);
							if (prepStatus != SQLITE_OK)std::cerr << "Error preparing delete statement: " << sqlite3_errmsg(db) << '\n';
							else {	//Bind the ID
							bindStatus = sqlite3_bind_int(preparedStatement, 1, customerID);
								if (bindStatus != SQLITE_OK)std::cerr << "Error binding Customer ID to statement: " << sqlite3_errmsg(db) << '\n';
								else {	//Execute the statement
									stepStatus = sqlite3_step(preparedStatement);
									if (stepStatus != SQLITE_DONE)std::cerr << "Error executing DELETE statement: " << sqlite3_errmsg(db) << '\n';
									else std::cout << "Addresses associated with customer " << deleteShortName << " deleted successfully.\n";
								}
							}
							//Whether the above statement executed properly or not, we want to finalise and destruct its DB connection.
							sqlite3_finalize(preparedStatement);

							//Once we have deleted the addresses, we need to delete the customer data.
							//First we delete from the address table.
							std::string deleteCustomer{ "DELETE FROM Customers WHERE Customer_ID = ?;" };
							prepStatus = sqlite3_prepare_v2(db, deleteCustomer.c_str(), -1, &preparedStatement, NULL);
							if (prepStatus != SQLITE_OK)std::cerr << "Error preparing DELETE statement: " << sqlite3_errmsg(db) << '\n';
							else {	//Bind the ID
								bindStatus = sqlite3_bind_int(preparedStatement, 1, customerID);
								if (bindStatus != SQLITE_OK)std::cerr << "Error binding Customer ID to statement: " << sqlite3_errmsg(db) << '\n';
								else {	//Execute the statement
									stepStatus = sqlite3_step(preparedStatement);
									if (stepStatus != SQLITE_DONE)std::cerr << "Error executing DELETE statement: " << sqlite3_errmsg(db) << '\n';
									else std::cout << "Customer data for " << deleteShortName << " deleted successfully.\n";
								}
							}
							//Whether the above statement executed properly or not, we want to finalise and destruct its DB connection.
							sqlite3_finalize(preparedStatement);
						}
						else std::cout << "Deletion of data aborted.\n";
					}

					//If we only want to delete a single address
					else {
						int addressID{ getAddressID(db,deleteShortName) };
						//First we handle the cases where something went wrong, or if there is no data to delete.
						if (addressID == -2)std::cout << "Customer " << deleteShortName << " is not associated with any addresses in the database.\n";
						else if (addressID == -1)std::cout << "Error fetching addresses associated with customer " << deleteShortName << ": " << sqlite3_errmsg(db) << '\n';

						//Otherwise we continue.
						else {
							std::cout << "This statement will delete address " << addressID << " from the database. Would you like to proceed? [y/n]\n";
							bool proceedWithDelete{ getYesNo() };
							if (proceedWithDelete) {
								std::string deleteAddress{ "DELETE FROM CustomerAddress WHERE Address_ID = ?;" };
								prepStatus = sqlite3_prepare_v2(db, deleteAddress.c_str(), -1, &preparedStatement, NULL);
								if(preparedStatement!=SQLITE_OK)
									if (prepStatus != SQLITE_OK)std::cerr << "Error preparing DELETE statement: " << sqlite3_errmsg(db) << '\n';
									else {	//Bind the ID
										bindStatus = sqlite3_bind_int(preparedStatement, 1, addressID);
										if (bindStatus != SQLITE_OK)std::cerr << "Error binding Customer ID to statement: " << sqlite3_errmsg(db) << '\n';
										else {	//Execute the statement
											stepStatus = sqlite3_step(preparedStatement);
											if (stepStatus != SQLITE_DONE)std::cerr << "Error executing DELETE statement: " << sqlite3_errmsg(db) << '\n';
											else std::cout << "Address " << addressID << " deleted successfully.\n";
										}
									}
								//Whether the above statement executed properly or not, we want to finalise and destruct its DB connection.
								sqlite3_finalize(preparedStatement);


							}
							else std::cout << "Deletion of address aborted.\n";
						}
					}
				}


			}
			break;


		case 5:				//-------CUSTOM ENTERED SQL-------//

			//Here we allow the user to enter as many custom SQL statements as they like. These statements can be destructive to the database.
			//I realise that in a real-world example of business code we would not want to do this under any circumstances as it is a huge, gaping security flaw.
			//But this is a personal project, and I want to keep that option in for my own debugging if nothing else.
			std::cout << "Enter custom SQL statement: \nWarning: This statement will be executed regardless of how destructive to the database it may be. \nRun command EXIT to exit.\n";
			while (true) {				
				std::string inputStatement{};
				std::getline(std::cin >> std::ws, inputStatement);
				trimWhiteSpace(inputStatement);
				if (inputStatement == "EXIT")break;


				std::cout << "Executing statement " << inputStatement << '\n';
				executeStatement(inputStatement, db);
				
			}
			break;
		}
		//This line is just for neat formatting for trips around the loop.
		std::cout << '\n';
	}



	//And now that we have done what we set out to do, we need to close our DB connection before exiting.
	closeStatus = sqlite3_close(db);
	if (closeStatus!=SQLITE_OK) {
		std::cerr << "Error closing DB: " << sqlite3_errmsg(db) << '\n';
	}
	else std::cout << "Closed Database Successfully.\n";
	return 0;
}


//This function exists to provide some pre-made sample data to the DB, and should be run if you're starting with an empty DB (or no DB at all).
// I would ordinarily place it above main as per usual convention, but it's a huge block of ugly SQL code which will only ever be used once to generate a new set of sample data, so I've left it here.
// While technically the INSERT OR IGNORE status means we could run this function at every program startup, I figure it's best to only run it when necessary.
void insertSampleData(sqlite3* inDB) {
	std::string stmt;
	stmt = "INSERT OR IGNORE INTO Customers (Customer_ID, Customer_Short_Name, First_Name, Last_Name,  Group_Name, Credit_Limit,  Outstanding_Credit, Created_On, Updated_On) VALUES( 1, 'JSMITH', 'John', 'Smith',  'SMITH FAMILY', ' 10000', ' 0', DATE('now'), DATE('now')); \
\
	INSERT OR IGNORE INTO Customers(Customer_ID, Customer_Short_Name, First_Name, Last_Name, Group_Name, Credit_Limit, Outstanding_Credit, Created_On, Updated_On) VALUES(2,'MSMITH', 'Mary', 'Smith', 'SMITH FAMILY', ' 10000', ' 0', DATE('now'), DATE('now')); \
\
	INSERT OR IGNORE INTO Customers(Customer_ID, Customer_Short_Name, First_Name, Last_Name, Group_Name, Credit_Limit, Outstanding_Credit, Created_On, Updated_On) VALUES(3,'BSMITH','Bob', 'Smith', 'SMITH FAMILY', ' 5000', ' 0', DATE('now'), DATE('now')); \
\
	INSERT OR IGNORE INTO Customers(Customer_ID, Customer_Short_Name, First_Name, Last_Name, Group_Name, Credit_Limit, Outstanding_Credit, Created_On, Updated_On) VALUES(4,'BJONES', 'Brian', 'Jones', 'JONES FAMILY', ' 5000', ' 0', DATE('now'), DATE('now')); \
\
	INSERT OR IGNORE INTO Customers(Customer_ID, Customer_Short_Name, First_Name, Last_Name, Group_Name, Credit_Limit, Outstanding_Credit, Created_On, Updated_On) VALUES(5,'DTRACEY', 'Donald', 'Tracey', 'TRACEY FAMILY', ' 3000', ' 0', DATE('now'), DATE('now')); \
\
	INSERT OR IGNORE INTO Customers(Customer_ID, Customer_Short_Name, First_Name, Last_Name, Group_Name, Credit_Limit, Outstanding_Credit, Created_On, Updated_On) VALUES(6, 'ABAKER', 'Anthony', 'Baker', 'BAKER FAMILY', ' 5000', ' 0', DATE('now'), DATE('now')); \
\
	INSERT OR IGNORE INTO Customers(Customer_ID, Customer_Short_Name, First_Name, Last_Name, Group_Name, Credit_Limit, Outstanding_Credit, Created_On, Updated_On) VALUES(7, 'AMCKECHNIE','Alastair', 'McKechnie', 'MCKECHNIE FAMILY', ' 7000', ' 0', DATE('now'), DATE('now')); \
\
	INSERT OR IGNORE INTO Customers(Customer_ID, Customer_Short_Name, First_Name, Last_Name, Group_Name, Credit_Limit, Outstanding_Credit, Created_On, Updated_On) VALUES(8, 'RGOULDING', 'Robert', 'Goulding', 'GOULDING', ' 5000', ' 0', DATE('now'), DATE('now')); ";

	std::cout << "Adding sample Customer data:\n";
	executeStatement(stmt, inDB);

	stmt = "INSERT OR IGNORE INTO  CustomerAddress (Address_ID, Customer_ID, Address_Type, Contact_Name, Address_Line_1, Address_Line_2, Address_Line_3, Address_Line_4, Address_Line_5, Created_On, Updated_On) VALUES(1,(select Customer_id from Customers where Customer_Short_Name = 'JSMITH'), 'HOME', '', '1 Regent Road', 'London', 'W12 5GG', '', '', DATE('now'), DATE('now')); \
		\
		INSERT OR IGNORE INTO  CustomerAddress(Address_ID, Customer_ID, Address_Type, Contact_Name, Address_Line_1, Address_Line_2, Address_Line_3, Address_Line_4, Address_Line_5, Created_On, Updated_On) VALUES(2,(select Customer_ID from Customers where Customer_Short_Name = 'MSMITH'), 'HOME', '', '1 Regent Road', 'London', 'W12 5GG', '', '', DATE('now'), DATE('now')); \
	\
		INSERT OR IGNORE INTO  CustomerAddress(Address_ID, Customer_ID, Address_Type, Contact_Name, Address_Line_1, Address_Line_2, Address_Line_3, Address_Line_4, Address_Line_5, Created_On, Updated_On) VALUES(3,(select Customer_ID from Customers where Customer_Short_Name = 'BSMITH'), 'HOME', '', '1 Regent Road', 'London', 'W12 5GG', '', '', DATE('now'), DATE('now')); \
	\
		INSERT OR IGNORE INTO  CustomerAddress(Address_ID, Customer_ID, Address_Type, Contact_Name, Address_Line_1, Address_Line_2, Address_Line_3, Address_Line_4, Address_Line_5, Created_On, Updated_On) VALUES(4,(select Customer_ID from Customers where Customer_Short_Name = 'JSMITH'), 'WORK', '', '26 Lombard Street', 'London', 'EC4', '', '', DATE('now'), DATE('now')); \
	\
		INSERT OR IGNORE INTO  CustomerAddress(Address_ID, Customer_ID, Address_Type, Contact_Name, Address_Line_1, Address_Line_2, Address_Line_3, Address_Line_4, Address_Line_5, Created_On, Updated_On) VALUES(5,(select Customer_ID from Customers where Customer_Short_Name = 'DTRACEY'), 'HOME', '', '5 Bright Street', 'Dorking', 'Surrey', '', '', DATE('now'), DATE('now')); \
	\
		INSERT OR IGNORE INTO  CustomerAddress(Address_ID, Customer_ID, Address_Type, Contact_Name, Address_Line_1, Address_Line_2, Address_Line_3, Address_Line_4, Address_Line_5, Created_On, Updated_On) VALUES(6,(select Customer_ID from Customers where Customer_Short_Name = 'ABAKER'), 'HOME', '', '21 Hope Street', 'Barnet', 'Middlesex', '', '', DATE('now'), DATE('now')); \
	\
		INSERT OR IGNORE INTO  CustomerAddress(Address_ID, Customer_ID, Address_Type, Contact_Name, Address_Line_1, Address_Line_2, Address_Line_3, Address_Line_4, Address_Line_5, Created_On, Updated_On) VALUES(7,(select Customer_ID from Customers where Customer_Short_Name = 'ABAKER'), 'WORK', '', '1 Canada Square', 'Canary Wharf', 'London', '', '', DATE('now'), DATE('now')); \
	\
		INSERT OR IGNORE INTO  CustomerAddress(Address_ID, Customer_ID, Address_Type, Contact_Name, Address_Line_1, Address_Line_2, Address_Line_3, Address_Line_4, Address_Line_5, Created_On, Updated_On) VALUES(8,(select Customer_ID from Customers where Customer_Short_Name = 'ABAKER'), 'UNKNOWN', '', '17 Broad Street', 'London', 'EC3', '', '', DATE('now'), DATE('now'));";

	std::cout << "Adding sample Address data: \n";
	executeStatement(stmt, inDB);
}