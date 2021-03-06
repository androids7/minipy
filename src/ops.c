#include "include/tm.h"

#define RET_NUM(v) \
    Object _num;\
    TM_TYPE(_num) = TYPE_NUM;\
    GET_NUM(_num) = v;\
    return _num;

void tm_assert(int value, char* msg) {
    if (!value) {
        tm_raise("assertion failed, %s", msg);
    }
}

const char* tm_type(int type) {
    switch (type) {
    case TYPE_STR:
        return "string";
    case TYPE_NUM:
        return "number";
    case TYPE_LIST:
        return "list";
    case TYPE_FUNCTION:
        return "function";
    case TYPE_DICT:
        return "dict";
    case TYPE_DATA:
        return "data";
    case TYPE_NONE:
        return "none";
    }
    return "unknown";
}

void tm_assert_type(Object o, int type, char* msg) {
    if (TM_TYPE(o) != type) {
        tm_raise("%s, expect %s but see %s", msg, 
            tm_type(type), tm_type(o.type));
    }
}

void tm_assert_int(double value, char* msg) {
    /* (int)(-1.2) = -1 */
    if (value - floor(value) > 0.000001) {
        tm_raise("%s require int", msg);
    }
}

int obj_eq_sz(Object obj, const char* value) {
    return TM_TYPE(obj) == TYPE_STR
            && (GET_STR(obj) == value || strcmp(GET_STR(obj), value) == 0);
}

void obj_set(Object self, Object k, Object v) {
    // gc_mark_single(k); // used between gc scan
    // gc_mark_single(v); // only need to mark single.
    switch (TM_TYPE(self)) {
    case TYPE_LIST: {
        tm_assert_type(k, TYPE_NUM, "obj_set");
        double d = GET_NUM(k);
        tm_assert_int(d, "list_set");
        list_set(GET_LIST(self), (int)d, v);
    }
        return;
    case TYPE_DICT:
        dict_set0(GET_DICT(self), k, v);
        return;
    }
    tm_raise("obj_set: Self %o, Key %o, Val %o", self, k, v);
}

Object obj_get(Object self, Object k) {
    Object v;
    switch (TM_TYPE(self)) {
    case TYPE_STR: {
        DictNode* node;
        if (TM_TYPE(k) == TYPE_NUM) {
            double d = GET_NUM(k);
            int n = d;
            if (n < 0) {
                n += GET_STR_LEN(self);
            }
            if (n >= GET_STR_LEN(self) || n < 0)
                tm_raise("String_get: index overflow ,len=%d,index=%d, str=%o",
                        GET_STR_LEN(self), n, self);
            return string_chr(0xff & GET_STR(self)[n]);
        } else if ((node = dict_get_node(GET_DICT(tm->str_proto), k)) != NULL) {
            return method_new(node->val, self);
        }
        break;
    }
    case TYPE_LIST:{
        DictNode* node;
        if (TM_TYPE(k) == TYPE_NUM) {
            return list_get(GET_LIST(self), GET_NUM(k));
        } else if ((node = dict_get_node(GET_DICT(tm->list_proto), k))!=NULL) {
            return method_new(node->val, self);
        }
        break;
    }
    case TYPE_DICT:{
        DictNode* node;
        node = dict_get_node(GET_DICT(self), k);
        if (node != NULL) {
            return node->val;
        } else if ((node = dict_get_node(GET_DICT(tm->dict_proto), k))!=NULL) {
            return method_new(node->val, self);
        }
        break;
    }
    case TYPE_FUNCTION:
        return get_func_attr(GET_FUNCTION(self), k);
    case TYPE_DATA:
        return GET_DATA(self)->get(GET_DATA(self), k);
    }
    tm_raise("keyError %o", k);
    return NONE_OBJECT;
}

