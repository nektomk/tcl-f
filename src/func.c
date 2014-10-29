#include <tcl.h>
#include <string.h>

#include "iter.h"
#include "func.h"
#include "call.h"
#include "chain.h"

#include "debug.h"
#include "inspect.h"

extern const Tcl_ObjType *cmdNameType;
extern const Tcl_ObjType *lambdaExprType;

static void funcObj_freeIntRepProc(Tcl_Obj *funcObj);
static void funcObj_dupIntRepProc(Tcl_Obj *funcObj,Tcl_Obj *dstObj);
static void funcObj_updateStringProc(Tcl_Obj *funcObj);
static int  funcObj_setFromAnyProc(Tcl_Interp *,Tcl_Obj *funcObj);

static const Tcl_ObjType funcTypeD = {
    "func",
    funcObj_freeIntRepProc,
    funcObj_dupIntRepProc,
    funcObj_updateStringProc,
    funcObj_setFromAnyProc
};
const Tcl_ObjType *funcType=&funcTypeD;

// Tcl_Obj *funcFromDecl(Tcl_Obj *formal,Tcl_Obj *body,Tcl_Namespace *namespаce,int nrOfCurry,Tcl_Obj **curry);
// Tcl_Obj *funcFromLambda(Tcl_Obj *lambdaExpr,int nrOfCurry,Tcl_Obj **curry);
// Tcl_Obj *funcFromFunc(Tcl_Obj *funcObj,int nrOfCurry,Tcl_Obj **curry);

#if !defined(memdup)
inline void *
memdup(void *src,int count) {
    void *dst;
    dst=Tcl_Alloc(count);
    memcpy(dst,src,count);
    return dst;
}
#endif


void funcUpdateLambda(Func *func) {
    if (func==NULL) return;
    if (func->lambda!=NULL) Tcl_DecrRefCount(func->lambda);
    if (func->nsName!=NULL)
        func->lambda=Tcl_NewListObj(3,&func->formal);
    else
        func->lambda=Tcl_NewListObj(2,&func->formal);
    Tcl_IncrRefCount(func->lambda);        
}

static void funcObjInit(Tcl_Obj *obj) {
    if (obj==NULL) return;
    Tcl_InvalidateStringRep(obj);
    FUNC(obj)=(Func *)Tcl_Alloc(sizeof(Func));
    memset(FUNC(obj),0,sizeof(Func));
    FUNC(obj)->cmdLine=Tcl_NewListObj(0,NULL);
    Tcl_IncrRefCount(FUNC(obj)->cmdLine);
}
static void
funcObj_freeIntRepProc(Tcl_Obj *funcObj) {
    if (funcObjClear(funcObj)==TCL_OK) {
        if (FUNC(funcObj)->cmdLine!=NULL) Tcl_DecrRefCount(FUNC(funcObj)->cmdLine);
        Tcl_Free((char *)FUNC(funcObj));
    }
    funcObj->typePtr=NULL;
}
static void
funcObj_dupIntRepProc(Tcl_Obj *srcObj,Tcl_Obj *dstObj) {
    Func *src,*dst;
    if (srcObj==NULL || srcObj->typePtr!=funcType || dstObj==NULL) return;
    funcObjInit(dstObj);
    src=FUNC(srcObj);
    dst=FUNC(dstObj);
    if (src->formal!=NULL) { dst->formal=src->formal; Tcl_IncrRefCount(dst->formal); }
    if (src->body!=NULL) { dst->body=src->body ; Tcl_IncrRefCount(dst->body); }
    dst->cmd=src->cmd;
    if (src->cmdName!=NULL) { dst->cmdName=src->cmdName; Tcl_IncrRefCount(dst->cmdName); }
    memcpy(&dst->cmdInfo,&src->cmdInfo,sizeof(Tcl_CmdInfo));
    dst->ns=src->ns;
    if (src->curry!=NULL) { dst->curry=src->curry ; Tcl_IncrRefCount(dst->curry); }
    if (src->lambda!=NULL) { dst->lambda=src->lambda; Tcl_IncrRefCount(dst->lambda); }
    if (src->cmdLine!=NULL) {
        dst->cmdLine=Tcl_DuplicateObj(src->cmdLine);
    } else {
        dst->cmdLine=Tcl_NewListObj(0,NULL);
    }
    Tcl_IncrRefCount(dst->cmdLine);
    Tcl_ListObjLength(NULL,dst->cmdLine,&dst->cmdLineLen);
}

