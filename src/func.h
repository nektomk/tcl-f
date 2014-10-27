#ifndef FUNC_H
#define FUNC_H 1

typedef struct Func {
    struct {
        Tcl_Obj *formal;    // формальные аргументы функции
        Tcl_Obj *body;      // тело функции
        Tcl_Obj *nsName;  // пространство имён
    };
    Tcl_Command cmd;    // токен команды
    Tcl_Obj *cmdName;   // полное имя команды
    Tcl_CmdInfo cmdInfo; // инф о исполняемой команде
    Tcl_Namespace *ns;  // пространство имён для исполнения
    Tcl_Obj *curry;     // каррированные аргументы
    //////////
    Tcl_Obj *lambda;    // скомпилированная lambda
    /////////
    Tcl_Obj *cmdLine;
    int cmdLineLen;
} Func;

extern const Tcl_ObjType *funcType;   // собственный тип Tcl

int initFuncSubsys(Tcl_Interp *interp);     // регистация типов и констант
int initFuncInstance(Tcl_Interp *interp,Tcl_Namespace *ns);   // регистрация команд

#define FUNC(obj) (*(Func **)&(obj)->internalRep.twoPtrValue.ptr1)

Tcl_Obj *funcObjNew();  // создать "пустую" функция, для дальнейших funcObjSetXXX

// создание функций из различных объектов
Tcl_Obj *funcObjFromDecl(Tcl_Interp *,Tcl_Obj *formal,Tcl_Obj *body,Tcl_Namespace *ns,int objc,Tcl_Obj *const objv[]);
Tcl_Obj *funcObjFromLambda(Tcl_Interp *,Tcl_Obj *lambda,int objc,Tcl_Obj *const objv[]);
Tcl_Obj *funcObjFromOther(Tcl_Interp *,Tcl_Obj *otherFuncObj,int objc,Tcl_Obj *const objv[]);
Tcl_Obj *funcObjFromArgs(Tcl_Interp *,int objc,Tcl_Obj *const objv[]);

// полностью очистить функцию
int funcObjClear(Tcl_Obj *funcObj);

// различные способы задания функции
int funcObjSetFromDecl(Tcl_Interp *,Tcl_Obj *funcObj,Tcl_Obj *formal,Tcl_Obj *body,Tcl_Namespace *ns,int objc,Tcl_Obj *const objv[]);
int funcObjSetFromArgs(Tcl_Interp *,Tcl_Obj *funcObj,int objc,Tcl_Obj *const objv[]);
int funcObjSetFromLambda(Tcl_Interp *,Tcl_Obj *funcObj,Tcl_Obj *lambda,int objc,Tcl_Obj *const objv[]);
int funcObjSetFromOther(Tcl_Interp *,Tcl_Obj *funcObj,Tcl_Obj *otherFuncObj,int objc,Tcl_Obj *const objv[]);
int funcObjSetFromArgs(Tcl_Interp *,Tcl_Obj *funcObj,int objc,Tcl_Obj *const objv[]);

// добавить (каррировать) аргумент в функцию
int funcObjCurry(Tcl_Interp *,Tcl_Obj *funcObj,int objc,Tcl_Obj *const objv[]);
// исполнить функцию с аргументом
int funcObjCall(Tcl_Interp *,Tcl_Obj *funcObj,int objc,Tcl_Obj *const objv[]);

//// регистрируемые команды Tcl
int funcObjProc(ClientData,Tcl_Interp *,int objc,Tcl_Obj *const objv[]);        // создание функции ```func args```
int callFuncObjProc(ClientData,Tcl_Interp *,int objc,Tcl_Obj *const objv[]);    // вызов/исполнение функции ```call funcObj args``` 

#endif