/**
 * slice string/list
 * string.slice(a,b) [a,b), b is not included
 * string.slice(0,-1) = string.slice(0,len(string))
 * @since 2016-11-19
 * <code>
 * 'test'.slice(0,1) = 't'
 * 'test'.slice(0,-1) = 'test'
 * </code>
 */
Object obj_slice(Object self, Object first, Object second) {
    int start = GET_NUM(first);
    int end = GET_NUM(second);
    Object ret = NONE_OBJECT;
    
    if (IS_STR(self)) {
        int length = GET_STR_LEN(self);
        start = start >= 0 ? start : start + length + 1;
        end   = end   >= 0 ? end   : end   + length + 1;
        if (start < 0 || start > length) {
            start = 0;
        } 
        if (end < 0 || end > length) {
            end = length; // do not overflow;
        } 
        if (end <= start) {
            return string_alloc("", 0);
        }
        return string_alloc(GET_STR(self) + start, end - start);
    } else if (IS_LIST(self)) {
        int length = LIST_LEN(self);
        start = start > 0 ? start : start + length + 1;
        end   = end   > 0 ? end   : end   + length + 1;
        if (start < 0 || start > length) {
            start = 0;
        } 
        if (end < 0 || end > length) {
            end = length;
        } 
        int i = 0;
        ret = list_new(end - start);
        for (i = start; i < end ; i++) {
            obj_append(ret, LIST_GET(self, i));
        }
    } else {
        tm_raise("slice not implemented for type %s", tm_type(self.type));
    }
    return ret;
}

Object obj_sub(Object a, Object b) {
    if (a.type == b.type) {
        if (a.type == TYPE_NUM) {
            GET_NUM(a) -= GET_NUM(b);
            return a;
        }
    }
    tm_raise("obj_sub: can not substract %o and %o", a, b);
    return NONE_OBJECT;
}

Object obj_add(Object a, Object b) {
    if (TM_TYPE(a) == TM_TYPE(b)) {
        switch (TM_TYPE(a)) {
        case TYPE_NUM:
            GET_NUM(a) += GET_NUM(b);
            return a;
        case TYPE_STR: {
            char* sa = GET_STR(a);
            char* sb = GET_STR(b);
            int la = GET_STR_LEN(a);
            int lb = GET_STR_LEN(b);
            if (la == 0) {return b;    }
            if (lb == 0) {return a;    }
            int len = la + lb;
            Object des = string_alloc(NULL, len);
            char*s = GET_STR(des);
            memcpy(s, sa, la);
            memcpy(s + la, sb, lb);
            return des;
        }
        case TYPE_LIST: {
            return list_add(GET_LIST(a), GET_LIST(b));
        }
        }
    }
    tm_raise("obj_add: can not add %o and %o", (a), (b));
    return NONE_OBJECT;
}

int obj_equals(Object a, Object b){
    if(TM_TYPE(a) != TM_TYPE(b)) return 0;
    switch(TM_TYPE(a)){
        case TYPE_NUM:return GET_NUM(a) == GET_NUM(b);
        case TYPE_STR: {
            String* s1 = GET_STR_OBJ(a);
            String* s2 = GET_STR_OBJ(b);
            return s1->value == s2->value || 
                (s1->len == s2->len && strncmp(s1->value, s2->value, s1->len) == 0);
        }
        case TYPE_LIST:    {
            if(GET_LIST(a) == GET_LIST(b)) return 1;
            int i;
            int len = GET_LIST(a)->len;
            Object* nodes1 = GET_LIST(a)->nodes;
            Object* nodes2 = GET_LIST(b)->nodes;
            for(i = 0; i < len; i++){
                if(!obj_equals(nodes1[i], nodes2[i]) ){
                    return 0;
                }
            }
            return 1;
        }
        case TYPE_NONE:return 1;
        case TYPE_DICT:return GET_DICT(a) == GET_DICT(b);
        case TYPE_FUNCTION: return GET_FUNCTION(a) == GET_FUNCTION(b);
        default: {
            const char* ltype = tm_type(a.type);
            const char* rtype = tm_type(b.type);
            tm_raise("equals(): not supported type %d:%s and %d:%s", TM_TYPE(a), ltype, TM_TYPE(b), rtype);
        } 
    }
    return 0;
}

