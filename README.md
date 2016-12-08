The third, and certainly most challenging, project of the Secure Software Course was a secure asynchronous messaging platform called "Geemail" written in c++.

Working with one other student, I had to create an application that allows users to register, login, view and send secure messages. Our project has a commandline interface, sqlite database, SHA256 hashing, and SALSA20 encryption/decryption. 

This project involved a lot gathering, handling, and checking user input. We had to create the database from scratch, write the queries necessary, and learn how to connect to and query our database from c++ code. 

The meat of this project was the encryption and hashing functionalities. None of the passwords or messages are stored in plain text. Rather, the passwords are stored in a hashed format using SHA256. For sending and reading emails, the user must input their own secret passcode. This is used to encrypt the message (using the SALSA20 stream cipher) being sent and decrypt the message being read. This way, the messages are not being stored in plain text either. The secret passcode is not stored in any capacity, since it is up to the sender and recipient to know their chosen, shared passcode. 

From this project, I got my first taste of hashing and encryption. I learned about the tools out there programmers can use to help them sure their data. I also gained a lot of experience working with an sqlite database through c++ code. I continued to grow my c++ skills by writing a lot of input/output logic, and juggling between strings and char arrays (strings needed for inputs and char arrays needed for the sqlite functions).
