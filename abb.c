#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "abb.h"
#include "pila.h"

typedef struct abb_nodo{
	struct abb_nodo* izq;
	struct abb_nodo* der;
	void* dato;
	char* clave;
}abb_nodo_t;

 struct abb{
	abb_nodo_t* raiz;
	abb_comparar_clave_t cmp;
	abb_destruir_dato_t destruir_dato;
	size_t cantidad;
};

struct abb_iter{
	pila_t* pila;
};

abb_t* abb_crear(abb_comparar_clave_t cmp, abb_destruir_dato_t destruir_dato){
	abb_t* abb=malloc(sizeof(abb_t));
	if (!abb) return NULL;
	abb->raiz=NULL;
	abb->cantidad=0;
	abb->destruir_dato=destruir_dato;
	abb->cmp=cmp;
	return abb;
}

abb_nodo_t* crear_nodo(){
	abb_nodo_t* nodo=calloc(4,sizeof(abb_nodo_t));
	if(!nodo) return NULL;
	return nodo;
}

void* abb_buscar(abb_nodo_t *arbol,const char *clave,abb_comparar_clave_t cmp){
	if (!arbol) return NULL;
	if (cmp(arbol->clave,clave)<0) return abb_buscar(arbol->der,clave,cmp);
	if (cmp(arbol->clave,clave)>0) return abb_buscar(arbol->izq,clave,cmp);
	return arbol;
}

void insertar_nodo(abb_nodo_t *arbol, abb_nodo_t* padre,const char *clave, void *dato,abb_nodo_t* nodo, abb_comparar_clave_t cmp,abb_t* abb){
	if (!arbol){	
		//AGREGA UN NODO NUEVO
		if (cmp(padre->clave,clave)>0) padre->izq=nodo;
		else padre->der=nodo;		
		abb->cantidad++;
		return;
	}
	else if (cmp(arbol->clave,clave)>0) insertar_nodo(arbol->izq,arbol,clave,dato,nodo,cmp,abb);
	else if (cmp(arbol->clave,clave)<0) insertar_nodo(arbol->der,arbol,clave,dato,nodo,cmp,abb);
	else { //REEMPLAZA EL DATO DE UN NODO EXISTENTE
		if (abb->destruir_dato) abb->destruir_dato(arbol->dato);
		free(nodo);
		free((void*)clave);
		arbol->dato=dato;
		return;
	}
}

bool abb_guardar(abb_t *arbol, const char *clave, void *dato){
	abb_nodo_t* nodo=crear_nodo();
	if(!nodo) return false;
	
	char* clave_copia=malloc((strlen(clave)+1)*sizeof(char));
	if (!clave_copia) return false;
 	strcpy(clave_copia,clave);
	nodo->clave=clave_copia;
	nodo->dato=dato;
	
	if (!arbol->raiz){
		arbol->raiz=nodo;
		arbol->cantidad++;
	}else{
		insertar_nodo(arbol->raiz,NULL,clave_copia,dato,nodo,arbol->cmp,arbol);	 
	}
	return true;
}

void* abb_nodo_destruir(abb_nodo_t* nodo){
	void* dato=nodo->dato;
	free(nodo->clave);
	free(nodo);
	return dato;
}

void* abb_borrar_hoja(abb_nodo_t* nodo, abb_nodo_t* padre, abb_t* arbol){
	if (padre==NULL) arbol->raiz=NULL;
	else if (padre->izq==nodo) padre->izq = NULL;
	else padre->der = NULL;	
	return abb_nodo_destruir(nodo);
}

void* abb_borrar_con_hijo_unico(abb_nodo_t* nodo, abb_nodo_t* padre, abb_t* arbol){
	if ((!nodo->der) && (nodo->izq)){
		if (!padre) arbol->raiz = nodo->izq;			
		else if (padre->der==nodo) padre->der = nodo->izq;
		else padre->izq = nodo->izq;
	}else{
		if (!padre) arbol->raiz=nodo->der;
		else if (padre->der==nodo) padre->der = nodo->der;
		else padre->izq = nodo->der;
	}
	return abb_nodo_destruir(nodo);
}

void modificar_padre(abb_nodo_t* padre,abb_nodo_t* nodo, abb_nodo_t* reemplazante, abb_t* arbol){
	if (!padre)	arbol->raiz=reemplazante;
	else if (padre->der==nodo) padre->der=reemplazante;
	else padre->izq=reemplazante;
}

