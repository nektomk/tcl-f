#ifndef TUPLE_H
#define TUPLE_H 1

#include <tcl.h>
#include "cons.h"
/** introduced  "tuple" Tcl object type **/
extern const Tcl_ObjType *tupleType;
extern Tcl_Obj *emptyTuple;

/** simplify "Tuple" - single list of Cons cells
*/
typedef struct Tuple {
    Cons *head;
    Cons *tail;
} Tuple;

int initTupleSubsys(Tcl_Interp *interp);
int initTupleInstance(Tcl_Interp *interp,char *nsPrefix);

/** "Tuple" stores in Tcl_Obj directly */
#define TUPLE(obj) ((Tuple *)&obj->internalRep.twoPtrValue)

void tupleObjInit(Tcl_Obj *tupleObj,Cons *head,Cons *tail);
// constructors :-)
Tcl_Obj *tupleObjNew();
Tcl_Obj *tupleObjFromObj(Tcl_Obj *);
Tcl_Obj *tupleObjFromArray(int objc,Tcl_Obj *const objv[]);
Tcl_Obj *tupleObjFromList(Tcl_Obj *);
Tcl_Obj *tupleObjFromTuple(Tcl_Obj *);
// converters
int tupleObjToList(Tcl_Obj *tupleObj,Tcl_Obj *listObj);
int tupleObjToAny(Tcl_Obj *tupleObj,Tcl_Obj *obj,Tcl_ObjType *);
// elemantary IO :-) с кортежами можно работать как с очередью и стеком
Tcl_Obj *tupleObjGet(Tcl_Obj *tupleObj);        // вернуть первый объект набора
#define tupleObjPop(tupleObj) tupleObjGet(tupleObj)
void tupleObjPut(Tcl_Obj *tupleObj,Tcl_Obj *);   // добавить объект в конец набора
int tupleObjUnget(Tcl_Obj *tupleObj,Tcl_Obj *);  // добавить объект в начало набора
#define tupleObjPush(tupleObj,obj) tupleObjUnget((tupleObj),(obj))

// methods :-)
int tupleObjIsEmpty(Tcl_Obj *); // no not-null elements
Cons *tupleObjRest(Tcl_Obj *);  // second not-null element
#define tupleObjHead(obj) (TUPLE(obj)->head)
#define tupleObjTail(obj) (TUPLE(obj)->tail)

Tcl_Obj *tupleRemoveNulls(Tcl_Obj *tupleObj);
/** Tcl proc`s
*/
int tupleObjProc(ClientData,Tcl_Interp *,int,Tcl_Obj *const []);

void consExpand(Cons *cons);

#endif


