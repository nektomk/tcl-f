/** коллекция разных полезних функций при работе с Tcl
*/
#include <string.h>
#include "tcl_tool.h"

#include "debug.h"

/** int stringIndexInArray(asciiz,arraySize,objArray)
    найти позицию (индекс с 0) строки в массиве объектов
    = найденный индекс, начиная с 0
    = -1, если строка не найдена или объект не конвертируется в список
    side-effects:
        - для компонентов будет вызван updateStringRep
**/    
int
stringIndexInArray(char *s,int objc,Tcl_Obj **objv)
{
    int t;
    if (s==NULL || s[0]=='\0' || objc==0 || objv==NULL) return -1;
    for(t=0;t<objc;t++) {
        char *content;
        content=Tcl_GetString(objv[t]);
        if (content==NULL || content[0]==0 || content[0]!=s[0]) continue;
        if (strcmp(s,content)==0) return t;
    }
    return -1;
}

/** int stringIndexInList(asciiz,listObj)
    найти позицию (индекс с 0) заданной строки в списке
    = найденный индекс, начиная с 0
    = -1, если строка не найдена или объект не конвертируется в список
    side-effects:
        - listObj будет сконвертирован в список
        - для компонентов будет вызван updateStringRep
*/    
int
stringIndexInList(char *s,Tcl_Obj *listObj) {
    Tcl_Obj **list_objv;
    int list_objc;
    if (Tcl_ListObjGetElements(NULL,listObj,&list_objc,&list_objv)!=TCL_OK) {
        return -1;
    }
    return stringFoundInArray(s,list_objc,list_objv);
}

/** resolveOrRegisterType(typeName,type,typeSavePtr)
    получить информацию о заданном (именованном) типе,
    либо регестрировать его если такого ещё нет
**/
int
resolveOrRegisterType(char *typeName,const Tcl_ObjType *myType,const Tcl_ObjType **savePtr)
{
    const Tcl_ObjType *tclType;
    // поиск типа по заданному имени. имя переданное в аргументе приоритетней чем заданное через myType->name
    if (typeName!=NULL)
        tclType=Tcl_GetObjType(typeName);     
    else if (myType!=NULL && myType->name!=NULL)
        tclType=Tcl_GetObjType(myType->name);
    else
        return TCL_ERROR;
    if (tclType==NULL && myType!=NULL) {
        // такого типа в системе нет, пользователь передал дескриптор - можно попробовать регестрировать
        if (myType->name == NULL || myType->name[0]=='\0') {
            // заведомо неправильное имя типа в дескрипторе
            WARN("bad type descriptor");
            return TCL_ERROR;
        } else if (typeName!=NULL && strcmp(typeName,myType->name)!=0) {
            // имена не совпадают - так низзя, это потенциальные проблемы
            WARN("type name сonflict");
            return TCL_ERROR;
        } else {
            // регистрация нового типа - оно всегда удачно :-)
            Tcl_RegisterObjType(myType);
            tclType=myType;
        }
    }
    if (savePtr!=NULL)
        *savePtr=myType;
    return TCL_OK;
}

/** objNew(string,len)
    создать объект с валидным текстовым и невалидным внутренним (безтиповой объект)
    если длина строки не задана - она будет определена до первого \0
    если сама строка не задана - будет возвращен пустой объект (Tcl_NewObj)
*/
Tcl_Obj *
objNew(char *s,int len)
{
    Tcl_Obj *obj;
    obj=Tcl_NewObj();
    if (s==NULL) return obj;
    if (len<0) len=strlen(s);
    obj->bytes=Tcl_Alloc(len+1);
    obj->bytes[len]=0;
    obj->length=len;
    return obj;
}
/** objFromString(typeName,type,value)
    Создать объект заданного типа из строки
    тип объекта задаётся именем или сразу дескриптором типа
    если заданны оба (имя и дескриптор) то дескриптор приоритетен
    если не задан ни один - выйдет строка :-)
**/    
Tcl_Obj *
objFromString(char *typeName,const Tcl_ObjType *type,char *value,int len)
{
    Tcl_Obj *obj;
    // получить дескриптор типа, если он не задан
    if (type==NULL) {
        // дескриптор не задан - попробовать получитьиз имени
        if (typeName != NULL && (type=Tcl_GetObjType(typeName))!=NULL) {
            return Tcl_NewStringObj(value,-1);
        }
    }
    // создать начальный объект
    obj=objNew(value,len);
    // сконвертировать новый объект в заданный тип    
    if (Tcl_ConvertToType(NULL,obj,type)!=TCL_OK) {
        // не удалось сконвертировать объект в заданный тип
        WARN("on creatе object %s from %s",(obj->typePtr==NULL?"untyped":obj->typePtr->name),(value==NULL?"NULL":value));
        // созданный объект будет возвращен в любом случае
    }
    return obj;
}

/** objFromAny(typeName,type,originalObj)
    Создать объект заданного типа из другого объекта
    параметры и поведение аналогичны objNew
**/
Tcl_Obj *
objFromAny(char *typeName,const Tcl_ObjType *type,Tcl_Obj *orig)
{
    Tcl_Obj *obj;
    // можно сразу создать объект как копию orig
    obj=Tcl_DuplicateObj(orig);
    // получить дескриптор типа, если он не задан
    if (type==NULL) {
        // дескриптор не задан - попробовать получитьиз имени
        if (typeName != NULL && (type=Tcl_GetObjType(typeName))!=NULL) {
            return obj;
        }
    }
    // сконвертировать новый объект в заданный тип    
    if (Tcl_ConvertToType(NULL,obj,type)!=TCL_OK) {
        // не удалось сконвертировать объект в заданный тип
        WARN("on creatе object %s from %s",(obj->typePtr==NULL?"untyped":obj->typePtr->name),(orig->typePtr==NULL?"untyped":orig->typePtr->name));
        // созданный объект будет возвращен в любом случае
    }
    return obj;
}
/** takeObj(obj)
    получить/взять объект для модификации. Если оригинал расшарен - вернёт копию, иначе оригинал
**/
Tcl_Obj *
takeObj(Tcl_Obj *obj) {
    if (Tcl_IsShared(obj)) return Tcl_DuplicateObj(obj);
    return obj;
}
