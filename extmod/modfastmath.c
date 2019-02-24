/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/mpconfig.h"

#include <string.h>
#include <stdlib.h>

#include "py/runtime.h"
#include "py/smallint.h"
#include "py/mphal.h"


#define MAX_DIMS 3

typedef struct _mp_obj_ndarray_t {
    mp_obj_base_t base;
    size_t ndims;
    size_t dims[MAX_DIMS];
    size_t length;
    mp_float_t array[];
} mp_obj_ndarray_t;

STATIC mp_obj_ndarray_t* mat_product(const mp_obj_ndarray_t* lhs, const mp_obj_ndarray_t* rhs)
{
    if( lhs->ndims > 2 || rhs->ndims > 2 ) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Invalid number of dimensions"));
    }
    size_t n_cols_l = lhs->dims[0];
    size_t n_rows_r = rhs->dims[rhs->ndims-1];
    if( n_cols_l != n_rows_r ) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "dimension mismatch"));
    }
    size_t n_rows_l = lhs->ndims == 2 ? lhs->dims[1] : 1;
    size_t n_cols_r = rhs->dims[0];
    mp_obj_ndarray_t* result = m_new_obj_var(mp_obj_ndarray_t, mp_float_t, n_rows_l*n_cols_r);
    result->base.type = lhs->base.type;
    result->ndims = 2;
    result->dims[0] = n_cols_r;
    result->dims[1] = n_rows_l;
    result->length = n_rows_l*n_cols_r;
    for( size_t r = 0; r < n_rows_l; r++ ) {
        for( size_t c = 0; c < n_cols_r; c++ ) {
            mp_float_t s = 0;
            for( size_t v = 0; v < n_cols_l; v++ ) {
                mp_float_t vl = lhs->array[v + r*n_cols_l];
                mp_float_t vr = rhs->array[c + v*n_cols_r];
                s += vl*vr;
            }
            result->array[c + r*n_cols_r] = s;
        }
    }
    return result;
}

STATIC mp_obj_ndarray_t* make_same_shape(mp_obj_ndarray_t* base) {
    mp_obj_ndarray_t* o = m_new_obj_var(mp_obj_ndarray_t, mp_float_t, base->length);
    o->base.type = base->base.type;
    o->ndims = base->ndims;
    memcpy(o->dims, base->dims, sizeof(size_t)*MAX_DIMS);
    o->length = base->length;
    return o;
}
STATIC mp_obj_t ndarray_make_new(const mp_obj_type_t* type, size_t n_args, size_t n_kw, const mp_obj_t* args) {
    // 引き数の個数のチェック (引き数の個数 = 1個)
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    size_t dim_count = 0;
    mp_obj_t* dims_array = NULL;
    size_t dims_values[MAX_DIMS] = {0};
    size_t length = 1;
    
    // 第1引き数は多次元配列の形状を表すリストなので内容を取得する
    mp_obj_get_array(args[0], &dim_count, &dims_array);
    if( dim_count > MAX_DIMS ) {
        // 次元数が多すぎ
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Too many dimensions"));
    }
    // 各次元の要素数をコピーしつつ、総要素数を計算
    for(size_t dim_index = 0; dim_index < dim_count; dim_index++ ) {
        mp_int_t dim = mp_obj_get_int(dims_array[dim_index]);
        dims_values[dim_index] = (size_t)dim;
        length *= (size_t)dim;
    }
    // インスタンスを作成
    // m_new_obj_varは末尾に可変長の配列をつけることができる。
    mp_obj_ndarray_t* o = m_new_obj_var(mp_obj_ndarray_t, mp_float_t, length);
    o->base.type = type;    // 型 (ndarray_type)
    o->ndims = dim_count;   // 形状の情報をコピー
    memcpy(o->dims, dims_values, sizeof(dims_values));
    o->length = length;
    return MP_OBJ_FROM_PTR(o);  // 作成したインスタンスをmp_obj_tとして返す。
}

