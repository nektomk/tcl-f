#include <tcl.h>
#include <string.h>
#include <tcl-private/generic/tclInt.h>
#include <tcl-private/generic/tclIntDecls.h>

#include "lazy.h"
#include "tuple.h"

#include "debug.h"
#include "inspect.h"

#if !defined(memdup)
// MinGW problem :-)
__attribute__((always_inline))
static inline
void *memdup(const void *orig,size_t size) {
    void *dup;
    if (orig==NULL) return NULL;
    dup=Tcl_Alloc(size);
    if (dup==NULL) return NULL;
    memcpy(dup,orig,size);
    return dup;
}
#endif

extern Tcl_Obj *emptyList;
Tcl_Interp *lazyLastResort=NULL;

int
lazyObjProc(ClientData cld,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[])
{
    Tcl_Obj *result;
    (void)cld;
    if (objc<2) {
        Tcl_WrongNumArgs(interp,objc,objv,"?script?");
        return TCL_ERROR;
    }
    
    // сразу приготовить объект (список) пригодный к исполнению через EvalObjEx
    result=lazyObjNew(interp,objc-1,objv+1);
    if (result==NULL)
        return TCL_ERROR;
    Tcl_SetObjResult(interp,result);
    return TCL_OK;
}
Tcl_Obj *
lazyObjNew(Tcl_Interp *interp,int objc,Tcl_Obj *const objv[])
{
    Tcl_Obj *obj;
    Lazy *lazy;
    obj=Tcl_NewObj();
    obj->typePtr=lazyType;
    obj->bytes=NULL;
    obj->length=0;
    lazy=LAZY(obj)=(Lazy *)Tcl_Alloc(sizeof(Lazy));
    lazy->interp=interp;
    lazy->ns=Tcl_GetCurrentNamespace(interp);
    lazy->command=Tcl_ConcatObj(objc,objv);//Tcl_NewListObj(objc,objv);
    Tcl_IncrRefCount(lazy->command);
    return obj;
}
void
lazyObjDestroy(Tcl_Obj *obj)
{
    if (obj==NULL || obj->typePtr!=lazyType || LAZY(obj)==NULL) return;
    if (LAZY(obj)->command!=NULL) Tcl_DecrRefCount(LAZY(obj)->command);
    Tcl_Free((char *)LAZY(obj));
    LAZY(obj)=NULL;
    obj->typePtr=NULL;
}
// TODO: должно возвращать значение
int
lazyExec(Tcl_Interp *interp,Tcl_Obj *obj)
{
    Lazy *lazy;
    Tcl_Obj *result;
    Tcl_InterpState save;
    int code;
    if (obj->typePtr!=lazyType) {
        ERR("Should be lazy!");
        return TCL_ERROR;
    }

    lazy=LAZY(obj);
    if (interp==NULL) {
        //TRACE("lazy interp=%p",interp);
        if (!Tcl_InterpDeleted(lazy->interp)) {
            interp=lazy->interp;
        } else if (lazyLastResort!=NULL) {
            interp=lazyLastResort;
        } else {
            Tcl_DecrRefCount(lazy->command);
            obj->bytes=Tcl_Alloc(1);
            obj->bytes[0]=0;
            obj->length=0;
            obj->typePtr=NULL;
            return TCL_OK;
        }
    }
    save=NULL;
//    save=Tcl_SaveInterpState(interp,0);
    do {
        Tcl_CallFrame *frame=0;
        Tcl_InterpState state;
        state=Tcl_SaveInterpState(lazy->interp,TCL_OK);
        TclPushStackFrame(lazy->interp,&frame,lazy->ns,0);
        code=Tcl_EvalObjEx(lazy->interp,lazy->command,TCL_EVAL_DIRECT);
        result=Tcl_GetObjResult(lazy->interp);
        Tcl_IncrRefCount(result);
        TclPopStackFrame(lazy->interp);
        Tcl_RestoreInterpState(lazy->interp,state);
        //Tcl_DecrRefCount(lazy->command);
        lazyObjDestroy(obj);
        if (code==TCL_OK || code==TCL_RETURN) {
            if (!Tcl_IsShared(result) && result->typePtr==tupleType) {
                Tuple *tuple;
                tuple=TUPLE(result);
                tupleObjInit(obj,tuple->head,tuple->tail);
                tupleObjInit(result,NULL,NULL);
            } else if (result->typePtr!=NULL && result->typePtr->dupIntRepProc!=NULL) {
                obj->typePtr=NULL;
                result->typePtr->dupIntRepProc(result,obj);
                obj->typePtr=result->typePtr;
            } else {
                obj->typePtr=NULL;
                obj->bytes=Tcl_GetStringFromObj(result,&obj->length);
                if (Tcl_IsShared(result)) {
                    obj->bytes=memdup(obj->bytes,obj->length+1);
                } else {
                    result->bytes=NULL;
                    result->length=0;
                }
            }
            
        } else {
            //inspect(result);
            obj->typePtr=NULL;
            obj->bytes=Tcl_Alloc(1);
            obj->bytes[0]=0;
            obj->length=0;
            obj->typePtr=NULL;
            break;
        }
        //Tcl_DecrRefCount(tmp);
        Tcl_DecrRefCount(result);
        //Tcl_ResetResult(interp);
    } while(obj->typePtr==lazyType);
    if (save!=NULL) {
        Tcl_RestoreInterpState(interp,save);
        Tcl_DiscardInterpState(save); 
    } else {
        Tcl_ResetResult(interp);
    }
    Tcl_DecrRefCount(result);
    return code;
}
static void
lazy_freeInternalRepProc(Tcl_Obj *obj) {
    Lazy *lazy;
    lazy=LAZY(obj);
    if (lazy->command!=NULL) Tcl_DecrRefCount(lazy->command);
}
static void
lazy_dupInternalRepProc(Tcl_Obj *obj,Tcl_Obj *dup) {
    lazyExec(NULL,obj);
    if (obj->typePtr->dupIntRepProc!=NULL) {
        obj->typePtr->dupIntRepProc(obj,dup);
    }
}
static void
lazy_updateStringProc(Tcl_Obj *obj) {
    unlazyObjProc(obj,LAZY(obj)->interp,0,NULL);
    //lazyExec(LAZY(obj)->interp,obj);
    if (obj->bytes==NULL && obj->typePtr!=NULL && obj->typePtr->updateStringProc!=NULL) {
        obj->typePtr->updateStringProc(obj);
    }
}
int
unlazyNreCallback(ClientData data[],Tcl_Interp *interp,int code)
{
    Tcl_Obj *lazyObj;
    Tcl_Obj *result;
    Lazy *lazy;
    result=Tcl_GetObjResult(interp);
    lazyObj=data[0];
    lazy=LAZY(lazyObj);
    Tcl_InvalidateStringRep(lazyObj);
    lazyObj->bytes=Tcl_GetStringFromObj(result,&lazyObj->length);
    lazyObj->bytes=memdup(lazyObj->bytes,lazyObj->length+1);
    if (lazy->command!=NULL) Tcl_DecrRefCount(lazy->command);
    lazyObj->typePtr=NULL;
    Tcl_SetObjResult(interp,result);
    return code;
}

