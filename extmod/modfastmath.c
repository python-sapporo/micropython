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

// モジュールに含まれる関数などの名前とオブジェクトの対応関係を保持する辞書の要素を定義する
STATIC const mp_rom_map_elem_t mp_module_fastmath_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_fastmath) },   // __name__
    { MP_ROM_QSTR(MP_QSTR_add), MP_ROM_PTR(&mod_fastmath_add_obj) },    // add(a, b)
};
// ↑の要素を持つ辞書オブジェクトを定義する
STATIC MP_DEFINE_CONST_DICT(mp_module_fastmath_globals, mp_module_fastmath_globals_table);

// fastmath モジュールのオブジェクトを定義する
const mp_obj_module_t mp_module_fastmath = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_fastmath_globals,
};