#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>


#define BUFLEN 512	// Tamanho do buffer
#define MAX 100 // Tamanho máximo das lista
   
   
/////Inicialização dos argumentos de arranque//////
int portoNoti;
int portoConfig;
char fileName[100];

/////Estrutura de Dados de Utilizadores//////
typedef struct {
    char username[BUFLEN];
    char password[BUFLEN];
    char type[BUFLEN];
} User;

/////Estrutura de Dados dos portos e endereços dos administadores UDP//////
typedef struct {
    char endereco[BUFLEN];
    int porto;
} Dados;

/////Estrutura de Dados de um Tópico//////
typedef struct {
    char idTopic[BUFLEN];
    char titulo[BUFLEN];
    char nomeJornalista[BUFLEN];
    char enderMult[BUFLEN];
    char porta[BUFLEN];
    char *clientes[MAX];
    char *noticias[MAX];
}Topico;

/////Lista de Tópicos//////
Topico topicos[MAX];
int n_topicos = 0;
int n_clientes = 0;
int n_noticias = 0;

/////Lista de Portos//////
Dados dados[MAX];
int n_dados = 0;

/////Lista de Utilizadores//////
User users[MAX];
int n_users = 0;

/////Função de Erro//////
void erro(char *s) {
    perror(s);
    exit(1);
}

/////Gerar endereços Multicast//////
void *gerarEnder(){

    srand(time(NULL));
    
    //Gerar quatro números aleatórios entre 224 e 239 - 0 e 255
    int a = rand() % 16 + 224;
    int b = rand() % 256;
    int c = rand() % 256;
    int d = rand() % 256;

    //Combinar os números em um único endereço IP de multicast
    struct in_addr address;
    address.s_addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
    char* ip = malloc(INET_ADDRSTRLEN);
    if (ip == NULL) erro("Erro na alocação de memória");

    if (inet_ntop(AF_INET, &address, ip, INET_ADDRSTRLEN) == NULL)erro("Erro ao converter o endereço para string");
     return ip;
}

/////Gerar portas multicast////////
char* gerarPortaMulticast() {
    char* porta = malloc(6 * sizeof(char)); // máximo 5 caracteres para a porta (incluindo o '\0')
    if (porta == NULL) {
        return NULL;
    }

    srand(time(NULL));
    int num_porta = rand() % (65535 - 1024 + 1) + 1024;

    sprintf(porta, "%d", num_porta);

    return porta;
}

/////Subscrever um Tópico//////
void *subs_topic(char *id,char *nome){
   char *output = malloc(sizeof(char) * BUFLEN);
   char *ender = malloc(sizeof(char) * BUFLEN);
   char *porta = malloc(sizeof(char) * BUFLEN);
   strcpy(ender,"0");
   strcpy(porta,"0");
   
   if (n_clientes == MAX) {
        	snprintf(output, BUFLEN, "Número máximo de clientes atingido no tópico pretendido\n;%s;%s",ender, porta);
        	return output;
   }
   
   if (n_topicos > 0){
     for (int i = 0; i < n_topicos; i++){
       if (strcmp(topicos[i].idTopic,id) == 0){
          topicos[i].clientes[n_clientes] = nome;
	n_clientes ++;
	snprintf(output, BUFLEN*BUFLEN, "Tópico adicionado com sucesso\n;%s;%s", topicos[i].enderMult, topicos[i].porta);
	return output;
       }
     }
   }
   
   snprintf(output, BUFLEN, "Tópico não encontrado\n;%s;%s",ender, porta);
   return output;
}

////////////Criar um tópico/////////////
void *criar_topic(char *tituloTopic, char *id,char *nome){
   char *output = malloc(sizeof(char) * BUFLEN);
   char *ender = malloc(sizeof(char) * BUFLEN);
   char *porta = malloc(sizeof(char) * BUFLEN);
   strcpy(ender,"0");
   strcpy(porta,"0");
    
    if (n_topicos == MAX) {
        snprintf(output, BUFLEN, "Número máximo de tópicos atingido\n;%s;%s",ender, porta);
        	return output;
    }
    
    // Verificar se já existe um tópico com o mesmo id
    for (int i = 0; i < n_topicos; i++){
        if (strcmp(topicos[i].idTopic,id) == 0){
            snprintf(output,BUFLEN*BUFLEN, "Já existe um tópico com o id inserido\n;%s;%s",topicos[i].enderMult, topicos[i].porta);
      	  return output;
        }
    }
    char *ip = gerarEnder();
    char *port = gerarPortaMulticast();
    
    Topico new_topic;
    strcpy(new_topic.idTopic,id);
    strcpy(new_topic.titulo, tituloTopic);
    strcpy(new_topic.nomeJornalista,nome);
    strcpy(new_topic.enderMult,ip);
    strcpy(new_topic.porta,port);
    topicos[n_topicos++] = new_topic;
    
    

   snprintf(output,BUFLEN*BUFLEN, "Tópico criado com sucesso\n;%s;%s",new_topic.enderMult, new_topic.porta);
   return output;

}