int
unlazyNreProc(ClientData data,Tcl_Interp *interp,int objc, Tcl_Obj * const objv[])
{
    Tcl_Obj *lazyObj;
    Lazy *lazy;
    if (data!=NULL) {
        // call from internals
        lazyObj=(Tcl_Obj *)data;
    } else {
        if (objc!=2) {
            // call from user
            Tcl_WrongNumArgs(interp,objc,objv,"lazyObj");
            return TCL_ERROR;
        }
        lazyObj=objv[1];
    }
    if (lazyObj->typePtr!=lazyType) {
        Tcl_SetObjResult(interp,objv[1]);
        return TCL_OK;
    }
    lazy=LAZY(lazyObj);
    Tcl_NRAddCallback(interp,unlazyNreCallback,lazyObj,NULL,NULL,NULL);
    return Tcl_NREvalObj(interp,lazy->command,0);
}
int unlazyObjProc(ClientData data,Tcl_Interp *interp,int objc, Tcl_Obj * const objv[])
{
    return Tcl_NRCallObjProc(interp,unlazyNreProc,data,objc,objv);    
}


Tcl_ObjType const lazyTypeD = {
    "lazy",
    lazy_freeInternalRepProc,
    lazy_dupInternalRepProc,
    lazy_updateStringProc,
    NULL //lazy_setFromAnyProc
};
const Tcl_ObjType *lazyType=&lazyTypeD;

int initLazyInstance(Tcl_Interp *interp,Tcl_Namespace *ns) {
    (void)interp;
    (void)ns;
    //Tcl_NRCreateCommand(interp,"::f::unlazy",unlazyObjProc,unlazyNreProc,NULL,NULL);
    return TCL_OK;    
}
int initLazySubsys(Tcl_Interp *interp) {
    (void)interp;
    Tcl_RegisterObjType(lazyType);
    return TCL_OK;
    if (lazyLastResort==NULL) {
        lazyLastResort=Tcl_CreateInterp();
        //lazyLastResort=Tcl_CreateSlave(interp,"lazy",0);
        Tcl_Eval(lazyLastResort,"package require f");
        Tcl_Eval(lazyLastResort,"namespace import ::f::*");
   }
    //Tcl_CallWhenDeleted(interp, onInterpDelete,NULL);
    return TCL_OK;
}
