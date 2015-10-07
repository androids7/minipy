#include "include/gc.h"
#include "include/tm.h"

/**
 * 1. mark all allocated object to be unused (0);
 * 2. mark objects can be reached from `root` to be used (1);
 * 3. mark objects can be reached from `frames` to be used (1);
 * 4. release objects which are marked unused (0).
 * 
 */

#define GC_CONSTANS_LEN 10
#define GC_REACHED_SIGN 1
#define GC_MARKED(o) (o).value.gc->marked

#define GC_DEBUG 0
#define get_char(n) ARRAY_CHARS[n]

#if GC_DEBUG
void log_debug(char* fmt, ...) {
    FILE* fp = fopen("gc.log", "a");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
    fclose(fp);
}
#endif


void gcInit() {
    int init_size = 100;
    int i;
    
    /* initialize constants */
    NUMBER_TRUE = tm_number(1);
	NUMBER_FALSE = tm_number(0);
	NONE_OBJECT.type = TYPE_NONE;
    UNDEF.type = -1;
    
    tm->init = 0;
    tm->debug = 0;
	tm->allocated = 0;
	tm->gcThreshold = 1024 * 8; // set 8k to see gc process

	tm->all = list_alloc_untracked(init_size);
    
    tm->root = list_new(100);
	tm->builtins = dict_new();
	APPEND(tm->root, tm->builtins);
	tm->modules = dict_new();
	APPEND(tm->root, tm->modules);

	tm->exList = list_new(10);
	APPEND(tm->root, tm->exList);
	
	tm->constants = dict_new();
	APPEND(tm->root, tm->constants);

    /* initialize chars */
	ARRAY_CHARS = list_new(256);
	for (i = 0; i < 256; i++) {
		APPEND(ARRAY_CHARS, newChar(i));
	}
	APPEND(tm->root, ARRAY_CHARS);
    
    tm->ex = NONE_OBJECT;
    
    /* initialize frames */
	tm->framesInitDone = 0;
	for (i = 0; i < FRAMES_COUNT; i++) {
		TmFrame* f = tm->frames + i;
		f->stack = f->top = tm->stack;
		/* f->ex = NONE_OBJECT; */
		// f->line = NONE_OBJECT;
		f->fnc = NONE_OBJECT;
		f->pc = NULL;
		f->maxlocals = 0;
		f->maxstack = 0;
		f->jmp = NULL;
		f->idx = i;
	}
	tm->framesInitDone = 1;
	tm->frame = tm->frames;
    tm_stack_end = tm->stack + STACK_SIZE;
}


void* tm_malloc(size_t size) {
    void* block;
    Object* func;

	if (size <= 0) {
		tmRaise("tm_malloc, attempts to allocate a memory block of size %d!", size);
		return NULL;
	}
	block = malloc(size);
#if GC_DEBUG
    if (size > 100)
        log_debug("%d -> %d , +%d\n", tm->allocated, tm->allocated + size, size);
#endif
	if (block == NULL) {
		tmRaise("tm_malloc: fail to malloc memory block of size %d", size);
	}
	tm->allocated += size;
	return block;
}

void* tm_realloc(void* o, size_t osize, size_t nsize) {
	void* block = tm_malloc(nsize);
	memcpy(block, o, osize);
	tm_free(o, osize);
	return block;
}

void tm_free(void* o, size_t size) {
	if (o == NULL)
		return;
#if GC_DEBUG
    if (size > 100)
	log_debug("Free %p, %d -> %d , -%d\n",o, tm->allocated, tm->allocated - size, size);
	if(size<=0) {
		tmRaise("tm_free: you are free a block of size %d", size);
	}
#endif
	free(o);
	tm->allocated -= size;
}

Object gc_track(Object v) {
	switch (v.type) {
	case TYPE_NUM:
	case TYPE_NONE:
		return v;
	case TYPE_STR:
		v.value.str->marked = GC_REACHED_SIGN;
		break;
	case TYPE_LIST:
		GET_LIST(v)->marked = GC_REACHED_SIGN;
		break;
	case TYPE_DICT:
		GET_DICT(v)->marked = GC_REACHED_SIGN;
		break;
	case TYPE_MODULE:
		GET_MODULE(v)->marked = GC_REACHED_SIGN;
		break;
	case TYPE_FUNCTION:
		GET_FUNCTION(v)->marked = GC_REACHED_SIGN;
		break;
	case TYPE_DATA:
		GET_DATA(v)->marked = GC_REACHED_SIGN;
		break;
	default:
		tmRaise("gc_track(), not supported type %d", v.type);
		return v;
	}
    SET_IDX(v, 0);
	_listAppend(tm->all, v);
	return v;
}

