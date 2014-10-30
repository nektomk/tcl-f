#include <tcl.h>

#include "inspect.h"
#include "tuple.h"
#include "func.h"
#include "map.h"
#include "chain.h"
#include "lazy.h"
#include "call.h"

#include "debug.h"

const Tcl_ObjType
        *listType=NULL,
	*stringType=NULL,
	*boolType=NULL,
	*dictType=NULL,
	*intType=NULL,
	*longType=NULL,
	*wideType=NULL,
	// специальные типы
	*lambdaExprType=NULL,
	*cmdNameType=NULL
	// собственные типы
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
void determineTypes(Tcl_Interp *interp) {
    Tcl_Obj *obj[5];
    obj[0]=Tcl_NewStringObj("::apply",-1);
    obj[1]=Tcl_NewStringObj("x { incr x}",-1);
    obj[2]=Tcl_NewStringObj("5",-1);
    Tcl_IncrRefCount(obj[0]);
    Tcl_IncrRefCount(obj[1]);
    Tcl_IncrRefCount(obj[2]);
    Tcl_EvalObjv(interp,3,obj,0);
    cmdNameType=obj[0]->typePtr;
    lambdaExprType=obj[1]->typePtr;
    Tcl_DecrRefCount(obj[0]);
    Tcl_DecrRefCount(obj[1]);
    Tcl_DecrRefCount(obj[2]);
}
int
F_Init(Tcl_Interp *interp)
{
    Tcl_Namespace *ns;
    if (Tcl_InitStubs(interp,TCL_VERSION,0)==NULL) {
        return TCL_ERROR;    
    }
    // стандартные типы
    listType=Tcl_GetObjType("list");
    stringType=Tcl_GetObjType("string");
    boolType=Tcl_GetObjType("boolean");
    dictType=Tcl_GetObjType("dict");
    intType=Tcl_GetObjType("int");
    longType=Tcl_GetObjType("long");
    wideType=Tcl_GetObjType("wide");
    lambdaExprType=Tcl_GetObjType("lambdaExpr");
    cmdNameType=Tcl_GetObjType("cmdName");
    determineTypes(interp);
    // часто используемые константы
    emptyObj=Tcl_NewObj(); Tcl_IncrRefCount(emptyObj); Tcl_IncrRefCount(emptyObj);
    emptyString=Tcl_NewStringObj(NULL,0);Tcl_IncrRefCount(emptyString);Tcl_IncrRefCount(emptyString);
    emptyList=Tcl_NewListObj(0,NULL);Tcl_IncrRefCount(emptyList);Tcl_IncrRefCount(emptyList);
    emptyDict=Tcl_NewDictObj(); Tcl_IncrRefCount(emptyDict);Tcl_IncrRefCount(emptyDict);
    trueObj=Tcl_NewBooleanObj(1);Tcl_IncrRefCount(trueObj);Tcl_IncrRefCount(trueObj);
    falseObj=Tcl_NewBooleanObj(0);Tcl_IncrRefCount(falseObj);Tcl_IncrRefCount(falseObj);
    // регистрация собственных типов
    Tcl_RegisterObjType(tupleType);
    Tcl_RegisterObjType(funcType);
    Tcl_RegisterObjType(lazyType);
    // пространство имён "::f::"
    ns=Tcl_CreateNamespace(interp,"::f::",NULL,NULL);
    Tcl_Export(interp,ns,"*",0);
    // процедуры
    if (Tcl_CreateObjCommand(interp,"::f::tuple",tupleObjProc,NULL,NULL)==NULL) {
	ERR("on create command %s",Tcl_GetStringResult(interp));
    }
    Tcl_CreateObjCommand(interp,"::f::func",funcObjProc,NULL,NULL);
    Tcl_CreateObjCommand(interp,"::f::inspect",inspectObjProc,NULL,NULL);
    Tcl_CreateObjCommand(interp,"::f::lazy",lazyObjProc,NULL,NULL);
    // Nre
    Tcl_NRCreateCommand(interp,"::f::map",mapNreProc,mapObjProc,NULL,NULL);
    Tcl_NRCreateCommand(interp,"::f::chain",chainNreProc,chainObjProc,NULL,NULL);
    Tcl_NRCreateCommand(interp,"::f::call",callNreProc,callObjProc,NULL,NULL);
    
    return TCL_OK;
}
