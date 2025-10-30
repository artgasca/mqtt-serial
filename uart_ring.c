#include "uart_rda_ring.h"

// ====== Ring buffer ======
static volatile uint8_t rb[UART_RING_SIZE];
static volatile uint16_t r_head = 0, r_tail = 0;

void uart_ring_init(void) {
   r_head = r_tail = 0;
}

uint16_t uart_ring_available(void) {
   uint16_t h = r_head, t = r_tail;
   return (h >= t) ? (h - t) : (UART_RING_SIZE - t + h);
}

int16_t uart_ring_read(void) {
   if (r_head == r_tail) return -1;
   uint8_t v = rb[r_tail];
   r_tail = (r_tail + 1) % UART_RING_SIZE;
   return (int16_t)v;
}

bool uart_ring_peek(uint8_t *b) {
   if (r_head == r_tail) return false;
   *b = rb[r_tail];
   return true;
}

void uart_ring_flush(void) {
   r_tail = r_head;
}

void uart_write_u8(uint8_t b) {
   fputc(b, UART_STREAM);
}

void uart_write_buf(const uint8_t *buf, uint16_t len) {
   while (len--) fputc(*buf++, UART_STREAM);
}

// ====== ISR RDA ======
#int_rda
void UART_RDA_ISR(void) {
   int c = fgetc(UART_STREAM);      // lee byte recibido
   uint16_t next = (r_head + 1) % UART_RING_SIZE;
   if (next != r_tail) {            // evita overflow
      rb[r_head] = (uint8_t)c;
      r_head = next;
   } else {
      // Overflow: descarta byte (o marca flag si deseas diagnosticar)
   }
}
