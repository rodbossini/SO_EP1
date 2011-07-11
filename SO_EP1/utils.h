/*
 * utils.h
 *
 *  Created on: 07/09/2010
 *      Author: rodrigo
 */


#ifndef UTILS_H_
#include "ListaLigada.h"

//esta estrutura será utilizada para armazenar os tokens recebidos do usuário
//ela também armazena os delimiters que foram utilizados para obter
//esta sequencia de tokens e armazena a string original que são os tokens separados pelo delimiter
struct Tokens{
	struct ListaLigada * tokens;
	char * delimiters;
	char * stringOriginal;
};

//estrutura usada  para  facilitar a vida ao criar processos para cada comando digitado pelo usuário (vide função de tratamento de redireção e pipes)
struct Processo{
	char *  comando;
	int fdIn;
	int fdOut;
	int fdErr;
	int pipe[2];
};

//devolve uma substring de str começando em inicio (inclusive) e terminando em fim (inclusive)
char * substring (const char * const str, int inicio, int final);
#define UTILS_H_


#endif /* UTILS_H_ */