void gcMarkList(TmList* list) {
	if (list->marked)
		return;
	list->marked = GC_REACHED_SIGN;
	int i;
	for (i = 0; i < list->len; i++) {
		gcMark(list->nodes[i]);
	}
}
/*
void gcMarkDict(TmDict* dict) {
	if (dict->marked)
		return;
	dict->marked = GC_REACHED_SIGN;
	DictNode* node = dict->head;
	while (node != NULL) {
		gcMark(node->key);
		gcMark(node->val);
		node = node->next;
	}
}
 */
 
 void gcMarkDict(TmDict* dict) {
	if (dict->marked)
		return;
	dict->marked = GC_REACHED_SIGN;
	int i;
    for(i = 0; i < dict->cap; i++) {
        if (dict->nodes[i].used) {
            gcMark(dict->nodes[i].key);
            gcMark(dict->nodes[i].val);
        }
    }
}

void gcMark(Object o) {
	if (o.type == TYPE_NUM || o.type == TYPE_NONE)
		return;
	switch (o.type) {
	case TYPE_STR: {
		if (o.value.str->marked)
			return;
		o.value.str->marked = GC_REACHED_SIGN;
		break;
	}
	case TYPE_LIST:
		gcMarkList(GET_LIST(o));
		break;
	case TYPE_DICT:
		gcMarkDict(GET_DICT(o));
		break;
	case TYPE_FUNCTION:
		if (GET_FUNCTION(o)->marked)
			return;
		GET_FUNCTION(o)->marked = GC_REACHED_SIGN;
		gcMark(GET_FUNCTION(o)->mod);
		gcMark(GET_FUNCTION(o)->self);
		gcMark(GET_FUNCTION(o)->name);
		break;
	case TYPE_MODULE:
		if (GET_MODULE(o)->marked)
			return;
		GET_MODULE(o)->marked = GC_REACHED_SIGN;
		gcMark(GET_MODULE(o)->code);
		gcMark(GET_MODULE(o)->file);
		/*gcMark(GET_MODULE(o)->constants);*/
		gcMark(GET_MODULE(o)->globals);
		break;
	case TYPE_DATA:
		if (GET_DATA(o)->marked)
			return;
		GET_DATA(o)->marked = GC_REACHED_SIGN;
		GET_DATA(o)->proto->mark(GET_DATA(o));
		break;
	default:
		tmRaise("gcMark(), unknown object type %d", o.type);
	}
}

void gcMarkLocalsAndStack() {
    TmFrame* f;
	for(f = tm->frames + 1; f <= tm->frame; f++) {
		gcMark(f->fnc);
		int j;
		/* mark locals */
		for(j = 0; j < f->maxlocals; j++) {
			gcMark(f->locals[j]);
		}
		/* mark operand stack */
		Object* temp;
		for(temp = f->stack; temp <= f->top; temp++) {
			gcMark(*temp);
		}
	}
}

void gcWipe() {
	int n, i;

	TmList* temp = list_alloc_untracked(200);
	TmList* all = tm->all;
	for (i = 0; i < all->len; i++) {
		if (GC_MARKED(tm->all->nodes[i])) {
			_listAppend(temp, all->nodes[i]);
		} else {
			objectFree(all->nodes[i]);
		}
	}
	list_free(tm->all);
	tm->all = temp;
}

#define MARK(v) \
    switch( v.type ){  \
    case TYPE_STR: \
        v.value.str->marked = 0;\
        break;\
    case TYPE_LIST:\
        GET_LIST(v)->marked = 0;\
        break;\
    case TYPE_DICT:\
        GET_DICT(v)->marked = 0;\
        break;\
    case TM_MOD:\
        GET_MODULE(v)->marked = 0;\
        break;\
    case TYPE_FUNCTION:\
        GET_FUNCTION(v)->marked = 0;\
        break;\
    }

/**
 * mark and sweep garbage collection
 *
 * TODO maybe we can mark the object with different value to
 * recognize the GC type of the object.
 */
