#include <tcl.h>

#include "tuple.h"
#include "inspect.h"
#include "debug.h"

extern const Tcl_ObjType *tupleType;
extern Tcl_Obj *emptyTuple;

extern const Tcl_ObjType *listType;
static void tupleObjFreeInternals(Tcl_Obj *self);
static void tupleObjDupInternals(Tcl_Obj *self,Tcl_Obj *to);
static void tupleObjUpdateString(Tcl_Obj *self);
static int tupleObjSetFromAny(Tcl_Interp *interp,Tcl_Obj *self);

static Tcl_ObjType tupleTypeD = {
    "tuple",
    tupleObjFreeInternals,
    tupleObjDupInternals,
    tupleObjUpdateString,
    tupleObjSetFromAny
};

void consExpand(Cons *cons);

int initTupleSubsys(Tcl_Interp *interp) {
    (void)interp;
    if (tupleType!=NULL) return TCL_ERROR;
    tupleType=&tupleTypeD;
    emptyTuple=tupleObjNew();
    if (emptyTuple==NULL) return TCL_ERROR;
    Tcl_IncrRefCount(emptyTuple);
    Tcl_IncrRefCount(emptyTuple);
    return TCL_OK;
}

static void
tupleObjFreeInternals(Tcl_Obj *tupleObj)
{
    if (tupleObj==NULL) return;
    if (tupleObj->typePtr!=tupleType) {
        WARN("not a tuple %s",(tupleObj->typePtr?tupleObj->typePtr->name:"NULL"));
        return;
    }
    consFree(TUPLE(tupleObj)->head,NULL);   // free(delete) all cons`es from ->head to ->tail
}
static void
tupleObjDupInternals(Tcl_Obj *from,Tcl_Obj *to)
{
    to->typePtr=tupleType;
    to->bytes=NULL;
    to->length=0;
    TUPLE(to)->head=consClone(TUPLE(from)->head,TUPLE(from)->tail,&TUPLE(to)->tail);
}
/* обновить строковое представление
   пока сделанно через создание временного списка
   ----
   PERF: можно ускорить :-)
*/   
static void
tupleObjUpdateString(Tcl_Obj *tupleObj)
{
    int objc;
    Tcl_Obj *list;
    Tcl_Obj *objv[16];
    Cons *curr;
    tupleObj->bytes=NULL;
    tupleObj->length=0;
    for(curr=TUPLE(tupleObj)->head,objc=0;curr!=NULL && objc<16;curr=curr->next) {
        consExpand(curr);
        if (curr->obj==NULL) continue;
        objv[objc++]=curr->obj;
    }
    if (objc==0 || (list=Tcl_NewListObj(objc,objv))==NULL) {
        // simple empty string
        tupleObj->bytes=Tcl_Alloc(1);
        tupleObj->bytes[0]=0;
        tupleObj->length=0;
        return;
    }
    Tcl_IncrRefCount(list);
    for(;curr!=NULL;curr=curr->next) {
        consExpand(curr);
        if (curr->obj==NULL) continue;
        if (Tcl_ListObjAppendElement(NULL,list,curr->obj)!=TCL_OK) goto FINAL;
    }
    tupleObj->bytes=Tcl_GetStringFromObj(list,&tupleObj->length);
    list->bytes=NULL;
    list->length=0;
FINAL:    
    Tcl_DecrRefCount(list);
}

