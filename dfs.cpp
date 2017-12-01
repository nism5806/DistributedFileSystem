#include <vector>
#include <string>
#include <fstream>
#include <dirent.h>
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
#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>


using namespace std;

#define MAXLINE 4096
#define LISTENQ 32

static vector<string> users;
static vector<string> plainTextPasswords;
static string rootDirectory;
static int portNum;
static bool keepLooping;

void stopLooping(int) {
  keepLooping = false;
 }

string getLineInfo(string token, string line){
  int delimIndex = line.find(' ');
  if(line.substr(0, delimIndex)==token){
    return line.substr(delimIndex+1);
  }else{
    return "";
  }
}

int findKeyInVector(string key, vector<string> v){
  for(int i = 0; i<v.size();i++){
    if(v.at(i).compare(key)==0){
      return i;
    }
  }
  return -1;
}

vector<string> parseCsvIntoVector(string csv){
  vector<string> v;
  int delimIndex = csv.find(',');
  while(delimIndex!=string::npos){
    string name = csv.substr(0,delimIndex);
    csv = csv.substr(delimIndex+1);
    delimIndex = csv.find(',');
    v.push_back(name);
  }
  return v;
}

int readConf(){
  ifstream conf;
  conf.open("dfs.conf");
  if(!conf.is_open()){
    return -1;
  }
  string line;
  while(getline(conf, line)){
    int delimIndex = line.find(' ');
    string token = line.substr(0,delimIndex);
    if(token.compare("Users:")==0){
      string csv = line.substr(delimIndex+1);
      users = parseCsvIntoVector(csv);
      for(int i = 0; i<users.size(); i++){
        if (mkdir((rootDirectory+"/"+users.at(i)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1){
          if( errno == EEXIST ) {
            // alredy exists
          } else {
            // something else
            cout << "cannot create sessionnamefolder error:" << strerror(errno) << endl;
            //throw runtime_exception( strerror(errno) );
          }
        }
      }
    }else if(token.compare("Passwords:")==0){
      string csv = line.substr(delimIndex+1);
      plainTextPasswords = parseCsvIntoVector(csv);
    }
  }
  conf.close();
  return 1;
}

int getCLA(int argc, char ** argv){
  if(argc != 3){
    return -1;
  }
  char buf[MAXLINE];
  getcwd(buf, MAXLINE);
  rootDirectory = string(buf)+string(argv[1]);
  portNum = atoi(argv[2]);
  return 1;
}

string listDir(string user){
  DIR *dir;
  struct dirent *ent;
  string fileList = "LIST "+rootDirectory+"\n";
  string directory = rootDirectory+"/"+user;
  if ((dir = opendir (directory.c_str())) != NULL) {
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL) {
      if(string(ent->d_name).compare(".")==0 || string(ent->d_name).compare("..")==0){
        continue;
      }
      //printf("adding %s\n", ent->d_name);
      fileList.append(ent->d_name);
      fileList.append("\n");
    }
    closedir (dir);
  } else {
    /* could not open directory */
    printf("failed to open directory\n");
    return "";
  }
  return fileList;
}

string writeFile(string user, string arg){
  int delimIndex = arg.find(' ');
  string fileName = arg.substr(0, delimIndex);
  string fileContents = arg.substr(delimIndex+1);
  ofstream outfile(rootDirectory+"/"+user+"/"+fileName);
  outfile << fileContents;
  outfile.close();
  return fileName+" written successfully to "+rootDirectory;
}

string getFile(string user, string arg){
  ifstream ifs;
  ifs.open(rootDirectory+"/"+user+"/"+arg);
  if(ifs.is_open()){
    string fileContents((istreambuf_iterator<char>(ifs)),
                        (istreambuf_iterator<char>()));
    return "GET "+arg+"\n"+fileContents;
  }else{
    return "GET Failed "+arg;
  }

}

string handleReq(string request){
  string user;
  string password;
  //Get First Line: Username
  int delimIndex = request.find('\n');
  string reqLine = request.substr(0, delimIndex);
  request = request.substr(delimIndex+1);
  user = getLineInfo("Username:", reqLine);
  if(user == ""){
    return "";
  }
  int userNum = findKeyInVector(user,users);
  if(userNum==-1){
    return "";
  }
  //Get Second Line: Password
  delimIndex = request.find('\n');
  reqLine = request.substr(0, delimIndex);
  request = request.substr(delimIndex+1);
  password = getLineInfo("Password:", reqLine);
  if(password != plainTextPasswords.at(userNum)){
    //Wrong password
    return "";
  }
  //Get Third Line: request
  delimIndex = request.find(' ');
  string req, arg;
  if(request == "LIST" || request == "LIST\n"){
    req = "LIST";
  }else{
    req = request.substr(0,delimIndex);
    arg = request.substr(delimIndex+1);
  }
  if(req == "LIST"){
    return listDir(user);
  }else if(req == "PUT"){
    return writeFile(user, arg);
  }else if(req == "GET"){
    return getFile(user, arg);
  }else{
    return "";
  }
}

int runServer(){
  int listenfd, connfd, n;
  pid_t childpid;
  socklen_t clilen;
  char buf[MAXLINE];
  struct sockaddr_in cliaddr, servaddr;

  if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0){
    perror("Problem in creating the socket");
    exit(2);
  }
  printf("%s\n", "Socket created");
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(portNum);
  int enable = 1;
  if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
    perror("couldn't allow socket reuse");
    exit(2);
  }
  //bind the socket
  bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
  //listen to the socket by creating a connection queue, then wait for clients
  listen (listenfd, LISTENQ);

  fd_set set;
  struct timeval timeout;
  //fcntl(listenfd, F_SETFL, fcntl(listenfd,F_GETFL, 0)|O_NONBLOCK);
  //keepLooping = (bool*)mmap(NULL, sizeof(*keepLooping), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  keepLooping = true;
  while(keepLooping) {
    int rv;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    FD_ZERO(&set);
    FD_SET(listenfd, &set);
    rv = select(listenfd+1, &set, NULL, NULL, &timeout);
    if(rv == -1){
      perror("select");
      keepLooping = false;
      continue;
    }else if(rv == 0){
      //printf("timeout\n");
      waitpid(-1, NULL, WNOHANG);
      continue;
    }
    //printf("accepting\n");
    clilen = sizeof(cliaddr);
    connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
    if(connfd < 0){
      //printf("pass\n");
      continue;
    }
    printf("forking child\n");
    if ((childpid = fork ()) == 0 ) {
      char buf[MAXLINE];
      if ( (n = recv(connfd, buf, MAXLINE,0)) > 0)  {
        string request{buf};
        printf("handle %s\n", buf);
        string response = handleReq(request);
        //printf("response: >%s<\n", response.c_str());
        char * sendBuff;
        int sendBuffSize = response.size();
        sendBuff = (char*)malloc(sendBuffSize+1*sizeof(char));
        memset(sendBuff,0, sendBuffSize+1);
        response.copy(sendBuff,sendBuffSize,0);
        //printf("sendBuff: >%s<\n", sendBuff);
        printf("%s %s\n", "Sending:", sendBuff);
        int res = send(connfd, sendBuff, sendBuffSize, 0);
        //printf("send returned: %d\n", res);
      }
      shutdown(connfd, SHUT_RDWR);
      close(connfd);
      exit(0);
    }
    waitpid(-1, NULL, WNOHANG);
  }
  //printf("exiting loop\n");
  close(listenfd);
  //printf("closed\n");
  //munmap(keepLooping, sizeof(*keepLooping));
  //exit(0);
  return 0;
}

int main(int argc, char ** argv){
  signal (SIGINT, stopLooping);
  getCLA(argc,argv);
  readConf();
  runServer();
  printf("server done\n");
  //exit(0);
}
