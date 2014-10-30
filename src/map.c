#include <stdio.h>
#include <string.h>

#include <tcl.h>

#include "iter.h"
#include "func.h"
#include "call.h"
#include "tclf.h"

#include "tuple.h"

const char *stateKey="::f::state";
/** 
*/
typedef struct IteratorState {
	unsigned long id;	// уникальный id 
	struct IteratorState *prevState; // предыдущее в стеке состояний
	Tcl_Namespace *privateNs;	// приватное пространство имён (для statevar)
	Tcl_Obj *arg;	// Аргумент - объект по котому ведутся итерации
	Iter *it;	// тут перебираются его элементы
	Tcl_Obj *funcObj;	// а эта функция над ними исполняется
	
	Tcl_Obj *ret;	// и тут собираются результаты
	int mode;	// "режим" работы: 0-map, 1-filter ,2-fold
} IteratorState;

unsigned long nextId=0;

IteratorState *stateAlloc();
void stateFree(IteratorState *);

/** стек состояний..приходится слегка повторять внутренности Tcl
**/

IteratorState *statePush(Tcl_Interp *interp);
IteratorState *statePop(Tcl_Interp *interp);
IteratorState *currentState(Tcl_Interp *);

/***
	map tail - (то есть без функции) - просто создать кортеж из tail (который может быть list)
	map ..expression.. tail - применять expression к tail, собирать результат
	------
	@REFACTOR: По факту - это Y комбинатор, надо-бы переименовать..
***/


int mapObjProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]) {
	return Tcl_NRCallObjProc(interp, mapNreProc, 
            data, objc, objv);
}

int mapNreStep(ClientData data[],Tcl_Interp *interp,int code)
{
	IteratorState *st;
	Tcl_Obj *result,*arg;
	(void)data;
	st=currentState(interp);
	if (st==NULL) {
		ERR("can`t access to state");
		return TCL_ERROR;
	}
	result=Tcl_GetObjResult(interp);
	if (code==TCL_OK) {
		if (st->mode==1) {
			// filter
			int yes;
			if (Tcl_GetBooleanFromObj(interp,result,&yes)==TCL_OK && yes) 
				tupleObjPut(st->ret,iterCurr(st->it));
		} else if (st->mode==2) {
			// fold
			tupleObjClear(st->ret);
			tupleObjPut(st->ret,result);
		} else {
			// map
			tupleObjPut(st->ret,result);
		}
	}
	if (code==TCL_OK && (arg=iterNext(st->it))!=NULL) {
		Tcl_NRAddCallback(interp,mapNreStep,NULL,NULL,NULL,NULL);
		if (st->mode==2) {
			return funcObjCallVA(interp,st->funcObj,st->ret,arg,NULL);
		}
		return funcObjCallVA(interp,st->funcObj,arg,NULL);
	}
	if (code==TCL_OK || code==TCL_RETURN) {
		Tcl_SetObjResult(interp,st->ret);
	}
	statePop(interp);
	return code;
}

