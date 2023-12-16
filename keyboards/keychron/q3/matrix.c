/* Copyright 2021 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "matrix.h"
#include "quantum.h"

// Pin connected to DS of 74HC595
#define DATA_PIN A7
// Pin connected to SH_CP of 74HC595
#define CLOCK_PIN B1
// Pin connected to ST_CP of 74HC595
#define LATCH_PIN B0

#define DEFINE_MATRIX_PINS()                                \
    const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;    \
    const pin_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;    \
    (void)row_pins;                                         \
    (void)col_pins;

#define ROWS_PER_HAND (MATRIX_ROWS)

static inline void writePinLow_atomic(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        writePinLow(pin);
    }
}

static inline void writePinHigh_atomic(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        writePinHigh(pin);
    }
}

static inline void setPinOutput_writeLow(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        setPinOutput(pin);
        writePinLow(pin);
    }
}

static inline void setPinInputHigh_atomic(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        setPinInputHigh(pin);
    }
}
#define compiler_barrier() __atomic_signal_fence(__ATOMIC_SEQ_CST)
// At 3.6V input, four nops (50ns) should be enough for all signals
#define small_delay() do {                      \
    compiler_barrier();                         \
    __asm__ __volatile__ ("nop;nop;nop;nop;");  \
    compiler_barrier();                         \
} while (0)

static void shiftOut(uint8_t dataOut) {
    ATOMIC_BLOCK_FORCEON {
#pragma GCC unroll 65534
        for (uint8_t i = 0; i < 8; i++) {
            compiler_barrier();
            if (dataOut & 0x1) {
                writePinHigh(DATA_PIN);
            } else {
                writePinLow(DATA_PIN);
            }
            dataOut = dataOut >> 1;
            compiler_barrier();
            writePinHigh(CLOCK_PIN);
            small_delay();
            writePinLow(CLOCK_PIN);
        }
        compiler_barrier();
        writePinHigh(LATCH_PIN);
        small_delay();
        writePinLow(LATCH_PIN);
        compiler_barrier();
    }
}

static void shiftout_single(uint8_t data) {
    ATOMIC_BLOCK_FORCEON {
        compiler_barrier();
        if (data & 0x1) {
            writePinHigh(DATA_PIN);
        } else {
            writePinLow(DATA_PIN);
        }
        compiler_barrier();
        writePinHigh(CLOCK_PIN);
        small_delay();
        writePinLow(CLOCK_PIN);
        compiler_barrier();
        writePinHigh(LATCH_PIN);
        small_delay();
        writePinLow(LATCH_PIN);
        compiler_barrier();
    }
}

static void shiftout_single_init(uint8_t data) {
    ATOMIC_BLOCK_FORCEON {
        if (data & 0x1) {
            writePinHigh(DATA_PIN);
        } else {
            writePinLow(DATA_PIN);
        }
    }
}

static void shiftout_single_clock(void) {
    ATOMIC_BLOCK_FORCEON {
        compiler_barrier();
        writePinHigh(CLOCK_PIN);
        small_delay();
        writePinLow(CLOCK_PIN);
        compiler_barrier();
    }
}

static void shiftout_single_latch(void) {
    ATOMIC_BLOCK_FORCEON {
        compiler_barrier();
        writePinHigh(LATCH_PIN);
        small_delay();
        writePinLow(LATCH_PIN);
        compiler_barrier();
    }
}

static bool select_col(uint8_t col) {
    DEFINE_MATRIX_PINS();

    pin_t pin = col_pins[col];

    if (pin != NO_PIN) {
#ifdef MATRIX_UNSELECT_DRIVE_HIGH
        writePinLow_atomic(pin);
#else
        setPinOutput_writeLow(pin);
#endif
        return true;
    } else {
        if (col == 8) {
            shiftout_single(0x00);
            shiftout_single_init(0x01);
        }
        shiftout_single_clock();
        return true;
    }
    return false;
}

static void unselect_col(uint8_t col) {
    DEFINE_MATRIX_PINS();

    pin_t pin = col_pins[col];

    if (pin != NO_PIN) {
#ifdef MATRIX_UNSELECT_DRIVE_HIGH
        writePinHigh_atomic(pin);
#else
        setPinInputHigh_atomic(pin);
#endif
    } else {
        shiftout_single_latch();
    }
}

static void unselect_cols(void) {
    DEFINE_MATRIX_PINS();

    // unselect column pins
    for (uint8_t x = 0; x < MATRIX_COLS; x++) {
        pin_t pin = col_pins[x];

        if (pin != NO_PIN) {
#ifdef MATRIX_UNSELECT_DRIVE_HIGH
            writePinHigh_atomic(pin);
#else
            setPinInputHigh_atomic(pin);
#endif
        }
        if (x == (MATRIX_COLS - 1))
            // unselect Shift Register
            shiftOut(0xFF);
    }
}

static void matrix_init_pins(void) {
    DEFINE_MATRIX_PINS();

    setPinOutput(DATA_PIN);
    setPinOutput(CLOCK_PIN);
    setPinOutput(LATCH_PIN);
#ifdef MATRIX_UNSELECT_DRIVE_HIGH
    for (uint8_t x = 0; x < MATRIX_COLS; x++) {
        if (col_pins[x] != NO_PIN) {
            setPinOutput(col_pins[x]);
        }
    }
#endif
    unselect_cols();
    for (uint8_t x = 0; x < MATRIX_ROWS; x++) {
        if (row_pins[x] != NO_PIN) {
            setPinInputHigh_atomic(row_pins[x]);
        }
    }
}

#if PAL_IOPORTS_WIDTH == 8
typedef uint8_t ioport_t;
#elif PAL_IOPORTS_WIDTH == 16
typedef uint16_t ioport_t;
#elif PAL_IOPORTS_WIDTH == 32
typedef uint32_t ioport_t;
#else
typedef ioportmask_t ioport_t;
#endif

static inline bool read_pin_port_required(uint8_t row_index)
{
    DEFINE_MATRIX_PINS();

    if (row_index == 0)
        return true;
    if (row_pins[row_index - 1] == NO_PIN)
        return true;
    return PAL_PORT(row_pins[row_index]) != PAL_PORT(row_pins[row_index - 1]);
}

#define read_pin_port(pin) ((ioport_t)palReadPort(PAL_PORT((pin))))

#define is_pin_set(val, pin) (((val) & (1U << PAL_PAD((pin)))) != 0)

static void matrix_read_rows_on_col(matrix_row_t current_matrix[], uint8_t current_col, matrix_row_t row_shifter) {
    DEFINE_MATRIX_PINS();

    bool key_pressed = false;
    ioport_t ports[ROWS_PER_HAND];

    // Select col
    if (!select_col(current_col)) { // select col
        return;                     // skip NO_PIN col
    }
    matrix_output_select_delay2(current_col);

    compiler_barrier();

    // Read all required pins
#pragma GCC unroll 65534
    for (uint8_t row_index = 0; row_index < ROWS_PER_HAND; row_index++) {
        const pin_t pin = row_pins[row_index];
        if (pin == NO_PIN)
            continue;
        // Only read port if we need to
        if (read_pin_port_required(row_index))
            ports[row_index] = read_pin_port(pin);
        else
            ports[row_index] = ports[row_index - 1];
    }

    compiler_barrier();

    // Unselect col
    unselect_col(current_col);

    compiler_barrier();

    // For each row...
#pragma GCC unroll 65534
    for (uint8_t row_index = 0; row_index < ROWS_PER_HAND; row_index++) {
        // Check row pin state
        const pin_t pin = row_pins[row_index];
        if (pin == NO_PIN)
            continue;
        if (!is_pin_set(ports[row_index], pin)) {
            // Pin LO, set col bit
            current_matrix[row_index] |= row_shifter;
            key_pressed = true;
        }
    }

    compiler_barrier();

    matrix_output_unselect_delay(current_col, key_pressed); // wait for all Row signals to go HIGH
}

void matrix_init_custom(void) {
    // initialize key pins
    matrix_init_pins();
}

__attribute__((flatten))
bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    matrix_row_t curr_matrix[MATRIX_ROWS] = {0};

    // Set col, read rows
    matrix_row_t row_shifter = MATRIX_ROW_SHIFTER;
#pragma GCC unroll 65534
    for (uint8_t current_col = 0; current_col < MATRIX_COLS; current_col++, row_shifter <<= 1) {
        matrix_read_rows_on_col(curr_matrix, current_col, row_shifter);
    }

    matrix_row_t changed = 0;
#pragma GCC unroll 65534
    for (uint8_t current_row = 0; current_row < MATRIX_ROWS; current_row++) {
        matrix_row_t i = curr_matrix[current_row];
        i ^= current_matrix[current_row];
        changed |= i;
    }

    if (changed) {
#pragma GCC unroll 65534
        for (uint8_t current_row = 0; current_row < MATRIX_ROWS; current_row++) {
            current_matrix[current_row] = curr_matrix[current_row];
        }
    }
    return changed;
}
