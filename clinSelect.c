#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#define IP_ADDR "127.0.0.1"
#define PORT_NUM 55555
#define BUF_SIZE 1024
#define QUIT "$quit$"

main()
{	
	int client_sockfd;//定义 客户端Socket文件描述符
	
	const char* ip = IP_ADDR;//定义一个 指向 ip存放地址 的指针

	struct sockaddr_in serverSockAddr;//服务器端 监听socket 地址

	char remote[INET_ADDRSTRLEN];
	char buf_send[BUF_SIZE];//写缓冲区
	char buf_recv[BUF_SIZE];//读缓冲区
	char str_recv[BUF_SIZE];

	if((client_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)//socket()
	{
		printf("error: %s (errno:%d)\n", strerror(errno), errno);
		exit(1);
	}
	
	bzero(&serverSockAddr, sizeof(serverSockAddr));//初始化serverSockAddr内存区
	serverSockAddr.sin_family = AF_INET;//该socket使用Ipv4协议
	serverSockAddr.sin_port = htons(PORT_NUM);//设置socket端口号
	inet_pton(AF_INET, ip, &serverSockAddr.sin_addr);//设置socket ip	
	if(connect(client_sockfd, (struct sockaddr*)&serverSockAddr, sizeof(serverSockAddr)) < 0)
	{
		printf("error: %s (errno:%d)\n", strerror(errno), errno);
		exit(1);
	}
	printf("connect server successful. input $quit$ to interrupt.\n");
	//创建进程
	pid_t id = fork();
	if(id < 0)
	{
		perror("erron");
	}
	//子进程实现从服务器接收数据并显示到标准输出终端
	else if(id == 0)
	{	
		while(1)
		{
			memset(buf_recv, '\0', sizeof(buf_recv));
			memset(str_recv, '\0', sizeof(str_recv));
			ssize_t  _size = read(client_sockfd,buf_recv,sizeof(buf_recv) - 1);
			if(_size < 0)
			{
				perror("read");
				break;
			}
			else if(_size == 0)
			{
				printf("Server exit.\n");
				break;
			}
			else
			{
				buf_recv[_size] = '\0';
//				printf("server:->");
				strcpy(str_recv,"server:->");
				strcat(str_recv,buf_recv);
				if(write(1,str_recv,sizeof(str_recv) - 1) < 0)
				{
					perror("write");
					break;
				}
			}
		}
	}
	//主进程实现从标准输入终端读取数据并发送给服务器
	else
	{
		while(1)
		{
			memset(buf_send,'\0',sizeof(buf_send));
			ssize_t  _size = read(0,buf_send,sizeof(buf_send) - 1);
			if(_size < 0)
			{
				perror("read");
				break;
			}
			else if((strcmp(buf_send,"$quit$\n")) == 0)
			{
				printf("client exit.\n");
				break;
			}
			else
			{	
				buf_send[_size] = '\0';
				if(write(client_sockfd,buf_send,sizeof(buf_send) - 1) < 0)
				{
					perror("write");
					break;
				}
			}
		}
	kill(id, SIGINT);
	wait(NULL);
	}
	close(client_sockfd);
}
