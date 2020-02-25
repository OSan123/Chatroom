#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>//网络通信头文件
#include<arpa/inet.h> //端口，ip转换头文件
#include<netinet/in.h>//socket结构体头文件
#include<sys/ipc.h>   //进程内存共享头文件
#include<sys/shm.h>   //进程内存共享头文件
#include<string.h>
#include<errno.h>
#include<time.h>
#include<unistd.h>    //进程相关头文件
#include<signal.h>

#define PERM S_IRUSR|S_IWUSR
#define MYPORT 3490
#define BACKLOG 10
#define WELCOME "|----------Welcome to the chat room!----------|"
//创建共享内存获得标识符
int shm_create()
{
	int shmid;
	if((shmid = shmget(IPC_PRIVATE, 1024, PERM)) == -1)
	{
		fprintf(stderr, "Create Share Memory Error:%s\n\a", strerror(errno));
		//stderr流直接输出到屏幕，strerror函数获取错误字符串的指针
		exit(1);
	}
	return shmid;
}

int bindPort(unsigned short int port)
{
	int sockfd;
	struct sockaddr_in my_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&my_addr,sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;//服务器多网卡同时监听
	if(bind(sockfd, (struct sockadrr*)&my_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("fail to bind");
		exit(1);
	}
	printf("bind success!\n");
	return sockfd;
}
void get_cur_time(char *time_str)
{
	struct timeval now;
	gettimeofday(&now,NULL);
	strcpy(time_str,ctime(&now.tv_sec));//C标准库time.h函数
}
int main()
{
	int sockfd,clientfd;
	int sin_size, recvbytes;
	pid_t pid, ppid;
	char *time_str, *buf, *read_addr, *write_addr, *temp;
	struct sockaddr_in their_addr;

	int shmid;
	shmid = shm_create();

	time_str = (char*)malloc(50);
	temp = (char*)malloc(255);

	sockfd = bindPort(MYPORT);//链接在sockfd文件描述符进行
	get_cur_time(time_str);
	printf("Time is : %s\n", time_str);
	if(listen(sockfd, BACKLOG) == -1)
	{
		perror("fail to listen");
		exit(1);
	}
	printf("listen...\n");
	while(1)
	{
		if((clientfd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size)) == -1)
		//通信在clientfd文件描述符进行
		//accept得到得到客户端地址信息，包括ip和端口号；返回地址信息大小
		{
			perror("fail to accept");
			exit(1);
		}
		char address[20];
		inet_ntop(AF_INET, &their_addr.sin_addr, address, sizeof(address));//ip转化为字符串型

		printf("accept from %s\n", address);
		send(clientfd, WELCOME, strlen(WELCOME), 0);
		buf = (char*)malloc(255);

		ppid = fork();
		if(ppid == 0)//子进程
		{
			pid = fork();//创建子进程
			while(1)
			{
				if(pid > 0)//父进程从共享内存中读出数据发送给客户端显示
				{
					memset(buf, 0, 255);
					//printf("OK\n");
					if((recvbytes = recv(clientfd, buf, 255, 0)) <= 0)
					{
						perror("fail to recv");
						close(clientfd);
						raise(SIGKILL);
						exit(1);
					}
					write_addr = shmat(shmid, 0, 0);
					memset(write_addr, '\0', 1024);

					strncpy(write_addr, buf, 1024);//将消息写入共享内存
					get_cur_time(time_str);
					strcat(buf, time_str);
					printf("%s\n", buf);
				}
				else if(pid == 0) //子进程接收客户端的消息写入共享内存
				{
					sleep(1);
					read_addr = shmat(shmid, 0, 0);

					if(strcmp(temp, read_addr) != 0)
					{
						strcpy(temp, read_addr);
						get_cur_time(time_str);
						strcat(read_addr, time_str);
						if(send(clientfd, read_addr, strlen(read_addr), 0) == -1)
						{
							perror("fail to send");
							exit(1);
						}
						memset(read_addr, '\0', 1024);
						strcpy(read_addr, temp);
					}
				}
				else
					perror("fail to work");
			}
		}
	}
	printf("-----------------------------\n");
	free(buf);
	close(sockfd);
	close(clientfd);
	return 0;
}
