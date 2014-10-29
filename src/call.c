#include <tcl.h>
#include <stdarg.h>

#include "call.h"
#include "func.h"

#include "debug.h"
#include "inspect.h"

/* --- funcObjCall(interp,funcListOrTuple,obj,objv) ---
   исполнить функцию с дополнительными параметрами,
   параметры передаются в массиве
   ---
*/    
int
funcObjCall(Tcl_Interp *interp,Tcl_Obj *funcObj,int objc,Tcl_Obj *const objv[])
{
    Func *func; // функция для исполнения
    int code;   // код результата от неё
    Tcl_Obj **cmd_objv; // её полые параметры
    int cmd_objc;   // и их кол-во
    SHOULD(funcObj!=NULL,return TCL_ERROR);
    SHOULD(funcObj->typePtr==funcType || Tcl_ConvertToType(interp,funcObj,funcType)==TCL_OK,return TCL_ERROR);
    func=FUNC(funcObj);
    Tcl_ListObjReplace(interp,func->cmdLine,func->cmdLineLen,0,objc,objv); // append args to cmdLine
    Tcl_ListObjGetElements(interp,func->cmdLine,&cmd_objc,&cmd_objv);      // прямой доступ к аргуметам для вызова
    if (func->cmd!=NULL) {
        code=func->cmdInfo.objProc(func->cmdInfo.objClientData,interp,cmd_objc,cmd_objv);
        //return Tcl_NRCmdSwap(interp,func->cmd,cmd_objc,cmd_objv,0);
    } else {
        code=Tcl_EvalObjv(interp,cmd_objc,cmd_objv,0);
        //return Tcl_NREvalObjv(interp,cmd_objc,cmd_objv,0);
    }
    Tcl_ListObjReplace(interp,func->cmdLine,func->cmdLineLen,objc,0,NULL); // remove args from cmdLine
    return code;
}
/** --- funcObjCallVA(interp,funcObj,...) ---
    исполнить функцию с доп.параметрами,
    параметры передаются в аргументах (через стек)
    ---
    прим.: единственный разумный способ передать результат одной функции в параметры другой
*/    
int
funcObjCallVA(Tcl_Interp *interp,Tcl_Obj *funcObj,...)
{
    Func *func; // функция для исполнения
    Tcl_Obj **cmd_objv; // полые параметры
    int cmd_objc;   // и их кол-во
    int code;   // код результата от неё
    Tcl_Obj *arg;   // следующий добавляемый аргумент
    int argc;   // всего добавлено аргументов
    va_list ap;
    
    SHOULD(funcObj!=NULL,return TCL_ERROR);
    SHOULD(funcObj->typePtr==funcType || Tcl_ConvertToType(interp,funcObj,funcType)==TCL_OK,return TCL_ERROR);
    func=FUNC(funcObj);
    va_start(ap,funcObj);
    argc=0;
    // формирование массива аргументов
    while((arg=va_arg(ap,Tcl_Obj *))!=NULL) {
        Tcl_ListObjAppendElement(interp,func->cmdLine,arg);
        argc++;
    }
    va_end(ap);
    Tcl_ListObjGetElements(interp,func->cmdLine,&cmd_objc,&cmd_objv);      // прямой доступ к аргуметам для вызова
    // собственно вызов
    if (func->cmd!=NULL) {
        code=func->cmdInfo.objProc(func->cmdInfo.objClientData,interp,cmd_objc,cmd_objv);
        //return Tcl_NRCmdSwap(interp,func->cmd,cmd_objc,cmd_objv,0);
    } else {
        code=Tcl_EvalObjv(interp,cmd_objc,cmd_objv,0);
        //return Tcl_NREvalObjv(interp,cmd_objc,cmd_objv,0);
    }
    // удалить добавленные аргументы
    Tcl_ListObjReplace(interp,func->cmdLine,func->cmdLineLen,argc,0,NULL);
    return code;
}

// ::f::call procedure
int callNreProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[])
{
    int code;
    (void)data;
    if (objc<2) {
        Tcl_WrongNumArgs(interp,objc,objv,"{functions} ?args..?");
        return TCL_ERROR;
    }
    if (objv[1]->typePtr!=funcType) {
        Tcl_Obj *funcObj;
        funcObj=funcObjFromArgs(interp,objc-1,objv+1);
        if (funcObj!=NULL) {
            Tcl_SetResult(interp,"not a function",NULL);
            return TCL_ERROR;
        };
        Tcl_IncrRefCount(funcObj);
        code=funcObjCall(interp,funcObj,0,NULL);
        Tcl_DecrRefCount(funcObj);
    } else {
        code=funcObjCall(interp,objv[1],objc-2,objv+2);
    }
    return code;
    
}
int callObjProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[])
{
    return Tcl_NRCallObjProc(interp,callNreProc,data,objc,objv);
}
