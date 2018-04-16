/**
 * @Author: YangGuang <sunshine>
 * @Date:   2018-04-06T09:42:05+08:00
 * @Email:  guang334419520@126.com
 * @Filename: server.cpp
 * @Last modified by:   sunshine
 * @Last modified time: 2018-04-16T18:31:20+08:00
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
#include <string>
#include <map>
//#include <hash_map>
#include <fstream>


const int KMaxLen = 4096;           // 消息Max
const int KServPort = 9996;         // 端口号
const int LISTENQ = 20;             // listenq
const int KUserNameMax = 12;        // max len of the username
const int KUserPassMax = 16;        // max len of the password
const int KGroupName = 12;          // max len of the group
const int KDataMax = 2048;          // max len of the data
const int KBufSize = 4096;
const char* KUserInfoFileName = "userinfo";   // file name

using Socket = int;       // 方便使用



/* 消息的类型 */
enum MessageFlags {
  LeftBorder = 0,
  PrivateChat,
  AddFriend,
  DelFriend,
  PublicChat,
  FindNear,
  AgreeOrRefused,
  SigOut,
  RightBorder,
  SigIn = 64,
  Register,
  Succees = 127,
  AccountOrPassError,
  AccountError,
  AgainErro,
  Quit
};
// 用于 好友同意，拒绝，或者登录成功或者失败的消息结构体
struct Flags {
  MessageFlags flags;
};

/* 注册账号 结构 */
struct RegisterSigin {
    char user_name[KUserNameMax];
    char password[KUserPassMax];
    MessageFlags flags;
};
/* 用户账户信息 */
struct User {
  char user_name[KUserNameMax];
  User(const char* s) {
    strcpy(user_name, s);
  }
  User() {}
};
bool operator<(const User&x, const User& y) {
    return strcmp(x.user_name, y.user_name) == -1;
}

bool operator>(const User&x, const User& y) {
  return y < x;
}

bool operator==(const User&x, const User& y) {
  return strcmp(x.user_name, y.user_name) == 0;
}

bool operator!=(const User&x, const User& y) {
  return ! (x == y);
}
/* 群组结构 */
struct Group {
  char group_name[KGroupName];
  char admin_name[KGroupName];
  std::vector<User> group_user;   //群成员
};
/* 用户所有信息 */
struct Users {
  User user;      //用户账户
  char password[KUserPassMax];
  std::vector<User> friends;      // 好友
  std::vector<Group> groups;      // 群组
};


/*
struct less {
  bool operator()(const User& x, const User& y) const
  {
    return strcmp(x.user_name, y.user_name) == -1;
  }
};
*/
//hash_map<int, int> s;
/* 消息结构体 */
struct Message {
  User sender;
  User receiver;
  char data[KDataMax];
  char time[30];
  MessageFlags flags;
};

/* 客户端socket信息，和账户名 */
struct Client {
  Socket fd;
  char name[KUserNameMax];
}client[FD_SETSIZE];

pthread_mutex_t mutex;      //互斥量，多线程所需 用来管理client
pthread_rwlock_t user_info_lock;
//全局变量 用来保存所有用户的信息 这里第一个类型必须是string，如果是User会导致find时出现bug，如果是const char*
//会导致char[] 与 char*比较出现异常
std::map<std::string, Users> users_info;

inline void error_deal(const char* s)     // 错误处理函数
{
  std::cerr << s << std::endl;
  exit(-1);
}

void private_chat(const Message& mesg);     // 私聊
void friend_add(const Message& mesg);       // 添加好友
void friend_delete(const Message& mesg);    // 删除好友
void public_chat(const Message& mesg);      // 群聊
void friend_agree_or_refused(const Message& mesg); //同意或者拒绝好友申请
void near_find(const Message& mesg);        // 附近的人
void left_border(const Message& mesg);

/* 函数指针数组 ， 用来保存各个消息类型所需的处理函数 */
void (*option_fun[])(const Message& mesg) = {
  left_border,
  private_chat,
  friend_add,
  friend_delete,
  public_chat,
  friend_agree_or_refused,
  near_find
};

void left_border(const Message& mesg)
{

}

