/*
 * listaLigada.h
 *
 *  Created on: 07/09/2010
 *      Author: rodrigo
 */

#ifndef LISTALIGADA_H_

//estrutura comum utilizada por uma lista ligada
struct Celula{
	char * dado;
	struct Celula * proxima;
	struct Celula * anterior;
};
//a lista ligada
struct ListaLigada{
	struct Celula * primeira;
	struct Celula * ultima;
	int total;
};
//adiciona no final (se estiver vazia obviamente adiciona no começo)
int adiciona (struct  ListaLigada * listaLigada , char * valor);
//adiciona na primeira posição
int adicionaNoComeco (struct ListaLigada * listaLigada, char* valor);
//recupera o valor na posição passada por parametro (retorna NULL se a posição não estiver ocupada
char * recupera (struct ListaLigada * listaLigada,int posicao);
//remove o item na posiçao passada por parametro
int removeItem (struct ListaLigada * listaLigada,int posicao);
//remove todos as células que tiverem o valor passado como parametro como seu valor
void removeValores (struct ListaLigada * listaLigada,char * valor);
//remove do começo
int removeDoComeco (struct ListaLigada * listaLigada);
//remove do fim
int removeDoFim (struct ListaLigada * listaLigada);
//mostra o tamanho
int tamanho(struct ListaLigada * listaLigada);
//verifica se a lista contem determinado valor
int contem (struct ListaLigada * listaLigada, char * valor);

#define LISTALIGADA_H_


#endif /* LISTALIGADA_H_ */
