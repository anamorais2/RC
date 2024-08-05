#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define BUFLEN 512
#define MAX 100 //Máximo de multicast em simultaneo

////Função de Erro/////
void erro(char *s) {
    perror(s);
    exit(1);
}

int portoNoti;
char endServer[100];
char *endMulti[MAX];
int sockets[MAX];
int portas[MAX];
int n_multi = 0;
pthread_t listaThreads[MAX];
int n_thread = 0;

/////Estruturas Auxiliares ás Threads//////
typedef struct{
   char *endereco;
   int porta;
   char *noticia;
}Thread_args;

typedef struct{
   char *endereco;
   int porta;
}Thread_args2;


/////////Enviar a mensagem Multicast////////////////
void *enviarMulticastThread(void *arg){
   Thread_args *args = (Thread_args *) arg;
   
   char *ender = args->endereco;
   int porta = args->porta;
   char *noti = args->noticia;
   int sock_fd;
   struct sockaddr_in addr;
   int nbytes;
   
   //Criar o socket UDP
   if((sock_fd = socket(AF_INET,SOCK_DGRAM,0)) < 0) erro("Erro ao criar multicast");
   
   //Configurar o endereço multicast para envio dos dados
   memset(&addr,0,sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = inet_addr(ender);
   addr.sin_port = htons(porta); 
   
   //Adicionar o socket ao grupo multicast
   struct ip_mreq mreq;
   mreq.imr_multiaddr.s_addr = inet_addr(ender);
   mreq.imr_interface.s_addr = INADDR_ANY;

   if (setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) erro("Erro no setsockopt");
   
   //Enviar mensagem em multicast
   nbytes = sendto(sock_fd,noti,strlen(noti),0,(struct sockaddr *) &addr,sizeof(addr));
   if (nbytes < 0) erro("Erro ao enviar o multicast");
   
   close(sock_fd);  
   
   pthread_exit(NULL); 
}

////Thread do Multicast Enviar/////
pthread_t enviarMulticast(char *ender, char *porta,char *noti) {
   pthread_t tid;
   
   Thread_args *args2 = malloc(sizeof(Thread_args));
   int port = atoi(porta);
   args2->endereco = malloc(strlen(ender) + 1);
   if (args2->endereco == NULL) erro("Erro ao alocar espaço para o endereço\n");
   strcpy(args2->endereco, ender);
   args2->porta = port;
   args2->noticia = malloc(strlen(noti) + 1);
   if (args2->noticia == NULL) erro("Erro ao alocar espaço para o endereço\n");
   strcpy(args2->noticia, noti);
   
   if (pthread_create(&tid, NULL, enviarMulticastThread, (void *) args2) != 0) erro("Erro ao criar a thread multicast\n");
   
   return tid;   

}

/////////Receber a mensagem Multicast////////////////
void *receberMulticastThread(void *arg){
    Thread_args2 *args = (Thread_args2 *) arg;
   
    char *ender = args->endereco;
    int porta = args->porta;
    int sock_mult = -1;
    struct sockaddr_in addr;
    char buffer[BUFLEN];
    
    if(n_multi != MAX){
    	//Criar o socket UDP
    	if((sock_mult = socket(AF_INET,SOCK_DGRAM,0)) < 0) erro("Erro ao criar multicast");
    	
    	//Configurar o endereço multicast
    	memset(&addr,0,sizeof(addr));
    	addr.sin_family = AF_INET;
    	addr.sin_addr.s_addr = htonl(INADDR_ANY);
    	addr.sin_port = htons(porta);
    	
    	
    	//Adicionar o socket ao grupo multicast
    	struct ip_mreq mreq;
    	mreq.imr_multiaddr.s_addr = inet_addr(ender);
    	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    	if(setsockopt(sock_mult, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) erro("Erro no setsockopt");
    	
    	endMulti[n_multi] = ender;
    	portas[n_multi] = porta;
          sockets[n_multi] = sock_mult;
          n_multi++;
       
    
    	//Receber mensagens do grupo multicast
    	while(1){
       	socklen_t addrlen = sizeof(addr);
       	if (recvfrom(sock_mult, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr, &addrlen) < 0) erro("Erro no recvfrom");
       	printf("ALERTA: Foi adicionada uma nova noticia vinda de %s\n NOTICIA: %s\n",inet_ntoa(addr.sin_addr),buffer);
    	}
    	
    	if(sock_mult >= 0)close(sock_mult);
    }
    
    pthread_exit(NULL); 
    
}

////Thread do Multicast Receber/////
pthread_t receberMulticast(char *ender, char *porta) {
   pthread_t tid;
   
   Thread_args2 *args2 = malloc(sizeof(Thread_args2));
   int port = atoi(porta);
   args2->endereco = malloc(strlen(ender) + 1);
   if (args2->endereco == NULL) erro("Erro ao alocar espaço para o endereço\n");
   strcpy(args2->endereco, ender);
   args2->porta = port;
   
   if (pthread_create(&tid, NULL, receberMulticastThread, (void *) args2) != 0) erro("Erro ao criar a thread multicast\n");
   return tid;
   
}

//////Thread Principal Cliente////////
void *clientThread(void*arg){

    int fd;
    struct sockaddr_in addr;
    struct hostent *hostPtr;
    char buffer[BUFLEN];
    int nread;
    
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Não consegui obter endereço");

    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons(portoNoti); //PORTO DE NOTICIAS, OU PORTO DO SERVIDOR

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("socket");

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("Connect");
        
 char username[BUFLEN],password[BUFLEN];
 char id[BUFLEN],titulo[BUFLEN];

 int isLeitor = 0;
 int isJornal = 0;
 while(1){
    	nread = read(fd, buffer,BUFLEN);
          if (nread < 0) {
             erro("Erro ao ler resposta do servidor");
          }
          buffer[nread] = '\0';
          printf("%s", buffer);
        	
        	fgets(username, BUFLEN, stdin);
    	//Enviar as credenciais do username
    	write(fd, username, strlen(username));
    		

    	nread = read(fd, buffer,BUFLEN);
          if (nread < 0) {
             erro("Erro ao ler resposta do servidor");
          }
          buffer[nread] = '\0';
          printf("%s", buffer);
          
        	fgets(password, BUFLEN, stdin);
    	//Enviar as credenciais da password
    	write(fd, password, strlen(password));
    	
     	char resposta[BUFLEN];
     	nread = read(fd, resposta,BUFLEN);
          if (nread < 0) {
             erro("Erro ao ler resposta do servidor");
          }
          resposta[nread] = '\0';	
    		
    	int aux = atoi(resposta);
    	
    	if (aux == 1){
    	 isLeitor = 1;    
    	 break;          	
    	}else if (aux == 2){
    	 isJornal = 1;
    	 break;
          }else{
           printf("%s",resposta); 
           write(fd, "Recebido", strlen("Recebido"));
          } 
   }
   
   char option[BUFLEN];
   pthread_t t;
   if(isLeitor == 1){
   	char endMultiAux[MAX][BUFLEN];
   	char portasAux[MAX][BUFLEN];
   
   	write(fd, "Recebido", strlen("Recebido"));
   	
   	 ///////////////
           
           nread = read(fd,endMultiAux,MAX*BUFLEN);
           if (nread < 0) erro("Erro no recv");
           int n = nread/BUFLEN;
           
           write(fd, "Recebido", strlen("Recebido"));
           
           nread = read(fd,portasAux,MAX*BUFLEN);
           if (nread < 0) erro("Erro no recv");
           
           write(fd, "Recebido", strlen("Recebido"));
            
           if (n != 0 && n < MAX){
           	for (int i = 0; i < n; i++){
		 	t = receberMulticast(endMultiAux[i],portasAux[i]);
		 	listaThreads[n_thread] = t;
		 	n_thread++;
           	 }
           }
           
           /////////////////
    
    	  
   	while(1){
   	
   	   nread = read(fd, buffer,BUFLEN);
             if (nread < 0) {
                erro("Erro ao ler resposta do servidor");
             }
             buffer[nread] = '\0';
             printf("%s", buffer);	
             
             fgets(option,BUFLEN,stdin);
             //Enviar a opção escolhida
             write(fd,option,BUFLEN);
             
    	   int op = atoi(option);
             
             if(op == 0){
             //SAIR
             	struct ip_mreq mreq;
   
             	for(int i = 0; i < n_multi; i++){
             	    mreq.imr_multiaddr.s_addr = inet_addr(endMulti[i]);
                        mreq.imr_interface.s_addr = inet_addr(endServer);
             	    if (setsockopt(sockets[i], IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,sizeof(mreq)) < 0) erro("Erro ao remover-se do grupo multicast");
        
             	}
             	
             	nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	    erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             }else if(op == 1){
             //LISTAR TÓPICOS
             	char n_top[BUFLEN];
             	nread = read(fd, n_top,BUFLEN);
             	if (nread < 0) {
                	    erro("Erro ao ler resposta do servidor");
             	}
             	
             	n_top[nread] = '\0';
             	
             	int n_topicos = atoi(n_top);
             	
             	write(fd, "Recebido", strlen("Recebido"));
             	
             	for (int i = 0; i < n_topicos; i++){
			nread = read(fd, buffer,BUFLEN);
             		if (nread < 0) {
                	    		erro("Erro ao ler resposta do servidor");
             		}
             		buffer[nread] = '\0';
             		
             		printf("%s\n", buffer);
             		
             		write(fd, "Recebido", strlen("Recebido"));
		}
		
             }else if(op == 2){
             //SUBSCREVER UM TÓPICO
                    nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	    	erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             	fgets(id,BUFLEN,stdin);
                    //Enviar o id
                    write(fd,id,strlen(id));
                    
                    nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	    	erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             	
             	write(fd, "Recebido", strlen("Recebido"));
             	
             	
             		
             	nread = read(fd,buffer,BUFLEN);
	  	if (nread < 0){ // Handle errors properly
               	   erro("Erro ao ler resposta do cliente");
 
             	}
             	buffer[nread] = '\0';
             	
             	
             	write(fd, "Recebido", strlen("Recebido"));
             	
                    char portaAux[BUFLEN];
                    
             	nread = read(fd,portaAux,BUFLEN);
	  	if (nread < 0){ // Handle errors properly
               	   erro("Erro ao ler resposta do cliente");
    
             	}
             	portaAux[nread] = '\0';
             	
             	if(strcmp(buffer,"0")!= 0){
             	     t = receberMulticast(buffer, portaAux);
             	     listaThreads[n_thread] = t;
		     n_thread++;
             	}
                    
                    write(fd, "Recebido", strlen("Recebido"));
             	
             }else{
             	nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	    	erro("Erro ao ler resposta do servidor");
             	}
             	
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	write(fd, "Recebido", strlen("Recebido"));
             }
             
   	}
   
   }else if(isJornal == 1){
   	char endMultiAux[MAX][BUFLEN];
   	char portasAux[MAX][BUFLEN];
   
   	write(fd, "Recebido", strlen("Recebido"));
   	
   	 ///////////////
           
           nread = read(fd,endMultiAux,MAX*BUFLEN);
           if (nread < 0) erro("Erro no recv");
           int n = nread/BUFLEN;
           
           write(fd, "Recebido", strlen("Recebido"));
           
           nread = read(fd,portasAux,MAX*BUFLEN);
           if (nread < 0) erro("Erro no recv");
           
           write(fd, "Recebido", strlen("Recebido"));
            
           if (n != 0 && n < MAX){
           	for (int i = 0; i < n; i++){
		 	t = receberMulticast(endMultiAux[i],portasAux[i]);
		 	listaThreads[n_thread] = t;
		 	n_thread++;
           	 }
           }
           
           /////////////////
   	char porta[BUFLEN],ender[BUFLEN];
   	while(1){
   	
   	   nread = read(fd, buffer,BUFLEN);
             if (nread < 0) {
                erro("Erro ao ler resposta do servidor");
             }
             buffer[nread] = '\0';
             printf("%s", buffer);	
             
             fgets(option,BUFLEN,stdin);
             //Enviar a opção escolhida
             write(fd,option,strlen(option));
             
    	   int op = atoi(option);
             
             if(op == 0){
             //SAIR
             	nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             }else if (op == 1){
             //CRIAR UM TÓPICO
                    nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             	fgets(id,BUFLEN,stdin);
             	//Enviar a opção escolhida
             	write(fd,id,strlen(id));
             	
             	nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             	fgets(titulo,BUFLEN,stdin);
             	//Enviar a opção escolhida
             	write(fd,titulo,strlen(titulo));
             	
             	nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             	
             	write(fd, "Recebido", strlen("Recebido"));
             	
             	nread = read(fd, ender,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	ender[nread] = '\0';
             	
             	write(fd, "Recebido", strlen("Recebido"));
             	
             	nread = read(fd,porta,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	porta[nread] = '\0';
             	
             	write(fd, "Recebido", strlen("Recebido"));
             	
             }else if (op == 2){
             //ENVIAR UMA NOTICIA
             	char noti[BUFLEN];
             	nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             	fgets(id,BUFLEN,stdin);
             	//Enviar a opção escolhida
             	write(fd,id,strlen(id));
             	
             	nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             	fgets(noti,BUFLEN,stdin);
             	//Enviar a opção escolhida
             	write(fd,noti,strlen(noti));
         
             	nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             	write(fd, "Recebido", strlen("Recebido"));
             	
             	nread = read(fd, ender,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	ender[nread] = '\0';
             	
             	write(fd, "Recebido", strlen("Recebido"));
             	
             	nread = read(fd,porta,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	porta[nread] = '\0';
             	
             	
             	if(strcmp(ender,"0")!= 0){
             	     t = enviarMulticast(ender, porta,noti);
             	     listaThreads[n_thread] = t;
		     n_thread++;
             	}
		
		write(fd, "Recebido", strlen("Recebido"));
             	
             	
             }else{
             	nread = read(fd, buffer,BUFLEN);
             	if (nread < 0) {
                	      erro("Erro ao ler resposta do servidor");
             	}
             	buffer[nread] = '\0';
             	printf("%s", buffer);
             	
             	write(fd, "Recebido", strlen("Recebido"));
             }
   	}
   }
  
 close(fd);
 pthread_exit(NULL);

}


/////////Receber e Enviar as mensagens do Servidor//////////////
int main(int argc, char *argv[]){

	
    if (argc != 3) erro("Usage: news_client {endereço do servidor} {PORTO_NOTICIAS}");

    strcpy(endServer, argv[1]);
     
    portoNoti = atoi(argv[2]);
    
    pthread_t client_thread;
    
    if(pthread_create(&client_thread,NULL,clientThread,NULL) != 0) erro("Erro ao criar a thread do servidor UDP");
    
    for (int i = 0; i < n_thread; i++){
    	 int ret = pthread_join(listaThreads[i], NULL);
            if (ret != 0) erro("Erro ao aguardar as thread!\n");
    }

    pthread_join(client_thread, NULL);
    return 0;
    
}
