# qt-ipc-server
A simple example of how to create an IPC Server on Qt

# To build
Import the project on Qt Creator

# How it works
1. At **"server name"** field set the server name 
2. At **"file name"** set the file name of the file that will sotre the data received from the clients
3. Click on **start** button to start the server

The server will be listening for incomming connections at **serner namer**.
Each new incomming connection will be handled by one thread.
The server can handle two types of requests:
1. Store data

Receives **N** bytes from the clients and store it (as text in append mode) on a file (**file name**)

2. Get data

Reads the entire file (**file name**) and send it to the client.
