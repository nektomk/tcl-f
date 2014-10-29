#ifndef CALL_H
#define CALL_H 1

#include <tcl.h>

// call function object with argumnets
int funcObjCall(Tcl_Interp *interp,Tcl_Obj *funcObj,int objc,Tcl_Obj *const objv[]);
int funcObjCallVA(Tcl_Interp *interp,Tcl_Obj *funcObj,...);
/* вызвать функцию с аргументами через ClientData
   data[0] - вызываемая функция
   data[1]..data[3] - аргументы
   ---
   
*/   
int funcObjCallNR(ClientData,Tcl_Interp *,int objc,Tcl_Obj *const objv[]);

// ::f::call procedure
int callObjProc(ClientData,Tcl_Interp *,int objc,Tcl_Obj *const objv[]);
int callNreProc(ClientData,Tcl_Interp *,int objc,Tcl_Obj *const objv[]);
#endif
