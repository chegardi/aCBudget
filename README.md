# aCBudget
A simple terminal budget program written in C, with SQLite3* to handle database.
Database must be set up beforehand in current state. Also, be careful not to delete entire databases, as it stands when using the implemented "aCBudget.select >" interface; any SQL command _WILL_ be executed.

Current features:
- quick-insertion to database
- quick update/divide command
- read from DNB and Sparebanken Sør files
- manual select statements on database
- predefined stats menu for quick printouts of balances
- configuration file for easy default setup
- backup functionality

Wishes from the future:
*. UI - This is in the making.
*. Network capability
*. Smartphone capability

Setup:
- See 'detailed_readme'.txt or use the accompanying Makefile (run 'make sql' if first time, then use 'make' if you've changed the code)

See http://www.sqlite.org/ for download/interface and information on SQLite. I have now added the folder 'sql/' with the required files - but be sure to spread the word of SQLite and their awesome work!