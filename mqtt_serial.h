#ifndef MQTT_SERIAL_H
#define MQTT_SERIAL_H

/* ============ Config (ajusta según tu RAM/uso) ============ */
#ifndef MQTT_MAX_PACKET
 #define MQTT_MAX_PACKET   512u
#endif
#ifndef MQTT_MAX_TOPIC
 #define MQTT_MAX_TOPIC    96u
#endif
#ifndef MQTT_MAX_SUBS
 #define MQTT_MAX_SUBS     6u
#endif

/* ============ Estados (sin enum) ============ */
#define MQTT_STATE_DISCONNECTED   (0u)
#define MQTT_STATE_CONNECTING     (1u)
#define MQTT_STATE_CONNECTED      (2u)

/* Aliases opcionales */
#define MQTT_DISCONNECTED MQTT_STATE_DISCONNECTED
#define MQTT_CONNECTING   MQTT_STATE_CONNECTING
#define MQTT_CONNECTED    MQTT_STATE_CONNECTED



/* ============ Firma de callback (typedef de puntero) ============ */
/* CCS se lleva mejor si el typedef de puntero a función está FUERA de structs */
typedef void (*mqtt_msg_cb_ptr)(char *, int8 *, int32);

/* ============ Estructura de suscripción (separada) ============ */
struct mqtt_sub_s {
  char              topic[MQTT_MAX_TOPIC];
  mqtt_msg_cb_ptr   cb;      /* puntero a función simple */
  int1                used;    /* 0/1 */
};
typedef struct mqtt_sub_s mqtt_sub_t;

/* ============ Estructura del cliente MQTT (con nombre) ============ */
struct mqtt_client_s {
  /* Conexión (sin const para paz con CCS) */
  char  *client_id;
  char  *user_id;
  char  *pass_id;
  int16    keepalive_sec;

  /* Runtime */
  int8     state;        /* MQTT_STATE_* */
  int32    last_rx_ms;
  int32    last_tx_ms;
  int16    next_pkt_id;

  /* Suscripciones */
  mqtt_sub_t subs[MQTT_MAX_SUBS];
};
typedef struct mqtt_client_s mqtt_client_t;

/* ============ API principal ============ */
void mqtt_init(mqtt_client_t *c,
               char *client_id,
               char *user_id,
               char *pass_id,
               uint16_t keepalive_sec);

int1 mqtt_connect(mqtt_client_t *c);                     /* envía CONNECT */
void mqtt_loop(mqtt_client_t *c, int32 now_ms);            /* procesa RX/keepalive */

int1 mqtt_publish_qos0(mqtt_client_t *c, char *topic,
                       int8 *payload, int32 len, int1 retain);

int1 mqtt_subscribe_qos0(mqtt_client_t *c, char *topic,
                         mqtt_msg_cb_ptr cb);

/* ============ Tick de plataforma (lo das tú) ============ */
int32 platform_millis(void);

/* ============ Transporte (tu ring/uart) ============ */
int16  mqtt_transport_available(void);
int16 mqtt_transport_read(void);
void mqtt_transport_write(int8 *buf, int16 len);
void mqtt_transport_write_u8(int8 b);

#endif /* MQTT_SERIAL_H */
