//==============================================================================
// xxYUV : Apple AMX Header
//
// Copyright (c) 2021 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#pragma once

#if defined(__APPLE__) && defined(__aarch64__)
//------------------------------------------------------------------------------
//  https://gist.github.com/dougallj/7a75a3be1ec69ca550e7c36dc75e0d6f
//  https://gist.github.com/dougallj/7cba721da1a94da725ee37c1e9cd1f21
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
//------------------------------------------------------------------------------
union amx_operands_access
{
    struct
    {
        uint64_t memory_offset:56;
        uint64_t register_index:6;
        uint64_t double_width:1;
        uint64_t dummy_63:1;
    };
    uint64_t value;
};
//------------------------------------------------------------------------------
union amx_operands_extract
{
    struct
    {
        uint64_t offset_y:10;
        uint64_t offset_x:10;
        uint64_t offset_z:7;
        uint64_t dummy_27:37;
    };
    uint64_t value;
};
//------------------------------------------------------------------------------
union amx_operands_scalar
{
    struct
    {
        uint64_t offset_y:10;
        uint64_t offset_x:10;
        uint64_t offset_z:7;
        uint64_t skip_z:1;
        uint64_t skip_y:1;
        uint64_t skip_x:1;
        uint64_t dummy_30:2;
        uint64_t disable_x:7;
        uint64_t dummy_39:2;
        uint64_t disable_y:7;
        uint64_t dummy_48:13;
        uint64_t mode_8:1;
        uint64_t mode_32:1;
        uint64_t vector_matrix_add:1;
    };
    uint64_t value;
};
//------------------------------------------------------------------------------
union amx_operands_vector
{
    struct
    {
        uint64_t offset_y:10;
        uint64_t offset_x:10;
        uint64_t offset_z:7;
        uint64_t count_y:2;
        uint64_t count_x:2;
        uint64_t dummy_31:1;
        uint64_t mask:10;
        uint64_t extended:4;
        uint64_t dummy_46:1;
        uint64_t neg:1;
        uint64_t add:1;
        uint64_t dummy_49:9;
        uint64_t shift_right:5;
        uint64_t sign:1;
    };
    uint64_t value;
};
//------------------------------------------------------------------------------
union amx_operands_matrix
{
    struct
    {
        uint64_t offset_y:10;
        uint64_t offset_x:10;
        uint64_t offset_z:7;
        uint64_t dummy_27:5;
        uint64_t mask:10;
        uint64_t extended:4;
        uint64_t dummy_46:1;
        uint64_t neg:1;
        uint64_t add:1;
        uint64_t dummy_49:9;
        uint64_t shift_right:5;
        uint64_t sign:1;
    };
    uint64_t value;
};
//------------------------------------------------------------------------------
#define amx_ldx(...)    __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 0 << 5) | 0)" ::"r"((amx_operands_access{__VA_ARGS__})) : "x0", "memory")
#define amx_ldy(...)    __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 1 << 5) | 0)" ::"r"((amx_operands_access{__VA_ARGS__})) : "x0", "memory")
#define amx_stx(...)    __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 2 << 5) | 0)" ::"r"((amx_operands_access{__VA_ARGS__})) : "x0", "memory")
#define amx_sty(...)    __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 3 << 5) | 0)" ::"r"((amx_operands_access{__VA_ARGS__})) : "x0", "memory")
#define amx_ldz(...)    __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 4 << 5) | 0)" ::"r"((amx_operands_access{__VA_ARGS__})) : "x0", "memory")
#define amx_stz(...)    __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 5 << 5) | 0)" ::"r"((amx_operands_access{__VA_ARGS__})) : "x0", "memory")
#define amx_ldzi(...)   __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 6 << 5) | 0)" ::"r"((amx_operands_access{__VA_ARGS__})) : "x0", "memory")
#define amx_stzi(...)   __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 7 << 5) | 0)" ::"r"((amx_operands_access{__VA_ARGS__})) : "x0", "memory")
#define amx_extrx(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 8 << 5) | 0)" ::"r"((amx_operands_extract{__VA_ARGS__})) : "x0", "memory")
#define amx_extry(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | ( 9 << 5) | 0)" ::"r"((amx_operands_extract{__VA_ARGS__})) : "x0", "memory")
#define amx_fma64(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | (10 << 5) | 0)" ::"r"((amx_operands_scalar{__VA_ARGS__})) : "x0", "memory")
#define amx_fms64(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | (11 << 5) | 0)" ::"r"((amx_operands_scalar{__VA_ARGS__})) : "x0", "memory")
#define amx_fma32(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | (12 << 5) | 0)" ::"r"((amx_operands_scalar{__VA_ARGS__})) : "x0", "memory")
#define amx_fms32(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | (13 << 5) | 0)" ::"r"((amx_operands_scalar{__VA_ARGS__})) : "x0", "memory")
#define amx_mac16(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | (14 << 5) | 0)" ::"r"((amx_operands_scalar{__VA_ARGS__})) : "x0", "memory")
#define amx_fma16(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | (15 << 5) | 0)" ::"r"((amx_operands_scalar{__VA_ARGS__})) : "x0", "memory")
#define amx_fms16(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | (16 << 5) | 0)" ::"r"((amx_operands_scalar{__VA_ARGS__})) : "x0", "memory")
#define amx_set()       __asm__ volatile("nop \n nop \n nop \n .word (0x201000 | (17 << 5) | 0)" ::: "memory")
#define amx_clr()       __asm__ volatile("nop \n nop \n nop \n .word (0x201000 | (17 << 5) | 1)" ::: "memory")
#define amx_vecint(...) __asm__ volatile("mov x0, %0        \n .word (0x201000 | (18 << 5) | 0)" ::"r"((amx_operands_vector{__VA_ARGS__})) : "x0", "memory")
#define amx_vecfp(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | (19 << 5) | 0)" ::"r"((amx_operands_vector{__VA_ARGS__})) : "x0", "memory")
#define amx_matint(...) __asm__ volatile("mov x0, %0        \n .word (0x201000 | (20 << 5) | 0)" ::"r"((amx_operands_matrix{__VA_ARGS__})) : "x0", "memory")
#define amx_matfp(...)  __asm__ volatile("mov x0, %0        \n .word (0x201000 | (21 << 5) | 0)" ::"r"((amx_operands_matrix{__VA_ARGS__})) : "x0", "memory")
#define amx_gemlut(...) __asm__ volatile("mov x0, %0        \n .word (0x201000 | (22 << 5) | 0)" ::"r"((amx_operands_access{__VA_ARGS__})) : "x0", "memory")
//------------------------------------------------------------------------------
inline void amx_dump(int index, int16_t hint)
{
    uint8_t row[64];
    amx_stz( .memory_offset = (uint64_t)row, .register_index = (uint64_t)index );
    printf("%2d : ", index);
    for (int i = 0; i < 64; ++i)
    {
        if (i == 0)
        {
            printf("(%04X) ", __builtin_bswap16(hint) & 0xFFFF);
        }
        printf("%02X", row[i]);
        if (i % 8 == 7)
            printf(" ");
    }
    printf("\n");
}
//------------------------------------------------------------------------------
#endif
