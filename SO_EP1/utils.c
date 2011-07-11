/*
 * utils.c
 *
 *  Created on: 08/09/2010
 *      Author: rodrigo
 */

#include "utils.h"
#include <stdlib.h>
#include <string.h>
char * substring (const char * const str,int inicio, int final){
	char * aux = (char *) calloc (strlen(str),sizeof(char));
	if (inicio < 0 || inicio > final || final > strlen(str))
		return NULL;
	int i, j;
	for (i = inicio, j = 0; i <=final; i++,j++){
		aux[j] = str[i];
	}
	return aux;
}
