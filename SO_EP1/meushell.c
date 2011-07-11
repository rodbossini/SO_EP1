/*
 * meushell.c
 *
 *  Created on: 06/09/2010
 *      Author: rodrigo
 */
#include <fcntl.h>
#include "ListaLigada.h"
#include "utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <errno.h>
#include <sys/types.h>
#include<sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
//estrutura que armazena os comandos próprios do shell
struct ListaLigada * comandosProprios;

//estrutura que armazena as variáveis de ambiente do shell
struct ListaLigada * ambiente;

//string que representa o formato em que o shell aceita os comandos
//esta string é usada pela função validaComando
//ela garante que o primeiro parametro não é um operador
//e garante que os parametros seguintes sejam todos "não-operador" ou sejam intercalados com operadores
//sempre separados por um ou mais espaços em branco
const char  const formatoPadrao [1000] = "^[^\\|<>2>]+(( +\\|| +<| +>| +2>)?( +[^\\|<>2>]+))*$";

//valida a sintaxe do comando utilizando expressões regulares (com o intuito de não permitir que operadores apareçam em sequencia)
int validaComando (const char * comando);

//função que inicializa a lista de comandos próprios do shell
int inicializaComandosProprios();

//função que inicializa a lista que contém os paths do shell
int inicializaAmbiente();

//função varre a lista de comandos do shell para determinar se o comando é um comando próprio do shell
int isComandoProprio (char * comando);

//determina qual comando executar
int executaComandoProprio (struct Tokens * tokens);

//encapsula a lógica para executar o comando path
int executaComandoPath (struct Tokens * tokens);

//encapsula a lógica para executar o comando cd
int executaComandoCD (struct Tokens * comando);

//encapsula a lógica para executar o comando exit
int executaComandoExit(struct Tokens * comando);

//função de tratamento de falha que exibe o valor de errno após uma falha de system call
void trataFalha ();

//recebe o comando que ja passou pela primeira validação sintática e o quebra em strings utilizando espaço em branco como delimiter
struct Tokens * getTokens (const char * const comando,const char * const del);

//função que trata os comandos que não pertencem ao shell
int trataComandosEmGeral(struct Tokens * tokens);

//esta função percorre a lista de tokens procurando um a um no diretório local e
//em cada um dos diretórios disponíveis em path
int concatenaDiretoriosAosComandos(struct Tokens * tokens);

//função que trata qualquer comando que não utilize nenhum operador
int trataComandoGeralSemOperadores (struct Tokens * tokens);

//esta função é usada para tratar comandos que utilizando operadores de redirecionamento mas que não
//utilizam pipe
int trataComandoGeralSemPipe (struct Tokens * tokens);

//esta função é usada para tratar comandos que utilizam pipe mas que não utilizam operadores redirecionamento
int trataComandoGeralRedirEPipe(struct Tokens * tokens);

//cria n pipes (vide função trataComandoGeralRedirEPipe)
void criaPipes(struct  Processo ** processos,int n);

//fecha n-1 pipes, excto os pipes na posição i (vide função trataComandoGeralRedirEPipe)
void fechaPipes (struct Processo ** processos, int n, int i);