int read_data(Socket sockfd, const char* name)
{
  Message mesg;
  char buf[KBufSize];
  memset(buf, 0, KBufSize);
  memset(&mesg, 0, sizeof(mesg));

  int n;
  if ( (n = recv(sockfd, buf, KBufSize, 0)) > 0) {
    memcpy(&mesg, buf, sizeof(mesg));
    std::cout << mesg.flags << std::endl;
    if (mesg.flags > LeftBorder && mesg.flags < RightBorder)
      option_fun[mesg.flags](mesg);
  }

  return n;
}

void friend_add(const Message& mesg)
{
  char sbuf[KBufSize];
  memset(sbuf, 0, KBufSize);
  memcpy(sbuf, &mesg, sizeof(mesg));
  for (auto i : client) {

    if (strcmp(i.name, mesg.receiver.user_name) == 0) {
      if (send(i.fd, sbuf, KBufSize, 0) < 0) {
        error_deal("send error");
      }
    }
  }
}
void friend_delete(const Message& mesg)
{

}

void friend_agree_or_refused(const Message& mesg)
{

}

void near_find(const Message& mesg)
{

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
  Users my_user = users_info[mesg.receiver.user_name];
  std::vector<User> group_users;
  for (auto group : my_user.groups) {
    if (memcmp(group.group_name, mesg.sender.user_name, KUserNameMax) == 0)
      group_users = group.group_user;
  }

  for (auto i : group_users) {
    for (auto j : client)
      if (memcmp(i.user_name, j.name, KUserNameMax) == 0)
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

    users_info.insert(std::make_pair(users.user.user_name, users));
  }
  in.close();
}

void save()
{
  std::ofstream out(KUserInfoFileName);
  for (auto user : users_info) {
    out << user.first.c_str() << ":" << user.second.password;
    if (user.second.friends.empty()) {
      out << '\n';
      continue;
    }
    else {
      out << ":";
      for (auto f : user.second.friends)
        out << f.user_name << " ";
      out.seekp( (long)out.tellp() - 1);
    }

    if (!user.second.groups.empty()) {
      out << '\n';
      continue;
    }
    else {
      out << ":";
      for (auto group : user.second.groups) {
        out << group.group_name << "-" << group.admin_name;
        for (auto f : group.group_user) {
          out << "-"<< f.user_name;
        }
        out << " ";
      }
      out.seekp( (long)out.tellp() - 1);
    }
    out << '\n';
  }
  out.close();
}
// return 0代表已经登录了，-1代表账户密码错误，1代表登录成功
bool sign_in(const RegisterSigin& regist_info, int i)
{
  Flags sig_mesg;
  char sbuf[KBufSize];
  //memset(sbuf, 0, KBufSize);
  for (auto c : client) {
    if (strcmp(c.name, regist_info.user_name) == 0) {
      sig_mesg.flags = AgainErro;
      memcpy(sbuf, &sig_mesg, sizeof(sig_mesg));
      if (send(client[i].fd, sbuf, KBufSize, 0) < 0)
        error_deal("error send");
      return false;
    }
  }

  pthread_rwlock_rdlock(&user_info_lock);
  if (strcmp(users_info[regist_info.user_name].password,
      regist_info.password) == 0) {
    pthread_rwlock_unlock(&user_info_lock);
    pthread_mutex_lock(&mutex);
    memcpy(client[i].name, regist_info.user_name,
           strlen(regist_info.user_name));
    pthread_mutex_unlock(&mutex);
    sig_mesg.flags = Succees;
    memcpy(sbuf, &sig_mesg, sizeof(sig_mesg));
    if (send(client[i].fd, sbuf, KBufSize, 0) < 0)
      error_deal("error send");
    Users user = users_info[client[i].name];
    memset(sbuf, 0, KBufSize);
    memcpy(sbuf, &user, sizeof(user));
    if (send(client[i].fd, sbuf, KBufSize, 0) != KBufSize)
      error_deal("error send");
    return true;
  }
  else {
    sig_mesg.flags = AccountOrPassError;
    memcpy(sbuf, &sig_mesg, sizeof(sig_mesg));
    if (send(client[i].fd, sbuf, KBufSize, 0) < 0)
      error_deal("error send");
    return false;
  }

}

