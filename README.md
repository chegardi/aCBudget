# aCBudget
A simple terminal budget program written in C, with SQLite3* to handle database.
Database must be set up beforehand in current state. Also, be careful not to delete entire databases, as it stands when using the implemented "aCBudget.select >" interface; any SQL command _WILL_ be executed.

Program has NOT been run with Valgrind or any other programs checking for memory-leaks, unused/uninitialized variables etc. As written in licence: Use at your own risk! 
But I can promise you I have not added any code trying to ruin your computer or fetch information about you.

Current features:
- quick-insertion to database
- quick update/divide command
- read from DNB and Sparebanken Sør files
- manual select statements on database
- predefined stats menu for quick printouts of balances
- configuration file for easy default setup
- backup functionality

Wishes from the future:
1. More solid and secure code and database manipulation.
* This has been done a lot in the last couple of months
2. [-G]UI
* Will remove the G, I am working on a user interface at the moment - and the 'insert' command has been implemented. More of this will be coming soon!
3. Network capability
4. Smartphone capability

Setup:
- See detailed_readme.txt or use the accompanying Makefile

*See http://www.sqlite.org/ for download/interface and information on SQLite. I have now added the folder 'sql/' with the required files - but be sure to spread the word of SQLite and their awesome work!