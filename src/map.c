#include <stdio.h>
#include <string.h>

#include <tcl.h>

#include "tclf.h"

#include "tuple.h"

/** 
*/
typedef struct IteratorState {
	unsigned long id;	// уникальный id 
	struct IteratorState *prevState; // предыдущее в стеке состояний
	Tcl_Obj *arg;	// Аргумент - объект по котому ведутся итерации
	Tcl_Obj *ret;	// предполагаемый результат
	Tcl_Command cmd;// исполняемая команда
	Tcl_Namespace *privateNs;	// приватное пространство имён (для statevar)
	union {
		// если аргумент - список, то это его данные и курсор
		struct {
			int listLength;
			Tcl_Obj **listElements;
			int listIndex;	// текущий индекс в нём
		};
		// если аргумет - кортеж, то это его курсор
		struct {
			Cons *currCons;
		};
	};
	int cmdLineLen;	// кол-во элементнов в cmdLine
	Tcl_Obj **cmdLineTail; 	// указатель на последний эл-т в cmdLine
	Tcl_Obj **cmdLineAcc;	// аккумулятор для итераторов а-ля fold
	Tcl_Obj *cmdLine[];// ВСЕГДА ПОСЛЕДНИЙ
} IteratorState;

unsigned long nextId=0;

IteratorState *stateAlloc(int);
void stateFree();

/** стек состояний..приходится слегка повторять внутренности Tcl
**/

IteratorState *statePush(Tcl_Interp *interp,int count);
IteratorState *statePop(Tcl_Interp *interp);
IteratorState *currentState(Tcl_Interp *);

/***
	map tail - (то есть без функции) - просто создать кортеж из tail (который может быть list)
	map ..expression.. tail - применять expression к tail, собирать результат
	------
	@REFACTOR: По факту - это Y комбинатор, надо-бы переименовать..
***/
static 
int iteratorNreStep(ClientData data[],Tcl_Interp *interp,int code);

int mapObjProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]) {
	return Tcl_NRCallObjProc(interp, mapNreProc, 
            data, objc, objv);
}

int mapNreProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[])
{
	register IteratorState *st;	// состояние итератора

	(void)data;

	//// АНАЛИЗ АРГУМЕНТОВ
	if (objc==1) {
		Tcl_WrongNumArgs(interp,objc,objv,"?..expression..? tail");
		return TCL_ERROR;
	}
	if (objc==2) {
		// не заданна функция
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
	// СОЗДАНИЕ СОСТОЯНИЯ
	if ((st=statePush(interp,objc+1))==NULL) {
		return TCL_ERROR;
	}
	st->arg=objv[objc-1];
	
	//Tcl_IncrRefCount(st->arg); // последний аргумент
	if (st->arg->typePtr==tupleType) {
		st->currCons=TUPLE(st->arg)->head;
	} else {
		if (st->arg->typePtr!=listType && Tcl_ConvertToType(interp,st->arg,listType)!=TCL_OK ) {
			ERR("don`t convert %s to %s\n",Tcl_GetString(st->arg),listType->name);
			//return TCL_ERROR;
			goto ERROR; // needs statePop
		}
		if (Tcl_ListObjGetElements(interp,st->arg,&st->listLength,&st->listElements)!=TCL_OK) {
			ERR("can`t acccess to list elements\n");
			//return TCL_ERROR;
			goto ERROR;
		}
		// st->listIndex=0;
	}
	//// ФОРМИРОВАНИЕ st->cmdLine
	// заголовок, используется при вызовах
	st->cmdLine[0]=NULL;	//lazyCmdName;					// глоб. "lazy"

	memcpy(st->cmdLine+1,objv,sizeof(Tcl_Obj *)*(objc-1));
	st->cmd=Tcl_GetCommandFromObj(interp,st->cmdLine[2]);
	if (st->cmd==NULL) {
		ERR("first arg should be a function");
		//return TCL_ERROR;
		goto ERROR;
	}
	//// ПОДГОТОВКА К ПЕРВОМУ ЗАПУСКУ
	// создать объект для приёма результата
	st->ret=tupleObjNew();
	Tcl_IncrRefCount(st->ret);
	// поместить очередной (первый) элемент в конец cmdLine
	if (st->arg->typePtr==tupleType) {
		while(st->currCons!=NULL && st->currCons->obj==NULL) {
			consExpand(st->currCons);
		}
		if (st->currCons!=NULL) {
			*st->cmdLineTail=st->currCons->obj;
			Tcl_NRAddCallback(interp,iteratorNreStep,st,NULL,NULL,*st->cmdLineTail);
			return Tcl_NRCmdSwap(interp,st->cmd, st->cmdLineLen-2,st->cmdLine+2,0);
		}
	} else {
		if (st->listLength!=0) {
			*st->cmdLineTail=st->listElements[st->listIndex];
			Tcl_NRAddCallback(interp,iteratorNreStep,st,NULL,NULL,*st->cmdLineTail);
			return Tcl_NRCmdSwap(interp,st->cmd, st->cmdLineLen-2,st->cmdLine+2,0);
		}
	}
	Tcl_SetObjResult(interp,emptyTuple);
	return TCL_OK;
ERROR:
	statePop(interp);
	return TCL_ERROR;
}

static 
int iteratorNreStep(ClientData data[],Tcl_Interp *interp,int code)
{
	register IteratorState *st;
	Tcl_Obj *result;
	Tcl_Obj *codeOptions=NULL,*valueObj;
	int level=0;
	(void)data;
	//// ПРОТОКОЛ ВЫЗОВА
	//st=(IteratorState *)data[0];	// первый аргумент - состояние
	st=currentState(interp);
	if (st==NULL) {
		ERR("No access to state");
		return TCL_ERROR;
	}
	//currObj=(Tcl_Obj *)data[3];	// последний - текщий объект
	
	result=Tcl_GetObjResult(interp);
	switch(code) {
		case TCL_RETURN:
			codeOptions=Tcl_GetReturnOptions(interp,code);
			INSPECT_OBJ(codeOptions,"FROM RETURN");
			if (Tcl_DictObjGet(interp,codeOptions,levelOptionName,&valueObj)!=TCL_OK) {
				goto ERROR;
			}
			if (Tcl_GetIntFromObj(interp,valueObj,&level)!=TCL_OK) {
				goto ERROR;
			}
			if (level!=0) {
				level--;
				Tcl_SetIntObj(valueObj,level);
				Tcl_DictObjPut(interp,codeOptions,levelOptionName,valueObj);
				break;	
			}
			code=TCL_OK;	// return with level=0
		case TCL_OK :
			// всё хорошо - добавить к общему результату
			tupleObjPut(st->ret,result);
			break;
		case TCL_CONTINUE:
			// сигнал contniue - просто перейти к следующему
			break;
		case TCL_BREAK:
			// сигнал break - прервать итерации
			break;
		case TCL_ERROR:
			// ошибка - result содержит информацию о ней
			break;
		default:
			(void)1;
			// какой-то неясный код заданный юзером
			// TODO: потом завернуть в OOB и добавить к результату
	}
	/// переход к следующей итерации
	if (code==TCL_OK || code==TCL_RETURN) {
		if (st->arg->typePtr==tupleType) {
			st->currCons=st->currCons->next;
			while(st->currCons!=NULL && st->currCons->obj==NULL) {
				consExpand(st->currCons);
			}
			if (st->currCons!=NULL) {
				*st->cmdLineTail=st->currCons->obj;
				Tcl_NRAddCallback(interp,iteratorNreStep,st,NULL,NULL,*st->cmdLineTail);
				return Tcl_NRCmdSwap(interp,st->cmd, st->cmdLineLen-2,st->cmdLine+2,0);
			}
		} else {
			st->listIndex++;
			if (st->listIndex<st->listLength) {
				*st->cmdLineTail=st->listElements[st->listIndex];
				Tcl_NRAddCallback(interp,iteratorNreStep,st,NULL,NULL,*st->cmdLineTail);
				return Tcl_NRCmdSwap(interp,st->cmd, st->cmdLineLen-2,st->cmdLine+2,0);
			}
		}
	}
	/// все итерации завершены, или получен сигнал к завершению
	if (code==TCL_OK || code==TCL_RETURN) {
		if (codeOptions!=NULL) {
			Tcl_SetReturnOptions(interp,codeOptions);
		}
		Tcl_SetObjResult(interp,st->ret);
	}
	//stateFree(st);
	statePop(interp);
	return code;
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
			TRACE("UNABLE TO CREATE NS %lu %s\n",st->id,Tcl_GetStringResult(interp));
			return TCL_ERROR;
		}
	}
	Tcl_Obj *ret;
	ret=Tcl_NewLongObj(st->id);
	Tcl_SetObjResult(interp,ret);
	return TCL_OK;
}
IteratorState *statePush(Tcl_Interp *interp,int count) {
	IteratorState *st,*prevState;
	prevState=(IteratorState *)Tcl_GetAssocData(interp,stateKey,NULL);
	
	st=stateAlloc(count);
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
stateAlloc(int count)
{
	register IteratorState *st;
	st=(IteratorState *)Tcl_Alloc(sizeof(IteratorState)+sizeof(Tcl_Obj *)*count);
	if (st==NULL)
		return NULL;
	memset(st,0,sizeof(IteratorState)+sizeof(Tcl_Obj *)*count);
	st->id=nextId++;
	st->cmdLineLen=count;
	st->cmdLineTail=&st->cmdLine[count-1];
	return st;
}
void
stateFree(register IteratorState *st) {
	if (st==NULL) return;
	//if (st->arg!=NULL) Tcl_DecrRefCount(st->arg);
	if (st->ret!=NULL) Tcl_DecrRefCount(st->ret);
	if (st->privateNs!=NULL) Tcl_DeleteNamespace(st->privateNs);
	Tcl_Free((char *)st);
}
