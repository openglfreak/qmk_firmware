#include "../via/keymap.c"

// Lower matrix delay

#if 1
#include "wait.h"

void matrix_output_select_delay2(uint8_t line) {
    if (line <= 8)
        wait_cpuclock(60);
    else
        wait_cpuclock(40);
    wait_cpuclock(80);
}

void matrix_output_unselect_delay(uint8_t line, bool key_pressed) {}
#endif

// Faster memset/memcpy

#if 1
#include <stddef.h>
#include <string.h>

__attribute__((always_inline))
static inline void *memset_const_num(void *ptr, unsigned char value, size_t num)
{
    unsigned char *dst = ptr;
    if (num >= 2 && (size_t)dst & 0x1)
    {
        *dst = value;
        num -= 1;
        dst += 1;
    }
    if (num >= 4 && (size_t)dst & 0x2)
    {
        *(unsigned short*)dst = value | (value << 8);
        num -= 2;
        dst += 2;
    }
#pragma GCC unroll 65534
    while (num >= 4)
    {
        *(unsigned int*)dst = value | (value << 8) | (value << 16)| (value << 24);
        num -= 4;
        dst += 4;
    }
#pragma GCC unroll 65534
    while (num >= 2)
    {
        *(unsigned short*)dst = value | (value << 8);
        num -= 2;
        dst += 2;
    }
#pragma GCC unroll 65534
    while (num >= 1)
    {
        *dst = value;
        num -= 1;
        dst += 1;
    }
    return ptr;
}

__attribute__((always_inline))
static inline void *memset_nonconst_num(void *ptr, unsigned char value, size_t num)
{
    unsigned char *dst = ptr;
    if (num >= 2 && (size_t)dst & 0x1)
    {
        *dst = value;
        num -= 1;
        dst += 1;
    }
    if (num >= 4 && (size_t)dst & 0x2)
    {
        *(unsigned short*)dst = value | (value << 8);
        num -= 2;
        dst += 2;
    }
    while (num >= 4)
    {
        *(unsigned int*)dst = value | (value << 8) | (value << 16)| (value << 24);
        num -= 4;
        dst += 4;
    }
    while (num >= 2)
    {
        *(unsigned short*)dst = value | (value << 8);
        num -= 2;
        dst += 2;
    }
    while (num >= 1)
    {
        *dst = value;
        num -= 1;
        dst += 1;
    }
    return ptr;
}

void *memset(void *ptr, int value, size_t num)
{
    if (__builtin_constant_p(num))
        return memset_const_num(ptr, (unsigned char)value, num);
    return memset_nonconst_num(ptr, (unsigned char)value, num);
}
#endif

#if 1
#include <stddef.h>
#include <string.h>

__attribute__((always_inline))
static inline void *memcpy_const_num(void *ptr1, const void *ptr2, size_t num)
{
    unsigned char *dst = ptr1;
    size_t num2 = num < 1024 ? num : 1024;
#pragma GCC unroll 65534
    while (num2 >= 8)
    {
        *(unsigned long long*)dst = *(const unsigned long long*)ptr2;
        dst += 8;
        ptr2 = (const void*)((const unsigned char*)ptr2 + 8);
        num -= 8;
        num2 -= 8;
    }
#pragma GCC unroll 65534
    while (num >= 4)
    {
        *(unsigned int*)dst = *(const unsigned int*)ptr2;
        dst += 4;
        ptr2 = (const void*)((const unsigned char*)ptr2 + 4);
        num -= 4;
    }
#pragma GCC unroll 65534
    while (num >= 2)
    {
        *(unsigned short*)dst = *(const unsigned short*)ptr2;
        dst += 2;
        ptr2 = (const void*)((const unsigned short*)ptr2 + 2);
        num -= 2;
    }
#pragma GCC unroll 65534
    while (num >= 1)
    {
        *dst = *(const unsigned char*)ptr2;
        dst += 1;
        ptr2 = (const void*)((const unsigned char*)ptr2 + 1);
        num -= 1;
    }
    return ptr1;
}

