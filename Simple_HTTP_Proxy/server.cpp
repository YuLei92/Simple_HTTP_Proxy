#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <cstring>
#include <time.h>
#include <vector>

using namespace std;
class CacheNode{
  public:
    CacheNode(){
    };
    string DomainName;
    string PagePath;
    string Body;
    time_t lastAccessedTime;
    string lastModifiedTime;
    string expire;
}; //The main data structure to store the information

map<string, CacheNode*> cache; // use map to construct the cache
#define MAX_PEDING 10
char* format = "%a, %d %b %Y %H:%M:%S %Z"; // the time format



void printCache(); // print the Cache
time_t stringToTimeT(string); // transform string to timt_t format
string timeTtoString(time_t); // transform time_t to string format
time_t getTime(); // get the current time
string cutURL(string); // cut the header "http://" or "https://" of the url
bool isFresh(CacheNode*); // find this cache is fresh or not
CacheNode* getPage(string); // send request to the web server and generate the cache node
CacheNode* inCache(string); // get the cache node if the url is in cache.
void eviction(CacheNode*);  //evict one node if the cache is full
CacheNode* notInCache(string);  // get the cache node if the url is not in cache.
string extractRequestURL(char*); // transfer the client request to the url
vector<string> strsplit(string, string);  // split certain part of string
CacheNode* createNode(string, string, string); // use the received response to generate the cache node.

CacheNode* createNode(string DomainName, string PagePath, string body){
  CacheNode* node = new CacheNode;
  node->DomainName = DomainName;
  node->PagePath = PagePath;
  string s_body = body;
  node->Body = s_body;
  time_t curr_t = getTime();
  node->lastAccessedTime = curr_t;

  string delete_sign("\r\n\r\n");
  vector<string> toplevel = strsplit(body, delete_sign); 
//  printf("Not allowed.. response should have only header and body.\n");
  printf("The size is: %d\n", toplevel.size());
  /*
  if(toplevel.size() == 1 || toplevel.size() > 2)
  {
    printf("Not allowed.. response should have only header and body.\n");
    exit(0);
  }
*/
  //now take the header and break it up
  string httpHdr = toplevel.at(0);
  delete_sign = "\r\n";
  vector<string> responseHdrFields = strsplit(httpHdr, delete_sign);
  if(responseHdrFields.size() == 0)
  {
    printf("No tokens found in the http header.\n");
    exit(0);
  }

  //get Expires and Last-modified

  int numFields = responseHdrFields.size();
  vector<string> fieldTokens;
  //cout<<"status line: "<<responseHdrFields.at(0)<<endl;
  delete_sign = ": ";
  for(int i=1; i<numFields; i++) //because first line is the status line
  {
    //first parse each line using ':'
    fieldTokens = strsplit(responseHdrFields.at(i), delete_sign);

    if(fieldTokens.at(0) == "Expires")
      node->expire = fieldTokens.at(1);
    else if(fieldTokens.at(0) == "Last-Modified")
      node->lastModifiedTime = fieldTokens.at(1);
  }
  return node;
}


vector<string> strsplit(string s, string delete_sign){
  vector<string> tokens;
  int start = 0;
  socklen_t len = s.length();
  size_t f;
  int found;
  string token;
  while(start < len)
  {
    f = s.find(delete_sign, start);
    if(f == string::npos)
    {
      //this could be the last token, or the
      //patter does not exist in this string
      tokens.push_back(s.substr(start));
      break;
    }

    found = int(f);
    token = s.substr(start, found - start);
    tokens.push_back(token);
    
    start = found + delete_sign.length();
  }
  return tokens;
}

