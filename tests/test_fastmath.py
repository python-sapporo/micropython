import array
import gc
class pyndarray(object):
    def __init__(self, shape:Tuple[int]):
        size = 1
        for n in shape:
            size *= n
        self.ndim = len(shape)
        self.size = size
        self.shape = shape
        self.values = array.array('f')
        for i in range(size):
            self.values.append(0.0)

    def fill(self, value:float):
        for i in range(self.size):
            self.values[i] = value
    
    def set(self, values:List[float]):
        if len(values) != self.size:
            raise ValueError()
        for i in range(self.size):
            self.values[i] = float(values[i])
    
    def __add__(self, rhs:pyndarray):
        if self.size != rhs.size:
            raise ValueError()
        r = pyndarray(self.shape)
        for i in range(self.size):
            r.values[i] = self.values[i] + rhs.values[i]
        return r
    
    def __sub__(self, rhs:pyndarray):
        if self.size != rhs.size:
            raise ValueError()
        r = pyndarray(self.shape)
        for i in range(self.size):
            r.values[i] = self.values[i] - rhs.values[i]
        return r

    def __mul__(self, rhs:pyndarray) -> pyndarray:
        if self.ndim > 2: raise ValueError('invalid size')
        if rhs.ndim  > 2: raise ValueError('invalid size')
        n_cols_l = self.shape[0]
        n_rows_r = rhs.shape[-1]
        if n_cols_l != n_rows_r: raise ValueError('dimension mismatch')
        n_rows_l = self.shape[1] if self.ndim == 2 else 1
        n_cols_r = rhs.shape[0]
        result = pyndarray((n_cols_r, n_rows_l))
        for r in range(n_rows_l):
            for c in range(n_cols_r):
                s = 0
                for v in range(n_cols_l):
                    vl = self.values[v + r*n_cols_l]
                    vr = rhs.values[c + v*n_cols_r]
                    s += vl*vr
                result.values[c + r*n_cols_r] = s
        return result

    def __getitem__(self, indices:Tuple[int]) -> float:
        block_size = 1
        offset = 0
        for dim_index in range(len(indices)):
            index_value = indices[dim_index]
            if index_value < 0 or self.shape[dim_index] <= index_value:
                raise KeyError("Index out of range")
            offset += block_size * index_value
            block_size *= self.shape[dim_index]
        return self.values[offset]
    def __setitem__(self, indices:Tuple[int], value:float) -> None:
        block_size = 1
        offset = 0
        for dim_index in range(len(indices)):
            index_value = indices[dim_index]
            if index_value < 0 or self.shape[dim_index] <= index_value:
                raise KeyError("Index out of range")
            offset += block_size * index_value
            block_size *= self.shape[dim_index]
        self.values[offset] = value

def add(lhs:int, rhs:int) -> int:
    return lhs+rhs

# @micropython.native
# def add_native(lhs:int, rhs:int) -> int:
#     return lhs+rhs
# 
# @micropython.viper
# def add_viper(lhs:int, rhs:int) -> int:
#     return lhs+rhs

def test(ndarray):
    a = ndarray((3,4))
    assert a.ndim == 2, 'a.ndim'
    assert a.shape == (3,4), 'a.shape'
    assert a.size == 3*4, 'a.size'
    for y in range(4):
        for x in range(3):
            assert a[x,y] == 0
    a.fill(1.0)
    for y in range(4):
        for x in range(3):
            assert a[x,y] == 1.0
    b = ndarray(a.shape)
    assert b.ndim == 2
    assert b.shape == a.shape
    assert b.size  == a.size
    b.fill(2.0)
    for y in range(4):
        for x in range(3):
            assert b[x,y] == 2.0
    c = a - b
    for y in range(4):
        for x in range(3):
            assert c[x,y] == -1.0
    
    l = ndarray((4,3))
    r = ndarray((2,4))
    l.set([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12])
    r.set([1, 2, 3, 4, 5, 6, 7, 8])
    m = l*r
    assert m.shape == (2,3)
    for y in range(3):
        for x in range(2):
            print(m[x,y], end=' ')
        print()

def process_add(add_func):
    for i in range(500000):
        a = 1
        b = -3
        add_func(a, b)

def process(ndarray):
    for i in range(10000):
        a = ndarray((4,4))
        a.fill(1.0)
        b = ndarray(a.shape)
        b.fill(2.0)
        c = a - b
        a.set([0, 1, 2, 3, 
            4, 5, 6, 7,
            8, 9, 0, 1,
            2, 3, 4, 5,])
        d = a * a

import ufastmath
print('Test pyndarray')
test(pyndarray)
print('Test ufastmath')
test(ufastmath.ndarray)

import utime as time

print('Bench python add')
gc.collect()
gc.collect()
t = time.ticks_ms()
process_add(add)
print(time.ticks_ms() - t) 

# print('Bench python add native')
# t = time.ticks_ms()
# process_add(add_native)
# print(time.ticks_ms() - t)
# 
# print('Bench python add viper')
# t = time.ticks_ms()
# process_add(add_viper)
# print(time.ticks_ms() - t)

print('Bench ufastmath add')
gc.collect()
gc.collect()
t = time.ticks_ms()
process_add(ufastmath.add)
print(time.ticks_ms() - t) 

print('Bench pyndarray')
gc.collect()
gc.collect()
t = time.ticks_ms()
process(pyndarray)
print(time.ticks_ms() - t)

print('Bench ufastmath')
gc.collect()
gc.collect()
t = time.ticks_ms()
process(ufastmath.ndarray)
print(time.ticks_ms() - t)
