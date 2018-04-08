/**
 * @Author: YangGuang <sunshine>
 * @Date:   2018-04-06T09:42:05+08:00
 * @Email:  guang334419520@126.com
 * @Filename: server.cpp
 * @Last modified by:   sunshine
 * @Last modified time: 2018-04-08T17:10:37+08:00
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
#include <vector>
#include <map>
#include <fstream>

const int KMaxLen = 4096;
const char* KServHost = "127.0.0.1";
const int KServPort = 9999;
const int LISTENQ = 20;
const int KUserNameMax = 12;
const int KUserPassMax = 16;
const int KGroupName = 12;
const int KDataMax = 2048;
const char* KUserInfoFileName = "userinfo";

using Socket = int;

pthread_mutex_t mutex;


enum MessageFlags {
  LeftBorder = 0,
  PrivateChat,
  AddFriend,
  DelFriend,
  PublicChat,
  FindNear,
  AgreeOrRefused,
  Quit,
  SigOut,
  RightBorder,
  SigIn = 64,
  Register,
  AccountOrPassError = 128,
  AccountError
};

struct RegisterSigin {
    char user_name[KUserNameMax];
    char password[KUserPassMax];
    MessageFlags flags;
};

struct User {
  char user_name[KUserNameMax];
  User(const char* s) {
    strcpy(user_name, s);
  }
  User() {}
};

struct Group {
  char group_name[KGroupName];
  char admin_name[KGroupName];
  std::vector<User> group_user;
};

struct Users {
  User user;
  char password[KUserPassMax];
  std::vector<User> friends;
  std::vector<Group> groups;
};

std::map<User, Users> users_info;

struct Message {
  User sender;
  User receiver;
  char data[KDataMax];
  char time[20];
  MessageFlags flags;
};

struct Client {
  Socket fd;
  char name[KUserNameMax];
}client[FD_SETSIZE];

inline void error_deal(const char* s)
{
  std::cerr << s << std::endl;
  exit(-1);
}

void private_chat(const Message& mesg);
void friend_add(const Message& mesg);
void friend_delete(const Message& mesg);
void public_chat(const Message& mesg);
void friend_agree_or_refused(const Message& mesg);
void near_find(const Message& mesg);

void (*option_fun[])(const Message& mesg) = {
  private_chat,
  friend_add,
  friend_delete,
  public_chat,
  friend_agree_or_refused,
  near_find
};



int read_data(Socket sockfd, const char* name)
{
  Message mesg;
  ssize_t n;
  if ( (n = recv(sockfd, &mesg, sizeof(mesg), 0)) > 0) {
    if (mesg.flags > LeftBorder && mesg.flags < RightBorder)
      option_fun[mesg.flags](mesg);
  }

  return n;
}

void private_chat(const Message& mesg)
{

  // 寻找对应的账号，发送数据
  int i;
  for (i = 0; i < FD_SETSIZE; ++i) {
    if (memcmp(mesg.sender.user_name, client[i].name, KUserNameMax) == 0) {
      if (send(client[i].fd, &mesg, strlen((char*)&mesg), 0) < 0)
        error_deal("send error");
      else
        return ;
    }
  }

  if (i == FD_SETSIZE) {
    //好友不在线
  }
}

void public_chat(const Message& mesg)
{
  Users my_user = users_info[mesg.receiver];
  std::vector<User> group_users;
  for (auto group : my_user.groups) {
    if (memcmp(group.group_name, mesg.sender.user_name, KUserNameMax) == 0)
      group_users = group.group_user;
  }

  for (auto i : group_users) {
    for (auto j : client)
      if (memcpy(i.user_name, j.name, KUserNameMax) == 0)
        if (send(j.fd, &mesg, strlen((char*)&mesg), 0) < 0)
          error_deal("public chat send error");
  }
}

int splict(const std::string& s, size_t pos, char c, std::string& result)
{
  auto pos_first = s.find_first_not_of(c, pos);
  if (pos_first == std::string::npos)
    return -1;
  auto pos_finish = s.find_first_of(c, pos_first);
  if (pos_finish == std::string::npos) {
    result =  s.substr(pos_first, s.size());
    return s.size();
  }
  else {
    result = s.substr(pos_first, pos_finish);
    return pos_finish;
  }

}

void read_user_info_file()
{
  char buf[512];
  std::ifstream in(KUserInfoFileName);
  while (in.getline(buf, 512)) {
    std::string s = buf;
    Users users;
    memset(&users, 0, sizeof(users));
    std::string result;
    int n;
    if ( (n = splict(s, 0, ':', result)) == -1)
      continue;
    memcpy(users.user.user_name, result.c_str(), result.size());
    if ( (n = splict(s, n, ':', result)) != -1)
      memcpy(users.password, result.c_str(), result.size());

    if ( (n = splict(s, ':', n, result)) != -1) {
      std::string cur;
      int i = 0;
      while (true) {
        if ( (i = splict(result, i, ' ', cur)) == -1)
          break;
        else
          users.friends.push_back(User(cur.c_str()));
      }
    }

    if ( (n = splict(s, ':', n, result)) != -1) {
      std::string cur;
      Group group;
      int i = 0;
      while (true) {
        if ( (i = splict(result, i, ' ', cur)) != -1) {
          std::string cur2;
          int j = 0;
          if ( (j = splict(cur, j, '-', cur2)) != -1)
            memcpy(group.group_name, cur2.c_str(), cur2.size());
          if ( (j = splict(cur, j, '-', cur2)) != -1)
            memcpy(group.admin_name, cur2.c_str(), cur2.size());
          while ( (j = splict(cur, j, '-', cur2)) != -1) {
              group.group_user.push_back(cur2.c_str());
          }
          users.groups.push_back(group);
        }
        else
          break;

      }
    }

    users_info.insert(std::make_pair(users.user, users));
  }
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
    pthread_mutex_lock(&mutex);
    if (mesg.flags == SigIn)
      if (!sign_in(mesg)) {
        mesg.flags = AccountOrPassError;
        send(client[i].fd, (char*)&mesg, strlen((char*)&mesg), 0);
        pthread_mutex_unlock(&mutex);
        continue;
      }
      else {
        pthread_mutex_unlock(&mutex);
        break;
      }
    else if (mesg.flags == Register) {
      if (!register_account(mesg)) {
        mesg.flags = AccountError;
        send(client[i].fd, (char*)&mesg, strlen((char*)&mesg), 0);
        pthread_mutex_unlock(&mutex);
        continue;
      }
      else {
        pthread_mutex_unlock(&mutex);
        break;
      }
    }
  }

  while (true) {
    if (read_data(client[i].fd, client[i].name) < 0)
      error_deal("recv error");
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
