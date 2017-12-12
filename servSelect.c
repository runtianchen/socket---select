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
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#define IP_ADDR "0.0.0.0"
#define PORT_NUM 55555
#define BUF_SIZE 1024
#define QUIT "$quit$"
#define BACKLOG 5
#define TRUE 1
#define FALSE 0
#define SELECTLENGTH 100

int cli_sockfd[SELECTLENGTH] = {0,1,2,0};//用一个数组 储存 多个客户端socket文件描述符
int cli_sockfd_length = 0;//记录cli_sockfd[]有效位长度
int i = 0;
			
fd_set read_fds;
char buf_recv[BUF_SIZE];//读缓冲区
char buf_send[BUF_SIZE];//写缓冲区	

//读线程
void* thread_read()
{
	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 0;
	
	fd_set read_fdss;
	FD_ZERO(&read_fdss);

	while(TRUE)
	{
		read_fdss = read_fds;
		if(select(cli_sockfd_length, &read_fdss, NULL, NULL, &t) < 0)
		{
			printf("selection error: %s (errno:%d)\n", strerror(errno), errno);
			break;
		}
		for(i=3;i < cli_sockfd_length;i++)
		{
			if(FD_ISSET(i, &read_fdss))
			{
				memset(buf_recv,'\0',sizeof(buf_recv));
				ssize_t  _size = read(cli_sockfd[i],buf_recv,sizeof(buf_recv) - 1);
				if(_size < 0)
				{
					perror("read");
					cli_sockfd[i] = -1;
					FD_CLR(i,&read_fds);
					close(cli_sockfd[i]);
					break;
				}
				else if(_size == 0)
				{
					printf("client[%d] release!\n",i);
					cli_sockfd[i] = -1;
					FD_CLR(i,&read_fds);
					close(cli_sockfd[i]);
					break;
				}
				else
				{
					buf_recv[_size] = '\0';
					printf("client[%d]:-> %s",cli_sockfd[i],buf_recv);
					//write(cli_sockfd[i],buf_recv,strlen(buf_recv));
				}
			}
		}
	}
}

/**
 *发送消息格式化函数
 *格式化成功 return 对应的文件描述符的值 格式化失败 return 0;
 */
int sendmsgformat(char* str)
{
	int index = 0;
	int length = strlen(str);
	char c = '\0';
	char str0[BUF_SIZE] = {'\0'};
	strcpy(str0, str);
	char str1[BUF_SIZE] = {'\0'};
	str1[0] = '0';
	char str2[BUF_SIZE] = {'\0'};
	i = 0;
	while(str0[9+i] >= '0' && str0[9+i] <= '9')
	{
		str1[i]=str0[9+i];
		i++;
	}
	index = atoi(str1);
	c = str0[9+i];
	if(length > 9)
	{
		strncpy(str2, str0+10+i, length-9-i);
	}		
	str0[9] = '\0';
	if((index == 0) || (strcmp(str0,"sendmsgto") != 0) || (c != '>')) 
	{
		return -1;
	}
	strcpy(str,str2);
	return index;
}

void* thread_write()
{
	int index = 0;
	while(TRUE)
	{
		memset(buf_send,'\0',sizeof(buf_send));
		ssize_t ret = read(0,buf_send,sizeof(buf_send) - 1);
		if(ret < 0)
		{
			perror("thread_write() error");
			continue;
		}
		//调用sendmsgformat函数将buf_send格式化
		if((index = sendmsgformat(buf_send)) < 0)
		{
			printf("The sending format is incorrect.\ntry sendmsgto + num.(above 0) + > + msg\n");
			continue;
		}
		if(cli_sockfd[index] == index)
		{
			if(write(cli_sockfd[index], buf_send, strlen(buf_send)) < 0)
			{
				perror("thread_write()1 erroe");
				continue;
			}
		}
		else
		{
			printf("The specified client was not found. please respecify\n");
		}
	}
}

void main()
{	
	int server_sockfd, client_sockfd;//定义 服务器端Socket文件描述符 以及 客户端Socket文件描述符
	int flag = TRUE;

	const char* ip = IP_ADDR;//定义一个 指向 ip存放地址 的指针	
	char remote[INET_ADDRSTRLEN];

	struct sockaddr_in serverSockAddr;//使用Ipv4的 服务器 socket地址
	struct sockaddr_in clientSockAddr;//使用Ipv4的 客户端 socket地址
	socklen_t client_addrlen = sizeof(clientSockAddr);

	pthread_t thread[2]={0};

	FD_ZERO(&read_fds);
	
      /*
	socket()
	返回一个服务器端的socket文件描述符
      */
	if((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)//socket()
	{
		printf("socket error: %s (errno:%d)\n", strerror(errno), errno);
		exit(1);
	}

      /*
	bind()
	将socket 与ip、端口绑定
      */	
	bzero(&serverSockAddr, sizeof(serverSockAddr));//初始化serverSockAddr内存区
	serverSockAddr.sin_family = AF_INET;//该socket使用Ipv4协议
	serverSockAddr.sin_port = htons(PORT_NUM);//设置socket端口号
	inet_pton(AF_INET, ip, &serverSockAddr.sin_addr);//设置socket ip
	if(bind(server_sockfd, (struct sockaddr*)&serverSockAddr, sizeof(serverSockAddr)) < 0)
	{
		printf("bind error: %s (errno:%d)\n", strerror(errno), errno);
		exit(1);	
	}

      /*
	listen()
	定义监听队列长度
      */	
	if(listen(server_sockfd, BACKLOG) < 0)
	{
		printf("listen error: %s (errno:%d)\n", strerror(errno), errno);	
		exit(1);	
	}
	printf("The server is created successfully. port : %d\nwaiting for client connection.\n", PORT_NUM);

      /*
	pthread_create()
	启动一个线程用来接收客户端发来的msg
      */
	if(pthread_create(&thread[0], NULL, (void *)thread_read, NULL) < 0)
	{
		printf("thread0 error: %s (errno:%d)\n", strerror(errno), errno);
		exit(1);
	}

      /*
	pthread_create()
	启动一个线程用来接收来自服务器输入终端的msg
      */
	if(pthread_create(&thread[1], NULL, (void *)thread_write, NULL) < 0)
	{
		printf("thread1 error: %s (errno:%d)\n", strerror(errno), errno);
		exit(1);
	}

      /*
	accept()
	接收
      */
	while(TRUE)
	{
		bzero(&clientSockAddr, sizeof(clientSockAddr));//初始化clientSockAddr内存区	
		if((client_sockfd = accept(server_sockfd, (struct sockaddr*)&clientSockAddr, &client_addrlen)) < 0)//返回一个链接到该服务器的客户端的socket文件描述符
		{
			printf("error: %s (errno:%d)\n", strerror(errno), errno);
			exit(1);
		}
		printf("A client has connected. num. : %d ip : %s port: %d\n", client_sockfd, inet_ntop(AF_INET, &clientSockAddr, remote, INET_ADDRSTRLEN), ntohs(clientSockAddr.sin_port));
		printf("input sendmsgto%d>'msg' sends the message to the client[%d].\n", client_sockfd, client_sockfd);
		cli_sockfd[client_sockfd] = client_sockfd;//记录该socket
		cli_sockfd_length = client_sockfd + 1;//记录cli_sockfd[]有效位长度
		FD_SET(client_sockfd, &read_fds);//将该客户端socket文件描述符注册给 select_read
	}
      /*
	close()
	关闭服务器端口
      */
	close(server_sockfd);
	
      /*
	等待线程结束
      */
	pthread_join(thread[0],NULL);
	pthread_join(thread[1],NULL);
}
