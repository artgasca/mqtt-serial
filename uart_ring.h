#ifndef UART_RING_H
#define UART_RING_H

#include <stdint.h>
#include <stdbool.h>

// ====== Config ======
#ifndef UART_RING_SIZE
#define UART_RING_SIZE   512u
#endif

// ====== API ======
void     uart_ring_init(void);
uint16_t uart_ring_available(void);
int16_t  uart_ring_read(void);          // -1 si vacío
bool     uart_ring_peek(uint8_t *b);    // true si hay dato
void     uart_ring_flush(void);

// Escritura directa a UART (bloqueante)
void     uart_write_u8(uint8_t b);
void     uart_write_buf(uint8_t *buf, uint16_t len);

// Debes definir el stream en tu main (o aquí si prefieres)
#ifndef UART_STREAM
#define UART_STREAM  TCP     // nombre del stream #use rs232(..., stream=TCP)
#endif

#include "uart_ring.c"
#endif
