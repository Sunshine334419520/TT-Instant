/**
 * @Author: YangGuang <sunshine>
 * @Date:   2018-04-10T15:19:20+08:00
 * @Email:  guang334419520@126.com
 * @Filename: client.cpp
 * @Last modified by:   sunshine
 * @Last modified time: 2018-04-15T17:25:44+08:00
 */

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
  #include <netinet/in.h>
  #include <unistd.h>
  #include <arpa/inet.h>
  #if __cplusplus < 201103L
    typedef int Socket;
  #else
    using Socket = int;
  #endif
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
 #include "Hash.hpp"

 #define max(a, b) (((a) > (b)) ? ((a) : (b)))


 const int KMaxLen = 4096;           // 消息Max
 const int KServPort = 9996;         // server 端口号
 const int LISTENQ = 20;             // listenq
 const int KUserNameMax = 12;        // max len of the username
 const int KUserPassMax = 16;        // max len of the password
 const int KGroupName = 12;          // max len of the group
 const int KDataMax = 2048;          // max len of the data
 const int KBufSize = 1024;
 const char* KServAddr = "127.0.0.1";

 //const char* KServAddr = "120.79.204.178";

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

 Users my_user_info;
 static void str_cli(Socket sockfd);
 static bool login(Socket sockfd);
 static void show_main_menu();
 static bool register_account(Socket sockfd);
 static void show_all_friend();
 static bool add_friend(const std::vector<std::string>&);
 static bool remove_friend(const std::vector<std::string>&);
 static bool private_chat(std::string);
 static bool public_chat(std::string);
 static void help();
 int splict(const std::string& s, size_t pos, char c, std::string& result);


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

 static void str_cli(Socket sockfd)
 {
   while (!login(sockfd))
    ;
   show_main_menu();
   std::cin.ignore();
   while (true) {
     std::string str;
     std::string cmd;
     std::cout << "\033[31m     " << my_user_info.user.user_name << ">\033[0m";
     getline(std::cin, str);
     size_t pos = 0;
     if (!(pos = splict(str, pos, ' ', cmd))) {
       std::cout << "error : 格式错误" << std::endl;
       continue;
     }
     switch (HashFunc(cmd.c_str())) {
       case HashCompile("sf"):
       case HashCompile("see friend"):
       {
         show_all_friend();
         break;
       }
       case HashCompile("af"):
       case HashCompile("add friend"):
       {
         std::vector<std::string> args;
         std::string arg;
         while ((pos = splict(str, pos, ' ', arg)))
            args.push_back(arg);
         if (args.empty()) {
           std::cout << "error : 格式错误, 没有参数" << std::endl;
           break;
         }
         add_friend(args);
         break;
       }
       case HashCompile("rf"):
       case HashCompile("remove friend"):
       {
         std::vector<std::string> args;
         std::string arg;
         while ((pos = splict(str, pos, ' ', arg)))
            args.push_back(arg);
         if (args.empty()) {
           std::cout << "error : 格式错误, 没有参数" << std::endl;
           break;
         }
         remove_friend(args);
         break;
       }
       case HashCompile("pc"):
       case HashCompile("private chat"):
       {
         std::string arg;
         if ( !(splict(str, pos, ' ', arg))) {
           std::cout << "error : 格式错误, 没有参数" << std::endl;
           break;
         }
         private_chat(arg);
       }
       case HashCompile("gc"):
       case HashCompile("group chat"):
       {
         std::string arg;
         if ( !(splict(str, pos, ' ', arg))) {
           std::cout << "error : 格式错误, 没有参数" << std::endl;
           break;
         }
         public_chat(arg);
       }
       case HashCompile("help"):
       {
         help();
         break;
       }
       default:
       {
         std::cout << " error : 没有这个命令" << std::endl;
       }
     }
   }

 }

 static bool login(Socket sockfd)
 {
   system("clear");
   printf("             <------------------ 1. 登 录 ------------------>\n");
   printf("             <------------------ 2. 注 册 ------------------>\n");
   printf("             <------------------ 0. 退 出 ------------------>\n");
   printf("     请输入: ");
   int n;
   scanf("%d", &n);
   if (n == 1) {
     char buf[KBufSize];
     //memset(buf, 0, KBufSize);
     std::string account;
     std::string password;
     printf("     账户: ");
     std::cin >> account;
     printf("     密码: ");
     std::cin >> password;
     if (account.size() > KUserNameMax || password.size() > KUserPassMax) {
       std::cout << "   error : 账户或者密码太长 " << std::endl;
       return false;
     }
     RegisterSigin mesg;
     memset(&mesg, 0, sizeof(mesg));
     strcpy(mesg.user_name, account.c_str());
     strcpy(mesg.password, password.c_str());
     mesg.flags = SigIn;
     memcpy(buf, &mesg, sizeof(mesg));
     if (send(sockfd, buf, KBufSize, 0) < 0) {
       std::cout << "send error" << std::endl;
       exit(-1);
     }
     Flags flags_mesg;
     memset(&flags_mesg, 0, sizeof(flags_mesg));
     memset(buf, 0, KBufSize);
     if (recv(sockfd, buf, KBufSize, 0) < 0) {
       std::cout << "recv error" << std::endl;
       exit(-1);
     }
     memcpy(&flags_mesg, buf, sizeof(flags_mesg));
     if (flags_mesg.flags == Succees) {
       if (recv(sockfd, (char*)&my_user_info, sizeof(Users), 0) < 0) {
         std::cout << "recv error" << std::endl;
         exit(-1);
       }
       std::cout << my_user_info.user.user_name << std::endl;
       return true;
     }
     else if (flags_mesg.flags == AccountError) {
       std::cout << "     账户或者密码错误" << std::endl;

       return false;
     } else if (flags_mesg.flags == AgainErro){
       std::cout << "     账户已经登录" << std::endl;
       return false;
     }
   }
   else if (n == 2) {
     return register_account(sockfd);
   }
   else {
     RegisterSigin flags_mesg;
     flags_mesg.flags = Quit;
     char buf[KBufSize];
     memset(buf, 0, KBufSize);
     memcpy(buf, &flags_mesg, sizeof(flags_mesg));
     if (send(sockfd, buf, KBufSize, 0) < 0) {
       std::cout << "recv error" << std::endl;
       exit(-1);
     }
     exit(0);
   }
   return false;
 }

