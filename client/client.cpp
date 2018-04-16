/**
 * @Author: YangGuang <sunshine>
 * @Date:   2018-04-10T15:19:20+08:00
 * @Email:  guang334419520@126.com
 * @Filename: client.cpp
 * @Last modified by:   sunshine
 * @Last modified time: 2018-04-16T18:48:44+08:00
 */


 #include <iostream>
 #include <stdlib.h>
 #include <stdio.h>
 #include <time.h>
 #include <fstream>
 #include <pthread.h>
 #include "client.hpp"
 #include "Hash.hpp"




 int main(int argc, char const *argv[]) {
 #if defined(_WIN32)
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
   pthread_mutex_init(&mutex, NULL);
   pthread_cond_init(&cond, NULL);
   pthread_t tid;
   pthread_create(&tid, NULL, receiv_message_thread, (void*)&sockfd);

   show_main_menu();
   std::cin.ignore();
   while (true) {
     std::string str;
     std::string cmd;
     std::cout << "\033[31m     " << my_user_info.user.user_name << ">\033[0m";
     pthread_mutex_lock(&mutex);
     while (!status) {
       pthread_cond_wait(&cond, &mutex);
     }
     pthread_mutex_unlock(&mutex);
     getline(std::cin, str);
     size_t pos = 0;
     if (!(pos = splict(str, pos, ' ', cmd))) {
       //std::cout << "error : 格式错误" << std::endl;
       continue;
     }
     switch (HashFunc(cmd.c_str())) {
       case HashCompile("sf"):
       case HashCompile("seefriend"):
       {
         show_all_friend();
         break;
       }
       case HashCompile("af"):
       case HashCompile("add"):
       {
         std::vector<std::string> args;
         std::string arg;
         while ((pos = splict(str, pos, ' ', arg)))
            args.push_back(arg);
         if (args.empty()) {
           std::cout << "error : 格式错误, 没有参数" << std::endl;
           break;
         }
         add_friend(sockfd, args);
         break;
       }
       case HashCompile("rf"):
       case HashCompile("remove"):
       {
         std::vector<std::string> args;
         std::string arg;
         while ((pos = splict(str, pos, ' ', arg)))
            args.push_back(arg);
         if (args.empty()) {
           std::cout << "error : 格式错误, 没有参数" << std::endl;
           break;
         }
         remove_friend(sockfd, args);
         break;
       }
       case HashCompile("pc"):
       case HashCompile("private"):
       {
         std::string arg;
         if ( !(splict(str, pos, ' ', arg))) {
           std::cout << "error : 格式错误, 没有参数" << std::endl;
           break;
         }
         private_chat(sockfd, arg);
       }
       case HashCompile("gc"):
       case HashCompile("group"):
       {
         std::string arg;
         if ( !(splict(str, pos, ' ', arg))) {
           std::cout << "error : 格式错误, 没有参数" << std::endl;
           break;
         }
         public_chat(sockfd, arg);
       }
       case HashCompile("help"):
       {
         help();
         break;
       }
       case HashCompile("quit"):
       {
         quit();
         break;
       }
       case HashCompile("\n"):
       {
         break;
       }
       default:
       {
         std::cout << " error : 没有这个命令" << std::endl;
       }
     }
   }

 }

 void* receiv_message_thread(void* arg)
 {
   Socket sockfd = *((Socket*)arg);
   char rbuf[KBufSize];
   while (true) {
     memset(rbuf, 0, KBufSize);
     if (recv(sockfd, rbuf, KBufSize, 0) < 0) {
       quit();
     }
     Message mesg;
     memcpy(&mesg, rbuf, sizeof(mesg));
     switch(mesg.flags) {
       case AddFriend:
       {
         std::cout << "\n         您收到一条来自" << mesg.sender.user_name << "的好友申请"
                   << "time : " << mesg.time;
         messages.push_back(mesg);
         break;
       }
       case PrivateChat:
       {
         std::cout << "\n         您收到一条来自" << mesg.sender.user_name << "的消息"
                   << "time : " << mesg.time;
         messages.push_back(mesg);
         break;
       }
       case PublicChat:
       {
         std::cout << "\n         您收到一条来自" << mesg.sender.user_name << "的群消息"
                   << "time : " << mesg.time;
         messages.push_back(mesg);
         break;
       }
       default:
       {
         break;
       }

     }
   }
 }

 static void quit()
 {

 }

 static bool login(Socket sockfd)
 {
   system("clear");
   printf("             <------------ 1. 登 录 ------------>\n");
   printf("             <------------ 2. 注 册 ------------>\n");
   printf("             <------------ 0. 退 出 ------------>\n");
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
  printf("              \033[31m|<-     查看好友( 'vf' or 'viewfriend' )    ->| \n");
  printf("\n");
  printf("              |<-     添加好友( 'af' or 'add' )           ->| \n");
  printf("\n");
  printf("              |<-     删除好友( 'rf' or 'remove' )        ->| \n");
  printf("\n");
  printf("              |<-     私人聊天( 'pc' or 'private' )       ->| \n");
  printf("\n");
  printf("              |<-     群组聊天( 'gc' or 'group' )         ->| \n");
  printf("\n");
  printf("              |<-     查看消息( 'vm' or 'viewmesg' )         ->| \n");
  printf("\n");
  printf("              |<-     退出登录( 'q' or 'quit')            ->| \n\033[0m");
  printf("\n");
}

static void show_all_friend()
{
  printf("sf");
}

static bool add_friend(Socket sockfd, const std::vector<std::string>& args)
{
  char sbuf[KBufSize];
  memset(sbuf, 0, KBufSize);
  Message mesg;
  memset(&mesg, 0, sizeof(mesg));
  mesg.sender = my_user_info.user;
  mesg.flags = AddFriend;

  for (auto arg : args) {
    mesg.receiver = User(arg.c_str());
    std::cout << mesg.receiver.user_name << std::endl;
    char* t = my_time();
    memcpy(mesg.time, t, strlen(t));
    memcpy(sbuf, &mesg, sizeof(mesg));
    if (send(sockfd, sbuf, KBufSize, 0) < 0)
      quit();
  }

  return true;
}

static bool remove_friend(Socket sockfd, const std::vector<std::string>& args)
{
  char sbuf[KBufSize];
  Message mesg;
  mesg.sender = my_user_info.user;
  mesg.flags = AddFriend;

  for (auto arg : args) {
    mesg.receiver = User(arg.c_str());
    char* t = my_time();
    memcpy(mesg.time, t, strlen(t));
    memcpy(sbuf, &mesg, sizeof(mesg));
    if (send(sockfd, sbuf, strlen(sbuf), 0) < 0)
      quit();
  }
  return true;
}

static bool private_chat(Socket sockfd, const std::string& arg)
{
  return true;
}
static bool public_chat(Socket sockfd, const std::string& arg)
{
  return true;
}

static void view_message()
{

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
