#include <tcl.h>

#include "tcl_bridge.h"

static long bridgeInstances=0;

typedef struct TypeDecl {
    char *name;     
    const Tcl_ObjType **resolvePtr;
    const Tcl_ObjType *type;
} TypeDecl;

typedef struct ConstDecl {
    Tcl_Obj **savePtr; // куда записать константу
    const Tcl_ObjType *type;    // тип константы
    char *typeName; // имя типа (если не задан type)
    char *value;
} ConstDecl;


static int bridgeTypes(Tcl_Interp *interp,int count,TypeDecl *tab);
static int bridgeConsts(Tcl_Interp *interp,int count,ConstDecl *tab);

int initBridgeSubsys(Tcl_Interp *interp) {
    if (bridgeInstances==0) {
        bridgeTypes(interp,-1,TypeResolver *);
    }
}


int
bridgeTypes(Tcl_Interp *interp,int count,TypeDecl *tab)
{
    TypeDecl *entry;
    Tcl_Obj *typesList;  // список всех известных типов
    Tcl_Obj **types_objv;// его элементы
    int types_objc;      // и их кол-во
    
    if (count==0) return TCL_OK;    // ничего делать не надо
    // получить список всех типов
    typesList=Tcl_ListObjNew(0,NULL);
    Tcl_IncrRefCount(typesList);
    if (Tcl_AppendAllObjTypes(interp,typesList)!=TCL_OK) {
        ERR("unable to get all type names");
        return TCL_ERROR;
    }
    // получить доступ к элементам
    if (Tcl_ListObjGetElements(interp,typesList,&types_objs,&types_objv)!=TCL_OK) {
        ERR("unable access to list elements");
        Tcl_DecrRefCount(typesList);
        return TCL_ERROR;
    }
    
    for(t=0,entry=tab;(count>0?t<count:(entry->name!=NULL)&&(entry->name[0]!='\0'));t++,entry++) {
        // entry->name и entry->type->name должны быть одинаковы, или это фигня какая-то
        if (entry->type!=NULL && (entry->name!=entry->type->name && strcmp(entry->name,entry->type->name)!=0) {
            ERR("internal type names conflict %s vs %s",entry->name,entry->type->name);
            return TCL_ERROR;
        }
        if (stringFoundInArray(entry->name,lists_objc,lists_objv)) {
            // такой тип уже есть в системе (видимо повторная инициализация)
            // максимум чем можно помоч - получить дескриптор
            if (entry->resolvePtr!=NULL) {
                Tcl_ObjType *type;
               *entry->resolvePtr=Tcl_GetObjType(entry->name);
            }
        } else {
            // такого типа нет в системе - может его надо сделать?
            if (entry->type!=NULL) {
                Tcl_RegisterObjType(entry->type);
                if (entry->resolvePtr!=NULL)
                    *entry->resolvePtr=entry->type;
            }
        }
    }
    Tcl_DecrRefCount(typesList);
    return TCL_OK;
}
