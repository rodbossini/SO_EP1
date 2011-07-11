/*
 * LinkedList.c
 *
 *  Created on: 05/09/2010
 *      Author: rodrigo
 */


#include "ListaLigada.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int adicionaNoComeco (struct  ListaLigada * listaLigada , char * valor){
	if (listaLigada != NULL){
		struct Celula * nova = (struct Celula *) calloc (1,sizeof(struct Celula));
		if (nova == NULL)
			return 0;
		nova->anterior = NULL;
		nova->proxima = listaLigada->primeira;
		if(listaLigada->total > 0)
			listaLigada->primeira->anterior = nova;
		nova->dado = (char *) calloc (strlen(valor)+1,sizeof(char));
		if (nova->dado == NULL)
			return 0;
		nova->dado = valor;
		listaLigada->primeira = nova;
		if (listaLigada->total == 0){
			listaLigada->ultima = listaLigada->primeira;
		}
		listaLigada->total++;
		return 1;
	}
	return 0;
}

int adiciona (struct  ListaLigada * listaLigada , char * valor){
	if (listaLigada != NULL){
		if (listaLigada->total == 0){
				return adicionaNoComeco(listaLigada,valor);
			}
			else{
				struct Celula * nova =(struct Celula *) calloc (1,sizeof(struct Celula));
				if (nova == NULL)
					return 0;
				nova->dado = (char *) calloc (strlen(valor)+1,sizeof(char));
				if (nova->dado == NULL)
					return 0;
				nova->dado = valor;
				listaLigada->ultima->proxima = nova;
				nova->anterior = listaLigada->ultima;
				listaLigada->ultima = nova;
				nova->proxima = NULL;
				listaLigada->total++;
				return 1;
			}
	}
	return 0;
}

char * recupera (struct ListaLigada * listaLigada, int posicao){
	if (listaLigada != NULL){
		if (posicao >= listaLigada->total)
			return NULL;
		int i = 0;
		struct Celula * atual = listaLigada->primeira;
		while (i++ < posicao && atual != NULL){
			atual = atual->proxima;
		}
		return atual->dado;
	}
	return 0;
}

int removeDoComeco (struct ListaLigada * listaLigada){
	if (listaLigada != NULL){
		listaLigada->primeira = listaLigada->primeira->proxima;
		if (listaLigada->total > 1)
			free (listaLigada->primeira->anterior);
		listaLigada->total--;

		if(listaLigada->total == 0){
			listaLigada->ultima = NULL;
			free (listaLigada->ultima);
		}
		return 1;
	}
	return 0;
}
int removeDoFim(struct ListaLigada * listaLigada){
	if (listaLigada != NULL){
		if (listaLigada->total == 1){
			return removeDoComeco(listaLigada);
		}
		else{
			struct Celula * penultima = listaLigada->ultima->anterior;
			penultima->proxima = NULL;
			free (penultima->proxima);
			listaLigada->ultima = penultima;
			listaLigada->total--;
			return 1;
		}
	}
	return 0;
}
void removeValores (struct ListaLigada * listaLigada,char * valor){
	if (listaLigada!= NULL){
		struct Celula * atual = listaLigada->primeira;
		while (contem(listaLigada,valor) && atual != NULL){
			if (strcmp(atual->dado,valor) == 0){
				if (listaLigada->total== 1){
					free (listaLigada->primeira);
					listaLigada->primeira = NULL;
					listaLigada->ultima = NULL;
					listaLigada->total--;
				}
				else if (atual == listaLigada->primeira){
					listaLigada->primeira = atual->proxima;
					listaLigada->primeira->anterior = NULL;
					struct Celula * apagar = atual;
					atual = atual->proxima;
					free (apagar);
					listaLigada->total--;
				}
				else if( atual ==  listaLigada->ultima){
					listaLigada->ultima = listaLigada->ultima->anterior;
					listaLigada->ultima->proxima = NULL;
					struct Celula * apagar = atual;
					atual = atual->proxima;
					free(apagar);
					listaLigada->total--;
				}
				else{
					struct Celula * anterior  = atual->anterior;
					struct Celula * proxima = atual->proxima;
					if (proxima != NULL)
						proxima->anterior = anterior;
					if (anterior != NULL)
						anterior->proxima = proxima;
					struct Celula * apagar = atual;
					atual = atual->proxima;
					free (apagar);
					listaLigada->total--;
				}
			}
			else{
				atual = atual->proxima;
			}
		}
	}
}

int removeItem (struct ListaLigada * listaLigada,int posicao){
	if (listaLigada != NULL){
		if (posicao == 0){
			return removeDoComeco(listaLigada);
		}
		if (posicao == listaLigada->total - 1){
			return removeDoFim(listaLigada);
		}
		if (posicao >= listaLigada->total)
			return 0;
		struct Celula * anterior = listaLigada->primeira;
		int i = 0;
		while (i++ < posicao - 1){
			anterior = anterior->proxima;
		}
		struct Celula * atual = anterior->proxima;
		struct Celula * proxima = atual->proxima;

		anterior->proxima = proxima;
		proxima->anterior = anterior;
		free (atual);
		listaLigada->total--;
		return 1;
	}
	return 0;
}
int contem (struct ListaLigada * listaLigada, char * valor){
	if (listaLigada != NULL){
		struct Celula * atual = listaLigada->primeira;
		while (atual != NULL){
			if (strcmp (atual->dado,valor) == 0)
				return 1;
			atual = atual->proxima;
		}
		return 0;
	}
	return 0;
}