bool register_account(const RegisterSigin& register_info, int i)
{

  Flags sig_mesg;
  char sbuf[KBufSize];

  pthread_rwlock_rdlock(&user_info_lock);
  if (users_info.find(register_info.user_name) != users_info.end()) {
    pthread_rwlock_unlock(&user_info_lock);
    sig_mesg.flags = AccountError;
    if (send(client[i].fd, (char*)&sig_mesg, sizeof(sig_mesg), 0) < 0)
      error_deal("error send");
    return false;
  }
/*
  for (auto u : users_info) {
    if (u.first == User(register_info.user_name)) {
      pthread_rwlock_unlock(&user_info_lock);
      sig_mesg.flags = AccountError;
      if (send(client[i].fd, (char*)&sig_mesg, sizeof(sig_mesg), 0) < 0)
        error_deal("error send");
      return false;
    }
  }

  */
    pthread_rwlock_unlock(&user_info_lock);
    pthread_mutex_lock(&mutex);
    memcpy(client[i].name, register_info.user_name, strlen(register_info.user_name));
    pthread_mutex_unlock(&mutex);
    sig_mesg.flags = Succees;
    memcpy(sbuf, &sig_mesg, sizeof(sig_mesg));
    if (send(client[i].fd, sbuf, KBufSize, 0) < 0)
      error_deal("error send");
    Users user;
    memset(&user, 0, sizeof(user));
    memcpy(user.user.user_name, register_info.user_name,strlen(register_info.user_name));
    memcpy( user.password, register_info.password, strlen(register_info.password));
    pthread_rwlock_wrlock(&user_info_lock);
    users_info.insert(std::make_pair(user.user.user_name, user));
    pthread_rwlock_unlock(&user_info_lock);
    memset(sbuf, 0, KBufSize);
    memcpy(sbuf, &user, sizeof(user));
    if (send(client[i].fd, sbuf, KBufSize, 0) < 0) {
      error_deal("error send");
    }
    return true;
  }


char* my_time()
{
  time_t now;
  time(&now);
  return ctime(&now);
}

void quit(int i)
{
  pthread_mutex_lock(&mutex);
  client[i].fd = -1;
  strcpy(client[i].name, " ");
  printf("客户端退出 \t%s\n",my_time());
  pthread_mutex_unlock(&mutex);
  pthread_exit(0);
}

void* client_thread(void* arg)
{
  int i = *((int*)arg);
  RegisterSigin mesg;
  char rbuf[KBufSize];

  while (true) {
    memset(rbuf, 0, KBufSize);
    if (recv(client[i].fd, rbuf, KBufSize, 0) < 0) {
      client[i].fd = -1;
      printf("[用户]%s异常离线 \t%s\n",client[i].name,my_time());
      strcpy(client[i].name," ");
      pthread_exit(0);
    }
    memcpy(&mesg, rbuf, sizeof(mesg));

    if (mesg.flags == SigIn) {
      if (sign_in(mesg, i)) {
          break;
      }
      else {
        continue;
      }
    }
    else if (mesg.flags == Register) {
      if (!register_account(mesg, i)) {
        continue;
      }
      else {
        break;
      }
    }
    else if (mesg.flags == Quit) {
      quit(i);
    }
  }

  while (true) {
    if (read_data(client[i].fd, client[i].name) < 0)
      quit(i);
  }

}

void sig_int(int signo)
{
  save();
  exit(0);
}


int main(int argc, char const *argv[]) {
  Socket connfd;
  socklen_t client_len;
  time_t now;
  pthread_t thid;

  if (signal(SIGINT, sig_int) < 0)
    error_deal("SIGCHLD error");

  pthread_mutex_init(&mutex, NULL);
  pthread_rwlock_init(&user_info_lock, NULL);
  /* 创建 监听套接字 */
  Socket listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listenfd < 0)
    error_deal("socket error");
  /* 初始化 sockaddr */
  struct sockaddr_in cliaddr, servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(KServPort);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  const int on = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    error_deal("setsockopt reuseaddr error");
  /* 地址绑定 */
  if (bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    error_deal("bind error");
  /* 进行监听 */
  if (listen(listenfd, LISTENQ) < 0)
    error_deal("listen error");

  for (auto& i : client) {
    i.fd = -1;
    strcpy(i.name, " ");
  }

  read_user_info_file();


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
      /* 每一个客户都单独利用一个线程进行服务 */
      pthread_create(&thid, NULL, client_thread,(void*)&i);
}

  return 0;
}