int obj_cmp(Object a, Object b) {
    if (TM_TYPE(a) == TM_TYPE(b)) {
        switch (TM_TYPE(a)) {
            case TYPE_NUM: {
                double diff = GET_NUM(a) - GET_NUM(b);
                if (diff > 0.0) {
                    return 1;
                } else if (diff < 0.0) {
                    return -1;
                }
                return 0;
            }
            case TYPE_STR: return strcmp(GET_STR(a), GET_STR(b));
        }
    }
    tm_raise("obj_cmp: can not compare %o and %o", a, b);
    return 0;
}

Object obj_mul(Object a, Object b) {
    if (a.type == b.type && a.type == TYPE_NUM) {
        GET_NUM(a) *= GET_NUM(b);
        return a;
    }
    if (a.type == TYPE_NUM && b.type == TYPE_STR) {
        Object temp = a;
        a = b;
        b = temp;
    }
    if (a.type == TYPE_STR && b.type == TYPE_NUM) {
        int len = GET_STR_LEN(a);
        Object des;
        if (len == 0)
            return a;
        int times = (int) GET_NUM(b);
        if (times <= 0)
            return sz_to_string("");
        if (times == 1)
            return a;
        des = string_alloc(NULL, len * times);
        char* s = GET_STR(des);
        int i;
        for (i = 0; i < times; i++) {
            strncpy(s, GET_STR(a), len);
            s += len;
        }
        return des;
    }
    tm_raise("obj_mul: can not multiply %o and %o", a, b);
    return NONE_OBJECT;
}

Object obj_div(Object a, Object b) {
    if (a.type == b.type && a.type == TYPE_NUM) {
        GET_NUM(a) /= GET_NUM(b);
        return a;
    }
    tm_raise("obj_div: can not divide %o and %o", a, b);
    return NONE_OBJECT;
}

Object obj_mod(Object a, Object b) {
    if (a.type == b.type && a.type == TYPE_NUM) {
        return tm_number((long) GET_NUM(a) % (long) GET_NUM(b));
    } else if (a.type == TYPE_STR) {
        Object *__mod__ = get_builtin("__mod__");
        if (__mod__ == NULL) {
            tm_raise("__mod__ is not defined");
        } else {
            arg_start();
            arg_push(a);
            arg_push(b);
            return call_function(*__mod__);
        }        
    }
    tm_raise("obj_mod: can not module %o and %o", a, b);
    return NONE_OBJECT;
}


/* parent has child
 * child in parent
 */
int obj_in(Object child, Object parent) {
    switch (TM_TYPE(parent)) {
    case TYPE_LIST: {
        return (list_index(GET_LIST(parent), child) != -1);
    }
    case TYPE_STR: {
        if (TM_TYPE(child) != TYPE_STR)
            return 0;
        return string_index(GET_STR_OBJ(parent), GET_STR_OBJ(child), 0) != -1;
    }
    case TYPE_DICT: {
        DictNode* node = dict_get_node(GET_DICT(parent), child);
        if (node == NULL) {
            return 0;
        }
        return 1;
    }
    }
    return 0;
}

int obj_not_in(Object child, Object parent) {
    return !obj_in(child, parent);
}

int is_true_obj(Object v) {
    switch (TM_TYPE(v)) {
    case TYPE_NUM:
        return GET_NUM(v) != 0;
    case TYPE_NONE:
        return 0;
    case TYPE_STR:
        return GET_STR_LEN(v) > 0;
    case TYPE_LIST:
        return LIST_LEN(v) > 0;
    case TYPE_DICT:
        return DICT_LEN(v) > 0;
    case TYPE_FUNCTION:
    case TYPE_DATA:
        return 1;
    }
    return 0;
}

