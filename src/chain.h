#ifndef CHAIN_H
#define CHAIN_H 1

// call chain of functions
// pass arguments to first function, pass function result to next, return result of last functions
int funcObjChain(Tcl_Interp *interp,Tcl_Obj *funcObjList,int objc,Tcl_Obj * const objv[]);
int funcObjChainVA(Tcl_Interp *interp,Tcl_Obj *funcObjList,...);

// ::f::chain procedure
int chainNreProc(ClientData,Tcl_Interp *interp,int objc,Tcl_Obj * const objv[]);
int chainObjProc(ClientData,Tcl_Interp *interp,int objc,Tcl_Obj * const objv[]);
#endif