//fecha n-2 pipes, exceto aqueles pipes nas posições i e k (vide função trataComandoGeralRedirEPipe)
void fechaPipes2 (struct Processo ** processos, int n, int i,int k);
//inicio do programa
 int main(int argc, char **argv) {

	if (! inicializaAmbiente() || ! inicializaComandosProprios()){
		trataFalha();

	}
	size_t * n = (size_t *) calloc (1,sizeof(size_t));
	if (n == NULL)
		trataFalha();
	*n = (size_t)4096 + 1;
	char * comandoAtual;
	char * dirAtual;
	while (1){
		//imprime o diretório atual
		dirAtual = getcwd(NULL,0);
		if (!dirAtual){
			perror("getcwd");
			continue;
		}
		printf ("%s", dirAtual);

		comandoAtual = (char *)calloc (*n, sizeof(char));
		if (comandoAtual == NULL){
			trataFalha();
		}
		//lê da entrada padrão o próximo comando
		getline (&comandoAtual,n,stdin);

		//retira o \n da string lida..
		comandoAtual = substring(comandoAtual,0,strlen(comandoAtual) - 2);
		//comandoAtual = "path + /media";

		if (!validaComando(comandoAtual)){
			//se o comando estiver sintaticamente incorreto, imprime mensagem de erro e pula para a próxima iteração
			printf ("Erro de sintaxe no seu comando %s...\n",comandoAtual);
			free (comandoAtual);
			free (dirAtual);
			continue;
		}

		//dá um "split" no comando usando espaço em branco
		struct Tokens * tokens = getTokens(comandoAtual, " ");

		//trata os comandos próprios
		if (isComandoProprio(recupera(tokens->tokens,0))){
			if (executaComandoProprio(tokens)){
				free (comandoAtual);
				free (tokens);
				free(dirAtual);
			}
			else{
				printf ("Você digitou um comando próprio do shell, porém o formato é inválido...\n");
				free (comandoAtual);
				free (tokens);
				free(dirAtual);
			}
		}
		else{
			trataComandosEmGeral(tokens);
			free (comandoAtual);
			free (tokens);
			free(dirAtual);
		}
	}
	return 0;
}

int validaComando (const char * comando){
	int status;
	 regex_t  re;
	 if (regcomp(&re,formatoPadrao,REG_EXTENDED |REG_NOSUB) != 0)  {
		 return(0);
	 }
	 status = regexec(&re, comando,(size_t)0, NULL, 0);
	 regfree(&re);
	 if (status != 0) {
		 return(0);
	 }
	 return(1);
}

//função que inicializa a lista de comandos próprios do shell
int inicializaComandosProprios(){
	comandosProprios = (struct ListaLigada *)malloc(sizeof(struct ListaLigada));
	if (comandosProprios == NULL)
		trataFalha();
	adicionaNoComeco(comandosProprios,"path");
	adicionaNoComeco(comandosProprios,"cd");
	adicionaNoComeco(comandosProprios,"exit");
	return 1;
}
int inicializaAmbiente (){
	ambiente = (struct ListaLigada *) calloc(1,sizeof(struct ListaLigada));
	if (ambiente == NULL)return 0;
	adicionaNoComeco(ambiente,".");
	//adiciona(ambiente,"/bin");
	//adiciona(ambiente,"/usr/bin");
	return 1;
}
int executaComandoProprio (struct Tokens * tokens){
	if (strcmp (recupera(tokens->tokens,0),"path") == 0)
		return executaComandoPath(tokens);
	else if (strcmp(recupera(tokens->tokens,0),"cd") == 0)
		return executaComandoCD(tokens);
	else if (strcmp(recupera(tokens->tokens,0),"exit") == 0)
		return executaComandoExit(tokens);
	return 0;
}
int isComandoProprio (char * comando){
	return (contem(comandosProprios,comando));
}
int executaComandoPath (struct Tokens * tokens){
	//se tiver mais que dois elementos, quer dizer que é para adicionar ou remover variáveis do path
	if (tokens->tokens->total > 2){
		//verifica se existem operadores após o comando próprio, se sim, nada é executado
		if (contem (tokens->tokens,"|") || contem (tokens->tokens,">") || contem (tokens->tokens,"<") || contem (tokens->tokens,"2>"))
			return 0;
		//se o proximo token for + adiciona tenta adicionar cada argumento ao ambiente
		if (strcmp(recupera (tokens->tokens,1),"+") == 0){
			int i = 2;
			while (i < tokens->tokens->total){
				//checa se o diretório existe antes de adicioná-lo...
				char * dir = recupera (tokens->tokens,i);
				DIR * op = opendir(dir);
				if (op == NULL){
					printf ("Não foi possível adicionar diretório %s ao path: %s\n",dir,strerror(errno));
					closedir(op);
				}
				else{
					closedir(op);
					adiciona(ambiente,dir);
				}
				i++;
			}
			return 1;
		}
		//se o comando for do tipo path - ... tenta remover os elementos ... do ambiente
		else if (strcmp(recupera (tokens->tokens,1),"-") == 0){
				int i = 2;
				while (i < tokens->tokens->total && ambiente->total >= 1){
					removeValores(ambiente,recupera(tokens->tokens,i));
					i++;
				}
				return 1;
		}

	}
	//se tiver somente um argumento, quer dizer que é pra imprimir as variáveis em path
	else if (tokens->tokens->total == 1){
		int i = 0;
		while (i < ambiente->total - 1){
			printf ("%s:",recupera(ambiente,i++));
		}
		if (ambiente->total >= 1)
		printf ("%s\n",recupera (ambiente,i));
		return 1;
	}


	return 0;
}
int executaComandoCD (struct Tokens * tokens){
	//checa o número de argumentos que foi passado
	//se foi passado mais de um, retorna erro
	//pois para trocar de diretório somente um diretório deve ser passado como parametro para cd
	//se não for passado nenhum também retorna erro
	if (tokens->tokens->total > 2 || tokens->tokens->total == 1){
		return 0;
	}
	char * dir  = recupera (tokens->tokens,1);
	int result = chdir(dir);
	if (result == 0)
		return 1;
	printf ("Falha: %s\n",strerror(errno));
	return 1;
}
int executaComandoExit (struct Tokens * tokens){
	exit(0);
}
void trataFalha (){
	printf ("Falha: %s\n",strerror(errno));
	exit(EXIT_FAILURE);
}
struct Tokens * getTokens (const char * const comando,const char * const del){