static void
funcObj_updateStringProc(Tcl_Obj *funcObj) {
    if (funcObj==NULL)
        return;
    if (funcObj->typePtr!=funcType || FUNC(funcObj)==NULL) {
        funcObj->bytes=Tcl_Alloc(1);
        funcObj->bytes[0]=0;
        funcObj->length=0;
        return;
    }
    funcObj->bytes=Tcl_GetStringFromObj(FUNC(funcObj)->cmdLine,&funcObj->length);
    funcObj->bytes=memdup(funcObj->bytes,funcObj->length+1);
}
int funcObj_setFromAnyProc(Tcl_Interp *interp,Tcl_Obj *obj) {
    Tcl_Obj *list;
    Tcl_Obj **objv;
    int objc;
    if (obj==NULL) return TCL_ERROR;
    FUNC(obj)=(Func *)Tcl_Alloc(sizeof(Func));
    list=Tcl_NewListObj(0,NULL);
    Tcl_IncrRefCount(list);
    list->bytes=obj->bytes;
    list->length=obj->length;
    list->typePtr->setFromAnyProc(interp,list);
    Tcl_ListObjGetElements(interp,list,&objc,&objv);
    funcObjSetFromArgs(interp,obj,objc,objv);
    list->bytes=NULL;
    list->length=0;
    Tcl_DecrRefCount(list);
    return TCL_OK;
}
Tcl_Obj *
funcObjNew()
{
    Tcl_Obj *funcObj;
    funcObj=Tcl_NewObj();
    funcObj->typePtr=funcType;
    funcObj->bytes=NULL;
    funcObj->length=0;
    FUNC(funcObj)=(Func *)Tcl_Alloc(sizeof(Func));
    memset(FUNC(funcObj),0,sizeof(Func));
    FUNC(funcObj)->cmdLine=Tcl_NewListObj(0,NULL);
    Tcl_IncrRefCount(FUNC(funcObj)->cmdLine);
    return funcObj;
}
int funcObjClear(Tcl_Obj *obj) {
    Func *func;
    if (obj==NULL || obj->typePtr!=funcType) return TCL_ERROR;
    func=FUNC(obj);
    if (func->formal!=NULL) Tcl_DecrRefCount(func->formal);
    if (func->body!=NULL) Tcl_DecrRefCount(func->body);
    if (func->nsName!=NULL) Tcl_DecrRefCount(func->nsName);
    if (func->cmdName!=NULL) Tcl_DecrRefCount(func->cmdName);
    if (func->curry!=NULL) Tcl_DecrRefCount(func->curry);
    if (func->lambda!=NULL) Tcl_DecrRefCount(func->lambda);
    if (func->cmdLine!=NULL) Tcl_DecrRefCount(func->cmdLine);
    memset(func,0,sizeof(Func));
    func->cmdLine=Tcl_NewListObj(0,NULL);
    Tcl_IncrRefCount(func->cmdLine);
    return TCL_OK;
}
Tcl_Obj *
funcObjLambda(Tcl_Interp *interp,Tcl_Obj *lambda,int objc,Tcl_Obj *const objv[]) {
    Tcl_Obj *funcObj;
    Func *func;
    funcObj=funcObjNew();
    func=FUNC(funcObj);
    Tcl_ListObjIndex(interp,lambda,0,&func->formal);    
    Tcl_ListObjIndex(interp,lambda,1,&func->body);
    Tcl_ListObjIndex(interp,lambda,2,&func->nsName);
    if (objc>0) {
        func->curry=Tcl_NewListObj(objc,objv);
        Tcl_IncrRefCount(func->curry);
    }
    return funcObj;
}
int
funcSetLambda(Tcl_Interp *interp,Func *func,Tcl_Obj *lambda) {
    Tcl_Obj *nsName;
    Tcl_Namespace *ns;
    int length;
    if (Tcl_ListObjLength(interp,lambda,&length)!=TCL_OK || (length!=2 && length!=3)) {
        ERR("bad lambda");
        return TCL_ERROR;
    }
    if (length==3) {
        // в lambda есть namespace
        Tcl_ListObjIndex(interp,lambda,2,&nsName);
        Tcl_IncrRefCount(nsName);
        ns=Tcl_FindNamespace(interp,Tcl_GetString(nsName),Tcl_GetCurrentNamespace(interp),0);
        Tcl_DecrRefCount(nsName);
        if (ns==NULL) {
            Tcl_SetResult(interp,"unknown namespace in lambda",NULL);
            return TCL_ERROR;
        }
        if (func->ns!=NULL) {
            if (func->ns!=ns) {
                Tcl_SetResult(interp,"namespace conflict between lambda and func",NULL);
                return TCL_ERROR;
            }
            if (func->nsName!=NULL) Tcl_DecrRefCount(func->nsName);
        }
        func->ns=ns;
        func->nsName=Tcl_NewStringObj(func->ns->fullName,-1);
        Tcl_IncrRefCount(func->nsName);
    }
    if (func->body!=NULL)
        Tcl_DecrRefCount(func->body);
    Tcl_ListObjIndex(interp,lambda,1,&func->body);
    Tcl_IncrRefCount(func->body);

    if (func->formal!=NULL)
        Tcl_DecrRefCount(func->formal);
    Tcl_ListObjIndex(interp,lambda,0,&func->formal);
    Tcl_IncrRefCount(func->formal);
    
    if (func->lambda!=NULL)
        Tcl_DecrRefCount(func->lambda);
    if (func->ns==NULL)     
        func->lambda=Tcl_NewListObj(2,&func->formal);
    else
        func->lambda=Tcl_NewListObj(3,&func->formal);
    Tcl_IncrRefCount(func->lambda);
    
    return TCL_OK;
}
int
funcObjSetFromOther(Tcl_Interp *interp,Tcl_Obj *funcObj,Tcl_Namespace *ns,Tcl_Obj *otherFuncObj,int objc,Tcl_Obj *const objv[])
{
    Func *func;
    Func *other;

    if (funcObjClear(funcObj)!=TCL_OK) return TCL_ERROR;

    func=FUNC(funcObj);
    other=FUNC(otherFuncObj);
    
    if (ns!=NULL && other->ns!=NULL && ns!=other->ns) {
        Tcl_SetResult(interp,"namespace conflict in function decl",NULL);
        return TCL_ERROR;
    }
    if (other->formal!=NULL) { func->formal=other->formal; Tcl_IncrRefCount(func->formal); }
    if (other->body!=NULL) { func->body=other->body ; Tcl_IncrRefCount(func->body); }
    func->cmd=other->cmd;
    if (other->cmdName!=NULL) { func->cmdName=other->cmdName; Tcl_IncrRefCount(func->cmdName); }
    memcpy(&func->cmdInfo,&other->cmdInfo,sizeof(Tcl_CmdInfo));
    func->ns=other->ns;
    if (other->curry!=NULL) { func->curry=other->curry ; Tcl_IncrRefCount(func->curry); }
    if (other->lambda!=NULL) { func->lambda=other->lambda; Tcl_IncrRefCount(func->lambda); }
    if (other->cmdLine!=NULL) {
        func->cmdLine=Tcl_DuplicateObj(other->cmdLine);
    } else {
        func->cmdLine=Tcl_NewListObj(0,NULL);
    }
    Tcl_IncrRefCount(func->cmdLine);
    Tcl_ListObjLength(interp,func->cmdLine,&func->cmdLineLen);
    Tcl_ListObjReplace(interp,func->cmdLine,func->cmdLineLen,0,objc,objv);
    func->cmdLineLen+=objc;
    return TCL_OK;
}
int
funcObjSetFromArgs(Tcl_Interp *interp,Tcl_Obj *funcObj,int objc,Tcl_Obj *const objv[])
{
    Func *func;
    int length;
    char *firstArg;
    (void)interp;
    if (funcObjClear(funcObj)!=TCL_OK) return TCL_ERROR;
    func=FUNC(funcObj);
FIRSTARG:    
    if (objv[0]->typePtr==funcType) return funcObjSetFromOther(interp,funcObj,func->ns,objv[0],objc-1,objv+1);
    if (Tcl_ListObjLength(interp,objv[0],&length)!=TCL_OK ) goto ERROR;
    if (length==1) {
        int firstArgLen;
        // первый аргумент - одно единственное слово
        firstArg=Tcl_GetStringFromObj(objv[0],&firstArgLen);
        if (func->ns==NULL && firstArg[0]=='@') {
            // пространство имён заданное через @
            if (firstArg[1]=='\0') {
                // текущее пространство имён
                func->ns=Tcl_GetCurrentNamespace(interp);
            } else {
                func->ns=Tcl_FindNamespace(interp,firstArg+1,Tcl_GetCurrentNamespace(interp),0);
            }
            if (func->ns==NULL) {
                Tcl_SetResult(interp,"unknown namespace",NULL);
                goto ERROR;
            }
            func->nsName=Tcl_NewStringObj(func->ns->fullName,-1);
            Tcl_IncrRefCount(func->nsName);
            objc--;
            objv++;
            goto FIRSTARG;
        }
        //
    }
    if ((func->cmd=Tcl_GetCommandFromObj(interp,objv[0]))!=NULL) {
        int cmdNameLen;
        char *name;
        func->cmdName=Tcl_NewStringObj(NULL,0);
        Tcl_IncrRefCount(func->cmdName);
        Tcl_GetCommandFullName(interp,func->cmd,func->cmdName);
        Tcl_GetCommandInfoFromToken(func->cmd,&func->cmdInfo);
        name=Tcl_GetStringFromObj(func->cmdName,&cmdNameLen);
        if (cmdNameLen==7 && strcmp(name,"::apply")==0) {
            // func ?@namespace? apply lambda ...
            // надо разобрать lambda
            if (funcSetLambda(interp,func,objv[1])!=TCL_OK)
                return TCL_ERROR;
            Tcl_ListObjAppendElement(interp,func->cmdLine,func->cmdName);
            Tcl_ListObjAppendElement(interp,func->cmdLine,func->lambda);
            objc--;
            objv++;
        } else if (cmdNameLen==6 && strcmp(name,"::eval")==0) {
            TRACE("TODO: special rule for eval");
            Tcl_ListObjAppendElement(interp,func->cmdLine,func->cmdName);
        } else if (cmdNameLen==6 && strcmp(name,"::expr")==0) {
            TRACE("TODO: special rule for expr");
            Tcl_ListObjAppendElement(interp,func->cmdLine,func->cmdName);
        } else {
            Tcl_ListObjAppendElement(interp,func->cmdLine,func->cmdName);
        }
        if (objc>1) {
            func->curry=Tcl_NewListObj(objc-1,objv+1);
            Tcl_IncrRefCount(func->curry);
            Tcl_ListObjAppendList(interp,func->cmdLine,func->curry);
        }
        //ToDo: Tcl_ConvertToType(interp,func->cmdName,"cmdName");
    }
    if (func->cmd==NULL) {
        func->formal=objv[0]; Tcl_IncrRefCount(func->formal);
        func->body=objv[1]; Tcl_IncrRefCount(func->body);
        if (objc>2) {
            func->curry=Tcl_NewListObj(objc-2,objv+2);
            Tcl_IncrRefCount(func->curry);
        }
        if (func->nsName!=NULL) {
            func->lambda=Tcl_NewListObj(3,&func->formal);
        } else {
            func->lambda=Tcl_NewListObj(2,&func->formal);
        }
        Tcl_IncrRefCount(func->lambda);
        Tcl_ConvertToType(interp,func->lambda,lambdaExprType);

        func->cmdName=Tcl_NewStringObj("::apply",7);
        Tcl_IncrRefCount(func->cmdName);
        func->cmd=Tcl_GetCommandFromObj(interp,func->cmdName);
        Tcl_GetCommandInfoFromToken(func->cmd,&func->cmdInfo);
        
        Tcl_ListObjAppendElement(interp,func->cmdLine,func->cmdName);
        Tcl_ListObjAppendElement(interp,func->cmdLine,func->lambda);
        if (func->curry!=NULL)
            Tcl_ListObjAppendList(interp,func->cmdLine,func->curry);
    }
    Tcl_ListObjLength(interp,func->cmdLine,&func->cmdLineLen);
    return TCL_OK;
ERROR:
    return TCL_ERROR;
}
Tcl_Obj *funcObjFromArgs(Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]) {
    Tcl_Obj *funcObj;
    funcObj=funcObjNew();
    funcObjSetFromArgs(interp,funcObj,objc,objv);
    return funcObj;
}
// ::f::func procedure
int funcObjProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]) {
    Tcl_Obj *funcObj;
    int code;
    (void) data;
    // func formal body args
    if (objc<3) {
        Tcl_WrongNumArgs(interp,objc,objv,"formal body ?curry..?");
        return TCL_ERROR;
    }
    funcObj=funcObjNew();
    if (funcObj==NULL) {
        return TCL_ERROR;
    }
    code=funcObjSetFromArgs(interp,funcObj,objc-1,objv+1);
    if (code!=TCL_OK)
        return TCL_ERROR;
    Tcl_SetObjResult(interp,funcObj);
    return code;
}

