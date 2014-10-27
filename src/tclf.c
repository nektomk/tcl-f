#include <tcl.h>
#include "tclf.h"
#include "tuple.h"
#include "lazy.h"
#include "func.h"
#include "inspect.h"

#define NS "::f::"
#define CORE "::"
static char *packageNsName=NS;

const char *stateKey="::f::stateKey";	// TODO: generate unique random

Tcl_Namespace *packageNamespace=NULL;

Tcl_Obj *applyCmdName=NULL,*evalCmdName=NULL;
Tcl_Obj *mapCmdName=NULL,*tupleCmdName=NULL;

Tcl_Command applyCmdToken=NULL,evalCmdToken=NULL;
Tcl_Command mapCmdToken=NULL,tupleCmdToken=NULL;
// определяемые типы Tcl
const Tcl_ObjType *listType;
	// стандартные типы
const Tcl_ObjType 
	*stringType,
	*boolType,
	*dictType,
	*intType,
	*longType,
	*wideType,
	// специальные типы
	*lambdaExprType,
	*cmdNameType,
	// собственные типы
	*tupleType
	;

// константы
Tcl_Obj
    // "пустые" объекты
	*emptyObj=NULL,
	*emptyString=NULL,
	*emptyList=NULL,
	*emptyDict=NULL,
	*emptyTuple=NULL,
    // булевы константы
	*trueObj=NULL,
	*falseObj=NULL,
    // часто используемые строки
	*levelOptionName=NULL    // "-level" 
    ;

#define EXPORT 1
#define NOEXPORT 0