	struct Tokens * tokens =(struct Tokens *) calloc (1,sizeof(struct Tokens));
	if (tokens == NULL)
		trataFalha();
	//cria uma cópia para que a original não seja alterada
	char *aux = strdup (comando);

	tokens->tokens =(struct ListaLigada *) calloc (1,sizeof(struct ListaLigada));

	if (tokens->tokens == NULL)
			trataFalha();
	tokens->stringOriginal = (char *) calloc (strlen(aux) + 1,sizeof(char));
	if (tokens->stringOriginal == NULL)
		trataFalha();
	tokens->delimiters = (char * ) calloc (strlen(del) + 1,sizeof(char));
	if (tokens->delimiters == NULL)
		trataFalha();
	strcpy (tokens->stringOriginal,aux);
	strcpy(tokens->delimiters,del);


	int i = 0;
	char * token = (char *)calloc (4096 + 1,sizeof(char));
	token = strtok(aux,del);
	do{
		adiciona(tokens->tokens,token);
		i++;
		token = strtok(NULL,del);
	}
	while (token != NULL);
	return tokens;
}

int trataComandosEmGeral(struct Tokens * tokens){
	if (contem(tokens->tokens,"path") || contem(tokens->tokens,"cd") || contem(tokens->tokens,"cd")){
		printf ("Não se pode utilizar comandos próprios do shell dessa forma...\n");
		return 0;
	}
	if (concatenaDiretoriosAosComandos(tokens)){
		//se o comando não contém nenhum operador, chama a função abaixo
		if (!contem(tokens->tokens,"|") && !contem(tokens->tokens,">") && !contem(tokens->tokens,"<") && !contem(tokens->tokens,"2>")){
			return trataComandoGeralSemOperadores(tokens);
		}
		//trata redirecionamento sem pipe
		else if (!contem(tokens->tokens,"|") && (contem(tokens->tokens,">") || contem(tokens->tokens,"<") || contem(tokens->tokens,"2>"))){
			return trataComandoGeralSemPipe(tokens);
		}
		//trata pipe sem redirecionamento
		else{
			return trataComandoGeralRedirEPipe(tokens);
		}

	}
	return 0;
}