int initFuncSubsys(Tcl_Interp *interp) {
    (void)interp;
    Tcl_RegisterObjType(funcType);
    return TCL_OK;
}
int initFuncInstance(Tcl_Interp *interp,Tcl_Namespace *ns) {
    char name[255];
    if (ns==NULL) ns=Tcl_GetCurrentNamespace(interp);
    snprintf(name,255,"%s::%s",ns->fullName,"func");
    if (Tcl_CreateObjCommand(interp,name,funcObjProc,NULL,NULL)==NULL) {
        ERR("unable to create command %s",name);
        return TCL_ERROR;
    }
    Tcl_Export(interp,ns,"func",0);
    
    snprintf(name,255,"%s::%s",ns->fullName,"call");
    
    if (Tcl_NRCreateCommand(interp,name,callObjProc,callNreProc,NULL,NULL)==NULL) {
        ERR("unable to create NR command %s",name);
        return TCL_ERROR;
    }
    Tcl_Export(interp,ns,"call",0);


    snprintf(name,255,"%s::%s",ns->fullName,"chain");
    if (Tcl_NRCreateCommand(interp,"chain",chainObjProc,chainNreProc,NULL,NULL)==NULL) {
        ERR("unable to create command %s",name);
        return TCL_ERROR;
    }
    Tcl_Export(interp,ns,"chain",0);
    
    return TCL_OK;
}
int
Func_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp,TCL_VERSION,1)==NULL) {
            fprintf(stderr,"Tcl_InitStubs failed for %s\n",TCL_VERSION);
            return TCL_ERROR;		
    }
    if (initFuncSubsys(interp)!=TCL_OK || initFuncInstance(interp,NULL)!=TCL_OK)
        return TCL_ERROR;
    return TCL_OK;
}