Object obj_neg(Object o) {
    if (o.type == TYPE_NUM) {
        GET_NUM(o) = -GET_NUM(o);
        return o;
    }
    tm_raise("obj_neg: can not handle %o", o);
    return NONE_OBJECT;
}


Object iter_new(Object collections) {
    switch(TM_TYPE(collections)) {
        case TYPE_LIST: return list_iter_new(collections);
        case TYPE_DICT: return dict_iter_new(collections);
        case TYPE_STR:  return string_iter_new(collections);
        case TYPE_DATA: return collections;
        default: tm_raise("iter_new(): can not create a iterator of %o", collections);
    }
    return NONE_OBJECT;
}

Object* next_ptr(Object iterator) {
    return GET_DATA(iterator)->next(GET_DATA(iterator));
}

void obj_del(Object self, Object k) {
    switch(TM_TYPE(self)) {
        case TYPE_DICT:{
            dict_del(GET_DICT(self), k);
            break;
        }
        case TYPE_LIST:{
            list_del(GET_LIST(self), k);
            break;
        }
        default:
            tm_raise("obj_del: not supported type %s", tm_type(self.type));
    }
}

Object obj_append(Object a, Object item) {
    if (IS_LIST(a)) {
        list_append(GET_LIST(a), item);
    }
    return a;
}

Object tm_get_global(Object globals, char *key) {
    Object okey = string_new(key);
    DictNode* node = dict_get_node(GET_DICT(globals), okey);
    if (node == NULL) {
        node = dict_get_node(GET_DICT(tm->builtins), okey);
        if (node == NULL) {
            tm_raise("NameError: name %o is not defined", okey);
        }
    }
    return node->val;
}

int tm_len(Object o) {
    int len = -1;
    switch (TM_TYPE(o)) {
    case TYPE_STR:
        len = GET_STR_LEN(o);
    case TYPE_LIST:
        len = LIST_LEN(o);
    case TYPE_DICT:
        len = DICT_LEN(o);
    }
    if (len < 0) {
        tm_raise("tm_len: %o has no attribute len", o);
    }
    return len;
}

// func Object tm_str(Object a)
Object tm_str(Object a) {
    char buf[100];
    memset(buf, 0, sizeof(buf));
    switch (TM_TYPE(a)) {
    case TYPE_STR:
        return a;
    case TYPE_NUM: {
        char s[20];
        double v = GET_NUM(a);
        number_format(s, a);
        return string_new(s);
    }
    case TYPE_LIST: {
        Object str = string_new("");

        str = string_append_char(str, '[');
        int i, l = LIST_LEN(a);
        for (i = 0; i < l; i++) {
            Object obj = GET_LIST(a)->nodes[i];
            /* reference to self in list */
            if (obj_equals(a, obj)) {
                str = string_append_sz(str, "[...]");
            } else if (obj.type == TYPE_STR) {
                str = string_append_char(str, '"');
                str = string_append_obj(str, obj);
                str = string_append_char(str, '"');
            } else {
                str = string_append_obj(str, obj);
            }
            if (i != l - 1) {
                str = string_append_char(str, ',');
            }
        }
        str = string_append_char(str, ']');
        return str;
    }
    case TYPE_DICT:
        sprintf(buf, "<dict at %p>", GET_DICT(a));
        return string_new(buf);
    case TYPE_FUNCTION:
        function_format(buf, a);
        return string_new(buf);
    case TYPE_NONE:
        return sz_to_string("None");
    case TYPE_DATA:
        return GET_DATA(a)->str(GET_DATA(a));
    default:
        tm_raise("str: not supported type %d", a.type);
    }
    return string_alloc("", 0);
}

/** get const id, this will be used to search the const value */
int get_const_id(Object const_value) {
    int i = dict_set(tm->constants, const_value, NONE_OBJECT);
    return i;
}
