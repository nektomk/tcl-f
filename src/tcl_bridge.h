#ifndef TCL_BRIDGE_H
#define TCL_BRIBGE_H 1
/** коллекций типов и констант из стандартныго Tcl
**/
int bridgeInited();    // true если всё подцеплено
int initBridgeSubsys(Tcl_Interp *interp);   // инициализация для каждого отдельного интерпретатора
int finiBridgeSubssy
// стандартные типы Tcl
extern const Tcl_ObjType 
	*stringType,
	*boolType,
	*dictType,
	*intType,
	*longType,
	*wideType,
        *lambdaExprType;
// часто исползуемые константы (когда надо вернуть константу и чтобы не плодить лишних объектов)
extern Tcl_Obj
    // "пустые" объекты
    *emptyObj,
    *emptyString,
    *emptyList,
    *emptyDict,
    // булевы константы
    *trueObj,
    *falseObj,
    // числовые константы
    *intZero,
    *intOne,
    *intMinusOne,
    *longZero,
    *longOne,
    *longMinusOne,
    *wideZero,
    *wideOne,
    *wideMinusOne,
    *doubleZero,
    *doubleOne,
    *doubleMinusOne,
    *doubleInf,
    *doubleNaN,
    *doublePi,
    *doubleE;
// часто используемые имена команд (не имеющие простых реализаций в C API)
extern Tcl_Obj
    *applyCmdName,
    *exprCmdName;
// и их токены
extern Tcl_Command *
    *applyCommand,
    *exprCommand;
    

#endif
