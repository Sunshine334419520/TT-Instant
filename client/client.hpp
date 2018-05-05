/**
 * @Author: YangGuang <sunshine>
 * @Date:   2018-04-16T12:14:50+08:00
 * @Email:  guang334419520@126.com
 * @Filename: client.hpp
 * @Last modified by:   sunshine
 * @Last modified time: 2018-05-05T16:40:53+08:00
 */

#ifndef __CLIENT_HPP
#define __CLIENT_HPP

#if defined(_WIN32)
 #include <winsock2.h>
 #pragma comment(lib,"ws2_32.lib")
 #if __cplusplus < 201103L
   typedef SOCKET Socket;
 #else
   using Socket = SOCKET;
 #endif
#endif
#if defined(__APPLE__) || defined(__linux__)
 #include <netinet/in.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #if __cplusplus < 201103L
   typedef int Socket;
 #else
   using Socket = int;
 #endif
#endif

#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <ctime>

//#define max(a, b) (((a) > (b)) ? ((a) : (b)))

const int KMaxLen = 4096;           // 消息Max
const int KServPort = 9996;         // server 端口号
const int LISTENQ = 20;             // listenq
const int KUserNameMax = 12;        // max len of the username
const int KUserPassMax = 16;        // max len of the password
const int KGroupName = 12;          // max len of the group
const int KDataMax = 2048;          // max len of the data
const int KBufSize = 4096;
const char* KServAddr = "127.0.0.1";
const unsigned int KRun = 1;
const unsigned int KStop = 0;
//const char* KServAddr = "120.79.204.178";
/* 消息的类型 */
enum MessageFlags {
  LeftBorder = 0,
  PrivateChat,
  AddFriend,
  DelFriend,
  PublicChat,
  FindNear,
  Agree,
  Refuse,
  Quit,
  SigOut,
  RightBorder,
  SigIn = 64,
  Register,
  Succees = 127,
  AccountOrPassError,
  AccountError,
  AgainErro,
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
  User() { }
};
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

/* 消息结构体 */
struct Message {
  User sender;
  User receiver;
  char data[KDataMax];
  char time[30];
  MessageFlags flags;
};

Users my_user_info;
std::vector<Message> messages;    // 接收到的消息结构体
std::vector<Message> request_friend;  //好友申请信息
pthread_mutex_t mutex;
pthread_cond_t cond;
int status = KRun;
static void str_cli(Socket sockfd);
static bool login(Socket sockfd);
static void show_main_menu();
static bool register_account(Socket sockfd);
static void show_all_friend();
static bool add_friend(Socket, const std::vector<std::string>&);
static bool remove_friend(Socket, const std::vector<std::string>&);
static bool private_chat(Socket, const std::string&);
static bool public_chat(Socket, const std::string&);
static void view_message();
static void help();
static void quit(Socket sockfd);
static void request_message_deal(Socket sockfd);
void receiv_message_thread(int arg);
int splict(const std::string& s, size_t pos, char c, std::string& result);

inline char* my_time()
{
  time_t now;
  time(&now);
  return ctime(&now);
}

#if !defined(_WIN32)
//挂起线程函数
inline void pthread_suspend()
{
  if (status == KRun) {
    pthread_mutex_lock(&mutex);
    status = KStop;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);
  }
}

inline void pthread_resume()
{
  if (status == KStop) {
    pthread_mutex_lock(&mutex);
    status = KRun;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);
  }
}
#endif




#endif
