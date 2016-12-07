#include <iostream>
#include <string>
#include <sqlite3.h>
#include <stdio.h>
#include <gcrypt.h>
#include <stdlib.h>
#include <vector>
#include <sstream>

int readOrWrite();
void read();
void write();

using namespace std;

// DATABASE TABLEScreate table USERS( ID integer primary key autoincrement, USERNAME text, PASSWORD text);                               
// sqlite> create table EMAILS( ID integer primary key autoincrement, RECIPIENT text, SENDER text, TITLE text, MESSAGE text);
// User Table:
// 	String username
// 	String passwordHash
// Email Table:
// 	String recipient
// 	String sender
// 	int id
// 	String title
// 	String message

string username;
string message;
string passHash;
int numberEmails;
string userExist;

static int getEmailMessage(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    message = argv[0];
    return 0;
}

static int getAllEmails(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    numberEmails = 0;
    for(i=0; i<argc; i++){
        numberEmails++;
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    // just print all the emails
    return 0;
}

static int getPassword(void *NotUsed, int argc, char **argv, char **azColName){
    passHash = argv[0];
    return 0;
}

static int doesUserExist(void *NotUsed, int argc, char **argv, char **azColName){
    userExist = argv[0];
    return 0;
}

static int insertion(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

// takes in the sql query and an int representing which type of query
// 0 is setting global var message
// 1 is setting global var vector for all messages
// 2 is setting global var password hash
// 3 is for insertions
// 4 is checking if user exists
void dbQuery(string query, int whichQuery){
	sqlite3* db;
    char *zErrMsg = 0;
    int rc;
    
    rc = sqlite3_open("gmail.db", &db);
    if(rc){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
    }
    else{
        char *sql = const_cast<char*>(query.c_str());
       if(whichQuery == 0){
       	rc = sqlite3_exec(db, sql, getEmailMessage, 0, &zErrMsg);
       }
       else if(whichQuery == 1){
       	rc = sqlite3_exec(db, sql, getAllEmails, 0, &zErrMsg);
       }
       else if(whichQuery == 2){
       	rc = sqlite3_exec(db, sql, getPassword, 0, &zErrMsg);
       }
       else if(whichQuery == 4){
       	rc = sqlite3_exec(db, sql, doesUserExist, 0, &zErrMsg);
       }
       else{
       	rc = sqlite3_exec(db, sql, insertion, 0, &zErrMsg);
       }
        
        if( rc != SQLITE_OK ){
        	fprintf(stderr, "SQL error: %s\n", zErrMsg);
          sqlite3_free(zErrMsg);
        }
        else{
         // cout<< "Query Successful" << endl;
        }    
        sqlite3_close(db);
    }

}


string encryptMessage(string message, string key) {
    char * textBuffer = (char *) malloc(message.length());
    strcpy(textBuffer, message.c_str());
    char * salsaKey = (char *) malloc(key.length());
    strcpy(salsaKey, key.c_str());
    string failed = "Failed Encryption";
	gcry_error_t     gcryError;
    gcry_cipher_hd_t gcryCipherHd;
    size_t           index;
    string vector = "AAAAAAAA";
    char * iniVector = (char *) malloc(vector.length());
    strcpy(iniVector, vector.c_str());
    int keyLen = strlen(salsaKey);
    
    if (keyLen > 32) {
        cout << "Key is too long" << endl;
        return failed;
    }
    
    if (keyLen < 32) {
        string buff;
        buff.append(32-keyLen, '0');
        char* temp = (char*) malloc(keyLen);
        strncpy(temp, salsaKey, keyLen);
        salsaKey = (char*) malloc(32);
        strncpy(salsaKey, temp, keyLen);
        strcat(salsaKey, buff.c_str());
        keyLen = 32;
        free(temp);
    }

    gcryError = gcry_cipher_open(
        &gcryCipherHd, // gcry_cipher_hd_t *
        GCRY_CIPHER_SALSA20,   // int
        GCRY_CIPHER_MODE_STREAM,   // int
        0);            // unsigned int
    if (gcryError)
    {
        printf("gcry_cipher_open failed:  %s/%s\n",
               gcry_strsource(gcryError),
               gcry_strerror(gcryError));
        return failed;
    }
    
    gcryError = gcry_cipher_setkey(gcryCipherHd, salsaKey, keyLen);
    if (gcryError)
    {
        printf("gcry_cipher_setkey failed:  %s/%s\n",
               gcry_strsource(gcryError),
               gcry_strerror(gcryError));
        return failed;
    }
    
    gcryError = gcry_cipher_setiv(gcryCipherHd, iniVector, 8);
    if (gcryError)
    {
        printf("gcry_cipher_setiv failed:  %s/%s\n",
               gcry_strsource(gcryError),
               gcry_strerror(gcryError));
        return failed;
    }
    
    size_t txtLength = message.length();
    char * encBuffer = (char*) malloc(txtLength);

    gcryError = gcry_cipher_encrypt(
        gcryCipherHd, // gcry_cipher_hd_t
        encBuffer,    // void *
        txtLength,    // size_t
        textBuffer,    // const void *
        txtLength);   // size_t
    if (gcryError)
    {
        printf("gcry_cipher_encrypt failed:  %s/%s\n",
               gcry_strsource(gcryError),
               gcry_strerror(gcryError));
        return failed;
    }
    free(textBuffer);
    free(salsaKey);
    free(iniVector);
    char * returnBuffer = (char *) malloc(txtLength*2);
        
    memset(returnBuffer, 0, txtLength*2);
    for (index = 0; index<txtLength; index++) {
        sprintf(returnBuffer + index*2, "%02X", (unsigned char)encBuffer[index]);
    }
    string result(returnBuffer);
    free(encBuffer);
    return result;
}

string decryptMessage(string message, string key){
    size_t           index;
    size_t txtLength = message.length();
    
    string fullEncrypt;
    
    for (index = 0; index<txtLength; index+=2) {
        unsigned int x;   
        stringstream ss;
        ss << std::hex << message.substr(index, 2).c_str();
        ss >> x;
        string sym;
        sym = (char) x;
        fullEncrypt.append(sym.c_str());
    }
    
    char * textBuffer = (char *) malloc(fullEncrypt.length());
    strcpy(textBuffer, fullEncrypt.c_str());
    txtLength = fullEncrypt.length();
    
    char * salsaKey = (char *) malloc(key.length());

    strcpy(salsaKey, key.c_str());
    string failed = "Failed Decryption";
	gcry_error_t     gcryError;
    gcry_cipher_hd_t gcryCipherHd;
    string vector = "AAAAAAAA";
    char * iniVector = (char *) malloc(vector.length());
    strcpy(iniVector, vector.c_str());
    
    int keyLen = strlen(salsaKey);
    
    if (keyLen > 32) {
        cout << "Key is too long" << endl;
        return failed;
    }
    
    if (keyLen < 32) {
        string buff;
        buff.append(32-keyLen, '0');
        char* temp = (char*) malloc(keyLen);
        strncpy(temp, salsaKey, keyLen);
        salsaKey = (char*) malloc(32);
        strncpy(salsaKey, temp, keyLen);
        strcat(salsaKey, buff.c_str());
        keyLen = 32;
        free(temp);
    }

    gcryError = gcry_cipher_open(
        &gcryCipherHd, // gcry_cipher_hd_t *
        GCRY_CIPHER_SALSA20,   // int
        GCRY_CIPHER_MODE_STREAM,   // int
        0);            // unsigned int
    if (gcryError)
    {
        printf("gcry_cipher_open failed:  %s/%s\n",
               gcry_strsource(gcryError),
               gcry_strerror(gcryError));
        return failed;
    }
    
    gcryError = gcry_cipher_setkey(gcryCipherHd, salsaKey, keyLen);
    if (gcryError)
    {
        printf("gcry_cipher_setkey failed:  %s/%s\n",
               gcry_strsource(gcryError),
               gcry_strerror(gcryError));
        return failed;
    }
    
    gcryError = gcry_cipher_setiv(gcryCipherHd, iniVector, 8);
    if (gcryError)
    {
        printf("gcry_cipher_setiv failed:  %s/%s\n",
               gcry_strsource(gcryError),
               gcry_strerror(gcryError));
        return failed;
    }
    char * decBuffer = (char*) malloc(txtLength);

    gcryError = gcry_cipher_decrypt(
        gcryCipherHd, // gcry_cipher_hd_t
        decBuffer,    // void *
        txtLength,    // size_t
        textBuffer,    // const void *
        txtLength);   // size_t
    if (gcryError)
    {
        printf("gcry_cipher_decrypt failed:  %s/%s\n",
               gcry_strsource(gcryError),
               gcry_strerror(gcryError));
        return failed;
    }
    free(textBuffer);
    free(salsaKey);
    free(iniVector);
    string result(decBuffer);
    free(decBuffer);
    return result;
}

string hashPassword(string message) {
    char * textBuffer = (char *) malloc(message.length());
    strcpy(textBuffer, message.c_str());
	size_t index;
    size_t txtLength = strlen(textBuffer);
    char * hashBuffer = (char *) malloc(33);
    memset(hashBuffer, 0, 33);
        
    gcry_md_hash_buffer(
        GCRY_MD_SHA256, // gcry_cipher_hd_t
        hashBuffer,    // void *
        textBuffer,    // const void *
        txtLength);   // size_t
            
    char * buffer = (char *) malloc(64);
        
    memset(buffer, 0, 64);
    for (index = 0; index<32; index++) {
        sprintf(buffer + index*2, "%02X", (unsigned char)hashBuffer[index]);
    }
    string result(buffer);
    free(textBuffer);
    free(hashBuffer);
    free(buffer);
    return result;
}
string registerUser(){
	// enter username
	cout << "Please enter a new username: ";
	cin >> username;
	
	// check if that name is available?
	string q = "select count(*) from users where username = '" + username +"';";
    dbQuery(q, 4);
	if(userExist.compare("0") != 0){
	    cout << "User already exists. Try again." << endl;
	    registerUser();
	    return "";
	}
	
	else{
	    // ask for password
    	string password;
    	cout << "Please enter your password: ";
    	cin >> password;
    	
    	// call hashPassword()
    	passHash = hashPassword(password);
    	
    	// store username and password hash database
    	string query = "INSERT INTO USERS (USERNAME, PASSWORD) VALUES ('" + username + "', '" + passHash + "');";
    	dbQuery(query, 3);
	}
	
	return username;
}



void read(){
	// prompt to ask what message (put in id)
	int id;
	cout << "Please enter an email id: ";
	cin >> id;
	// prompt to ask for passphrase
	string passphrase;
	cout << "Please enter a secret passphrase: ";
	cin >> passphrase;
	// pull message out of db
	stringstream stream;
    stream << id;
	string query = "SELECT MESSAGE FROM EMAILS WHERE ID = '" + stream.str() + "';";
	dbQuery(query, 0);
	// decrypt message using given passphrase (call decryptMessage on global var message)
	decryptMessage(message, passphrase);
	// print the decrypted message
	cout << "Your message: " << decryptMessage(message, passphrase) << endl;
	int readNext = readOrWrite();
	if(readNext == 1) {
		read();
	} 
	if(readNext == 0){
		write();
	}
	else{
	    return;
	}
}


void write(){
	// ask user to type in recipient
	// ask user to type in title
	// ask user to type in message
	string recip;
	string title;
	cout << "Please enter your recipient: ";
	cin >> recip;
	cout << "Please enter your title: ";
	cin.get();
	getline(cin, title);
	string inputPassPhrase;
	cout << "Please enter your secret passphrase: ";
	cin >> inputPassPhrase;
	string message;
	cout << "Enter the message \n";
	  cin.get();
	getline(cin, message);
  cout<<"Your Message: " << message << "\n";

	// encrypt message using passphrase
	string decryptedMess = encryptMessage(message, inputPassPhrase);
	// store new email in db
	string query = "INSERT INTO EMAILS (RECIPIENT, SENDER, TITLE, MESSAGE) VALUES ('" + recip + "', '" + username + "', '" + title + "', '" + decryptedMess + "');";
	dbQuery(query, 3);
	cout << "Message Sent :)" << endl;
	bool readNext = readOrWrite();
	if(readNext) {
		read();
	} 
	else{
		write();
	}
}


int readOrWrite() {
	//Return 1 if they want to read, 0 if they want to write;
	int rorW;
	cout << "Please enter 1 if you want to read, enter 0 if you want to write, or 2 to quit: ";
	cin >> rorW;
	return rorW;
}

bool validatePassword(string pword){
	// will hash this password and see if it matches the stored password
	
	string query = "SELECT password FROM users WHERE username = '" + username + "';";
	dbQuery(query, 2); // sets global var passHash to this hash, check this
	if(passHash.compare(hashPassword(pword)) == 0){
		return true;
	}
	else{
		return false;
	}
}


void showMail() {
	//Query for all emails for recipient, sorted by sender name
	string query = "SELECT id,sender,title FROM EMAILS WHERE recipient = '" + username + "' ORDER BY SENDER ASC;";
	dbQuery(query, 1);
	// Print number of emails
	cout << "Number of Emails: " << numberEmails << endl;
}

bool login(){
		//prompt user for username
	cout << "Please enter your username: ";
	cin >> username;
		// prompt user for password
	string password;
	cout << "Please enter your password: ";
	cin >> password;
	if(validatePassword(password)){
		return true;
	}
	else{
		return false;
	}
}

string loginOrRegister() {
	// Prompt user for “login” or “register”
	int isLogin;
	cout << "Please enter 0 for register or 1 for login: ";
	cin >> isLogin;
	if (isLogin) {
		if(login()){
			cout<< "You logged in successfully!" <<endl;
		}
		else{
			cout << "Wrong info!" <<endl;
			loginOrRegister();
		}
	}
	else{
		username = registerUser();
	}
	return username;
}


//g++ geemail.cpp -lgcrypt -lsqlite3

int main(int argc, char** argv) {
	string username;
	username = loginOrRegister();
	showMail();
	int isRead;
	isRead = readOrWrite();
	if(isRead == 1) {
		read();
	} 
	if (isRead == 0){
		write();
	}
	else{
	    return 0;
	}
}