int concatenaDiretoriosAosComandos (struct Tokens * tokens){
	char * diretorioAtual;
	struct ListaLigada * novosTokens = calloc (1,sizeof(struct ListaLigada *));
	if (!novosTokens)
		//será fatal para o sistema
		trataFalha();

	int i = 0;
	while (i < tokens->tokens->total){
		int j;
		int ok = 0;
		for (j = 0; j < ambiente->total; j++){
			diretorioAtual = recupera (ambiente,j);
			struct dirent **  arquivos;
			int result = scandir(diretorioAtual,&arquivos,NULL,NULL);
			if (result < 0){
				//se der erro aqui o resultado final será completamente alterado
				//portanto estou considerando este erro como fatal para o programa
				trataFalha();
			}
			while (result--){
				if (strcmp(recupera(tokens->tokens,i),arquivos[result]->d_name) == 0){
					char * aux = (char *)calloc (4096 + 1,sizeof(char *));
					strcpy(aux,diretorioAtual);
					strcat(aux,"/");
					strcat(aux,recupera(tokens->tokens,i));
					adiciona (novosTokens,aux);
					ok = 1;
					free (arquivos[result]);
					break;
				}
				free(arquivos[result]);

			}
			free (arquivos);
			if (ok){
				//para a procura se ja tiver encontrado no diretorio atual e passa para o proximo token
				break;
			}
		}
		if (!ok){
			printf ("Comando %s não encontrado) no seu path\n",recupera(tokens->tokens,i));
			return 0;
		}
		//percorre a sequencia de tokens até encontrar um outro comando, que somente podera vir depois de pipe
		while (i < tokens->tokens->total - 1 &&(strcmp(recupera(tokens->tokens,i),"|")!= 0 )){
			i++;
			adiciona(novosTokens,recupera(tokens->tokens,i));
		}
		//soma mais um para um para o proximo token que será o próximo comando após o operador
		i++;
	}
	struct ListaLigada * apagar = tokens->tokens;
	tokens->tokens = novosTokens;
	free(apagar);
	return 1;
}
int trataComandoGeralSemOperadores (struct Tokens * tokens){
	int pid = fork();
	if (pid == 0){
		//processo filho
		char ** comando = (char **)  calloc(tokens->tokens->total + 1,sizeof(char*));
		if (!comando){
			perror("calloc");
			return 0;
		}
		int i;
		for (i = 0; i < tokens->tokens->total;i++){
			comando[i] = recupera(tokens->tokens,i);
		}


		int result = execv (comando[0], comando);
		if (result < 0){
			perror("ERRO:");
			exit(EXIT_FAILURE);
		}


	}
	else if (pid < 0){
		perror("Falha na criação do processo filho");
		return 0;
	}
	else{
		//processo pai
		int status;
		int res = wait (&status);
		if (res < 0)
			perror("child status");
	}
	return 1;
}
int trataComandoGeralSemPipe (struct Tokens * tokens){
	int pid = fork();
	if (pid == 0){
		int fdIn=0,fdOut=1,fdErr=2;
		char ** comando = (char**) calloc(tokens->tokens->total + 1,sizeof(char*));
		if(!comando){
			perror("calloc");
			return 0;
		}
		int i = 0;
		while (strcmp(recupera(tokens->tokens,i),">") != 0 && strcmp(recupera(tokens->tokens,i),"<") != 0 && strcmp(recupera(tokens->tokens,i),"2>") != 0 && i < tokens->tokens->total){
			comando[i] = recupera(tokens->tokens,i);
			i++;
		}
		while (i < tokens->tokens->total - 1){
			if (strcmp(recupera(tokens->tokens,i),">") == 0){
				if (fdOut != 1){
					int result = close (fdOut);
					if (result < 0){
						perror("close");
						return 0;
					}
				}
				fdOut = open(recupera(tokens->tokens,i + 1),O_CREAT | O_RDWR);
				if (fdOut < 0){
					perror("open");
					return 0;
				}
			}
			else if (strcmp(recupera(tokens->tokens,i),"<") == 0){
				if (fdIn != 0){
					int result = close (fdIn);
					if (result < 0){
						perror("close");
						return 0;
					}
				}
				fdIn = open(recupera(tokens->tokens,i + 1),O_RDONLY);
				if (fdIn < 0){
					perror("open");
					return 0;
				}
			}
			else if (strcmp(recupera(tokens->tokens,i),"2>") == 0){
				if (fdErr != 2){
					int result = close(fdErr);
					if (result < 0){
						perror("close");
						return 0;
					}
				}

				fdErr = open(recupera(tokens->tokens,i + 1),O_CREAT | O_RDWR);
				if (fdErr < 0){
					perror("open");
					return 0;
				}
			}
			i++;
		}
		int result;
		result = dup2(fdIn,0);
		if (result < 0){
			perror("dup2");
			return 0;
		}
		result = dup2(fdOut,1);
		if (result < 0){
			perror("dup2");
			return 0;
		}
		result = dup2(fdErr,2);
		if (result < 0){
			perror("dup2");
			return 0;
		}
		result = execv (comando[0], comando);
		if (result < 0){
			perror("ERRO:");
			exit(EXIT_FAILURE);
		}
	}
	else if (pid < 0){
		//erro
			perror("fork");
			return 0;
	}
	else{
		//pai espera filho
		int status;
		int w = wait(&status);
		if (!w)
			perror("wait");
		return 1;
	}
	return 1;
}
int trataComandoGeralRedirEPipe(struct Tokens * tokens){

	//determina quantos processos serão criados
	int i = 0, contProcessos = 1;
	for (i = 0;i < tokens->tokens->total;i++){
		if (strcmp(recupera(tokens->tokens,i),"|") == 0)
			contProcessos++;
	}

	//vetor de processos
	struct Processo ** processos = NULL;

	int j = 0;
	//percorre os tokens digitados pelo usuário e cria um processo para cada comando separado por |
	//a cada vez que encontra um operador de redireção (exceto pipe), ele atualiza o fileDescriptor correspondente
	for (i = 0; i < contProcessos; i++){

		processos = (struct Processo **)realloc(processos,(i + 1) * sizeof(struct Processo *)) ;
		if (!processos){
			perror("realloc");
			return 0;
		}
		processos [i] = malloc (sizeof(struct Processo));
		struct Processo * processo = processos[i];

		processos[i]->fdIn = 0;
		processo->fdOut = 1;
		processo->fdErr = 2;
		processo->comando = (char *)calloc(4096 + 1, sizeof(char));
		if (!processo->comando){
			perror("calloc");
			free (processos);
			return 0;
		}

		while ( j < tokens->tokens->total && strcmp(recupera(tokens->tokens,j),"|")!= 0){
			if (strcmp(recupera(tokens->tokens,j),"<") == 0){
				if (processo->fdIn != 0){
					close(processo->fdIn);
				}
				processo->fdIn = open (recupera(tokens->tokens,j + 1),O_RDONLY);
				j++;
				while (strcmp(recupera(tokens->tokens,j),">") != 0 && strcmp(recupera(tokens->tokens,j),"<") != 0 && strcmp(recupera(tokens->tokens,j),"2>") != 0 && strcmp(recupera(tokens->tokens,j),"|") != 0 )
					j++;
			}
			else if (strcmp(recupera(tokens->tokens,j),">") == 0){
				if (processo->fdOut != 1){
					close(processo->fdOut);
				}
				processo->fdOut = open (recupera(tokens->tokens,j + 1),O_CREAT | O_RDWR);
				j++;
				while (j < tokens->tokens->total && strcmp(recupera(tokens->tokens,j),">") != 0 && strcmp(recupera(tokens->tokens,j),"<") != 0 && strcmp(recupera(tokens->tokens,j),"2>") != 0 && strcmp(recupera(tokens->tokens,j),"|") != 0 )
					j++;
			}
			else if (strcmp(recupera(tokens->tokens,j),"2>") == 0){
				if (processo->fdErr != 2){
					close(processo->fdErr);
				}
				processo->fdErr = open (recupera(tokens->tokens,j + 1),O_CREAT | O_RDWR);
				j++;
				while (strcmp(recupera(tokens->tokens,j),">") != 0 && strcmp(recupera(tokens->tokens,j),"<") != 0 && strcmp(recupera(tokens->tokens,j),"2>") != 0 && strcmp(recupera(tokens->tokens,j),"|") != 0 )
					j++;
			}
			else{
				strcat (processo->comando, recupera (tokens->tokens,j));
				strcat(processo->comando, " ");
				j++;
			}
		}
		//verifica se este processo esta tentando redirecionar stdOut para um arquivo e pipe ao mesmo tempo
		if (j < tokens->tokens->total && strcmp(recupera(tokens->tokens,j),"|") == 0 && processo->fdOut != 1){
			printf ("Erro de sintaxe: stdOut direcionado para mais de uma entidade...\n");
			return 0;
		}
		j++;
	}

	//nesta parte todos os processos ja estao prontos para serem colocados no pipe
	//estou criando um processo filho para fazer esse serviço, e o processo principal espera por ele
	int pid = fork();
	if (pid == 0){
		//printf ("filho geral %d\n",getpid());
		//filho que vai colocar todos os processos no pipe
		//crio todos os pipes que serão necessários
		criaPipes(processos,contProcessos);
		//crio um processo para o ultimo processo na estrutura
		int pidUltimo = fork();
		if (pidUltimo == 0){
			//printf ("ultimo processo: %d\n",getpid());
			fechaPipes(processos,contProcessos,contProcessos-1);
			close(processos[contProcessos - 1]->pipe[1]);
			dup2(processos[contProcessos - 1]->pipe[0],0);
			dup2(processos[contProcessos -1 ]->fdErr,2);
			dup2(processos[contProcessos - 1]->fdOut,1);
			struct Tokens * auxTokens = getTokens (processos[contProcessos - 1]->comando, " ");
			int j;
			char ** comandos =(char **) calloc(auxTokens->tokens->total,sizeof(char*));
			for (j = 0; j < auxTokens->tokens->total;j++){
				comandos[j] =recupera(auxTokens->tokens,j);
			}
			execv(comandos[0],comandos);
			exit(EXIT_FAILURE);
		}
		else if (pidUltimo < 0){
			perror ("fork");
			exit (EXIT_FAILURE);
		}
		else{
			int j;
			for (j = contProcessos -2; j >0; j--){
				if(fork() == 0){
					//printf ("sub filho dentro do for %d\n",getpid());
					fechaPipes2(processos,contProcessos,j+1,j);
					close(processos[j+1]->pipe[0]);
					dup2(processos[j+1]->pipe[1],1);
					close(processos[j]->pipe[1]);
					dup2(processos[j]->pipe[0],0);
					//dup2(processos[j]->fdOut,1);
					struct Tokens * auxTokens = getTokens (processos[j]->comando, " ");
					int z;
					char ** comandos =(char **) calloc(auxTokens->tokens->total,sizeof(char*));
					for (z = 0; z < auxTokens->tokens->total;z++){
						comandos[z] =recupera(auxTokens->tokens,z);
					}
					execv(comandos[0],comandos);
					exit(EXIT_FAILURE);
				}
				else{
					//perror("fork");
					//exit(EXIT_FAILURE);
				}
			}
			if (fork() == 0){
				//printf ("processo 1 %d\n",getpid());
				fechaPipes(processos,contProcessos,1);
				close(processos[1]->pipe[0]);
				dup2(processos[1]->pipe[1],1);
				dup2(processos[0]->fdIn,0);
				dup2(processos[0]->fdErr,2);
				struct Tokens * auxTokens = getTokens (processos[0]->comando, " ");
				int j;
				char ** comandos =(char **) calloc(auxTokens->tokens->total,sizeof(char*));
				for (j = 0; j < auxTokens->tokens->total;j++){
					comandos[j] =recupera(auxTokens->tokens,j);
				}
				execv(comandos[0],comandos);
				exit(EXIT_FAILURE);
			}
			else {
				//sleep(5);
				//printf ("%d esperando ultimo processo %d\n",getpid(),pidUltimo);
				wait(NULL);
				exit (EXIT_SUCCESS);
			}
		}


	}
	else if (pid < 0){
		perror("fork");
		return 0;
	}
	else{
		int w = wait (NULL);
		if (w < 0){
			perror ("wait");
			return 0;
		}
		return 1;
	}
	return 0;
}
void criaPipes(struct  Processo ** processos,int n){
	int i;
	for (i = 0; i < n; i++){
		if (pipe(processos[i]->pipe) < 0){
			perror("erro no pipe");
			break;
		}

	}
}
void fechaPipes (struct Processo ** processos, int n, int i){
	int j;
	for (j = 0; j < n; j++){
		if (j != i){
			close(processos[j]->pipe[0]);
			close(processos[j]->pipe[1]);
		}
	}
}

void fechaPipes2 (struct Processo ** processos, int n, int i,int k){
	int j;
	for (j = 0; j < n; j++){
		if (j != i && j!= k){
			close(processos[j]->pipe[0]);
			close(processos[j]->pipe[1]);
		}
	}
}

