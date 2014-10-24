#ifndef CONS_H
#define CONS_H 1

#include <tcl.h>

#define CONS_POOL 1

typedef struct Cons {
    struct Cons *next;
    Tcl_Obj *obj;
} Cons;

Cons *consInit(Cons *cons,Tcl_Obj *obj);

Cons *consAlloc(Tcl_Obj *obj);
Cons *consAllocFromPool(Tcl_Obj *obj,Cons *pool);
Cons *consFreeToPool(Cons *cons,Cons *pool);

#ifdef CONS_POOL
   extern Cons *consPool;
#define consNew(obj) consAllocFromPool(obj,consPool)
#define consFromObj(obj) consAllocFromPool(obj,consPool)
#else
#define consFromObj(obj) consAlloc(obj)
#define consNew(obj) consAlloc(obj)
#endif

Cons *consFromArray(int objc,Tcl_Obj *const objv[],Cons **saveTailPtr);

/* free cons, variant:
    consFree(NULL,cons) - free single cons, return cons->next
    consFree(from,NULL) - free all cons list, return NULL
    consFree(from,to)   - free cons`es from "from" to "to" (inclusive), return "to->next"
*/    
Cons *consFree(Cons *from,Cons *to);

/* клонировать (дублировать) список ячеек,
   сохранить указатель по последнюю дубированную
   -----
   Варианты использования:
   consClone(NULL,cons) - дублировать конкретную ячейку
   consClone(from,NULL) - дублировать до самой последней
   consClone(from,to)   - соотв. от и до
   -----
   возвращает первую ячейку из дубликата,
   если задан savePtr - то туда сохранится указатель на последнюю
   
*/
Cons *consClone(Cons *from,Cons *to,Cons **saveTailPtr);
#define consCopy(from,to,saveTailPtr) consClone((from),(to),(saveTailPtr))
Cons *consRemoveNulls(Cons *from,Cons *to,Cons **saveTailPtr);

#endif
