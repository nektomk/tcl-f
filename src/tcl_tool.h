#ifndef TCL_TOOL_H
#define TCL_TOOL_H 1

#include <tcl.h>

// найти позицию строки в массиве
int stringIndexInArray(char *s,int objc,Tcl_Obj **objv); 
#define stringFoundInArray(s,objc,objv) (stringIndexInArray(s,objc,objv) >= 0?1:0)

// найти позицию строки в списке
int stringIndexInList(char *s,Tcl_Obj *listObj);    
#define stringFoundInList(s,listObj) (stringIndexInList(s,obj)>=0)

// определить новый тип или получить дескриптор существующего 
int resolveOrRegisterType(char *typeName,const Tcl_ObjType *myType,const Tcl_ObjType **savePtr);

// создать без-типовый объект (валидное строчное и невалидное внутренне представление
Tcl_Obj *objNew(char *s,int len);
// создать произвольный объект из строки
Tcl_Obj *objFromString(char *typeName,const Tcl_ObjType *type,char *value,int len);
// создать произвольный объект из другого
Tcl_Obj *objFromAny(char *typeName,const Tcl_ObjType *type,Tcl_Obj *valueObj);
// "взять" объект для изменений - если оригинал не шарен, то взять его иначе - копию
Tcl_Obj *takeObj(Tcl_Obj *orig);

//// а-ля константы - то есть дважды зарефенные объекты, которые должны оставаться неизменными
// создать новый "константный оъект" c заданным строкой значением
Tcl_Obj *constObjNew(char *typeName,const Tcl_ObjType *type,char *s);
// создать новый "константный объект" из другого 
int constObjFromAny(char *typeName,const Tcl_ObjType *type,Tcl_Obj **saveConstHere,Tcl_Obj *valueObj);

#define constUnref(constObj) (Tcl_DecrRefCount(constObj),Tcl_DecrRefCount(obj))

// проверить, что объект похож на константу
#define objIsConst(constObj) Tcl_IsShared(constObj)


#endif