STATIC mp_obj_t ndarray_fill(mp_obj_t self_in, mp_obj_t value)
{
    mp_obj_ndarray_t* self = MP_OBJ_TO_PTR(self_in);    // インスタンスの実体を取得
    mp_float_t value_float = mp_obj_get_float(value);   // 引き数をfloat型として取得
    mp_float_t* p = self->array;
    for(size_t i = 0; i < self->length; i++, p++) {     // 全要素に値を設定
        *p = value_float;
    }
    return mp_const_none;   // 戻り値がないのでNoneを返す。
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(ndarray_fill_obj, ndarray_fill);

STATIC mp_obj_t ndarray_set(mp_obj_t self_in, mp_obj_t values)
{
    mp_obj_ndarray_t* self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t* objs = NULL;
    size_t count = 0;
    mp_obj_get_array(values, &count, &objs);

    if( count != self->length ) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Length mismatch"));
    }
    for(size_t i = 0; i < self->length; i++ ) {
        self->array[i] = mp_obj_get_float(objs[i]);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(ndarray_set_obj, ndarray_set);

STATIC mp_obj_t ndarray_binary_op(mp_binary_op_t op, mp_obj_t lhs, mp_obj_t rhs)
{
    switch(op) {
    case MP_BINARY_OP_ADD:
    case MP_BINARY_OP_SUBTRACT: {   // 加減算
        mp_obj_ndarray_t* lhs_obj = MP_OBJ_TO_PTR(lhs);
        mp_obj_ndarray_t* rhs_obj = MP_OBJ_TO_PTR(rhs);
        if( lhs_obj->length != rhs_obj->length ) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Length mismatch"));
        }
        
        mp_obj_ndarray_t* r = make_same_shape(lhs_obj);
        if( op == MP_BINARY_OP_ADD ) {
            for(size_t i = 0; i < lhs_obj->length; i++ ) {
                r->array[i] = lhs_obj->array[i] + rhs_obj->array[i];
            }
        } else {
            for(size_t i = 0; i < lhs_obj->length; i++ ) {
                r->array[i] = lhs_obj->array[i] - rhs_obj->array[i];
            }
        }
        return MP_OBJ_FROM_PTR(r);
    }
    case MP_BINARY_OP_MULTIPLY: {   // 乗算
        mp_obj_ndarray_t* lhs_obj = MP_OBJ_TO_PTR(lhs);
        mp_obj_ndarray_t* rhs_obj = MP_OBJ_TO_PTR(rhs);
        mp_obj_ndarray_t* result_obj = mat_product(lhs_obj, rhs_obj);
        return MP_OBJ_FROM_PTR(result_obj);
    }
    default:
        return MP_OBJ_NULL;
    }
}

STATIC void ndarray_attr(mp_obj_t self_in, qstr attr, mp_obj_t* dest)
{
    mp_obj_ndarray_t* self = MP_OBJ_TO_PTR(self_in);
    bool is_load = dest[0] == MP_OBJ_NULL;

    switch(attr)
    {
    case MP_QSTR_set: { // obj.set()
        if( is_load ) {
            dest[0] = (mp_obj_t)&ndarray_set_obj;
            dest[1] = self_in;
        }
        break;
    }
    case MP_QSTR_fill: { // obj.fill()
        if( is_load ) {
            dest[0] = (mp_obj_t)&ndarray_fill_obj;
            dest[1] = self_in;
        }
        break;
    }
    case MP_QSTR_size: { // obj.size
        if( is_load ) {
            size_t size = 1;
            for(size_t i = 0; i < self->ndims; i++) {
                size *= self->dims[i];
            }
            dest[0] = mp_obj_new_int(size);
        }
        break;
    }
    case MP_QSTR_ndim: { // obj.ndim
        if( is_load ) {
            dest[0] = mp_obj_new_int(self->ndims);
        }
        break;
    }
    case MP_QSTR_shape: { // obj.shape
        if( is_load ) {
            mp_obj_t dims[MAX_DIMS];
            for(size_t i = 0; i < self->ndims; i++) {
                dims[i] = mp_obj_new_int(self->dims[i]);
            }
            dest[0] = mp_obj_new_tuple(self->ndims, dims);
        }
        break;
    }
    }
}

STATIC mp_obj_t ndarray_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    mp_obj_ndarray_t* self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t* index_array = NULL;
    size_t index_count = 0;
    mp_obj_get_array(index, &index_count, &index_array);
    if( index_count != self->ndims ) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_KeyError, "Invalid number of indices"));
    }
    size_t block_size = 1;  // 今見ている次元のインデックスあたりの要素数
    size_t offset = 0;      // 要素のオフセット
    for(size_t dim_index = 0; dim_index < index_count; dim_index++ ) {
        mp_int_t index_value = mp_obj_get_int(index_array[dim_index]);
        if( index_value < 0 || self->dims[dim_index] <= index_value ) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_KeyError, "Index out of range"));
        }
        offset += block_size * index_value;     // オフセットに現在のブロック内での要素位置を加算
        block_size *= self->dims[dim_index];    // ブロックサイズに要素数をかけて新たなブロックサイズを計算
    }

    if( value != MP_OBJ_SENTINEL && value != MP_OBJ_NULL ) {    // ストアの場合、値を設定する。
        self->array[offset] = mp_obj_get_float(value);
    }
    return mp_obj_new_float(self->array[offset]);   // 要素の値を返す
}

STATIC const mp_rom_map_elem_t ndarray_locals_dict_table[] = {
    //{ MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&ndarray_fill_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ndarray_locals_dict, ndarray_locals_dict_table);

STATIC const mp_obj_type_t ndarray_type = {
    { &mp_type_type },
    .name = MP_QSTR_ndarray,
    .make_new    = ndarray_make_new,
    .attr        = ndarray_attr,
    .subscr      = ndarray_subscr,
    .binary_op   = ndarray_binary_op,
    .locals_dict = (void*)&ndarray_locals_dict,
};

// 加算処理の本体
STATIC mp_obj_t mod_fastmath_add(mp_obj_t a_in, mp_obj_t b_in) {
    mp_int_t a = mp_obj_get_int(a_in); // 引数を整数に変換(失敗するとエラー)
    mp_int_t b = mp_obj_get_int(b_in); // /
    return mp_obj_new_int(a + b);      // 加算結果を表すintオブジェクトを作って返す
}
// 加算関数を呼び出すための関数オブジェクトを定義
// 引数が2つなので MP_DEFINE_CONST_FUN_OBJ_2 を使う
// 0～3引数の関数まで定義できる
// それ以上は可変引数関数として定義する必要がある
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_fastmath_add_obj, mod_fastmath_add);

STATIC mp_obj_t mod_fastmath_opadd(mp_obj_t a_in, mp_obj_t b_in) {
    return mp_binary_op(MP_BINARY_OP_ADD, a_in, b_in);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_fastmath_opadd_obj, mod_fastmath_opadd);

STATIC const mp_rom_map_elem_t mp_module_fastmath_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_fastmath) },
    { MP_ROM_QSTR(MP_QSTR_add), MP_ROM_PTR(&mod_fastmath_add_obj) },
    { MP_ROM_QSTR(MP_QSTR_opadd), MP_ROM_PTR(&mod_fastmath_opadd_obj) },
    { MP_ROM_QSTR(MP_QSTR_ndarray), MP_ROM_PTR(&ndarray_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_fastmath_globals, mp_module_fastmath_globals_table);

const mp_obj_module_t mp_module_fastmath = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_fastmath_globals,
};