/////Adicionar uma noticia a um tópico//////
void *noticia(char *id,char *noti){
	char *output = malloc(sizeof(char) * BUFLEN);
          char *ender = malloc(sizeof(char) * BUFLEN);
          char *porta = malloc(sizeof(char) * BUFLEN);
          strcpy(ender,"0");
          strcpy(porta,"0");
	
	if (n_noticias == MAX) {
              snprintf(output, BUFLEN, "Número máximo de notícias atingido\n;%s;%s",ender, porta);
   	    return output;
          }

	for (int i = 0; i < MAX; i++){
		if(strcmp(topicos[i].idTopic,id) == 0){
			topicos[i].noticias[n_noticias++] = noti;
			
			snprintf(output,BUFLEN*BUFLEN, "Notícia enviada com sucesso\n;%s;%s",topicos[i].enderMult, topicos[i].porta);
   			return output;
		}
	}
	
	snprintf(output, BUFLEN, "Erro ao enviar a notícia\n;%s;%s",ender, porta);
          return output;
}

/////Ler do ficheiro inicial de Login//////
void read_file() {

    FILE* file = fopen(fileName, "r"); 
    if (file == NULL) {
        erro("Erro ao abrir o ficheiro");
    }
    char line[100];
    while (fgets(line, sizeof(line), file)) {
        char *username, *password, *tipo;

        username = strtok(line, ";");
        password = strtok(NULL, ";");
        tipo = strtok(NULL, "\n");

        strcpy(users[n_users].username, username);
        strcpy(users[n_users].password, password);
        strcpy(users[n_users].type, tipo);
	   
        n_users++;
  
    }

    fclose(file);
}

/////Escrever no ficheiro de Login//////
void write_file() {

    FILE* file = fopen(fileName, "w"); 
    if (file == NULL) {
        erro("Erro ao abrir o ficheiro");
    }
    for(int i = 0; i < n_users; i++){
   		fprintf(file,"%s;%s;%s\n",users[i].username,users[i].password,users[i].type);
	}
    fclose(file);
}

/////Adicionar um utilizador//////
char* add_user(char *username, char *password, char *type) {
    char* output;
  
    if (n_users == MAX) {
        output = "Número máximo de utilizadores atingido\n";
        return output;
    }
    // Verificar se já existe um utilizador com o mesmo nome
    for (int i = 0; i < n_users; i++){
        if (strcmp(users[i].username, username) == 0){
            output = "Já existe um utilizador com o nome inserido";
      		return output;
    	}
  	}
    User new_user;
    strcpy(new_user.username, username);
    strcpy(new_user.password, password);
    strcpy(new_user.type, type);
    users[n_users++] = new_user;

    output = "Utilizador adicionado com sucesso\n";  
  
    write_file();

    return output;
}

/////Eliminar um utilizador//////
char* del_user(char *username) {
    int i;
    char* output;

    for (i = 0; i < n_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            n_users--;
            users[i] = users[n_users];

            output = "Utilizador eliminado com sucesso\n";
            write_file();
            return output;
        }
    }

    output = "Utilizador não encontrado\n";
    return output;
}

/////Confirmar se o utilizador é administrador//////
int autenticacao_admin(char *username, char *password) {
    int i;
    char* ad;
    int aux = 0;
    ad = "administrador";
    
    for (i = 0; i < n_users; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password)== 0 && strcmp(users[i].type, ad) == 0) {
            aux = 1;
            break;
        }
     }
    return aux;
}

