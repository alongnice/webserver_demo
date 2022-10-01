#define MAX_EVENT_NUMBER 1000  //监听的最大数量
#define MAX_FD 65535  //最大文件描述符个数
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "locker.h"
#include "threadpool.h"
#include <signal.h>
#include "http_conn.h"

//添加信号捕捉
//用于处理套接字中断后仍然发送数据产生的信号
void addsig(int sig,void(handler)(int)){
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig,&sa,NULL);
}

//添加文件描述符,到epoll中
extern void addfd(int epollfd,int fd,bool one_shot);
//从epoll中删除文件描述符
extern void removefd(int epoll,int fd);
//修改文件描述符
extern void modfd(int epollfd,int fd,int ev);   


int main(int argc,char* argv[]){
    if(argc<=1){
        printf("按照如下格式运行: %s port_number\n",basename(argv[0]));
        return 1;
    }

    //获取端口号
    int port = atoi(argv[1]);

    //对sigpie信号进行处理
    addsig(SIGPIPE,SIG_IGN);

    //创建线程池,初始化线程池
    threadpool< http_conn > *pool = NULL;
    try{
        pool = new threadpool<http_conn>;
    }catch( ... ){
        return 1;   //没有创建好则退出
    }

    //创建一个数组保存全部连接的客户信息(任务信息)
    http_conn * users = new http_conn[ MAX_FD ];  //最大的文件描述符个数
    //网络部分代码,直接监听
    int listenfd = socket(PF_INET,SOCK_STREAM,0);


    //绑定
    struct sockaddr_in address;
    address.sin_family = AF_INET;  //ipv4
    address.sin_addr.s_addr = INADDR_ANY; //任意主机
    address.sin_port = htons(port);

    //设置端口复用
    int reuse = 1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    bind(listenfd,(struct sockaddr*)&address,sizeof(address));
    //进行监听
    listen(listenfd,5);

    //epoll多路复用
    //创建epoll对象,事件数组,添加监听的文件描述符
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    
    //将监听的文件描述符到epoll对象中
    addfd(epollfd,listenfd,false);
    http_conn::m_epollfd = epollfd;
    while(true){
        int num = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);    //检测到了几个事件
        if((num < 0)&&(errno!=EINTR)){
            //说明调用epoll失败
            printf("epoll failure\n");
            break;
        }
        
        //循环遍历事件数组
        for(int i = 0;i<num;i++){
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd){
                //说明有客户端连接进来
                //定义客户端对象
                struct sockaddr_in client_address;
                socklen_t client_addrlen = sizeof(client_address);
                int connfd = accept(listenfd,(struct sockaddr*)&client_address,&client_addrlen);
                if(connfd<0){
                    printf("error is %d\n",errno);
                    continue;
                }

                //判断一下
                if(http_conn::m_user_count >= MAX_FD){
                    //目前的连接数满了,回写数据显示正忙

                    close(connfd);
                    continue;

                }
                //将新的客户的数据初始化,存入数组中
                users[connfd].init(connfd,client_address);
            }else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                //对方异常断开活错误等事件
                //关闭连接
                users[sockfd].close_conn();
            }else if(events[i].events & EPOLLIN){
                if(users[sockfd].read()){
                    //read成员函数一次性将所有数据读完
                    pool->append(users + sockfd);
                }else{
                    users[sockfd].close_conn();
                }
            }else if(events[i].events & EPOLLOUT){
                if(!users[sockfd].write())
                    //write函数一次性写完全部数据
                    users[sockfd].close_conn();

            }

        }
    }
    close(epollfd);
    close(listenfd);
    delete [] users;
    delete pool;
    return 0;
}