void gcFull() {
	int i;
	long t1, t2;
	t1 = clock();
#if GC_DEBUG
	int old = tm->allocated;
#endif
    /* mark all objects to be unused */
	for (i = 0; i < tm->all->len; i++) {
		GC_MARKED(tm->all->nodes[i]) = 0;
	}
	gcMark(tm->root);
	gcMarkLocalsAndStack();

	/* wipe garbage */
	gcWipe();
	tm->gcThreshold = tm->allocated * 2;
	t2 = clock();
#if GC_DEBUG
    log_debug("fullGC %dK => %dK, elasped time = %ld\n",
			old / 1024, tm->allocated / 1024, t2 - t1);
#endif
}

void gcFree() {
	TmList* all = tm->all;
	int i;
	for (i = 0; i < all->len; i++) {
		objectFree(all->nodes[i]);
	}

	list_free(tm->all);

#if !PRODUCT
	if (tm->allocated != 0) {
		printf("\n***memory leak happens***\ntm->allocated=%d\n", tm->allocated);
	}
#endif
}


Object newObj(int type, void * value) {
	Object o;
	TM_TYPE(o) = type;
	switch (type) {
	case TYPE_NUM:
		o.value.num = *(double*) value;
		break;
	case TYPE_STR:
		o.value.str = value;
		break;
	case TYPE_LIST:
		GET_LIST(o) = value;
		break;
	case TYPE_DICT:
		GET_DICT(o) = value;
		break;
	case TYPE_MODULE:
		GET_MODULE(o) = value;
		break;
	case TYPE_FUNCTION:
		GET_FUNCTION(o) = value;
		break;
	case TYPE_NONE:
		break;
	default:
		tmRaise("obj_new: not supported type %d", type);
	}
	return o;
}


void objectFree(Object o) {
	switch (TM_TYPE(o)) {
	case TYPE_STR:
		string_free(GET_STR_OBJ(o));
		break;
	case TYPE_LIST:
		list_free(GET_LIST(o));
		break;
	case TYPE_DICT:
		freeDict(GET_DICT(o));
		break;
	case TYPE_FUNCTION:
		functionFree(GET_FUNCTION(o));
		break;
	case TYPE_MODULE:
		moduleFree(o.value.mod);
		break;
	case TYPE_DATA:
		GET_DATA_PROTO(o)->free(GET_DATA(o));
		break;
	}
}

Object* baseNext(TmBaseIterator* iterator) {
    iterator->ret = callFunction2(iterator->func);
    if (iterator->ret.type != -1) {
        return &iterator->ret;
    } else {
        return NULL;
    }
}

void baseMark(DataObject* data) {
    gcMark(((TmBaseIterator*)data)->func);
}

DataProto* getBaseIterProto() {
	if(!baseIterProto.init) {
		initDataProto(&baseIterProto);
		baseIterProto.dataSize = sizeof(TmBaseIterator);
		baseIterProto.next = baseNext;
        baseIterProto.mark = baseMark;
	}
	return &baseIterProto;
}

Object* dataNext(DataObject* data) {
    tmRaise("next is not defined!");
    return NULL;
}

DataProto* getDefaultDataProto() {
	if(!defaultDataProto.init) {
		defaultDataProto.init = 1;
		defaultDataProto.mark = dataMark;
		defaultDataProto.free = dataFree;
		defaultDataProto.next = dataNext;
		defaultDataProto.get = dataGet;
		defaultDataProto.set = dataSet;
		defaultDataProto.str = dataStr;
		defaultDataProto.dataSize = sizeof(DataProto);
	}
	return &defaultDataProto;
}

void initDataProto(DataProto* proto) {
	proto->mark = dataMark;
	proto->free = dataFree;
	proto->get = dataGet;
	proto->set = dataSet;
	proto->next = dataNext;
	proto->str = dataStr;
	proto->init = 1;
	proto->dataSize = sizeof(TmData);
}

Object dataNew(size_t dataSize) {
	Object data;
	data.type = TYPE_DATA;
	GET_DATA(data) = tm_malloc(dataSize);
/*	GET_DATA_PROTO(data)->next = dataNext;
	GET_DATA_PROTO(data)->mark = dataMark;
	GET_DATA_PROTO(data)->free = dataFree;
	GET_DATA_PROTO(data)->get = dataGet;
	GET_DATA_PROTO(data)->set = dataSet;*/
	GET_DATA_PROTO(data) = getDefaultDataProto();
	return gc_track(data);
}

void dataMark(DataObject* data) {
    /* */
}

void dataFree(TmData* data) {
	tm_free(data, data->proto->dataSize);
}

Object dataGet(Object self, Object key) {
	return NONE_OBJECT;
}

void dataSet(Object self, Object key, Object value) {

}

Object dataStr(Object self) {
	return string_alloc("data", -1);
}