void* abb_borrar_con_2hijos(abb_nodo_t* nodo, abb_nodo_t* padre, abb_t* arbol){	
	abb_nodo_t* reemp=nodo->der;
	abb_nodo_t* nuevo=NULL; 
	while (reemp->izq!=NULL){
		nuevo=reemp;
		reemp=reemp->izq;
	}
	abb_nodo_t* reemplazante;
	if (nuevo){
		reemplazante=nuevo->izq;
		nuevo->izq=reemplazante->der;
		reemplazante->izq=nodo->izq;
		reemplazante->der=nodo->der;
	}else{
		reemplazante=nodo->der;
		reemplazante->izq=nodo->izq;
	}	
	modificar_padre(padre,nodo,reemplazante,arbol);
	return abb_nodo_destruir(nodo);
}

void* _abb_borrar(abb_nodo_t* nodo, abb_nodo_t* padre, int lado, const char* clave, abb_t* arbol){
	if (arbol->cmp(nodo->clave, clave)>0) return _abb_borrar(nodo->izq, nodo, 1, clave, arbol);
	if (arbol->cmp(nodo->clave, clave)<0) return _abb_borrar(nodo->der, nodo, -1, clave, arbol);
		// Si nodo no tiene hijos
	if (!nodo->izq && !nodo->der) return abb_borrar_hoja(nodo, padre, arbol);		
		// Si tiene un solo hijo
	else if ((!nodo->der && nodo->izq) || (!nodo->izq && nodo->der)) return abb_borrar_con_hijo_unico(nodo, padre, arbol);
		//Si tiene dos hijos
	return abb_borrar_con_2hijos(nodo, padre, arbol);
}

void* abb_borrar(abb_t *arbol, const char *clave){
	if (!abb_pertenece(arbol, clave)) return NULL;
	arbol->cantidad--;
	return _abb_borrar(arbol->raiz, NULL, 0, clave, arbol);
}

void* abb_obtener(const abb_t *arbol, const char *clave){
	abb_nodo_t* nodo=abb_buscar(arbol->raiz,clave,arbol->cmp);
	if (!nodo) return NULL;
	return nodo->dato;
}

bool abb_pertenece(const abb_t *arbol, const char *clave){	
	if (!abb_buscar(arbol->raiz,clave,arbol->cmp)) return false;
	return true;
}

size_t abb_cantidad(abb_t *arbol){
	return arbol->cantidad;
}

static void _abb_destruir(abb_nodo_t* arbol,abb_destruir_dato_t destruir_dato){
	if (arbol->izq!=NULL) _abb_destruir(arbol->izq,destruir_dato);
	if (arbol->der!=NULL) _abb_destruir(arbol->der,destruir_dato);
	void* dato=abb_nodo_destruir(arbol);
	if (destruir_dato!=NULL) destruir_dato(dato);
	return;
}

void abb_destruir(abb_t *arbol){
	if (arbol->raiz) _abb_destruir(arbol->raiz,arbol->destruir_dato);
	free(arbol);
}

void iterar_abb(abb_nodo_t *arbol, bool visitar(const char *, void *, void *), void *extra, bool* aux){ 
	if(!arbol||!visitar) return;
	iterar_abb(arbol->izq,visitar,extra,aux);
	if (visitar&&*aux){
		*aux=visitar(arbol->clave,arbol->dato,extra);
		if(!*aux) return;
	}	
	if(!*aux) return;	
	iterar_abb(arbol->der,visitar,extra,aux);
}

void abb_in_order(abb_t *arbol, bool visitar(const char *, void *, void *), void *extra){
	bool aux=true;
	iterar_abb(arbol->raiz,visitar,extra,&aux);
}

abb_iter_t *abb_iter_in_crear(const abb_t *arbol){
	abb_iter_t* abb_iter=malloc(sizeof(abb_iter_t));
	if (!abb_iter) return NULL;
	pila_t* pila=pila_crear();
	if (!pila) return NULL;
	abb_nodo_t* nodo=arbol->raiz;
	while (nodo!=NULL){
		pila_apilar(pila,nodo);
		nodo=nodo->izq;
	}
	abb_iter->pila=pila;
	return abb_iter;
}

bool abb_iter_in_al_final(const abb_iter_t *iter){
	if (pila_esta_vacia(iter->pila)) return true;
	return false;
}

bool abb_iter_in_avanzar(abb_iter_t *iter){
	if (abb_iter_in_al_final(iter))	return false;
	abb_nodo_t* elem=pila_desapilar(iter->pila);
	abb_nodo_t* aux=elem->der;
	while(aux){
		pila_apilar(iter->pila,aux);
		aux=aux->izq;
	}
	return true;
}

const char *abb_iter_in_ver_actual(const abb_iter_t *iter){
	if (abb_iter_in_al_final(iter)) return NULL;
	abb_nodo_t* elem=pila_ver_tope(iter->pila);
	return elem->clave;
}

void abb_iter_in_destruir(abb_iter_t* iter){
	pila_destruir(iter->pila);
	free(iter);
}