/////Confirmar se o utilizador é leitor//////
int autenticacao_leitor(char *username, char *password) {
    int i;
    char* ad;
    int aux = 0;
    ad = "leitor";
    
    for (i = 0; i < n_users; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password)== 0 && strcmp(users[i].type, ad) == 0) {
            aux = 1;
            break;
        }
     }
    return aux;
}

/////Confirmar se o utilizador é jornalista//////
int autenticacao_jornal(char *username, char *password) {
    int i;
    char* ad;
    int aux = 0;
    ad = "jornalista";
    
    for (i = 0; i < n_users; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password)== 0 && strcmp(users[i].type, ad) == 0) {
            aux = 1;
            break;
        }
     }
    return aux;
}

/////Verificar se o porto UPD está em acesso//////
int verificaDados(int porta){

	for (int i = 0; i < n_dados; i++) {
        	   if (dados[i].porto == porta) {
               return 1;
             }
    	 } 
    	 	 
    return 0;
}

/////Comunicação TCP entre o Servidor e Cliente//////
void *process_client(void *fd){     
    int nread;
    int yesJornal = 0; 
    int yesLeitor = 0;
    
    int client_fd = *((int*) fd);
    
    //Autenticação do Leitor!Jornalista
    char username[BUFLEN],password[BUFLEN],buffer[BUFLEN];
    char *output;
    
    while(1){
  
    	char frase_user[]= "Introduza o seu username: ";
    	write(client_fd, frase_user, strlen(frase_user));
        	
        	
    	nread = read(client_fd,username,BUFLEN);
     	if (nread < 0){ // Handle errors properly
            erro("Erro ao ler resposta do cliente");
    
          }
     	
     	username[nread-1] = '\0';
    		
    		
    	char frase_pass[] = "Introduza a sua password: ";
    	write(client_fd,frase_pass,strlen(frase_pass));
    	
    	nread = read(client_fd,password,BUFLEN);
     	if (nread < 0){ // Handle errors properly
            erro("Erro ao ler resposta do cliente");
    
         }
     	
     	password[nread-1] = '\0';
     		
    		
    	yesJornal = autenticacao_jornal(username,password);
    	yesLeitor = autenticacao_leitor(username,password);
    	
    	//Envia uma destas informações
    	if (yesJornal == 1){
    	  write(client_fd,"2",1);
    	  break;
    	}else if (yesLeitor == 1){
    	  write(client_fd,"1",1);
    	  break;
    	}else{
    	  char frase_erro[] = "Erro: as credenciais introduzidas não são válidas. Por favor, tente novamente.\n";
    	  write(client_fd,frase_erro,strlen(frase_erro));
    	  nread = read(client_fd,buffer,BUFLEN);
	  if (nread < 0){ // Handle errors properly
                erro("Erro ao ler resposta do cliente");
    
             }
    	}		
    }
    char option[BUFLEN];
    int op;
    if (yesJornal == 1){
    	
    	   nread = read(client_fd,buffer,BUFLEN);
	     if (nread < 0){ // Handle errors properly
                   erro("Erro ao ler resposta do cliente");
               }
   
     	     char enderMulticast[MAX];
     	     char portasMulticast[MAX];
     	     int n_end = 0;
     	     
               for (int i = 0; i < n_topicos; i++) {
    		for (int j = 0; j < n_clientes; j++) {
        			if (strcmp(topicos[i].nomeJornalista, username) == 0) {
            			strcpy(&enderMulticast[n_end],topicos[i].enderMult);
            			strcpy(&portasMulticast[n_end],topicos[i].porta);
            			n_end++;
            			break;
        			}
    		}
	     }	
               
               //Enviar a lista de endereços á qual este pertence
    	     write(client_fd,enderMulticast,sizeof(enderMulticast));
    	
    	     nread = read(client_fd,buffer,BUFLEN);
	     if (nread < 0){ // Handle errors properly
                erro("Erro ao ler resposta do cliente");
    
               }
               
               //Enviar a lista de endereços á qual este pertence
    	     write(client_fd,portasMulticast,sizeof(portasMulticast));
    	
    	     nread = read(client_fd,buffer,BUFLEN);
	     if (nread < 0){ // Handle errors properly
                erro("Erro ao ler resposta do cliente");
    
               }
               
             
             while(1){
           	char id[BUFLEN],titulo[BUFLEN],noti[BUFLEN];
    		char menuJornal[BUFLEN] = "\nOpções Disponíveis:\n0-Sair\n1-Criar um tópico\n2-Enviar uma notícia\nEscolha uma opção: ";
    		write(client_fd,menuJornal,strlen(menuJornal));
    		
    		nread = read(client_fd,option,BUFLEN);
    		if (nread < 0){ // Handle errors properly
                       erro("Erro ao ler resposta do cliente");
    
                    }
                    
    		option[nread-1] = '\0';
    		
    		op = atoi(option);
    		
    		if (op == 0){
    		//Sair
    	             write(client_fd,"A Sair...",strlen("A Sair..."));
    	    	   close(client_fd);   
    	    	   pthread_exit(NULL);	
    		}else if (op == 1){
    		//Criar um tópico
    	   		write(client_fd,"Id do tópico: ",strlen("Id do tópico: "));
		   	nread = read(client_fd,id,BUFLEN);
		   	if (nread < 0){ // Handle errors properly
                                 erro("Erro ao ler resposta do cliente");
    
                              }
                              
                              id[nread-1] = '\0';
                              
		   	write(client_fd,"Título do tópico: ",strlen("Título do tópico: "));
		   	nread = read(client_fd,titulo,BUFLEN);
		   	if (nread < 0){ // Handle errors properly
                                 erro("Erro ao ler resposta do cliente");
    
                              }
                              
                              titulo[nread-1] = '\0';
                              
		   	output = criar_topic(titulo,id,username);
			output = strtok(output,";");
			char *ender = strtok(NULL,";");
			char *porta = strtok(NULL,"\0");
             		
			write(client_fd,output,strlen(output));
			
			nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
             		
             		write(client_fd,ender,strlen(ender));
             		
             		nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
             		
             		write(client_fd,porta,4);
             		
             		nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
    
            		 
    		}else if (op == 2){
    		//Enviar uma notícia
		   	write(client_fd,"Id do tópico: ",strlen("Id do tópico: "));
		   	nread = read(client_fd,id,BUFLEN);
		   	if (nread < 0){ // Handle errors properly
                                  erro("Erro ao ler resposta do cliente");
    
                              }
                              id[nread-1] = '\0';
                              
		   	write(client_fd,"Notícia: ",strlen("Notícia: "));
		   	
		   	nread = read(client_fd,noti,BUFLEN);
		   	if (nread < 0){ // Handle errors properly
                                 erro("Erro ao ler resposta do cliente");
  
                              }
                              noti[nread-1] = '\0'; 
                              
    	             	output = noticia(id,noti);
			output = strtok(output,";");
			char *ender = strtok(NULL,";");
			char *porta = strtok(NULL,"\0");
             		
			write(client_fd,output,strlen(output));
			
			nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
             		
             		write(client_fd,ender,strlen(ender));
             		
             		nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
             		
             		write(client_fd,porta,4);
             		
             		nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
    		}else{
    		  //Resposta errada
    		  char er[] = "Opção inválida\n";
    		  write(client_fd,er,strlen(er));
    		  nread = read(client_fd,buffer,BUFLEN);
	  	  if (nread < 0){ // Handle errors properly
               		 erro("Erro ao ler resposta do cliente");
    
             	  }
    		}
       	     }
       	     close(client_fd);
    	}else if (yesLeitor == 1){
    	
    	     nread = read(client_fd,buffer,BUFLEN);
	     if (nread < 0){ // Handle errors properly
                   erro("Erro ao ler resposta do cliente");
               }
   
     	     char enderMulticast[MAX];
     	     char portasMulticast[MAX];
     	     int n_end = 0;
     	     
               for (int i = 0; i < n_topicos; i++) {
    		for (int j = 0; j < n_clientes; j++) {
        			if (strcmp(topicos[i].clientes[j], username) == 0) {
            			strcpy(&enderMulticast[n_end],topicos[i].enderMult);
            			strcpy(&portasMulticast[n_end],topicos[i].porta);
            			n_end++;
            			break;
        			}
    		}
	     }	
               
               //Enviar a lista de endereços á qual este pertence
    	     write(client_fd,enderMulticast,sizeof(enderMulticast));
    	
    	     nread = read(client_fd,buffer,BUFLEN);
	     if (nread < 0){ // Handle errors properly
                erro("Erro ao ler resposta do cliente");
    
               }
               
               //Enviar a lista de endereços á qual este pertence
    	     write(client_fd,portasMulticast,sizeof(portasMulticast));
    	
    	     nread = read(client_fd,buffer,BUFLEN);
	     if (nread < 0){ // Handle errors properly
                erro("Erro ao ler resposta do cliente");
    
               }
               
              
               
       	     while(1){
    		char menuLeitor[BUFLEN] = "\nOpções Disponíveis:\n0- Sair\n1-Listar Tópicos\n2-Subscrever um tópico\nEscolha uma opção: ";
    		write(client_fd,menuLeitor,strlen(menuLeitor));
    		
    		nread = read(client_fd,option,BUFLEN);
    		if (nread < 0){ // Handle errors properly
                         erro("Erro ao ler resposta do cliente");
    
                    }
                    
                    option[nread-1] = '\0';
    		
    		op = atoi(option);
    		
    		if (op == 0){
    	  	//Sair
	    	   write(client_fd,"A Sair...",strlen("A Sair..."));
    	    	   close(client_fd);   
    	    	   pthread_exit(NULL); 
    		}else if (op == 1){
    	  	//Listar tópicossa
    	  		char n_top[BUFLEN];
    	  		sprintf(n_top,"%d",n_topicos);
    	  		write(client_fd,n_top,strlen(n_top));
    	  	
    	  		nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
             		
			for (int i = 0; i < n_topicos; i++){
				char aux[strlen(topicos[i].titulo) + 1];  // +1 para incluir o caractere '\0' no final
                              	strcpy(aux, topicos[i].titulo);
                              
				strcat(aux,"- id -> ");
			
				char aux2[strlen(topicos[i].idTopic) + 1];
				strcpy(aux2, topicos[i].idTopic);
			
				strcat(aux,aux2);
			
				write(client_fd,aux,strlen(aux));
			
				nread = read(client_fd,buffer,BUFLEN);
				if (nread < 0){ // Handle errors properly
                                 	    erro("Erro ao ler resposta do cliente");
    
                              	}
			}
             	
    		}else if (op == 2){
		   //Subscrever um tópico
			char id[BUFLEN];
			char *ender,*porta;
			
			write(client_fd,"Id do tópico: ",strlen("Id do tópico: "));
			
			nread = read(client_fd,id,BUFLEN);
			if (nread < 0){ // Handle errors properly
                                 erro("Erro ao ler resposta do cliente");
    
                              }
    			id[nread-1] = '\0';
    			
			output = subs_topic(id,username);
			output = strtok(output,";");
			ender = strtok(NULL,";");
			porta = strtok(NULL,"\0");
             		
			write(client_fd,output,strlen(output));
			
			nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
             		
             		write(client_fd,ender,strlen(ender));
             		
             		nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
             		
             		write(client_fd,porta,4);
             		
             		nread = read(client_fd,buffer,BUFLEN);
	  		if (nread < 0){ // Handle errors properly
               		   erro("Erro ao ler resposta do cliente");
    
             		}
			
    		}else{
    		  //Resposta errada
    		  char er[] = "Opção inválida\n";
    		  write(client_fd,er,strlen(er));
    		  nread = read(client_fd,buffer,BUFLEN);
	            if (nread < 0){ // Handle errors properly
                		erro("Erro ao ler resposta do cliente");
    
             	}
    		}
       	     }
       	}
     	close(client_fd);
     	pthread_exit(NULL);
}

