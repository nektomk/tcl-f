#ifndef LAZY_H
#define LAZY_H 1

#include <tcl.h>

#include "tuple.h"

typedef struct Lazy {
    Tcl_Obj *command;   // отложенная команда (список)
    Tcl_Interp *interp; // интерпретатор
    Tcl_Namespace *ns;  // пространство имён
    void *stackFrame;   // -- для внутреннего использования в Nre --
} Lazy;

#define LAZY(obj) (*(Lazy **)&(obj)->internalRep.twoPtrValue.ptr1)

int initLazyInstance(Tcl_Interp *interp,Tcl_Namespace *ns);
int initLazySubsys(Tcl_Interp *);

extern const Tcl_ObjType *lazyType;

int lazyObjProc(ClientData cld,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);

int unlazyNreProc(ClientData cld,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
int unlazyObjProc(ClientData cld,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);

//int lazyExec(Tcl_Interp *interp,Tcl_Obj *obj);

Tcl_Obj *lazyObjNew(Tcl_Interp *,int objc,Tcl_Obj *const objv[]);

#endif
