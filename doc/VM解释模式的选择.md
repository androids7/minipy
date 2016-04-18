### �������������ջ�ͻ��ڼĴ���

#### ����ջ
1. ����ʵ�֣����÷���Ĵ���
2. ռ�õ��ֽ��볤��СһЩ
3. �����׷����룬�������ֽ��뼶���Ż��ĳ̶ȿ��Ը����ŵ�&ȱ�㣩

#### ���ڼĴ���
1. ��Ҫ����Ĵ�������΢����һ�㣬����ʹ������Ĵ������ѶȲ���
2. һ��ָ���������������䣬�����в���ջ��GC��ʱ���ñ�ǲ���ջ
3. ����ִ�и��죬��Ϊû�д�����ջ����
4. ָ�����


### ѡ��

һ��ʼ��ѡ���˻���ջ��������������Ŀ���Ҳ�ǻ���ջ�������ģ����ڳ����޸ĳɻ��ڼĴ����ģ���Ҫԭ����

1. ϣ����tm2c�м���GC�����ڼĴ����ĸ��ô���

����
```
def convert(a):
    # ����GC���
    # ����ջ��Ҫ����op_stack��locals
    # ���ڼĴ�����ֻ��Ҫ����locals
    # ����ֻ��Ҫ
    # tm2c_push_locals(locals, localsCnt);
    # // function body
    # // check gc
    # tm2c_pop_locals(locals, localsCnt);
    # return ret_val;
    
    # ����ջ����Ҫ
    # tm2c_push_locals(locals, localsCnt);
    # // function body
    # // check gc state (op_stack is empty?). check gc.
    # tm2c_pop_locals(locals, localsCnt);
    return a * 10;

def innerFunc(a,b):
    print(a,b)

def test():
    innerFunc(convert(a), convert(b));
    # ���ڼĴ���
    # t1 = convert(a)
    # t2 = convert(b)
    # innerFunc(t1,t2)
    
    # ����ջ
    # innerFunc(convert(a), convert(b));
    
```

����ʹ��ջ�Ļ����Ը�Ϊ
```
innerFunc(convert(a), convert(b));

==>
t1 = convert(a)
t2 = convert(b)
innerFunc(t1,t2)
```

Ҳ����˵����������ȫ��ʹ���м�������洢(�ͻ��ڼĴ�����һ��),����ֻ��Ҫ�޸�tm2c.

2. ��������

2.1 ��ͨ����
```
global:a = b + c

register-based
r2 = r0 + r2 # locals[1] = locals[0] + locals[1]
setglobal a , r2

stack-based
push b
push c
add # stack[0] = b
    # stack[1] = c
    # stack[0] = stack[0] + stack[1]
storeglobal a
```
register-based win!

2.2 ��������
```
print(convert(a,b))

register-based
push a             # stack[0] = a
push b             # stack[1] = b
r = call convert   # locals[2] = call convert
push r             # stack[0] = locals[2]
call print         # call print

stack-based
push print         # stack[0] = print
push convert       # stack[1] = convert
push a             # stack[2] = a
push b             # stack[3] = b
call 2             # call convert
call 1             # call print
pop                # stack[0] = empty
```

register-based win!

### ʵ�ʲ��Խ��

2016.04.03
global 
stack-based (optimized) : 500
reg-based               : 2000

local
stack-based : 400
reg-based   : 10000

unbelievable!