__attribute__((always_inline))
static inline void *memcpy_nonconst_num(void *ptr1, const void *ptr2, size_t num)
{
    unsigned char *dst = ptr1;
    while (num >= 4)
    {
        *(unsigned int*)dst = *(const unsigned int*)ptr2;
        dst += 4;
        ptr2 = (const void*)((const unsigned char*)ptr2 + 4);
        num -= 4;
    }
    while (num >= 2)
    {
        *(unsigned short*)dst = *(const unsigned short*)ptr2;
        dst += 2;
        ptr2 = (const void*)((const unsigned short*)ptr2 + 2);
        num -= 2;
    }
    while (num >= 1)
    {
        *dst = *(const unsigned char*)ptr2;
        dst += 1;
        ptr2 = (const void*)((const unsigned char*)ptr2 + 1);
        num -= 1;
    }
    return ptr1;
}

void *memcpy(void *ptr1, const void *ptr2, size_t num)
{
    if (ptr1 == ptr2)
        return ptr1;
    if (__builtin_constant_p(num))
        return memcpy_const_num(ptr1, ptr2, num);
    return memcpy_nonconst_num(ptr1, ptr2, num);
}
#endif

// Flatten matrix_scan_custom

#if 0
#include "debounce.h"
#include "matrix.h"
#ifdef SPLIT_KEYBOARD
#    include "split_common/split_util.h"
#    include "split_common/transactions.h"

#    define ROWS_PER_HAND (MATRIX_ROWS / 2)
#else
#    define ROWS_PER_HAND (MATRIX_ROWS)
#endif

/* matrix state(1:on, 0:off) */
extern matrix_row_t raw_matrix[MATRIX_ROWS];  // raw values
extern matrix_row_t matrix[MATRIX_ROWS];      // debounced values

bool matrix_scan_custom(matrix_row_t current_matrix[]);

__attribute__((flatten))
static bool do_matrix_scan_custom(matrix_row_t current_matrix[]) {
    return matrix_scan_custom(raw_matrix);
}

uint8_t matrix_scan(void) {
    bool changed = do_matrix_scan_custom(raw_matrix);

#ifdef SPLIT_KEYBOARD
    changed = debounce(raw_matrix, matrix + thisHand, ROWS_PER_HAND, changed) | matrix_post_scan();
#else
    changed = debounce(raw_matrix, matrix, ROWS_PER_HAND, changed);
    matrix_scan_kb();
#endif

    return changed;
}
#endif

// Other stuff

#if 0
#define TARGET_CYCLES (CPU_CLOCK / 12000L)

static uint32_t prev_cyccnt = 0;

void delay(void) {
    while (DWT->CYCCNT - prev_cyccnt < TARGET_CYCLES - 7L);
    prev_cyccnt = prev_cyccnt + TARGET_CYCLES;
}

extern void (*delay_ptr)(void);

void delay_init(void) {
    prev_cyccnt = DWT->CYCCNT;
    delay_ptr = delay;
}

void (*delay_ptr)(void) = delay_init;

void matrix_scan_kb(void) {
    delay_ptr();
}
#endif

#if 0
static inline void *measure(uint32_t *out, void *(*func)(void*), void *arg)
{
    uint32_t start, end;
    void *ret;

    __asm__ __volatile__ ("":::"memory");
    start = DWT->CYCCNT;
    __asm__ __volatile__ ("":::"memory");
    ret = func(arg);
    __asm__ __volatile__ ("":::"memory");
    end = DWT->CYCCNT;
    __asm__ __volatile__ ("":::"memory");
    *out = end - start;
    return ret;
}

uint32_t matrix_output_unselect_delay_actual;

static inline void *do_matrix_output_unselect_delay(void *argp)
{
    struct {
        uint8_t line;
        bool key_pressed;
    } *arg = argp;
    matrix_output_unselect_delay(arg->line, arg->key_pressed);
    return 0;
}

static inline void matrix_output_unselect_delay_measured(uint8_t line, bool key_pressed) {
    struct {
        uint8_t line;
        bool key_pressed;
    } arg = {line, key_pressed};
    measure(&matrix_output_unselect_delay_actual, do_matrix_output_unselect_delay, &arg);
}
#endif
