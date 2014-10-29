#ifndef ITER_H
#define ITER_H 1

#include <tcl.h>
#include "tuple.h"
/** простой итератор для перебора элементов чего-то там
    пока поддерживаются только list и tuple (todo: dict)
**/
typedef struct Iter {
    Tcl_Obj *obj;   // объект чьи элементы перебираются
    Tcl_Interp *interp; // просто чтобы не тащить его параметром во все функции
    int flags;      // разные флаги см. ITER_XXX
    union {
        // итератор по списку
        struct {
            Tcl_Obj **list_objv;
            int list_objc;
            int list_position;
        };
        // итератор по кортежу
        struct {
            Cons *cons;
        };
    };
} Iter;
enum {
    ITER_DEFAULT=0,     // по умолчанию - вложенные кортежи разворачиваются, отложенные процедуры исполняются, пустые объекты игнорируются
    ITER_ALLOW_NULLS=1, // разрешить возврат NULL как текущее значения (актуально для tuple)
    ITER_ALLOW_TUPLES=2,  // разрешить возврат tuple как текущего значения (не разворачивать вложенные кортежи)
    ITER_ALLOW_LAZY=4,  // разрешить возврат lazy как текщего значения (не исполнять отложенные процедуры)
    ITER_ASIS=7          // возвращать всё как есть - и не выпендриваться :-)
};
/**
    Варинты использования:
    Iter it;
    if (iterInit(interp,&it,listOrTuple,flags)!=TCL_OK) {
        ERR();
    }
    for(obj=iterFirst(&it,NULL);obj;obj=iterNext(&it,NULL)) {
       ...
    }
    iterDone(&it); // или iterStop(&it) что однозначно
    
    for(obj=iterStart(interp,&it,listOrTuple,flags);!iterEnd(&it);obj=iterNext(&it)) {
       ...
    }
    iterStop(&it);
*/
// начать итерации (инициализовать итератор
int iterInit(Tcl_Interp *,Iter *iter,Tcl_Obj *,int flags);  // инициализовать
Iter *iterNew(Tcl_Interp *,Tcl_Obj *,int flags);
void iterFree(Iter *);

Tcl_Obj *iterStart(Tcl_Interp *,Iter *iter,Tcl_Obj *,int flags);   // инициализовать и сразу вернуть первый объект
void iterDone(Iter *);  // завершить итерации
#define iterStop(x) iterDone(x)

Tcl_Obj *iterFirst(Iter *); // установить на первое значение
Tcl_Obj *iterNext(Iter *);  // идти к следующему
Tcl_Obj *iterCurr(Iter *);  // значение текущего

int iterEnd(Iter *);    // true если достигнут конец
int iterEmpty(Iter *);  // true если нечего перебирать
// TODO:
int iterReplace(Iter *,Tcl_Obj *); 
int iterDelete(Iter *);
int iterInsBefore(Iter *,Tcl_Obj *);
int iterInsAfter(Iter *,Tcl_Obj *);

#endif
