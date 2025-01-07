#include <unistd.h>  
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h> 
#include <sys/sem.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h>   


#define SERV_PORT       8080
#define SERV_HOST_ADDR "192.168.1.2"
#define BUF_SIZE        100
#define BACKLOG         5                 

int ID_GLOBAL;

//key generator
key_t keyGen(){
    key_t key;
    key=ftok ("/bin/ls", 33);
    if (key == (key_t)-1){
		printf("Error key...\n");
		exit(0);
	}
    return key;
}
//Semaphore initialization
void initSem(int semId,int semNum,int val) {
    union semnum {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    }argument;
    argument.val=val;
    semctl(semId,semNum,SETVAL,argument);
}
//Semaphore open
void down(int semId,int i) {
    struct sembuf buf;
    buf.sem_num = i;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    semop(semId, &buf, 1);
}
//Semaphore Close
void up(int semId,int i) {
    struct sembuf buf;
    buf.sem_num = i;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    semop(semId, &buf, 1);
}

enum state {LISTO,EJECUCION};

//Node Structure
struct nodo {
    char nombre[10];
    int ID;
    int tiempoEjecucion;
    int tiempoEspera;
    int cuantum;
    enum state estado;
    struct nodo *next;
};
//List structure
struct Lista{
    struct nodo* last;
};
//Empty list initialization
struct Lista inicializarLista(struct Lista L){
    L.last=NULL;
    return L;
}
//Method to push at the end of the list
struct Lista insertarFinal(char nom[],int ID, int tiempoEj, int tiempoEs,struct Lista L){
    struct nodo* temp;
    int i;
    temp = (struct nodo*)malloc(sizeof(struct nodo));
    strcpy(temp->nombre,nom);
    temp->ID=ID;
    temp->tiempoEjecucion=tiempoEj;
    temp->tiempoEspera=tiempoEs;
    temp->estado=LISTO;
    if (L.last == NULL) {
        temp->next = temp;
        L.last = temp;
    }
    else {
        temp->next = L.last->next;
        L.last->next = temp;
        L.last = temp;
    }
    return L;
}
//Delete element from the list
struct Lista eliminarNodo(int ID,struct Lista L){
    if (L.last == NULL)
        printf("\nLista vacia\n");
    else{
        if(ID==L.last->ID&&L.last==L.last->next){
            L.last=NULL;
            return L;
        }
        struct nodo* temp;
        temp = (struct nodo*)malloc(sizeof(struct nodo));
        struct nodo* temp1;
        temp1 = (struct nodo*)malloc(sizeof(struct nodo));
        temp = L.last->next;
        if(ID==L.last->next->ID)L.last->next=L.last->next->next;
        else{
            while (temp->next->ID!=ID)temp=temp->next;  
            if(temp->next==L.last){
                temp->next=L.last->next;
                L.last=temp;
            }else{
                temp1=temp->next;
                temp->next=temp1->next;
            }  
        }       
    }
    return L;
}
//Shows the content of the list
void verLista(struct Lista L){
    if (L.last == NULL)
        printf("\nLista Vacia\n");
    else {
        struct nodo* temp;
        temp = (struct nodo*)malloc(sizeof(struct nodo));
        temp = L.last->next;
        do {
            printf("Nombre: %s | ID: %d | Tiempo de Ejecucion:%d | Tiempo de Espera: %d\n", temp->nombre,temp->ID,temp->tiempoEjecucion,temp->tiempoEspera);
            temp = temp->next;
        } while (temp != L.last->next);
    }
}
//Returns a ramdom number in the specified renge
int aleatorio(int n, int m) {
	return (rand()%(m-n+1))+n;
}
//Cuantum Generator
int cuantum(struct nodo* temp){
    int cuantum;
    if(temp->tiempoEjecucion==0)return 0;
    do{
        cuantum=aleatorio(1,3);
    }while(temp->tiempoEjecucion<cuantum);
    return cuantum;
}
//Returns the waiting time
int tiempoEspera(struct nodo* temp,struct Lista L){
    int Te=0,tiempo=0;
    struct nodo* temp1;
    temp1 = (struct nodo*)malloc(sizeof(struct nodo));
    temp1 = L.last->next;
    do {
            if(temp1->tiempoEjecucion<(temp->tiempoEjecucion-temp->cuantum)&&temp!=temp1){
                Te=Te+temp1->tiempoEjecucion;
            }
            temp1 = temp1->next;
        } while (temp1 != L.last->next);
    if(Te==0)return temp->tiempoEjecucion;
    else return Te-(temp->tiempoEjecucion);
}
//Returns the node with the smaller time
struct nodo* menor(struct Lista L){
    struct nodo* temp;
    struct nodo* aux;
    temp = (struct nodo*)malloc(sizeof(struct nodo));
    aux = (struct nodo*)malloc(sizeof(struct nodo));
    aux=L.last->next;
    temp=aux;
    do{
        if(aux->tiempoEjecucion<temp->tiempoEjecucion)temp=aux;
        aux=aux->next;
    }while(aux!=L.last->next);
    return temp;
}
//Recives the list and saves process in the list
struct Lista processManager(char buf[],struct Lista L){
    char *token;
    char *token1;
    char *name;
    char *time;
    token=strtok(buf,"|");
    while(token!=NULL){
        name=token;
        token=strtok(NULL,"|");
        time=token;
        token=strtok(NULL,"|");
        L=insertarFinal(name,ID_GLOBAL,atoi(time),0,L);
        ID_GLOBAL++;
        //printf("Nombre: %s Tiempo: %s\n",name,time);
    }
    printf("\n");
    return L;
}
//Dispatcher simulation
int dispatcher(struct Lista L){
    char p[10];
    int flag=0;
    int semId;
    key_t semKey=keyGen();
    int shmId;
    struct nodo* temp;
    struct nodo* aux;
    temp = (struct nodo*)malloc(sizeof(struct nodo));
    aux = (struct nodo*)malloc(sizeof(struct nodo));
    int sockfd,connfd;
    unsigned int len;
    struct sockaddr_in servaddr,client; 
    int  len_rx, len_tx = 0;
    char buff_tx[BUF_SIZE] = "[Mensaje Servidor]:Lista aceptada\n";
    char buff_rx[BUF_SIZE];
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    /*----------------Creacion Memoria compartida-----------------*/
    shmId=shmget(1111,sizeof(temp),IPC_CREAT|0666);
    temp=shmat(shmId,NULL,0);
    /*----------------Fin Memoria compartida-----------------*/
    /*----------------Creacion Semaforo-----------------*/
    semId=semget(semKey,1,IPC_CREAT|0777);
    if(errno == EEXIST || semId == -1) {
        perror("semget");
        exit(1);
    }
    initSem(semId,0,1);
    /*----------------Fin Semaforo-----------------*/
    switch(fork()){
        case -1:
            printf("Error fork()");
            exit(-1);
            break;
        case 0://Despachador a corto plazo
            while(1){
                down(semId,0);
                if(temp->tiempoEjecucion!=0){
                    printf("\nProceso aceptado para ejecucion:\n");
                    printf("Nombre: %s | ID: %d | Tiempo de Ejecucion:%d | Tiempo de Espera:%d | Cuantum:%d\n", temp->nombre,temp->ID,temp->tiempoEjecucion,temp->tiempoEspera,temp->cuantum);
                    sleep(temp->cuantum);
                    temp->tiempoEjecucion-=temp->cuantum;
                    printf("Ejecucion de proceso terminada...\n");
                    if(temp->tiempoEjecucion==0)printf("\n[[[Proceso eliminado]]]\n");
                }
                up(semId,0);
            }
            break;
        default://Despachador a largo plazo
            if (sockfd == -1) { 
                fprintf(stderr, "[SERVER-error]: Fallo en la creacion del socket... %d: %s \n", errno, strerror( errno ));
                return -1;
            } 
            else printf("[SERVER]: Socket creado exitosamente...\n"); 
            memset(&servaddr, 0, sizeof(servaddr));

            servaddr.sin_family      = AF_INET; 
            servaddr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR); 
            servaddr.sin_port        = htons(SERV_PORT); 
    
            if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
                fprintf(stderr, "[SERVER-error]: Error bind. %d: %s \n", errno, strerror( errno ));
                return -1;
            } 
            if ((listen(sockfd, BACKLOG)) != 0) { 
                fprintf(stderr, "[SERVER-error]: Error listen. %d: %s \n", errno, strerror( errno ));
                return -1;
            } 
            else printf("[SERVER]: Ecuchando en el Puerto %d \n\n", ntohs(servaddr.sin_port) ); 
            len = sizeof(client); 
            while(1){
                connfd = accept(sockfd, (struct sockaddr *)&client, &len); 
                if (connfd < 0) { 
                    fprintf(stderr, "[SERVER-error]: Conexion rechazada. %d: %s \n", errno, strerror( errno ));
                    return -1;
                } 
                else{              
                    while(1){  
                        len_rx = read(connfd, buff_rx, sizeof(buff_rx)); 
                        if(len_rx == -1)fprintf(stderr, "[SERVER-error]: connfd no se puede leer. %d: %s \n", errno, strerror( errno ));
                        else if(len_rx == 0){
                            printf("[SERVER]: Socket cliente cerrado...\n");
                            close(connfd);
                            break; 
                        }
                        else{
                            if(flag==1){
                                strcpy(buff_tx,"Proceso terminado: ");
                                strcat(buff_tx,p);
                            } else{
                                strcpy(buff_tx,"Proceso ejecutado: ");
                                strcat(buff_tx,p);
                            }
                            write(connfd, buff_tx, strlen(buff_tx));
                            printf("\n[SERVER]: Lista de procesos recibida... \n\n");
                            down(semId,0);
                            L=processManager(buff_rx,L);
                            aux=menor(L);
                            aux->cuantum=cuantum(aux);
                            aux->tiempoEspera=tiempoEspera(aux,L);

                            strcpy(p,aux->nombre);
                            strcpy(temp->nombre,aux->nombre);
                            temp->ID=aux->ID;
                            temp->tiempoEjecucion=aux->tiempoEjecucion;
                            temp->cuantum=aux->cuantum;
                            temp->tiempoEspera=aux->tiempoEspera;
                            temp->estado=EJECUCION;
                                if(aux->tiempoEjecucion-aux->cuantum==0){
                                    L=eliminarNodo(aux->ID,L);
                                    flag=1;
                                }
                                else{
                                    aux->tiempoEjecucion=aux->tiempoEjecucion-aux->cuantum;
                                    aux->estado=LISTO;
                                    flag=0;
                                }
                            up(semId,0);
                        }            
                    }  
                }                      
            }
        break;
        
    }
}

int main(int argc, char* argv[]){ 
    struct Lista L;
    L=inicializarLista(L);
    dispatcher(L);
} 