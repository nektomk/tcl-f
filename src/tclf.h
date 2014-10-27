#ifndef TCLF_H
#define TCLF_H 1

#include <tcl.h>

#include "debug.h"
#include "inspect.h"

int initTupleSubsys(Tcl_Interp *interp);

/** Итератор map
	map formal body args tail
*/
int mapObjProc(ClientData,Tcl_Interp *,int objc,Tcl_Obj *const objv[]);	// объектный map
int mapNreProc(ClientData,Tcl_Interp *,int objc,Tcl_Obj *const objv[]);	// не-рекурсивный map

// глобальные переменныe
extern Tcl_Obj *applyCmdName,
    *evalCmdName;
extern Tcl_Command applyCmdToken,
    evalCmdToken;
// ключ!!
extern const char *stateKey;
// определяемые типы Tcl
extern const Tcl_ObjType *listType;
	// стандартные типы
extern const Tcl_ObjType 
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
extern Tcl_Obj
    // "пустые" объекты
    *emptyObj,
    *emptyString,
    *emptyList,
    *emptyDict,
    *emptyTuple,
    // булевы константы
    *trueObj,
    *falseObj,
    // часто используемые строки
    *levelOptionName    // "-level" 
    ;
#endif

