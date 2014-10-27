#include <tcl.h>
#include "inspect.h"
#include "tuple.h"
#include "lazy.h"

extern const Tcl_ObjType *listType;
extern const Tcl_ObjType *tupleType;
extern const Tcl_ObjType *dictType;

static void placeIndent(int indent);
static void inspectLazyObj(int,int,Tcl_Obj *);
/** Tcl: inspect $obj..
*/
int
inspectObjProc(ClientData data,Tcl_Interp *interp,int objc,Tcl_Obj *const objv[]) {
	int t;
	(void)data;
	if (objc<2) {
		Tcl_WrongNumArgs(interp,objc,objv,"obj..");
	}
	for(t=1;t<objc;t++) {
		inspectObj(0,objv[t]);
	}
	return TCL_OK;
}

void
inspectObj(int indent,Tcl_Obj *obj)
{
	if (obj==NULL) { printf("NULL\n"); return;}
	printf("%p ",obj);
	if (obj->refCount<2) {
		printf("refCount:%d owned ",obj->refCount);
	} else { printf("refCount:%d shared ",obj->refCount); }
	if (obj->typePtr!=NULL) {
		printf("(%s) ",obj->typePtr->name);
		if (obj->typePtr==listType) {
			inspectListObj(indent,5,obj);
		}
		if (obj->typePtr==tupleType) {
			inspectTupleObj(indent,5,obj);
		}
		if (obj->typePtr==dictType) {
			inspectDictObj(indent,5,obj);
		}
		if (obj->typePtr==lazyType) {
			inspectLazyObj(indent,5,obj);
		}
	} else printf("(untyped) ");
	if (obj->bytes!=NULL && obj->length!=0) {
		printf("= %*s",obj->length,obj->bytes);
	}
	putc('\n',stderr);
}
static void
inspectArrayContent(int indent,int count,int objc,Tcl_Obj * const *objv) {
	int c;
	// content:
	for(c=0;c<count && c<objc;c++) {
		placeIndent(indent+1);
		printf("[%d]:",c);
		inspectObj(indent+1,objv[c]);
	}
	if (c!=objc) {
		placeIndent(indent+1);
		printf("...\n");
		placeIndent(indent+1);
		printf("[%d]:",objc-1);
		inspectObj(indent+1,objv[objc-1]);
	}
	
}
static void
inspectTupleContent(int indent,int count,Tcl_Obj *tupleObj) {
	Cons *cons;
	int t;
	for(t=0,cons=TUPLE(tupleObj)->head;cons!=NULL && t<count; t++,cons=cons->next) {
		placeIndent(indent+1);
		printf("[- %p]:",cons);
		inspectObj(indent+1,cons->obj);
	}
	if (cons!=NULL) {
		if (cons->next!=NULL) {
			placeIndent(indent+1);
			printf("...\n");
		}
		printf("[* %p] ",TUPLE(tupleObj)->tail);
		inspectObj(indent+1,TUPLE(tupleObj)->tail->obj);
	}
}
static void
inspectListContent(int indent,int count,Tcl_Obj *listObj) {
	Tcl_Obj **objv;
	int objc;
	if (Tcl_ListObjGetElements(NULL,listObj,&objc,&objv)!=TCL_OK)
		return;
	inspectArrayContent(indent,count,objc,objv);
}
void
inspectTupleObj(int indent,int count,Tcl_Obj *obj) {
	printf("from %p to %p\n",TUPLE(obj)->head,TUPLE(obj)->tail);
	inspectTupleContent(indent+1,count,obj);
}
void
inspectObjArray(int indent,int count,int objc,Tcl_Obj * const *objv) {
	if (count<0) count=objc; // default - inspect all array
	printf("%p ",objv);
	printf("[%d] array\n",objc);
	if (count==0) return;	// no inspect content
	inspectArrayContent(indent+1,count,objc,objv);
}
void
inspectListObj(int indent,int count,Tcl_Obj *obj) {
	int listLength;
	Tcl_Obj **listElements;
	if (Tcl_ListObjGetElements(NULL,obj,&listLength,&listElements)!=TCL_OK) {
		printf("INSPECT FAULT\n");
		return;
	}
	printf("of %d elements",listLength);
	if (obj->bytes!=NULL) printf("=%*s\n", obj->length,obj->bytes);
	else putchar('\n');
	inspectArrayContent(indent+1,count,listLength,listElements);
}
void
inspectDictObj(int indent,int count,Tcl_Obj *dictObj) {
	Tcl_DictSearch search;
	Tcl_Obj *key,*value;
	int done;
	(void)count;
	if (Tcl_DictObjFirst(NULL,dictObj,&search,&key,&value,&done)!=TCL_OK)
		return;
	putchar('\n');
	for(;!done;Tcl_DictObjNext(&search,&key,&value,&done)) {
		placeIndent(indent+1);
		printf("[ %s ] :",Tcl_GetString(key));
		inspectObj(indent+1,value);
	}
}
static void
inspectLazyObj(int indent,int count,Tcl_Obj *lazyObj) {
	printf("for exec later in interp %p\n",LAZY(lazyObj)->interp);
	inspectListContent(indent+1,count,LAZY(lazyObj)->command);
}
static
void placeIndent(int indent) {
	while(indent) {
		putchar('\t');
		indent--;
	}
}