int
mapNreProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[])
{
	register IteratorState *st;	// состояние итератора
	int mode;
	int minObjc;
	switch((int)data) {
		case 1 : mode=1;minObjc=2;break;
		case 2 : mode=2;minObjc=3;break;
		default: mode=0;minObjc=2;break;
	}
	//// АНАЛИЗ АРГУМЕНТОВ
	if (objc<minObjc) {
		if (mode==2)
			Tcl_WrongNumArgs(interp,objc,objv,"?function..? acc tail");
		else
			Tcl_WrongNumArgs(interp,objc,objv,"?function..? tail");
		return TCL_ERROR;
	}
	if (objc==minObjc) {
		// не заданна функция - надо вернуть аргумент в виде tuple
		if (objv[1]->typePtr==tupleType) {
			Tcl_SetObjResult(interp,tupleRemoveNulls(objv[1]));
		} else {
			int length;
			if (Tcl_ListObjLength(interp,objv[1],&length)!=TCL_OK)
				return TCL_ERROR;
			if (length==0)
				Tcl_SetObjResult(interp,emptyTuple);
			else
				Tcl_SetObjResult(interp,tupleObjFromList(objv[1]));
			return TCL_OK;
		}
	}
	// создание итератора
	st=statePush(interp);
	st->mode=mode;
	st->it=iterNew(interp,objv[objc-1],0);	// maybe static?
	if (iterEmpty(st->it)) {
		iterFree(st->it);
		Tcl_SetObjResult(interp,emptyTuple);
		return TCL_OK;
	}
	st->arg=objv[objc-1];
	Tcl_IncrRefCount(st->arg);
	// создание исполняемой функции
	if (mode!=2) {
		// map,filter
		st->funcObj=funcObjFromArgs(interp,objc-2,objv+1);
	} else {
		// fold
		st->funcObj=funcObjFromArgs(interp,objc-3,objv+1);
	}
	if (st->funcObj==NULL) {
		goto ERROR;
	}
	Tcl_IncrRefCount(st->funcObj);
	
	st->ret=tupleObjNew();	// тут собираем результаты
	Tcl_IncrRefCount(st->ret);
	// поместить очередной (первый) элемент в конец cmdLine
	// исполняем
	Tcl_NRAddCallback(interp,mapNreStep,NULL,NULL,NULL,NULL);
	if (mode==2) {
		// fold
		return funcObjCallVA(interp,st->funcObj,objv[objc-2],iterFirst(st->it),NULL);
	}
	return funcObjCallVA(interp,st->funcObj,iterFirst(st->it),NULL);
ERROR:
	statePop(interp);
	return TCL_ERROR;
}

int
stateidObjProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]) {
	char request[64]={};
	IteratorState *st;
	(void)data;
	if (objc!=1) {
		Tcl_WrongNumArgs(interp,objc,objv,"");
		return TCL_OK;
	}
	st=currentState(interp);
	if (st==NULL) {
		Tcl_SetResult(interp,"no states outside iterators",NULL);
		return TCL_ERROR;
	}
	if (st->privateNs==NULL) {
		snprintf(request,64,"::f::__%lu",st->id);
		st->privateNs=Tcl_CreateNamespace(interp,request,NULL,NULL);
		if (st->privateNs==NULL) {
			return TCL_ERROR;
		}
	}
	Tcl_Obj *ret;
	ret=Tcl_NewLongObj(st->id);
	Tcl_SetObjResult(interp,ret);
	return TCL_OK;
}
IteratorState *statePush(Tcl_Interp *interp) {
	IteratorState *st,*prevState;
	prevState=(IteratorState *)Tcl_GetAssocData(interp,stateKey,NULL);
	
	st=stateAlloc();
	if (st==NULL) {
		ERR("unable to create state");
		return NULL;
	}
	st->prevState=prevState;
	Tcl_SetAssocData(interp,stateKey,NULL,st);
	return st;
}
IteratorState *statePop(Tcl_Interp *interp) {
	IteratorState *st,*nextState;
	st=(IteratorState *)Tcl_GetAssocData(interp,stateKey,NULL);
	if (st==NULL) return NULL;
	nextState=st->prevState;
	stateFree(st);
	Tcl_SetAssocData(interp,stateKey,NULL,nextState);
	return nextState;
}
inline IteratorState *currentState(Tcl_Interp *interp) {
	return (IteratorState *)Tcl_GetAssocData(interp,stateKey,NULL);
}
IteratorState *
stateAlloc()
{
	register IteratorState *st;
	st=(IteratorState *)Tcl_Alloc(sizeof(IteratorState));
	if (st==NULL)
		return NULL;
	memset(st,0,sizeof(IteratorState)+sizeof(Tcl_Obj *));
	st->id=nextId++;
	return st;
}
void
stateFree(register IteratorState *st) {
	if (st==NULL) return;
	//if (st->arg!=NULL) Tcl_DecrRefCount(st->arg);
	if (st->it!=NULL) iterFree(st->it);
	if (st->funcObj!=NULL) Tcl_DecrRefCount(st->funcObj);
	if (st->arg!=NULL) Tcl_DecrRefCount(st->arg);
	if (st->privateNs!=NULL) Tcl_DeleteNamespace(st->privateNs);
	Tcl_Free((char *)st);
}
