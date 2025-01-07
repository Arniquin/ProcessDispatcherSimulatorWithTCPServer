#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <unistd.h>

#define SERVER_ADDRESS  "192.168.1.2"
#define PORT            8080 

char buf[100];    
char buf_rx[100];
 
// Returns a random number in the specified range
int aleatorio(int n, int m) {
	return (rand()%(m-n+1))+n;
}
//Genera un proceso
void generaProceso(char proc[]){
	char tiempo[3];
	sprintf(proc,"P%d",aleatorio(300,500));
	sprintf(tiempo,"%d",aleatorio(3,9));
	strcat(proc,"|");
	strcat(proc,tiempo);
} 

int main(){
    //Process amunt
    int n;
    int sockfd; 
    char proc[6];
    struct sockaddr_in servaddr; 
    while(1){
        sockfd=socket(AF_INET, SOCK_STREAM, 0); 
        if(sockfd==-1){ 
            printf("CLIENTE: Fallo en la creacion del socket...\n"); 
            return -1;  
        } 
        else printf("CLIENTE: Socket creado exitosamente..\n"); 
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET; 
        servaddr.sin_addr.s_addr = inet_addr( SERVER_ADDRESS ); 
        servaddr.sin_port = htons(PORT); 
        //Process generation
        n=aleatorio(1,5);
        for(int i=0;i<n;i++){
			generaProceso(proc);
				if(i==0){
					strcpy(buf,proc);
					if(n!=1)strcat(buf,"|");
				}
				else {
					strcat(buf,proc);
					if(i!=n-1)strcat(buf,"|");
				}
		}
		printf("\n%s\n\n",buf);
        if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){ 
            printf("Fallo de conexion con el servidor...\n");  
        }else {
            printf("Conexion con servidor realizada..\n"); 
            write(sockfd,buf,sizeof(buf));    
            read(sockfd, buf_rx, sizeof(buf_rx)); 
            printf("%s\n",buf_rx);
            close(sockfd); 
        }
        sleep(aleatorio(1,3));
    }
} 