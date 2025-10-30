#ifndef MQTT_SERIAL_H
#define MQTT_SERIAL_H

#include <stdint.h>
#include <stdbool.h>

// ====== Config ======
#ifndef MQTT_MAX_PACKET
#define MQTT_MAX_PACKET   512u
#endif

#ifndef MQTT_MAX_TOPIC
#define MQTT_MAX_TOPIC    96u
#endif

#ifndef MQTT_MAX_SUBS
#define MQTT_MAX_SUBS     6u
#endif

typedef void (*mqtt_msg_cb_t)(const char *topic, const uint8_t *payload, uint32_t len);

// ====== Estados ======
typedef enum {
  MQTT_DISCONNECTED = 0,
  MQTT_CONNECTING,
  MQTT_CONNECTED
} mqtt_state_t;

typedef struct {
  // conexión
  const char *client_id;
  const char *user;
  const char *pass;
  uint16_t keepalive_sec;

  // runtime
  mqtt_state_t state;
  uint32_t     last_rx_ms;
  uint32_t     last_tx_ms;
  uint16_t     next_pkt_id;

  // subs (filtros exactos)
  struct {
    char topic[MQTT_MAX_TOPIC];
    mqtt_msg_cb_t cb;
    bool used;
  } subs[MQTT_MAX_SUBS];

} mqtt_client_t;

// ====== API principal ======
void mqtt_init(mqtt_client_t *c,
               const char *client_id,
               const char *user,
               const char *pass,
               uint16_t keepalive_sec);

bool mqtt_connect(mqtt_client_t *c);            // envía CONNECT
void mqtt_loop(mqtt_client_t *c, uint32_t now_ms); // procesa RX/keepalive
bool mqtt_publish_qos0(mqtt_client_t *c, const char *topic,
                       const uint8_t *payload, uint32_t len, bool retain);
bool mqtt_subscribe_qos0(mqtt_client_t *c, const char *topic, mqtt_msg_cb_t cb);

// Debes brindar un “tick” en ms
uint32_t platform_millis(void);

// Lado transporte (implementado en mqtt_serial.c usando uart_rda_ring)
uint16_t mqtt_transport_available(void);
int16_t  mqtt_transport_read(void);
void     mqtt_transport_write(const uint8_t *buf, uint16_t len);
void     mqtt_transport_write_u8(uint8_t b);

#endif
