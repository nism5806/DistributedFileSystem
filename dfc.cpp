#include <openssl/md5.h>
#include <vector>
#include <string>
#include <fstream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <signal.h>
#include <netdb.h>
#include <sstream>
#include <iostream>
#include <string.h>


using namespace std;

#define MAXLINE 4096

struct fileParts{
  string name;
  bool one;
  bool two;
  bool three;
  bool four;
};

string DFS1ip;
string DFS2ip;
string DFS3ip;
string DFS4ip;
string userName;
string plainTextPassword;

string getLineInfo(string token, string line){
  int delimIndex = line.find(' ');
  if(line.substr(0, delimIndex)==token){
    return line.substr(delimIndex+1);
  }else{
    return "";
  }
}

int readConf(){
  ifstream conf;
  conf.open("dfc.conf");
  if(!conf.is_open()){
    return -1;
  }
  string line;
  while(getline(conf, line)){
    //cout << line << endl;
    int delimIndex = line.find(' ');
    string token = line.substr(0,delimIndex);
    if(token.compare("Server")==0){
      string val = line.substr(delimIndex+1);
      delimIndex = val.find(' ');
      string serv = val.substr(0, delimIndex);
      if(serv=="DFS1"){
        DFS1ip = getLineInfo(serv, val);
      }else if(serv == "DFS2"){
        DFS2ip = getLineInfo(serv, val);
      }else if(serv == "DFS3"){
        DFS3ip = getLineInfo(serv, val);
      }else if(serv == "DFS4"){
        DFS4ip = getLineInfo(serv, val);
      }
    }else if(token.compare("Username:")==0){
      userName = getLineInfo("Username:", line);
      //cout<<userName<<endl;
    }else if(token.compare("Password:")==0){
      plainTextPassword = getLineInfo("Password:", line);
      //cout<<plainTextPassword<<endl;
    }
  }
  conf.close();
  return 1;
}

int startClient(string IP, string PORT){
  struct addrinfo *servinfo;
  int rv, sockfd;
  //printf("%s:%s\n", IP.c_str(), PORT.c_str());
  if ((rv = getaddrinfo(IP.c_str(), PORT.c_str(), NULL, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return -1;
  }
  if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol)) == -1) {
    perror("client: socket");
    return -1;
  }
  if (connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    close(sockfd);
    perror("client: connect");
    return -1;
  }
  freeaddrinfo(servinfo);
  return sockfd;
}

int findKeyInVector(string key, vector<fileParts> v){
  for(int i = 0; i<v.size();i++){
    if(v.at(i).name.compare(key)==0){
      return i;
    }
  }
  return -1;
}

