#include <tcl.h>
#include <debug.h>

#include "cons.h"

#ifdef CONS_POOL
Cons consPoolD={0};
Cons *consPool=&consPoolD;
#endif

Cons *
consAllocFromPool(Tcl_Obj *obj,Cons *pool) {
    Cons *cons;
    cons=NULL;
    if (pool!=NULL) {
        cons=pool->next;
        if (cons!=NULL) pool->next=cons->next;
    }
    if (cons==NULL) cons=(Cons *)Tcl_Alloc(sizeof(Cons));
    return consInit(cons,obj);
}
Cons *
consFreeToPool(Cons *cons,Cons *pool) {
    Cons *next;
    if (cons==NULL) return NULL;
    next=cons->next;
    if (cons->obj!=NULL) Tcl_DecrRefCount(cons->obj);
    if (pool!=NULL) {
        cons->next=pool->next;
        pool=cons;
    } 
    return next;
}

Cons *
consInit(Cons *cons,Tcl_Obj *obj)
{
    if (cons==NULL) return NULL;
    cons->next=NULL;
    if (obj!=NULL) Tcl_IncrRefCount(obj);
    cons->obj=obj;
    return cons;
}
Cons *
consAlloc(Tcl_Obj *obj)
{
    Cons *cons=(Cons *)Tcl_Alloc(sizeof(Cons));
    return consInit(cons,obj);
}
Cons *
consFree(Cons *from,Cons *to)
{
    Cons *next;
    if (from==NULL) {
        if (to==NULL) return NULL;
        from=to;
    }
    do {
        if (from->obj!=NULL) {
            Tcl_DecrRefCount(from->obj);
        }
        next=from->next;    // save next pointer
        Tcl_Free((char *)from);
        if (from==to) break;
        from=next;
    } while(from!=NULL);
    if (from!=NULL) return from->next;
    return from;
}
Cons *
consClone(Cons *from,Cons *to,Cons **saveTailPtr) {
    Cons *head,*tail;   // соотв. начало и конец создаваемого списка Cons
    if (from==NULL) {
        if (to==NULL) return NULL;
        from=to;
    }
    head=tail=consNew(from->obj);
    while((from=from->next)!=NULL) {
        tail=tail->next=consNew(from->obj);
        if (from==to) break;
    }
    if (saveTailPtr!=NULL) *saveTailPtr=tail;
    return head;
}
Cons *
consFromArray(int objc,Tcl_Obj * const objv[],Cons **saveTailPtr) {
    Cons *head,*tail;
    int c;
    head=tail=NULL;
    if (objc==0) goto FINAL;
    head=tail=consNew(objv[0]);
    for(c=1;c<objc;c++)
        tail=tail->next=consNew(objv[c]);
FINAL:        
    if (saveTailPtr!=NULL) *saveTailPtr=tail;
    return head;
}

Cons *
consRemoveNulls(Cons *head,Cons *tail,Cons **saveTailPtr) {
    Cons *next;
    Cons *first=NULL;
    if (head!=NULL) {
        next=head->next;
        do {
            if (head->obj==NULL) {
                consFree(NULL,head);
            } else if (first!=NULL) {
                first=head;
            }
            head=next;
        } while(head!=NULL);
    }
    if (saveTailPtr!=NULL) *saveTailPtr=tail;
    return first;
}








