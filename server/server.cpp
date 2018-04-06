/**
 * @Author: YangGuang <sunshine>
 * @Date:   2018-04-06T09:42:05+08:00
 * @Email:  guang334419520@126.com
 * @Filename: server.cpp
 * @Last modified by:   sunshine
 * @Last modified time: 2018-04-06T17:19:26+08:00
 */

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>

const int KMaxLen = 4096;
const char* KServHost = "127.0.0.1";
const int KServPort = 9999;
const int LISTENQ = 20;

using Socket = int;

pthread_mutex_t mutex;

enum MessageFlags {
  LeftBorder = 0,
  TextChat,
  AddFriend,
  DelFriend,
  TextGroup,
  FindNear,
  AgreeOrRefused,
  RightBorder,
  SigIn = 64,
  Register,
  AccountOrPassError = 128,
  AccountError
};

struct RegisterSigin {
    char username[12];
    char password[16];
    MessageFlags flags;
};

struct Message {
  const char* sender;
  const char* receiver;
  const char* data;
  MessageFlags flags;
};

struct Client {
  Socket fd;
  char* name;
}client[FD_SETSIZE];

inline void error_deal(const char* s)
{
  std::cerr << s << std::endl;
  exit(-1);
}

void friend_chat(const Message& mesg);
void friend_add(const Message& mesg);
void friend_delete(const Message& mesg);
void group_chat(const Message& mesg);
void friend_agree_or_refused(const Message& mesg);
void near_find(const Message& mesg);

void (*option_fun[])(const Message& mesg) = {
  friend_chat,
  friend_add,
  friend_delete,
  group_chat,
  friend_agree_or_refused,
  near_find
};



int read_data(Socket sockfd)
{
  Message mesg;
  ssize_t n;
  if ( (n = read(sockfd, &mesg, sizeof(mesg))) > 0) {
    if (mesg.flags > LeftBorder && mesg.flags < RightBorder)
      option_fun[mesg.flags](mesg);
  }

  return n;
}

void friend_chat(const Message& mesg)
{
  // 寻找对应的账号的IP地址，发送数据
}

bool sign_in(const RegisterSigin& user_info)
{

}

bool register_account(const RegisterSigin& user_info)
{

}

char* my_time()
{
  time_t now;
  time(&now);
  return ctime(&now);
}

void* client_thread(void* arg)
{
  int i = *((int*)arg);
  RegisterSigin mesg;
  while (true) {
    if (recv(client[i].fd, &mesg, sizeof(mesg), 0) < 0) {
      client[i].fd = -1;
      printf("[用户]%s异常离线 \t%s\n",client[i].name,my_time());
      strcpy(client[i].name," ");
      pthread_exit(0);
    }
    if (mesg.flags == SigIn)
      if (!sign_in(mesg)) {
        mesg.flags = AccountOrPassError;
        send(client[i].fd, (char*)&mesg, strlen((char*)&mesg), 0);
        continue;
      }
      else
        break;
    else if (mesg.flags == Register) {
      if (!register_account(mesg)) {
        mesg.flags = AccountError;
        send(client[i].fd, (char*)&mesg, strlen((char*)&mesg), 0);
        continue;
      }
    }
  }
  
}



int main(int argc, char const *argv[]) {
  Socket connfd;
  socklen_t client_len;
  time_t now;
  pthread_t thid;

  pthread_mutex_init(&mutex, NULL);
  Socket listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0)
    error_deal("socket error");
  struct sockaddr_in cliaddr, servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(KServPort);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  const int on = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    error_deal("setsockopt reuseaddr error");
  if (bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    error_deal("bind error");

  if (listen(listenfd, LISTENQ) < 0)
    error_deal("listen error");

  for (auto& i : client) {
    i.fd = -1;
    strcpy(i.name, " ");
  }

  while (true) {
      if ( (connfd = accept(listenfd, (sockaddr*)&cliaddr,&client_len)) < 0)
        error_deal("error accept");
      int i;
      for (i = 0; i < FD_SETSIZE; ++i)
        if (client[i].fd < 0) {
          client[i].fd = connfd;
          break;
        }

      if (i == FD_SETSIZE)
        error_deal("too many clients");
      time(&now);
      printf("有新的连接：ip:%s\t%s\n",inet_ntoa(cliaddr.sin_addr),ctime(&now));
      pthread_create(&thid, NULL, client_thread,(void*)&i);
}

  return 0;
}