int listFiles(){
  int sockfd1,sockfd2, sockfd3, sockfd4;
  int delimIndex = DFS1ip.find(':');
  sockfd1 = startClient(DFS1ip.substr(0, delimIndex), DFS1ip.substr(delimIndex+1));
  sockfd2 = startClient(DFS2ip.substr(0, delimIndex), DFS2ip.substr(delimIndex+1));
  sockfd3 = startClient(DFS3ip.substr(0, delimIndex), DFS3ip.substr(delimIndex+1));
  sockfd4 = startClient(DFS4ip.substr(0, delimIndex), DFS4ip.substr(delimIndex+1));
  string reqMessage = "Username: "+userName+"\nPassword: "+plainTextPassword+"\nLIST\n";
  int reqSize = reqMessage.size();
  char * sendBuff;
  char buf[MAXLINE];
  int rv;
  vector<fileParts> files;
  sendBuff = (char*)malloc((reqSize+1)*sizeof(char));
  memset(sendBuff,0,sizeof(sendBuff));
  reqMessage.copy(sendBuff,reqSize,0);
  send(sockfd1, reqMessage.c_str(), reqSize, 0);
  send(sockfd2, reqMessage.c_str(), reqSize, 0);
  send(sockfd3, reqMessage.c_str(), reqSize, 0);
  send(sockfd4, reqMessage.c_str(), reqSize, 0);
  rv = recv(sockfd1, buf, MAXLINE-1, 0);
  if(rv!=-1){
    string received{buf};
    int delimIndex = received.find('\n');
    if(received.substr(0,delimIndex)== "ERROR"){
      printf("LIST FAILED\n");
    }else{
      received = received.substr(delimIndex+1);
      delimIndex = received.find('\n');
      while(delimIndex != string::npos){
        string fp = received.substr(0,delimIndex);
        string fileName = fp.substr(0, fp.size()-2);
        received = received.substr(delimIndex+1);
        delimIndex = received.find('\n');
        int part = stoi(fp.substr(fp.size()-1));
        int loc = findKeyInVector(fileName, files);
        if(loc == -1){
          struct fileParts newFP;
          newFP.name = fileName;
          newFP.one = part == 1;
          newFP.two = part == 2;
          newFP.three = part == 3;
          newFP.four = part == 4;
          files.push_back(newFP);
        }else{
          files.at(loc).one = (files.at(loc).one || part == 1);
          files.at(loc).two = (files.at(loc).two || part == 2);
          files.at(loc).three = (files.at(loc).three || part == 3);
          files.at(loc).four = (files.at(loc).four || part == 4);
        }
      }
    }
  }
  rv = recv(sockfd2, buf, MAXLINE-1, 0);
  if(rv!=-1){
    string received{buf};
    int delimIndex = received.find('\n');
    if(received.substr(0,delimIndex)== "ERROR"){
      printf("LIST FAILED\n");
    }else{
      received = received.substr(delimIndex+1);
      delimIndex = received.find('\n');
      while(delimIndex != string::npos){
        string fp = received.substr(0,delimIndex);
        string fileName = fp.substr(0, fp.size()-2);
        received = received.substr(delimIndex+1);
        delimIndex = received.find('\n');
        int part = stoi(fp.substr(fp.size()-1));
        int loc = findKeyInVector(fileName, files);
        if(loc == -1){
          struct fileParts newFP;
          newFP.name = fileName;
          newFP.one = part == 1;
          newFP.two = part == 2;
          newFP.three = part == 3;
          newFP.four = part == 4;
          files.push_back(newFP);
        }else{
          files.at(loc).one = (files.at(loc).one || part == 1);
          files.at(loc).two = (files.at(loc).two || part == 2);
          files.at(loc).three = (files.at(loc).three || part == 3);
          files.at(loc).four = (files.at(loc).four || part == 4);
        }
      }
    }
  }
  rv = recv(sockfd3, buf, MAXLINE-1, 0);
  if(rv!=-1){
    string received{buf};
    int delimIndex = received.find('\n');
    if(received.substr(0,delimIndex)== "ERROR"){
      printf("LIST FAILED\n");
    }else{
      received = received.substr(delimIndex+1);
      delimIndex = received.find('\n');
      while(delimIndex != string::npos){
        string fp = received.substr(0,delimIndex);
        string fileName = fp.substr(0, fp.size()-2);
        received = received.substr(delimIndex+1);
        delimIndex = received.find('\n');
        int part = stoi(fp.substr(fp.size()-1));
        int loc = findKeyInVector(fileName, files);
        if(loc == -1){
          struct fileParts newFP;
          newFP.name = fileName;
          newFP.one = part == 1;
          newFP.two = part == 2;
          newFP.three = part == 3;
          newFP.four = part == 4;
          files.push_back(newFP);
        }else{
          files.at(loc).one = (files.at(loc).one || part == 1);
          files.at(loc).two = (files.at(loc).two || part == 2);
          files.at(loc).three = (files.at(loc).three || part == 3);
          files.at(loc).four = (files.at(loc).four || part == 4);
        }
      }
    }
  }
  rv = recv(sockfd4, buf, MAXLINE-1, 0);
  if(rv!=-1){
    string received{buf};
    int delimIndex = received.find('\n');
    if(received.substr(0,delimIndex)== "ERROR"){
      printf("LIST FAILED\n");
    }else{
      received = received.substr(delimIndex+1);
      delimIndex = received.find('\n');
      while(delimIndex != string::npos){
        string fp = received.substr(0,delimIndex);
        string fileName = fp.substr(0, fp.size()-2);
        received = received.substr(delimIndex+1);
        delimIndex = received.find('\n');
        int part = stoi(fp.substr(fp.size()-1));
        int loc = findKeyInVector(fileName, files);
        if(loc == -1){
          struct fileParts newFP;
          newFP.name = fileName;
          newFP.one = part == 1;
          newFP.two = part == 2;
          newFP.three = part == 3;
          newFP.four = part == 4;
          files.push_back(newFP);
        }else{
          files.at(loc).one = (files.at(loc).one || part == 1);
          files.at(loc).two = (files.at(loc).two || part == 2);
          files.at(loc).three = (files.at(loc).three || part == 3);
          files.at(loc).four = (files.at(loc).four || part == 4);
        }
      }
    }
  }
  printf("Files retrieved: \n");
  for(int i = 0; i<files.size(); i++){
    struct fileParts fp = files.at(i);
    if(fp.one && fp.two && fp.three && fp.four){
      printf("%s\n", fp.name.c_str());
    }else{
      printf("%s %s\n", fp.name.c_str(), "[incomplete]");
    }
  }
  return 1;
}

