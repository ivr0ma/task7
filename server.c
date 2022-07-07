#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <netinet/ip.h>

#define ADDRESS "mysocket" /* адрес для связи */
#define STRMAX 20          /* максимальная длина сообщения */
int fd[2];    
int fnum[2];
int number = 1;    /* число на которое сервер будет увеличивать другие числа */
int clients = 0;   /* кол-во клиентов */
int temp_sock;     /* сокет-дескриптор */

/*==================================================
 * обработчик сигнала для отца, 
 * изменение глобального параметра client
 *==================================================
*/
void SigClient(int n) {
	/*printf("make --\n");*/
	clients--;
}

/*==================================================
 * обработчик сигнала для сына, 
 * изменение глобального параметра number
 *==================================================
*/
void SigSon(int n) {
	char buf[STRMAX];
	
	read(fd[0], buf, STRMAX);
	sscanf(buf, "%d", &number);
}

/*==================================================
 * обработчик сигнала 2 для сына, 
 * отправляем сообщение клиенту, написавшему
 * первое число
 *==================================================
*/
void SigSon2(int n) {
	char buf[STRMAX];
	
	read(fnum[0], buf, STRMAX);
	send(temp_sock, buf, STRMAX, 0);
	write(fnum[1], " ", STRMAX);
}

/*==================================================
 * обработчик сигнала для отца, записываем в канал
 * измененный глобальный параметр number
 *==================================================
*/
void SigDad(int n) {
	int i;
	char buf[STRMAX];
	
	read(fd[0], buf, STRMAX);
	
	for (i=1; i <= clients+1; i++) {     /* всем клиентам и СЕБЕ! */
		write(fd[1], buf, STRMAX);
	}
	
	kill(0, SIGUSR1);
}

int main (int argc, char ** argv) {
	char str[STRMAX];
	int i, main_sock, len, temp_len, port;
	int temp1 = 0, temp2 = 0;  /* 1-ое и 2-ое числа клиентов */
	int pid;                   /* pid процесса, принявшего 1-ое число */
	char str2[STRMAX];         
	struct sockaddr_in main_adr, temp_adr;
	
	pipe(fd);      /* канал для передачи глобального параметра number */
	pipe(fnum);    /* канал для передачи  pid процесса и первого числа*/
	
	printf("server begin\n");
	
	/* получаем свой сокет-дескриптор: */
	if ( (main_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
		perror("client: socket");
		exit(1);
	}
	
	/* создаем адрес, c которым будут связываться клиенты */
	//main_adr.sun_family = AF_UNIX;
	//strcpy(main_adr.sun_path, ADDRESS);
	
	sscanf(argv[1], "%d", &port);
     
    	main_adr.sin_family = AF_INET;
    	main_adr.sin_port = htons(port);
    	main_adr.sin_addr.s_addr = INADDR_ANY;
	
	/* связываем адрес с сокетом; уничтожаем файл с именем ADDRESS, 
	 * если он существует, для того, чтобы вызов bind завершился успешно
	*/
	unlink(ADDRESS);
	len = sizeof(main_adr);
	if ( bind(main_sock, (struct sockaddr *) &main_adr, len) < 0 ) {
		perror("server: bind"); 
		exit(1);
	}
	
	/* слушаем запросы на сокет */
	if ( listen(main_sock, 5) < 0 ) {
		perror("server: listen"); 
		exit(1); 
	}

	write(fnum[1], " ", STRMAX);
	
	if (pid = fork()) {
		signal(SIGUSR2, SIG_IGN);
		signal(SIGUSR1, SIG_IGN);
		signal(SIGTERM, SIG_DFL);
		
		close(fd[0]);
		close(fd[1]);
		close(fnum[0]);
		close(fnum[1]);
		
		for (;;) {
			scanf("%s", str);
			/*printf("    s=%s\n", str);*/
			if (strcmp(str, "\\e") == 0) {
				kill(0, SIGINT);
				break;
			}
			
		}
		exit(0);
	}
	
	for (;;) {
		/* связываемся с клиентом через неименованный сокет с дескриптором d1: */
		temp_len = sizeof temp_adr; 
		if ( (temp_sock = accept(main_sock, (struct sockaddr *) &temp_adr, &temp_len)) < 0 ) {
			perror("server: accept");
			exit(1);
		}
		
		if ( fork() == 0) {
			signal(SIGUSR1, SigSon);
			signal(SIGUSR2, SigSon2);
			
			for (;;) {
				recv(temp_sock, str, STRMAX, 0);
				if ( str[0] != '\\' && (str[0] < '0' || str[0] > '9') ) {
					printf("incorrect format\n");
					break;
				}
				
				if (strcmp(str, "\\+") == 0) {
					recv(temp_sock, str, STRMAX, 0);
					write(fd[1], str, STRMAX);
					kill(getppid(), SIGUSR2);
					send(temp_sock, "ok", STRMAX, 0);
				}
				else if (strcmp(str, "\\?") == 0) {
					sprintf(str, "%d", number);
					send(temp_sock, str, STRMAX, 0);
				}
				else if (strcmp(str, "\\-") == 0) {
					kill(getppid(), SIGTERM);
					break;
				}
				else {
					sscanf(str, "%d", &temp2);
					
					read(fnum[0], str2, STRMAX);
					if (strcmp(str2, " ") == 0) {
						temp1 = temp2;
						sprintf(str2, "%d", getpid());
						write(fnum[1], str2, STRMAX);
						write(fnum[1], str, STRMAX);
					}
					else {
						sscanf(str2, "%d", &pid);
						read(fnum[0], str2, STRMAX);
						sscanf(str2, "%d", &temp1);
						temp2 += temp1 + number;
						sprintf(str, "%d", temp2);
						write(fnum[1], str, STRMAX);
						
						if ( send(temp_sock, str, STRMAX, 0) == -1 ) {
							printf("can't send a message\n");
							break;
						}
						
						if (pid != getpid()) {
							kill(pid, SIGUSR2);
						}
						else {
							read(fnum[0], str, STRMAX);
							write(fnum[1], " ", STRMAX);
						}
					}
					
				}
			}
			
			/* закрывает сокет и разрывает все соединения с этим сокетом */
			close(temp_sock);
			close(fd[0]);
			close(fd[1]);
			close(fnum[0]);
			close(fnum[1]);
			exit(0);
		}

		else {
			clients++;
			signal(SIGUSR2, SigDad);
			signal(SIGUSR1, SigSon);
			signal(SIGTERM, SigClient);
			/* закрывает сокет и разрывает все соединения с этим сокетом */
			close(temp_sock);
			continue;
		}
		
		// --------------------------

	}
	
	close(fd[0]);
	close(fd[1]);
	close(fnum[0]);
	close(fnum[1]);
	exit(0);
}
