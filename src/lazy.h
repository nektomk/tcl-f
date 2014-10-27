#ifndef LAZY_H
#define LAZY_H 1

#include <tcl.h>

#include "tuple.h"

typedef struct Lazy {
    Tcl_Interp *interp; // интерпретатор
    Tcl_Obj *command;   // отложенная команда (список)
} Lazy;

#define LAZY(obj) ((Lazy *)&((obj)->internalRep))

int initLazySubsys(Tcl_Interp *);

extern const Tcl_ObjType *lazyType;

int lazyObjProc(ClientData cld,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]);
int lazyExec(Tcl_Interp *interp,Tcl_Obj *obj);

Tcl_Obj *lazyObjNew(Tcl_Interp *,int objc,Tcl_Obj *const objv[]);

#endif
