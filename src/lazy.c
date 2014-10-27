#include <tcl.h>
#include <string.h>
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
    lazy=LAZY(obj);
    lazy->interp=interp;
    lazy->command=Tcl_NewListObj(objc,objv);
    Tcl_IncrRefCount(lazy->command);
    return obj;
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
            interp=interp;
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
        code=Tcl_EvalObjEx(interp,lazy->command,0);
        result=Tcl_GetObjResult(interp);
        Tcl_DecrRefCount(lazy->command);
        lazy->command=NULL;
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
            ERR("IN LAZY EXEC");
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
        //Tcl_ResetResult(interp);
    }
    return code;
}
static void lazy_freeInternalRepProc(Tcl_Obj *obj) {
    Lazy *lazy;
    lazy=LAZY(obj);
    if (lazy->command!=NULL) Tcl_DecrRefCount(lazy->command);
}
static void lazy_dupInternalRepProc(Tcl_Obj *obj,Tcl_Obj *dup) {
    lazyExec(NULL,obj);
    if (obj->typePtr->dupIntRepProc!=NULL) {
        obj->typePtr->dupIntRepProc(obj,dup);
    }
}
static void lazy_updateStringProc(Tcl_Obj *obj) {
    lazyExec(NULL,obj);
    if (obj->bytes==NULL && obj->typePtr!=NULL && obj->typePtr->updateStringProc!=NULL) {
        obj->typePtr->updateStringProc(obj);
    }
}

Tcl_ObjType const lazyTypeD = {
    "lazy",
    lazy_freeInternalRepProc,
    lazy_dupInternalRepProc,
    lazy_updateStringProc,
    NULL //lazy_setFromAnyProc
};
const Tcl_ObjType *lazyType=&lazyTypeD;

static void onInterpDelete(ClientData data,Tcl_Interp *interp) {
    ERR("Interp delete - create last-resort for lazy");
    (void)data;
    (void)interp;
    if (lazyLastResort==NULL) {
        lazyLastResort=Tcl_CreateInterp();
        Tcl_Eval(lazyLastResort,"package require f");
        Tcl_Eval(lazyLastResort,"namespace import ::f::*");
    }
}

int initLazySubsys(Tcl_Interp *interp) {
    (void)interp;
    Tcl_RegisterObjType(lazyType);
    if (lazyLastResort==NULL) {
        lazyLastResort=Tcl_CreateInterp();
        Tcl_Eval(lazyLastResort,"package require f");
        Tcl_Eval(lazyLastResort,"namespace import ::f::*");
    }
//    Tcl_CallWhenDeleted(interp, onInterpDelete,NULL);
    return TCL_OK;
}