int putFile(string fileName){
  int sockfd11, sockfd12, sockfd21, sockfd22, sockfd31, sockfd32, sockfd41, sockfd42;
  int delimIndex = DFS1ip.find(':');
  sockfd11 = startClient(DFS1ip.substr(0, delimIndex), DFS1ip.substr(delimIndex+1));
  sockfd12 = startClient(DFS1ip.substr(0, delimIndex), DFS1ip.substr(delimIndex+1));
  sockfd21 = startClient(DFS2ip.substr(0, delimIndex), DFS2ip.substr(delimIndex+1));
  sockfd22 = startClient(DFS2ip.substr(0, delimIndex), DFS2ip.substr(delimIndex+1));
  sockfd31 = startClient(DFS3ip.substr(0, delimIndex), DFS3ip.substr(delimIndex+1));
  sockfd32 = startClient(DFS3ip.substr(0, delimIndex), DFS3ip.substr(delimIndex+1));
  sockfd41 = startClient(DFS4ip.substr(0, delimIndex), DFS4ip.substr(delimIndex+1));
  sockfd42 = startClient(DFS4ip.substr(0, delimIndex), DFS4ip.substr(delimIndex+1));
  ifstream fileToSend;
  stringstream fileContents;
  fileToSend.open(fileName);
  fileToSend.seekg(0, fileToSend.end);
  size_t sz = fileToSend.tellg();
  fileToSend.seekg(0, fileToSend.beg);
  int partitionSize = sz/4;
  char fileBuff[sz];
  fileContents << fileToSend.rdbuf();
  fileContents.str().copy(fileBuff,sz, 0);
  unsigned char result[MD5_DIGEST_LENGTH];
  MD5((unsigned char*)fileBuff, sz, result);
  int x = (result[0])%4;
  char filePart1[partitionSize+1];
  char filePart2[partitionSize+1];
  char filePart3[partitionSize+1];
  char filePart4[sz-(3*partitionSize)+1];
  memset(filePart1, 0, partitionSize+1);
  memset(filePart2, 0, partitionSize+1);
  memset(filePart3, 0, partitionSize+1);
  memset(filePart4, 0, partitionSize+1);
  fileContents.str().copy(filePart1, partitionSize, 0);
  fileContents.str().copy(filePart2, partitionSize, partitionSize);
  fileContents.str().copy(filePart3, partitionSize, partitionSize*2);
  fileContents.str().copy(filePart4, sz-3*partitionSize, partitionSize*3);
  string sendHeader = "Username: "+userName+"\nPassword: "+plainTextPassword+"\nPUT ";
  string send1 = sendHeader+fileName+".1 "+filePart1;
  string send2 = sendHeader+fileName+".2 "+filePart2;
  string send3 = sendHeader+fileName+".3 "+filePart3;
  string send4 = sendHeader+fileName+".4 "+filePart4;
  int rv;
  char buf[MAXLINE];
  if(x == 0){
    send(sockfd11, send1.c_str(), send1.size(), 0);
    if(rv = recv(sockfd11, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd12, send2.c_str(), send2.size(), 0);
    if(rv = recv(sockfd12, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd21, send2.c_str(), send2.size(), 0);
    if(rv = recv(sockfd21, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd22, send3.c_str(), send3.size(), 0);
    if(rv = recv(sockfd22, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd31, send3.c_str(), send3.size(), 0);
    if(rv = recv(sockfd31, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd32, send4.c_str(), send4.size(), 0);
    if(rv = recv(sockfd32, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd41, send4.c_str(), send4.size(), 0);
    if(rv = recv(sockfd41, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd42, send1.c_str(), send1.size(), 0);
    if(rv = recv(sockfd42, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
  }else if(x == 1){
    send(sockfd11, send4.c_str(), send4.size(), 0);
    if(rv = recv(sockfd11, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd12, send1.c_str(), send1.size(), 0);
    if(rv = recv(sockfd12, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd21, send1.c_str(), send1.size(), 0);
    if(rv = recv(sockfd21, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd22, send2.c_str(), send2.size(), 0);
    if(rv = recv(sockfd22, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd31, send2.c_str(), send2.size(), 0);
    if(rv = recv(sockfd31, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd32, send3.c_str(), send3.size(), 0);
    if(rv = recv(sockfd32, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd41, send3.c_str(), send3.size(), 0);
    if(rv = recv(sockfd41, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd42, send4.c_str(), send4.size(), 0);
    if(rv = recv(sockfd42, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
  }else if(x == 2){
    send(sockfd11, send3.c_str(), send3.size(), 0);
    if(rv = recv(sockfd11, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd12, send4.c_str(), send4.size(), 0);
    if(rv = recv(sockfd12, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd21, send4.c_str(), send4.size(), 0);
    if(rv = recv(sockfd21, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd22, send1.c_str(), send1.size(), 0);
    if(rv = recv(sockfd22, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd31, send1.c_str(), send1.size(), 0);
    if(rv = recv(sockfd31, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd32, send2.c_str(), send2.size(), 0);
    if(rv = recv(sockfd32, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd41, send2.c_str(), send2.size(), 0);
    if(rv = recv(sockfd41, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd42, send3.c_str(), send3.size(), 0);
    if(rv = recv(sockfd42, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
  }else if(x == 3){
    send(sockfd11, send2.c_str(), send2.size(), 0);
    if(rv = recv(sockfd11, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd12, send3.c_str(), send3.size(), 0);
    if(rv = recv(sockfd12, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd21, send3.c_str(), send3.size(), 0);
    if(rv = recv(sockfd21, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd22, send4.c_str(), send4.size(), 0);
    if(rv = recv(sockfd22, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd31, send4.c_str(), send4.size(), 0);
    if(rv = recv(sockfd31, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd32, send1.c_str(), send1.size(), 0);
    if(rv = recv(sockfd32, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd41, send1.c_str(), send1.size(), 0);
    if(rv = recv(sockfd41, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
    send(sockfd42, send2.c_str(), send2.size(), 0);
    if(rv = recv(sockfd42, buf, MAXLINE-1, 0)!=-1){
      printf("%s\n", buf);
    }
  }
  fileToSend.close();
}

int getFile(string fileName){
  int sockfd1,sockfd2, sockfd3, sockfd4;
  int delimIndex = DFS1ip.find(':');
  int sockfds[16];
  int i;
  for(i=0;i<4;i++){
    sockfds[i] = startClient(DFS1ip.substr(0, delimIndex), DFS1ip.substr(delimIndex+1));
  }
  for(;i<8;i++){
    sockfds[i] = startClient(DFS2ip.substr(0, delimIndex), DFS2ip.substr(delimIndex+1));
  }
  for(;i<12;i++){
    sockfds[i] = startClient(DFS3ip.substr(0, delimIndex), DFS3ip.substr(delimIndex+1));
  }
  for(;i<16;i++){
    sockfds[i] = startClient(DFS4ip.substr(0, delimIndex), DFS4ip.substr(delimIndex+1));
  }
  string sendHeader = "Username: "+userName+"\nPassword: "+plainTextPassword+"\nGET "+fileName;
  string send1 = sendHeader+".1";
  string send2 = sendHeader+".2";
  string send3 = sendHeader+".3";
  string send4 = sendHeader+".4";
  struct fileParts fp;
  string fp1, fp2, fp3, fp4;
  int rv;
  char buf[MAXLINE];
  fp.name = fileName;
  fp.one = false;
  fp.two =false;
  fp.three = false;
  fp.four = false;
  //Server 1-4 part 1
  for(i=0;i<4;i++){
    send(sockfds[i*4], send1.c_str(), send1.size(),0);
    memset(buf,0,MAXLINE);
    if((rv = recv(sockfds[i*4], buf, MAXLINE-1, 0))!=-1){
      string response{buf};
      int delim1 = response.find(' ');
      int delim2 = response.find("\n");
      if(!fp.one && response.substr(delim1+1,delim2-delim1-1)==fileName+".1"){
        fp.one = true;
        fp1 = response.substr(delim2+1);
      }
    }
  }
  //server 1-4 part 2
  for(i=0;i<4;i++){
    send(sockfds[i*4+1], send2.c_str(), send2.size(),0);
    memset(buf,0,MAXLINE);
    if((rv = recv(sockfds[i*4+1], buf, MAXLINE-1, 0))!=-1){
      string response{buf};
      int delim1 = response.find(' ');
      int delim2 = response.find("\n");
      if(!fp.two && response.substr(delim1+1,delim2-delim1-1)==fileName+".2"){
        fp.two= true;
        fp2 = response.substr(delim2+1);
      }
    }
  }
  //Server 1-4 part 3
  for(i=0;i<4;i++){
    send(sockfds[i*4+2], send3.c_str(), send3.size(),0);
    memset(buf,0,MAXLINE);
    if((rv = recv(sockfds[i*4+2], buf, MAXLINE-1, 0))!=-1){
      string response{buf};
      int delim1 = response.find(' ');
      int delim2 = response.find("\n");
      if(!fp.three && response.substr(delim1+1,delim2-delim1-1)==fileName+".3"){
        fp.three = true;
        fp3 = response.substr(delim2+1);
      }
    }
  }
  //Server 1-4 part 4
  for(i=0;i<4;i++){
    send(sockfds[i*4+3], send4.c_str(), send4.size(),0);
    memset(buf,0,MAXLINE);
    if((rv = recv(sockfds[i*4+3], buf, MAXLINE-1, 0))!=-1){
      string response{buf};
      int delim1 = response.find(' ');
      int delim2 = response.find("\n");
      if(!fp.four && response.substr(delim1+1,delim2-delim1-1)==fileName+".4"){
        fp.four = true;
        fp4 = response.substr(delim2+1);
      }
    }
  }
  if(fp.one && fp.two && fp.three && fp.four){
    string fc = fp1 + fp2 + fp3 + fp4;
    ofstream out;
    out.open(fileName);
    out << fc;
    out.close();
    printf("%s\n", "File download successful");
  }else{
    printf("%s\n","File could not be assembled");
  }

}

int runClient(){

  string input;
  do{
    cout<< "Waiting for input..." <<endl;
    getline(cin, input);
    //cout << input<<endl;
    int delimIndex = input.find(' ');
    if(delimIndex == string::npos){
      //printf("Listing files\n");
      listFiles();
    }else{
      string op = input.substr(0, delimIndex);
      string arg = input.substr(delimIndex+1);
      if(op == "PUT"){
        //printf("Putting\n");
        putFile(arg);
      }else if(op == "GET"){
        getFile(arg);
      }
    }
  }while(input != "q");
}

int main(int argc, char** argv){
  readConf();
  runClient();
}