static bool register_account(Socket sockfd)
{
  std::string account, password;
  printf("    账号: ");
  std::cin >> account;

  printf("    密码: ");
  std::cin >> password;
  if (account.size() > KBufSize || account.size() < 5 ||
      password.size() > KBufSize || password.size() < 5) {
    return false;
  }
  char buf[KBufSize];
  RegisterSigin mesg;
  memset(buf, 0, KBufSize);
  strcpy(mesg.user_name, account.c_str());
  strcpy(mesg.password, password.c_str());
  mesg.flags = Register;
  memcpy(buf, &mesg, sizeof(mesg));
  if (send(sockfd, buf, KBufSize, 0) < 0) {
    std::cout << "send error" << std::endl;
    exit(-1);
  }
  Flags flags_mesg;
  memset(buf, 0, KBufSize);
  if (recv(sockfd, buf, KBufSize, 0) < 0) {
    std::cout << "recv error" << std::endl;
    exit(-1);
  }
  memcpy(&flags_mesg, buf, sizeof(flags_mesg));
  if (flags_mesg.flags == Succees) {
    memset(buf, 0, KBufSize);
    if (recv(sockfd, buf, KBufSize, 0) < 0) {
      std::cout << "recv error" << std::endl;
      exit(-1);
    }
    memcpy(&my_user_info, buf, sizeof(my_user_info));
    std::cout << my_user_info.user.user_name << std::endl;
    return true;
  }
  else if (flags_mesg.flags == AccountError) {
    std::cout << "账户或者密码格式不对" << std::endl;
    return false;
  }


  return true;
}

 static void show_main_menu()
 {
   system("clear");
   printf("              \033[31m|<-     查看好友( 'sf' or 'see friend' )     ->| \n");
   printf("\n");
   printf("              |<-     添加好友( 'af' or 'add friend' )     ->| \n");
   printf("\n");
   printf("              |<-     删除好友( 'rf' or 'remove friend' )  ->| \n");
   printf("\n");
   printf("              |<-     私人聊天( 'pc' or 'private chat' )   ->| \n");
   printf("\n");
   printf("              |<-     群组聊天( 'gc' or 'group chat' )     ->| \n");
   printf("\n");
   printf("              |<-     退出登录( 'q' or 'quit')             ->| \n\033[0m");
   printf("\n");
 }

 static void show_all_friend()
 {
   printf("sf");
 }

 static bool add_friend(const std::vector<std::string>& args)
 {
   printf("af");
   return true;
 }

 static bool remove_friend(const std::vector<std::string>& args)
 {
   return true;
 }

 static bool private_chat(std::string)
 {
   return true;
 }
 static bool public_chat(std::string)
 {
   return true;
 }
 static void help()
 {

 }

 int splict(const std::string& s, size_t pos, char c, std::string& result)
 {
   auto pos_first = s.find_first_not_of(c, pos);
   if (pos_first == std::string::npos)
     return 0;
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