static int
tupleObjSetFromAny(Tcl_Interp *interp,Tcl_Obj *tupleObj) {
    Cons *head,*tail;
    int c;
    int objc;
    Tcl_Obj **objv;
    // mutate to list and get his elements
    if (Tcl_ListObjGetElements(interp,tupleObj,&objc,&objv)!=TCL_OK) {
        return TCL_ERROR;
    }
    head=tail=NULL;
    if (objc>0) {
        head=tail=consNew(objv[0]);
        for(c=1;c<objc;c++) {
            tail=tail->next=consNew(objv[c]);
        }
    }
    listType->freeIntRepProc(tupleObj);
    tupleObjInit(tupleObj,head,tail);
    return TCL_OK;
}
void
tupleObjInit(Tcl_Obj *obj,Cons *head,Cons *tail)
{
    if (obj==NULL) return;
    obj->typePtr=tupleType;

    TUPLE(obj)->head=head;
    TUPLE(obj)->tail=tail;
}
Tcl_Obj *
tupleObjNew()
{
    Tcl_Obj *tupleObj;
    tupleObj=Tcl_NewObj();
    tupleObj->typePtr=tupleType;
    tupleObj->bytes=NULL;
    tupleObj->length=0;
    TUPLE(tupleObj)->head=TUPLE(tupleObj)->tail=NULL;
    return tupleObj;
}
Tcl_Obj *
tupleObjFromArray(int objc,Tcl_Obj * const objv[]) {
    Tcl_Obj *tupleObj;
    tupleObj=tupleObjNew();
    if (tupleObj==NULL) return NULL;
    TUPLE(tupleObj)->head=consFromArray(objc,objv,&TUPLE(tupleObj)->tail);
    return tupleObj;
}
Tcl_Obj *
tupleObjFromList(Tcl_Obj *list) {
    int objc;
    Tcl_Obj **objv;
    if (Tcl_ListObjGetElements(NULL,list,&objc,&objv)!=TCL_OK) return NULL;
    return tupleObjFromArray(objc,objv);
}
Tcl_Obj *
tupleObjFromTuple(Tcl_Obj *obj) {
    Tcl_Obj *tupleObj;
    tupleObj=tupleObjNew();
    TUPLE(tupleObj)->head=consCopy(TUPLE(obj)->head,NULL,&TUPLE(tupleObj)->tail);
    return tupleObj;
}
Tcl_Obj *
tupleFromObj(Tcl_Obj *obj) {
    if (obj==NULL) return NULL;
    if (obj->typePtr==tupleType) return tupleObjFromTuple(obj);
    return tupleObjFromList(obj);
}
void
tupleObjPut(Tcl_Obj *tupleObj,Tcl_Obj *obj) {
    if (tupleObj==NULL || obj==NULL) return;
    if (TUPLE(tupleObj)->head==NULL)
        TUPLE(tupleObj)->head=TUPLE(tupleObj)->tail=consNew(obj);
    else
        TUPLE(tupleObj)->tail=TUPLE(tupleObj)->tail->next=consNew(obj);
}
int
tupleObjProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[])
{
    Tcl_Obj *tupleObj;
    (void)data;
    tupleObj=tupleObjFromArray(objc-1,objv+1);
    if (tupleObj==NULL) return TCL_ERROR;
    Tcl_SetObjResult(interp,tupleObj);
    return TCL_OK;
}
Tcl_Obj *
tupleRemoveNulls(Tcl_Obj *tupleObj) {
    TUPLE(tupleObj)->head=consRemoveNulls(TUPLE(tupleObj)->head,NULL,&TUPLE(tupleObj)->tail);
    if (TUPLE(tupleObj)->head==NULL) return emptyTuple;
    return tupleObj;
}
void
consExpand(Cons *cons) {
    Cons *head,*tail; 
    if (cons==NULL) return;
    if (cons->obj==NULL || cons->obj->typePtr!=tupleType) return;
    if (Tcl_IsShared(cons->obj)/*cons->obj->refCount>1*/) {
        // object shared
        head=consClone(TUPLE(cons->obj)->head,NULL,&tail);
    } else {
        // object not shared or shared only by me
        head=TUPLE(cons->obj)->head;
        tail=TUPLE(cons->obj)->tail;
        tupleObjInit(cons->obj,NULL,NULL);
    }
    Tcl_DecrRefCount(cons->obj);
    cons->obj=NULL;
    if (head!=NULL) {
        tail->next=cons->next;
        cons->next=head;
    }
}
