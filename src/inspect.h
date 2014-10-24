#ifndef INSPECT_H
#define INSPECT_H 1

#include <tcl.h>

int inspectObjProc(ClientData,Tcl_Interp *,int,Tcl_Obj *const objv[]);


void inspectObj(int indent,Tcl_Obj *);
void inspectObjArray(int indent,int,int,Tcl_Obj *const *);
void inspectListObj(int indent,int count,Tcl_Obj *);
void inspectTupleObj(int indent,int count,Tcl_Obj *obj);
void inspectDictObj(int indent,int count,Tcl_Obj *dictObj);
#define INSPECT_OBJ(obj,formal,...) do { \
	fprintf(stderr,formal,##__VA_ARGS__); \
	fputc('\n',stderr); \
	inspectObj(0,(obj)); \
} while(0);

#define INSPECT_LIST(count,obj,formal,...) do { \
	fprintf(stderr,formal,##__VA_ARGS__); \
	fputc('\n',stderr); \
	inspectListObj(0,(count),(obj)); \
} while(0);

#define INSPECT_ARRAY(count,objc,objv,formal,...) do { \
	fprintf(stderr,formal,##__VA_ARGS__); \
	fputc('\n',stderr); \
	inspectObjArray(0,(count),(objc),(objv)); \
} while(0);

#define INSPECT_TUPLE(count,obj,formal,...)  do { \
	fprintf(stderr,formal,##__VA_ARGS__); \
	fputc('\n',stderr); \
	inspectTupleObj(0,(count),(objc),(objv)); \
} while(0);

#define INSPECT_DICT(count,obj,formal,...) do { \
	fprintf(stderr,formal,##__VA_ARGS__); \
	fputc('\n',stderr); \
	inspectDictObj(0,(count),(obj)); \
} while(0);


#endif
