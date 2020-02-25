#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>  //网络通信头文件
#include<arpa/inet.h>   //端口，ip转换头文件
#include<netinet/in.h>  //套接字结构头文件
#include<unistd.h>      //进程头文件
#include<netdb.h>       //域名，主机获取ip头文件
#include<string.h>
#include<signal.h>
int main(int argc, char *argv[])
{
	struct sockaddr_in clientaddr;
	pid_t pid;
	int clientfd, sendbytes, recvbytes;
	struct hostent *host;
	char *buf, *buf_read;
	if(argc < 4)
	{
		printf("wrong usage");
		printf("%s host port name\n", argv[0]);
		exit(1);
	}
	host = gethostbyname(argv[1]);
	if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("fail to create socket");
		exit(1);
	}
	bzero(&clientaddr, sizeof(clientaddr));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons((uint16_t)atoi(argv[2]));
	clientaddr.sin_addr = *((struct in_addr*)host->h_addr);
	if(connect(clientfd, (struct sockaddr*)&clientaddr, sizeof(struct sockaddr)) == -1)
	{
		perror("fail to connect");
		exit(1);
	}
	buf = (char*)malloc(120);
	memset(buf, 0, 120);
	buf_read = (char*)malloc(100);
	if(recv(clientfd, buf, 100, 0) == -1)
	{
		perror("fail to recv");
		exit(1);
	}
	printf("\n%s\n", buf);
	pid = fork();
	while(1)
	{
		if(pid > 0)  //父进程负责发送消息
		{
			strcpy(buf, argv[3]);
			strcat(buf, ":");
			memset(buf_read, 0, 100);
			fgets(buf_read, 100, stdin);  //从stdin流中读取一行数据存入buf_read
			strncat(buf, buf_read, strlen(buf_read) - 1);
			if((sendbytes = send(clientfd, buf, strlen(buf), 0)) == -1)
			{
				perror("fail to send");
				exit(1);
			}
		}
		else if(pid == 0) //子进程负责接收消息
		{
			memset(buf, 0, 100);
			if(recv(clientfd, buf, 100, 0) <= 0)
			{
				perror("fail to recv");
				close(clientfd);
				raise(SIGSTOP);
				exit(1);
			}
			printf("%s\n", buf);
		}
		else
			perror("fork error");
	}
	close(clientfd);
	return 0;
}
