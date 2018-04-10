/**
 * @Author: YangGuang <sunshine>
 * @Date:   2018-04-10T15:19:20+08:00
 * @Email:  guang334419520@126.com
 * @Filename: client.cpp
 * @Last modified by:   sunshine
 * @Last modified time: 2018-04-10T21:47:23+08:00
 */

 #if defined(_WIN32)
  #include <winsock2.h>
  #pragma comment(lib,"ws2_32.lib")
  #if defined(C11)
  using Socket = SOCKET;
  #else
  typedef SOCKET Socket;
  #endif
 #endif
 #if defined(__APPLE__) || defined(LINUX)
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  using Socket = int;
 #endif

 #include <iostream>
 #include <stdlib.h>
 #include <stdio.h>
 #include <cstring>
 #include <time.h>
 #include <vector>
 #include <string>
 #include <map>
 #include <fstream>

 #define max(a, b) ((a) > (b)) ? ((a) : (b)))

 const int KMaxLen = 4096;           // 消息Max
 const int KServPort = 9999;         // server 端口号
 const int LISTENQ = 20;             // listenq
 const int KUserNameMax = 12;        // max len of the username
 const int KUserPassMax = 16;        // max len of the password
 const int KGroupName = 12;          // max len of the group
 const int KDataMax = 2048;          // max len of the data
 const char* KServAddr = "127.0.0.1";

 /* 消息的类型 */
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
   Succees = 127,
   AccountOrPassError,
   AccountError,
   AgainErro
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
   char time[20];
   MessageFlags flags;
 };

 void str_cli(Socket sockfd);
 bool login(Socket sockfd);
 void show_main_menu();
 bool register_account(Socket sockfd);
 Users my_user_info;

 int main(int argc, char const *argv[]) {
 #if defined(_WIN32_PLATFROM_)
    WORD sock_version = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sock_version, &wsaData) != 0) {
      printf("wsa start error\n");
      return 0;
    }

 #endif
    Socket clientfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientfd < 0) {
      return -1;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(KServPort);
    #if !defined(_WIN32)
      if (inet_pton(AF_INET, KServAddr, &servaddr.sin_addr) < 0) {
        printf("inet_pton error\n");
        return -1;
      }
    #else
      servaddr.sin_addr.S_un.S_addr = inet_addr(KServAddr);
    #endif

    if (connect(clientfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
      printf("connect error\n");
      return -1;
    }

    str_cli(clientfd);
   return 0;
 }

 void str_cli(Socket sockfd)
 {
   while (!login(sockfd))
    ;
   show_main_menu();
 }

 bool login(Socket sockfd)
 {
   printf("             <----- 1. 登 录 ----->\n");
   printf("             <----- 2. 注 册 ----->\n");
   printf("             <----- 0. 注 册 ----->\n");
   printf("     请输入: ");
   int n;
   scanf("%d", &n);
   if (n == 1) {
     std::string account;
     std::string password;
     printf("   账户: ");
     std::cin >> account;
     printf("   密码: ");
     std::cin >> password;
     if (account.size() > KUserNameMax || password.size() > KUserPassMax) {
       std::cout << "   error : 账户或者密码太长 " << std::endl;
       return false;
     }
     RegisterSigin mesg;
     strcpy(mesg.user_name, account.c_str());
     strcpy(mesg.password, password.c_str());
     mesg.flags = SigIn;
     if (send(sockfd, (char*)&mesg, strlen((char*)&mesg), 0) < 0) {
       std::cout << "send error" << std::endl;
       exit(-1);
     }
     Flags flags_mesg;
     if (recv(sockfd, (char*)&flags_mesg, sizeof(flags_mesg), 0) < 0) {
       std::cout << "recv error" << std::endl;
       exit(-1);
     }
     if (flags_mesg.flags == Succees) {
       if (recv(sockfd, (char*)&my_user_info, sizeof(Users), 0) < 0) {
         std::cout << "recv error" << std::endl;
         exit(-1);
       }
       return true;
     }
     else if (flags_mesg.flags == AccountError) {
       std::cout << "账户或者密码错误" << std::endl;
       return false;
     } else {
       std::cout << "账户已经登录" << std::endl;
       return false;
     }
   }
   else if (n == 2) {
     register_account(sockfd);
   }
   else {
     exit(0);
   }
   return false;
 }

bool register_account(Socket sockfd)
{
  return true;
}

 void show_main_menu()
 {

 }
