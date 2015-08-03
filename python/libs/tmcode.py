from boot import *
cheader = '''/*
* @author xupingmao<578749341@qq.com>
* @generated by Python
* @date %s
*/
#ifndef INSTRUCTION_H_
#define INSTRUCTION_H_
'''
ctail = "\n#endif\n\n"

_opcode_names = [
    'NEW_STRING', 'NEW_NUMBER',
    'ADD', 'SUB', 'MUL', 'DIV', 'MOD', 'NEG',
    'NOT', 'GT', 'LT', 'GTEQ', 'LTEQ', 'EQEQ', 'NOTEQ', 'OP_IN', 'NOTIN',
    'AND', 'OR', 'SET', 'GET',
    'LOAD_NONE', 'STORE_LOCAL', 'STORE_GLOBAL', 'LOAD_LOCAL', 'LOAD_GLOBAL', 'LOAD_CONSTANT',
    'LOAD_GLOBALS', 'POP', 
    'DICT_SET', 'LIST', 'DICT', 'LIST_APPEND',
    'JUMP', 'UP_JUMP', 'POP_JUMP_ON_FALSE', 'JUMP_ON_FALSE', 'JUMP_ON_TRUE',
    #TAGSIZE
    'TM_UNARRAY', 'TM_ROT', 'TM_DEL', 'TM_FOR', 'TM_NEXT', 'ITER_NEW', 'LOAD_EX',
    'SETJUMP', 'CALL', 'TM_DEF', 'RETURN', 'LOAD_PARAMS', 'TM_NARG', 'TM_EOF',
    'TM_EOP',
    # mulity assignment, eg. x,y = 1,2
    'TM_DEBUG',
    'TAG', 
    # unused instructions
    'LT_JUMP_ON_FALSE', 
    'GT_JUMP_ON_FALSE',
    'LTEQ_JUMP_ON_FALSE',
    'GTEQ_JUMP_ON_FALSE',
    'EQEQ_JUMP_ON_FALSE',
    'NOTEQ_JUMP_ON_FALSE'
]

# update global values.
i = 0
while i < len(_opcode_names):
    name = _opcode_names[i]
    globals()[name] = i + 1
    i += 1

def export_clang_define(des):
    # check modified time
    # if src is modified before des and cdes, do not convert.
    if not exists(des):
        pass
    elif mtime(ARGV[0]) > mtime(des):
        return
    defines = []
    i = 0
    while i < len(_opcode_names):
        x = _opcode_names[i]
        defines.append('#define '+ x + ' ' + str(i+1))
        i += 1
    ctext = cheader % str(asctime()) + '\n'.join(defines) + ctail
    save(des, ctext)
    # save('../include/instruction.h', ctext)

if __name__ == '__main__':
    export_clang_define('instruction.h')