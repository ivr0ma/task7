#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/ip.h>
#include <signal.h>

#define ADDRESS "mysocket"  /* адрес для связи */
#define STRMAX 20           /* максимальная длина сообщения */

int main (int argc, char ** argv) {
	char str[STRMAX]="";  
	int i, main_sock, len, port;
	struct sockaddr_in main_adr;
	
	printf("client begin\n");
	
	/* получаем сокет-дескриптор: */
	if ( (main_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("client: socket");
		exit(1);
	}
	
	/* создаем адрес, по которому будем связываться с сервером: */
	//main_adr.sun_family = AF_UNIX;
	//strcpy(main_adr.sun_path, ADDRESS);
	
	/* port = atoi(argv[1]); */
	sscanf(argv[1], "%d", &port);
     
    	main_adr.sin_family = AF_INET;
    	main_adr.sin_port = htons(port);
    	main_adr.sin_addr.s_addr = INADDR_ANY;
	
	/* пытаемся связаться с сервером (с адресом ADDRESS): */
	len = sizeof(main_adr);
	if ( connect(main_sock, (struct sockaddr *) &main_adr, len) < 0 ) { 
		perror("client: connect");
		exit(1);
	}
	
	if (fork() == 0) {
		for (;;) {
			recv(main_sock, str, STRMAX, 0); 
			printf("%s\n", str);
		}
	}
	
	for (;;) {
		printf("command: ");
		scanf("%s", str);
		
		if ( str[0] != '\\' && (str[0] < '0' || str[0] > '9') ) {
			printf("incorrect format\n");
			continue;
		}
		
		send(main_sock, str, STRMAX, 0); 
		
		if (strcmp(str, "\\+") == 0) {
			scanf("%s", str);
			send(main_sock, str, STRMAX, 0); 
		}
		else if (strcmp(str, "\\-") == 0) {
			kill(0, SIGKILL);
			break;
		}
		
		sleep(0.1);
		//recv(main_sock, str, STRMAX, 0);
		//printf("%s\n", str);
	}
	
	/* закрывает сокет и разрывает все соединения с этим сокетом */
	close(main_sock);
	printf("client stoped\n");
	exit(0);
}
