#include <tcl.h>

#include "iter.h"
#include "func.h"
#include "call.h"

#include "debug.h"
#include "inspect.h"

static int
chainNreStep(ClientData data[],Tcl_Interp *interp,int code)
{
    Iter *it;
    it=(Iter *)data[0];
    if (code==TCL_OK) {
        Tcl_Obj *funcObj;
        funcObj=iterNext(it);
        if (funcObj!=NULL) {
            Tcl_NRAddCallback(interp,chainNreStep,it,0,0,0);
            return funcObjCallVA(interp,funcObj,Tcl_GetObjResult(interp),NULL);
        }
    }
    iterFree(it);
    return code;
}

int
chainNreProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[])
{
    Iter *it;
    Tcl_Obj *funcObj;
    (void)data;
    if (objc<2) {
        Tcl_WrongNumArgs(interp,objc,objv,"funcList ?args..?");
        return TCL_ERROR;
    }
    if (objv[1]->typePtr==funcType) {
        // всего одна функция в аргументе - нифига не список
        return TCL_ERROR;
    }
    it=iterNew(interp,objv[1],0);
    Tcl_NRAddCallback(interp,chainNreStep,it,0,0,0);
    funcObj=iterFirst(it);
    return funcObjCall(interp,funcObj,objc-2,objv+2);
}

int
chainObjProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]) {
    return Tcl_NRCallObjProc(interp,chainNreProc,data,objc,objv);
}
