# DistributedFileSystem
Programming Assignment 3
Nick Smith

Make the server with g++ -std=c++11 -o dfs dfs.cpp
Make the client with g++ -std=c++11 -o dfc dfc.cpp -L/user/lib -lssl -lcrypto

Run each server with ./dfs /DFS* 1000* where * is the server number, 1-4
Run the client with ./dfc

The client commands are:
LIST - lists all the files on the servers
GET fileName - gets fileName from the servers if possible and writes it in the current directory
PUT fileName - sends fileName to the servers. Client handles partitioning