int main(int argc, char*argv[]) 
{
    struct sockaddr_in sin;
    u_int len;
    int socket_id, new_socket_id; //This is the socket
    char copybuffer[1024];
    char recvbuffer[1024];
    
    
    if(argc != 3){
        fprintf(stderr, "usage: simplex - talk host\n");
        exit(0);
    }
    
    char* server_address = argv[1];
    char* port_no = argv[2];
    printf("The IP address from command line is %s\n",server_address);
    printf("The port number from command line is %s\n",port_no);
    
//    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(server_address); // maysbe ip address;
    sin.sin_port = htons(atoi(port_no));
    
    /* setup passive open*/
    if((socket_id = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Simplex - talk : socket");
        exit(0);
    }
    printf("Socket successfully created!\n");
    
    
    if((bind(socket_id, (struct sockaddr *)&sin, sizeof(sin))) < 0){
        perror("simplex -talk: bind\n");
        exit(0);
    }else{
        printf("Socket binded.\n");
    }
    
    printf("Listening to the client...\n");
    
    if(listen(socket_id, MAX_PEDING)< 0){
        perror("Cannot find the client!\n");
    }
    
    fd_set readfds, master;
    int fdmax, i;
    
    FD_ZERO(&readfds);
    FD_ZERO(&master);
    
    FD_SET(socket_id, &master);
    fdmax = socket_id + 1;
    
    while(1){
        readfds = master;
        memset(copybuffer, 0, sizeof(copybuffer));
        memset(recvbuffer, 0, sizeof(recvbuffer));
//        printf("Begin to SELECT!\n");
        if(select(fdmax, &readfds, NULL ,NULL, NULL) < 0){
            perror("Error: select\n");
            close(socket_id);
            exit(0);
        
        }
        for(i = 0; i < fdmax; i++){
            if(FD_ISSET(i, &readfds)){
                if(i == socket_id){
                    if((new_socket_id = accept(socket_id,(struct sockaddr *)&sin, &len)) <0){
                        perror("simplex - talk: accept\n");
                        exit(0);
                    }
                    FD_SET(new_socket_id, &master);
                    if(new_socket_id > (fdmax - 1)){
                        fdmax = new_socket_id + 1;
                    }
                }else{
                    int recv_len = recv(i, recvbuffer, sizeof(recvbuffer), 0); 
                    if(recv_len <= 0){
                        FD_CLR(i, &master);
                        continue;
                    }
                    string str(recvbuffer, recv_len);
                    strcpy(copybuffer, recvbuffer);
                    printf("The request from the client is : \n %s \n", copybuffer);
                    string temp_url = extractRequestURL(copybuffer);
                    string url = cutURL(temp_url);
                    printf("The following page has been requested: %s \n", url.c_str());
                    CacheNode* page;
                    if(cache.count(url) == 0){
                        page = notInCache(url); 
                    }else{
                        page = inCache(url);
                    }
                    string body = page->Body;
                    send(i, body.c_str(), body.length(), 0);
                    printCache();
                }
            }
        }
    }
    return 0;
}

string extractRequestURL(char* buffer){
  char* token = (char*)malloc(sizeof(char*)); 
  token = strtok(buffer, " ");
  char* tokens[3];
  int i = 0;
  while(token != NULL)
  {
    tokens[i] = token;
    token = strtok(NULL, " ");
    i++;
  }

  //now get the uri
  string result(tokens[1]);
  return result;
}

CacheNode* notInCache(string url){
    CacheNode* new_Node = getPage(url);
    if(new_Node->expire.length() > 1 || new_Node->lastModifiedTime.length() > 1){
      if(cache.size() >= 10){
      //choose the evict
        eviction(new_Node);
      }
      cache.insert(std::pair<string, CacheNode*>(url, new_Node));
    }
    return new_Node;
}

void eviction(CacheNode* new_Node){
  string temp_key, evict_key;
  time_t evict_t = 0;
  CacheNode* temp_node;
  map<string, CacheNode*>::iterator iter = cache.begin();
  for(; iter != cache.end(); iter++){
    temp_key = iter->first;
    temp_node = iter->second;
    if(evict_t == 0 || evict_t > temp_node->lastAccessedTime){
      evict_key = temp_key;
      evict_t = temp_node->lastAccessedTime;
    }
  }
  cache.erase(evict_key);
}

CacheNode* inCache(string url){
    CacheNode* temp_node = cache.find(url)->second; //find the current node;
    if(isFresh(temp_node)){
      time_t currT = getTime();
      temp_node->lastAccessedTime = currT;
    }else{
      cache.erase(url);
      temp_node = getPage(url);
      cache.insert(std::pair<string, CacheNode*>(url, temp_node));
    }
    return temp_node;
}

bool isFresh(CacheNode* node){
  time_t expire_t, lmd_t, curr_t;
  curr_t = getTime();
  if(node->expire.length() > 0){
    expire_t = stringToTimeT(node->expire);
    if(expire_t >= curr_t){
      return true;
    }
  }

  if(node->lastModifiedTime.length() > 0){
    lmd_t = stringToTimeT(node->lastModifiedTime);
    if((node->lastAccessedTime + 24*60*60 < curr_t) && (lmd_t + 30*24*60*60 < curr_t)){
      return false;
    }
  }
  return true;
}

CacheNode* getPage(string url){
  string std_url, DomainName, PagePath;
  std_url = cutURL(url);
  int sign_index = std_url.find("/");
  DomainName = std_url.substr(0, sign_index);
  PagePath = std_url.substr(sign_index, std_url.length() - 1);

  printf("The requested DomainName is: %s\n", DomainName.c_str());
  printf("The requested PagePath is: %s\n", PagePath.c_str());

  int http_socket;
  if((http_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
      perror("Simplex - talk : socket");
      exit(0);
  }

  struct addrinfo *httpinfo;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if(getaddrinfo(DomainName.c_str(), "http", &hints, &httpinfo) != 0)
  {
    perror("Simplex - talk : getaddrinfo");
    exit(0);
  }

  if(connect(http_socket, httpinfo->ai_addr, httpinfo->ai_addrlen) == -1)
  {
    perror("Simplex - talk : connect");
    exit(0);
  }

  string message;
  message = "GET "+url+" HTTP/1.0\r\n\r\n";
  printf("The request send to the web server is : \n %s \n", message.c_str());

  if((send(http_socket, message.c_str(), message.length(), 0)) <=0)
  {
    perror("Simplex - talk : connect");
    exit(0);
  }
  
  int recv_len = 0;
  char recvbuffer[1048576]; // We just recv 1MB documents.
  if((recv_len = recv(http_socket, recvbuffer, 1048576, 0)) <= 0)
  {
    perror("Simplex - talk : connect");
    exit(0);
  }
  
  string recv_body(recvbuffer, recv_len);
  printf("Receive the response: \n %s\n", recv_body.c_str());
  CacheNode* node = createNode(DomainName, PagePath, recv_body);
  return node;
}

string cutURL(string url){
  int sign_index;
  string result;
  sign_index = url.find("/");
  if((url.at(sign_index + 1)) == '/'){
    result = url.substr(sign_index + 2, url.length() - 1);
  }else{
    result = url;
  }
  return result;
}

time_t getTime()
{
  struct tm* tm_t;
  time_t local = time(NULL);
  tm_t = gmtime(&local);
  time_t result = timegm(tm_t);
  return result;
}

string timeTtoString(time_t timeT){
  struct tm* tm_t = gmtime(&timeT);
  char stime[30];
  strftime(stime, 30, format, tm_t);
  string result(stime, 30);
  return result;
}


time_t stringToTimeT(string timeString){
  if(timeString.empty()){
    printf("The time string is null!\n");
    return -1;
  }
  struct tm tm_t;
  char* ret = strptime(timeString.c_str(), format, &tm_t);
  if(ret == NULL)
  {
    printf("The string cannot be recognized");
    return -1;
  }
  time_t result = timegm(&tm_t);
  return result; 
}


void printCache(){
  printf("================= Cache List ================= \n");
  int cache_count = 1;
  string s_LA;
  CacheNode* temp_node;
  map<string, CacheNode*>::iterator iter = cache.begin();
  for(; iter != cache.end(); iter++){
    temp_node = iter->second;
    printf("%d. %s/%s\n", cache_count, temp_node->DomainName.c_str(), temp_node->PagePath.c_str());
    s_LA = timeTtoString(temp_node->lastAccessedTime);
    printf("Last Access: %s\n", s_LA.c_str());
    printf("Last Modified: %s\n", temp_node->lastModifiedTime.c_str());
    printf("Expire: %s\n\n", temp_node->expire.c_str());
  }
  printf("================= End List ================= \n");
}
