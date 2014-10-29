#include <tcl.h>
#include <string.h>

#include "iter.h"

#include "debug.h"
#include "inspect.h"

extern const Tcl_ObjType *listType;
extern const Tcl_ObjType *tupleType;

int
iterInit(Tcl_Interp *interp,Iter *it,Tcl_Obj *obj,int flags)
{
    if (it==NULL || obj==NULL)
        return TCL_ERROR;
    if (obj->typePtr!=tupleType && obj->typePtr!=listType) {
        if (Tcl_ConvertToType(interp,obj,listType)!=TCL_OK)
            return TCL_ERROR;
    }
    memset(it,0,sizeof(Iter));
    
    it->interp=interp;
    it->obj=obj;
    Tcl_IncrRefCount(obj);
    it->flags=flags;        
    if (it->obj->typePtr==listType) {
        if (Tcl_ListObjGetElements(interp,it->obj,&it->list_objc,&it->list_objv)!=TCL_OK)
            goto ERROR;
        it->list_position=it->list_objc;
    } else if (it->obj->typePtr==tupleType) {
        it->cons=NULL;
    }
    return TCL_OK;
ERROR:
    ERR("!!!!!!!!!!!!!!!!!!");
    INSPECT_OBJ(obj,"collection");
    Tcl_DecrRefCount(it->obj);
    it->obj=NULL;
    return TCL_ERROR;
}
Iter *
iterNew(Tcl_Interp *interp,Tcl_Obj *obj,int flags)
{
    Iter *it;
    it=(Iter *)Tcl_Alloc(sizeof(Iter));
    if (iterInit(interp,it,obj,flags)==TCL_OK) return it;
    Tcl_Free((char *)it);
    return NULL;
}
void
iterFree(Iter *it) {
    if (it==NULL) return;
    iterDone(it);
    Tcl_Free((char *)it);
}
void iterDone(Iter *it)
{
    if (it==NULL) return;
    if (it->obj!=NULL) Tcl_DecrRefCount(it->obj);
    memset(it,0,sizeof(Iter));
}

Tcl_Obj *
iterStart(Tcl_Interp *interp,Iter *it,Tcl_Obj *obj,int flags) {
    if (iterInit(interp,it,obj,flags)!=TCL_OK) return NULL;
    return iterFirst(it);
}
static inline Tcl_Obj *
iterListNext(Iter *it) {
    if (it->list_objc==0) return NULL;
    it->list_position++;
    if (it->list_position>=it->list_objc) return NULL;
    return it->list_objv[it->list_position];
}
static inline Tcl_Obj *
iterTupleNext(Iter *it) {
    if (TUPLE(it->obj)->head==NULL) return NULL;
    if (it->cons==NULL) return NULL;
    for(it->cons=it->cons->next;it->cons!=NULL;it->cons=it->cons->next) {
        if (!(it->flags & ITER_ALLOW_NULLS) && it->cons->obj==NULL) continue;  // пустые cons пропускаются
        // if (it->cons->obj->typePtr==lazyType)... // отложенные функции исполняются
        if (!(it->flags & ITER_ALLOW_TUPLES) && it->cons->obj->typePtr==tupleType) {    // вложенные наборы разворачиваются
            consExpand(it->cons);
            continue;
        }
        break;
    }
    if (it->cons==NULL) return NULL;
    return it->cons->obj;
}
Tcl_Obj *
iterFirst(Iter *it) {
    if (it==NULL || it->obj==NULL) return NULL;
    if (it->obj->typePtr==listType) {
        if (it->list_objc==0) return NULL;
        it->list_position=0;
        return it->list_objv[it->list_position];
    }
    if (it->obj->typePtr==tupleType) {
        it->cons=consHead(TUPLE(it->obj)->head);
        if (it->cons==NULL) return NULL;
        return it->cons->obj;
    }
    return NULL;
}

Tcl_Obj *
iterNext(Iter *it) {
    if (it==NULL || it->obj==NULL) return NULL;
    if (it->obj->typePtr==listType) {
        return iterListNext(it);
    }
    if (it->obj->typePtr==tupleType) {
        return iterTupleNext(it);
    }
    return NULL;
}
    
Tcl_Obj *
iterCurr(Iter *it) {
    if (it==NULL || it->obj==NULL)
        return NULL;
    if (it->obj->typePtr==listType) {
        if (it->list_position>=it->list_objc) return NULL;
        return it->list_objv[it->list_position];
    }
    if (it->obj->typePtr==tupleType) {
        if (it->cons==NULL) return NULL;
        return it->cons->obj;
    }
    return NULL;
}
int
iterEnd(Iter *it)
{
    if (it==NULL || it->obj==NULL) return 1;
    if (it->obj->typePtr==listType) {
        if (it->list_objc==0) return 1;
        if (it->list_position>=it->list_objc) return 1;
        return 0;
    }
    if (it->obj->typePtr==tupleType) {
        if (it->cons==NULL) return 1;
        return 0;
    }
    return 1;
}
int
iterEmpty(Iter *it)
{
    if (it==NULL || it->obj==NULL) return 1;
    if (it->obj->typePtr==listType) {
        if (it->list_objc>0) return 0;
        return 1;
    } else if (it->obj->typePtr==tupleType) {
        if (TUPLE(it->obj)->head!=NULL) return 0;
        return 1;
    }
    return 1;
}