int stateidObjProc(ClientData,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
// регистрируемые команды
static struct CommandEntry {
	char *ns;
	char *name;
	int mark;
	int (*objProc)(ClientData,Tcl_Interp *,int,Tcl_Obj *const[]);
	int (*nreProc)(ClientData,Tcl_Interp *,int,Tcl_Obj *const[]);
	// для копий во внешние переменные
	Tcl_Obj **saveNamePtr;
	Tcl_Command *saveTokenPtr;
	// заполяются при успешной регистрации:
	Tcl_Obj *nameObj;
	Tcl_Command token;
} installTable[] = {
	{ NS,"map", EXPORT,mapObjProc,mapNreProc,&mapCmdName,&mapCmdToken,NULL,NULL },
	{ NS,"tuple", EXPORT,tupleObjProc,NULL,&tupleCmdName,&tupleCmdToken,NULL,NULL },
	{ NS,"stateid",EXPORT,stateidObjProc,NULL,NULL,NULL,NULL,NULL },
	{ NS,"lazy",EXPORT,lazyObjProc,NULL,NULL,NULL,NULL,NULL},
	{ NS,"inspect",EXPORT,inspectObjProc,NULL,NULL,NULL,NULL,NULL},
	{ 0 }
}, resolveTable[] = {
	{ CORE,"apply",0,NULL,NULL,&applyCmdName,&applyCmdToken,NULL,NULL},
	{ CORE,"eval",0,NULL,NULL,&evalCmdName,&evalCmdToken,NULL,NULL},
	{ 0 }
};
static struct ConstEntry {
	char *name;
	Tcl_Obj **objPtr;
	Tcl_ObjType const **typePtr;
	char *value;
} constTable[]={
	// "пустые" объекты - когда надо что-то там вернуть
	{"emptyObj",&emptyObj,NULL,NULL},
	{"emptyString",&emptyString,NULL,""},
	{"emptyList",&emptyList,&listType,NULL},
	{"emptyDict",&emptyDict,&dictType,NULL},
	{"emptyTuple",&emptyTuple,&tupleType,NULL},
	// булевы константы
	{"trueObj",&trueObj,&boolType,"true"},
	{"falseObj",&falseObj,&boolType,"false"},
	// часто исползуемые строки
	{"levelOptionName",&levelOptionName,NULL,"-level"},
	{0}
};
static int
installConsts(Tcl_Interp *interp,struct ConstEntry *table) {
	struct ConstEntry *entry;
	if (table==NULL) return TCL_ERROR;
	for(entry=table+0;entry->objPtr!=NULL;entry++) {
		Tcl_Obj *obj;
		if (*entry->objPtr!=NULL) {
			//WARN("const %s already defined\n",entry->name);
			continue;
		}
		if (entry->value==NULL)
			obj=Tcl_NewObj();
		else
			obj=Tcl_NewStringObj(entry->value,-1);
		Tcl_IncrRefCount(obj);
		if (entry->typePtr!=NULL && *entry->typePtr!=NULL) {
			if (Tcl_ConvertToType(interp,obj,*entry->typePtr)!=TCL_OK) {
				ERR("in convert const %s to %s",entry->name,(*entry->typePtr)->name);
				Tcl_DecrRefCount(obj);
				return TCL_ERROR;
			}
		}
		Tcl_IncrRefCount(obj);
		*entry->objPtr=obj;
	}
	return TCL_OK;
}
static int
resolveCommands(Tcl_Interp *interp,struct CommandEntry *table)
{
	struct CommandEntry *entry;
	if (table==NULL) return TCL_ERROR;
	for(entry=table+0;entry->name!=NULL;entry++) {
		entry->nameObj=Tcl_NewStringObj(entry->name,-1);
		Tcl_IncrRefCount(entry->nameObj);
		entry->token=Tcl_GetCommandFromObj(interp,entry->nameObj);
		if (entry->token==NULL) {
			ERR("Unresolved %s",entry->name);
			return TCL_ERROR;
		}
		if (entry->saveNamePtr!=NULL) {
			*entry->saveNamePtr=entry->nameObj;
			Tcl_IncrRefCount(entry->nameObj);
		}
		if (entry->saveTokenPtr!=NULL) {
			*entry->saveTokenPtr=entry->token;
		}
	}
	return TCL_OK;
}
static int
installCommands(Tcl_Interp *interp,struct CommandEntry *table)
{	
	struct CommandEntry *entry;
	char fqn[128];
	// регистрация всех команд в таблице
	for(entry=table+0;entry->name!=NULL;entry++) {
		// регистрация отдельной команды
		// должна быть заданна хоть одна процедура
		if (entry->objProc==NULL && entry->nreProc==NULL) {
			WARN("command %s not fully declared",entry->name);
			continue;
		}
		// полное имя, включая namespace
		snprintf(fqn,128,"%s%s",entry->ns,entry->name);
		//
		if (entry->nreProc!=NULL) {
			// регистрация как NRE 
			entry->token=Tcl_NRCreateCommand(interp,fqn,entry->objProc,entry->nreProc,entry,NULL);
		} else {
			// регистрация как обычной процедуры
			entry->token=Tcl_CreateObjCommand(interp,fqn,entry->objProc,entry,NULL);
		}
		if (entry->token==NULL) {
			ERR("unable to register command %s\n",fqn);
			return TCL_ERROR;
		}
		// создать объект с именем команды
		entry->nameObj=Tcl_NewStringObj(fqn,-1);
		if (entry->nameObj==NULL) {
			return TCL_ERROR; // ENOMEM
		}
		Tcl_IncrRefCount(entry->nameObj);
		// ... возможны дальнейшие проверки ...
		// сохранение результата во внешних переменных
		if (entry->saveTokenPtr!=NULL) *entry->saveTokenPtr=entry->token;
		if (entry->saveNamePtr!=NULL) { *entry->saveNamePtr=entry->nameObj; Tcl_IncrRefCount(entry->nameObj); }
		// разрешить/нет экспорт
		if (entry->mark) {
			if (Tcl_Export(interp,packageNamespace,entry->name,0)!=TCL_OK) {
				WARN("unable to export %s : %s",entry->name,Tcl_GetStringResult(interp));
			}
		}
	}
	return TCL_OK;
}
static int
resolveTypes(Tcl_Interp *interp) {
	Tcl_Obj *objv[6];
	Tcl_Obj *list;
	Tcl_Obj *dict;
	// определение типа lambdaExpr
	objv[0]=Tcl_NewStringObj("apply",-1); 
	objv[1]=Tcl_NewStringObj("x { expr $x + 1}",-1);
	objv[2]=Tcl_NewIntObj(1),-1;
	Tcl_IncrRefCount(objv[0]);
	Tcl_IncrRefCount(objv[1]);
	Tcl_IncrRefCount(objv[2]);
	if (Tcl_EvalObjv(interp,3,objv,0)!=TCL_OK) {
		ERR("in call apply");
		INSPECT_ARRAY(-1,3,objv,"command");
		return TCL_ERROR;
	}
	cmdNameType=objv[0]->typePtr;
	lambdaExprType=objv[1]->typePtr;
	// определение типа List
	
	list=Tcl_NewListObj(3,objv);
	listType=list->typePtr;
	if (listType==NULL || listType->name==NULL || listType->name[0]=='\0') {
		ERR("in resolve listType");
		return TCL_ERROR;
	}
	Tcl_DecrRefCount(list);
	Tcl_DecrRefCount(objv[0]);
	Tcl_DecrRefCount(objv[1]);
	Tcl_DecrRefCount(objv[2]);
	// определение типа dict
	dict=Tcl_NewDictObj();
	if (dict==NULL || dict->typePtr==NULL) {
		ERR("in resolve dictType");
		return TCL_ERROR;
	}
	dictType=dict->typePtr;
	return TCL_OK;
}
static int nrOfInstances=0;
int
Tclf_Init(Tcl_Interp *interp)
{	// Инициализация библиотеки
	const char *version;
	if ((version=Tcl_InitStubs(interp,TCL_VERSION,1))==NULL) {
		fprintf(stderr,"Tcl_InitStubs failed for %s\n",TCL_VERSION);
		return TCL_ERROR;		
	}
	// создание namespace
	packageNamespace=Tcl_CreateNamespace(interp,packageNsName,NULL,NULL);
	if (packageNamespace==NULL) {
		ERR("in Tcl_CreateNamespace");
		return TCL_ERROR;
	}
	// создание типов, объектов и констант делается раз и навсегда
	if (nrOfInstances==0) {
		if (initTupleSubsys(interp)!=TCL_OK) {
			ERR("on Tuple init");
			return TCL_ERROR;
		}
		if (initFuncSubsys(interp)!=TCL_OK) {
			ERR("on Func init");
			return TCL_ERROR;
		}
		if (initLazySubsys(interp)!=TCL_OK) {
			ERR("on Lazy init");
			return TCL_ERROR;
		}
		nrOfInstances++;
		if (resolveTypes(interp)!=TCL_OK) {
			ERR("resolve types");
			return TCL_ERROR;
		}
		if (installConsts(interp,constTable)!=TCL_OK) {
			ERR("install consts");
			return TCL_ERROR;
		}
	}
	// а вот команды разные у каждого интерпретатора
	if (resolveCommands(interp,resolveTable)!=TCL_OK) {
		ERR("resolve commands");
		return TCL_ERROR;
	}
	if (installCommands(interp,installTable)!=TCL_OK) {
		ERR("install commands");
		return TCL_ERROR;
	}
	if (initFuncInstance(interp,packageNamespace)!=TCL_OK)
		return TCL_ERROR;
	return TCL_OK;
}
int
F_Init(Tcl_Interp *interp) {
	return Tclf_Init(interp);
}