//Criar a thread TCP
void *tcp_server(void *arg){
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    
    bzero((void*)&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(portoNoti);
    
    if((server_fd = socket(AF_INET,SOCK_STREAM,0)) < 0) erro("Erro ao criar o socket");
    
    if(bind(server_fd,(struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) erro("Erro ao associar o socket ao endereço");
    
    if(listen(server_fd,5) < 0) erro("Erro ao colocar o socket em modo de escuta");
    
    
    //A cada cliente cria uma nova pthread
    while(1){
    
      client_addr_size = sizeof(client_addr);
      client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_size);
      if(client_fd < 0) erro("Erro ao aceitar a conexão do cliente");
      
      pthread_t client_thread;
      if(pthread_create(&client_thread,NULL,process_client,(void*)&client_fd) != 0) erro("Erro ao criar uma thread para o cliente");
      
      if(pthread_detach(client_thread) != 0) erro("Erro ao desanexar a thread do cliente");
     
     }
    
    close(server_fd);
    pthread_exit(NULL);
    
}

//Criar thread UDP
void* udp_server(void* arg) {
    struct sockaddr_in si_minha, si_outra;

	int s,recv_len;
	socklen_t slen = sizeof(si_outra);
	
	char option[BUFLEN];
    char* output;
   
	//Criar um socket para recepção de pacotes UDP
	if((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		erro("Erro na criação do socket");
	}

  //Preencher da socket address structure
	si_minha.sin_family = AF_INET;
	si_minha.sin_port = htons(portoConfig);
	si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

	//Associar o socket à informação de endereço
	if(bind(s,(struct sockaddr*)&si_minha, sizeof(si_minha)) == -1) {
		erro("Erro no bind");
	}

    read_file();

//Autenticação do administrador
    
    int is_admin = 0;
    int y = 0;
    char l[BUFLEN];
   
        
	while(y != 1){
	
		for(int i = 0; i<5;i++){ // LOOP devido ao -v do netcat
    		if((recv_len = recvfrom(s, l, strlen(l), 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) 
    			{   
        		erro("Erro no recvfrom");
    		}		

    	l[recv_len]='\0';
    
    	}
     while (is_admin != 1) { 
     
    	char admin_username[BUFLEN], admin_password[BUFLEN];
    	char frase_user[]= "Introduza o seu username de administrador: ";
        
    	sendto(s, frase_user, strlen(frase_user), 0, (struct sockaddr *) &si_outra, slen);
        
    	if((recv_len = recvfrom(s, admin_username, BUFLEN, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) 
    		{   
        		erro("Erro no recvfrom");
    		}		
    
    	admin_username[recv_len-1]='\0';
    	
    	
    	
    	char frase_pass[] = "Introduza a sua password de administrador: ";
    	sendto(s, frase_pass, strlen(frase_pass), 0, (struct sockaddr *) &si_outra, slen);
        
    	if((recv_len = recvfrom(s, admin_password, BUFLEN, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) 
    		{
        		erro("Erro no recvfrom");
    		}
    		
    	admin_password[recv_len-1]='\0';
    	
    	
    	int yes = autenticacao_admin(admin_username,admin_password);
    	
    	if (yes == 1)
    		{
        		char frase_erro[] = "As credenciais introduzidas são válidas.\n";
        		sendto(s, frase_erro, strlen(frase_erro), 0, (struct sockaddr *) &si_outra, slen);
        		
        		//Adiciona o porto ao array de dados
        		Dados new_dados;
        		strcpy(new_dados.endereco,inet_ntoa(si_outra.sin_addr));
        		new_dados.porto = ntohs(si_outra.sin_port);
        		dados[n_dados++] = new_dados;
        		
        	
        		is_admin = 1;
    	} 
    		else 
    	{
        	char frase_erro[] = "Erro: as credenciais introduzidas não são válidas. Por favor, tente novamente.\n";
        	sendto(s, frase_erro, strlen(frase_erro), 0, (struct sockaddr *) &si_outra, slen);
    	}  
       }
       
      
      
      char options[] = "\n\nOpções disponíveis:\n1. Adicionar utilizador\n2. Eliminar um utilizador\n3. Lista utilizadores\n4. Sair da consola\n5. Desligar servidor\nEscolha a sua opção: ";
      
      int v = verificaDados(ntohs(si_outra.sin_port)); //Verifica se o porto do administrador está na lista de dados (endereços e portos)
  

      while(v == 1){
         
         sendto(s, options, strlen(options), 0, (struct sockaddr *) &si_outra, slen);

	//Esperar recepção de mensagem (a chamada é bloqueante)
	  	if((recv_len = recvfrom(s, option,BUFLEN, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
	      	erro("Erro no recvfrom");
	  	}
	  	
	// Para ignorar o restante conteúdo (anterior do buffer)
	  	option[recv_len]='\0';
	  	
	  	int op = atoi(option); 
	  		   
        if (op == 1) {
       		char new_username[BUFLEN], new_password[BUFLEN], new_tipo[BUFLEN];
        //Opção 1 - Adicionar um utilizador
        	char novo_user[] = "Introduza o username do novo utilizador: ";
        	sendto(s, novo_user, strlen(novo_user), 0, (struct sockaddr *) &si_outra, slen);
        //Esperar a recepção de mensagem (a chamada é bloqueante)
	  		if((recv_len = recvfrom(s, new_username, BUFLEN, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
	      erro("Erro no recvfrom");
	  		}

	  		new_username[recv_len-1]='\0';
        	
        	char nova_password[]= "Introduza a password do novo utilizador: ";
        	sendto(s, nova_password, strlen(nova_password), 0, (struct sockaddr *) &si_outra, slen);
        	
        //Esperar a recepção de mensagem (a chamada é bloqueante)
	  		if((recv_len = recvfrom(s, new_password, BUFLEN, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
	      		erro("Erro no recvfrom");
	  		}
		
	  		new_password[recv_len-1]='\0';
			
        	char novo_tipo[] = "Introduza o tipo do novo utilizador (administrador/leitor/jornalista): ";
        	sendto(s, novo_tipo, strlen(novo_tipo), 0, (struct sockaddr *) &si_outra, slen);
        //Esperar a recepção de mensagem (a chamada é bloqueante)
	  		if((recv_len = recvfrom(s, new_tipo, BUFLEN, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
	      		erro("Erro no recvfrom");
	  		}
		
	  		new_tipo[recv_len-1]='\0';
	
			
	  		output = add_user(new_username, new_password,new_tipo);
        	sendto(s, output, strlen(output), 0, (struct sockaddr *) &si_outra, slen);

       } else if (op == 2) {
        
      //Opção 2 - Eliminar um utilizador
        char del_username[BUFLEN];
        char input[] = "Introduza o username do utilizador a eliminar: ";
        sendto(s, input, strlen(input), 0, (struct sockaddr *) &si_outra, slen);
        if((recv_len = recvfrom(s, del_username, BUFLEN, 0, (struct sockaddr *) &si_outra, (socklen_t *)&slen)) == -1) {
	      erro("Erro no recvfrom");
	  }
	
	  	del_username[recv_len-1]='\0';

        output = del_user(del_username);
        sendto(s, output, strlen(output), 0, (struct sockaddr *) &si_outra, slen);

       } else if (op == 3) {
      //Opção 3 - Lista de utilizadores
      		char listar[] = "Lista de Utilizadores: \n";
      		sendto(s, listar, strlen(listar), 0, (struct sockaddr *) &si_outra, slen);
      		sendto(s,"\n",strlen("\n"),0, (struct sockaddr *) &si_outra, slen);
      		for (int i = 0; i < n_users; i++) {
        		sendto(s, users[i].username, strlen(users[i].username), 0, (struct sockaddr *) &si_outra, slen);
        		sendto(s,"\n",strlen("\n"),0, (struct sockaddr *) &si_outra, slen);
    		}
  
    	} else if (op == 4) {
      //Opção 4 - Sair da consola
      		printf("O administrador saiu\n");
      		for (int i = 0; i < n_dados; i++) {
        		if (dados[i].porto == (ntohs(si_outra.sin_port)) ) {
            		n_dados--;
            		dados[i] = dados[n_dados];
           
    		}	
    	   }
    	   	is_admin = 0;
    	   	v = verificaDados(ntohs(si_outra.sin_port));
	  		
    	} else if (op == 5) {
      //Opção 5 - Desligar o servidor
          sendto(s, "O servidor foi desconectado\n", strlen("O servidor foi desconectado\n"), 0, (struct sockaddr *) &si_outra, slen);
          close(s);
   
    	} else {
      		sendto(s, "Opcao Invalida", strlen("Opcao Invalida"), 0, (struct sockaddr *) &si_outra, slen);
    	}
	}
}

    pthread_exit(NULL);
}

/////Criação das pthreads UDP e TPC//////
int main(int argc, char *argv[]) {

    if (argc != 4){
    	erro("Usage: news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {ficheiro configuração}");
    }
    
    strcpy(fileName,argv[3]);
    
    portoNoti = atoi(argv[1]);
    portoConfig = atoi(argv[2]);
    
    pthread_t tcp_thread;
    pthread_t udp_thread;
    
    if(pthread_create(&udp_thread,NULL,udp_server,NULL) != 0) erro("Erro ao criar a thread do servidor UDP");
    	
    if(pthread_create(&tcp_thread,NULL,tcp_server,NULL) != 0) erro("Erro ao criar a thread do servidor TCP");
    
    pthread_join(udp_thread,NULL);
    
    pthread_join(tcp_thread,NULL);
    
    return 0;
}

