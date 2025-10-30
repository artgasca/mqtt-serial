#include "mqtt_serial.h"
#include "uart_ring.h"
#include <string.h>

// ===== Transporte =====
int16 mqtt_transport_available(void){ return uart_ring_available(); }
int16  mqtt_transport_read(void){ return uart_ring_read(); }
void     mqtt_transport_write(uint8_t *b, uint16_t l){ uart_write_buf(b,l); }
void     mqtt_transport_write_u8(int8 b){ uart_write_u8(b); }

// ===== Utilidades =====
static void wr_u8(uint8_t b){ mqtt_transport_write_u8(b); }
static void wr_buf( void* p, uint16_t n){ mqtt_transport_write(( uint8_t*)p, n); }

static void wr_utf8_str( char *s){
   /*int16 l_ = (uint16_t)strlen(s);
   int8 h = (uint8_t)((l_>>8)&0xFF), l_=(int8)(l_&0xFF);
   wr_u8(h); wr_u8(l_); wr_buf(s, l_);*/
}

static void wr_rem_len(uint32_t x){
   // MQTT varint
   do{
     uint8_t d = x % 128;
     x /= 128;
     if (x>0) d |= 0x80;
     wr_u8(d);
   }while(x>0);
}

static bool rd_rem_len(uint32_t *out){
   int32 mult=1, value=0; uint8_t encoded;
   for (int i=0;i<4;i++){
     int16 v = mqtt_transport_read(); if (v<0) return false;
     encoded = (uint8_t)v;
     value += (encoded & 0x7F)*mult;
     if (!(encoded & 0x80)) { *out=value; return true; }
     mult *= 128;
   }
   return false;
}

static int16 next_id(mqtt_client_t *c){
  if (++c->next_pkt_id==0) c->next_pkt_id=1;
  return c->next_pkt_id;
}

// ===== API =====
void mqtt_init(mqtt_client_t *c,
               char *client_id,
               char *user_id,
               char *pass_id,
               uint16_t keepalive_sec){
  memset(c,0,sizeof(*c));
  c->client_id = client_id;
  c->user_id      = user_id;
  c->pass_id      = pass_id;
  c->keepalive_sec = keepalive_sec;
  c->state = MQTT_DISCONNECTED;
  c->next_pkt_id = 1;
  for (int i=0;i<MQTT_MAX_SUBS;i++) c->subs[i].used=false;
}

bool mqtt_connect(mqtt_client_t *c){
  // Fixed header: CONNECT (0x10)
  // Variable header: "MQTT", level=4, flags, keepalive
  // Payload: ClientID, [User, Pass]
  uint8_t vh[10]; // enough for "MQTT" + level/flags/keepalive
  // Precompute remaining length
  int16 len_id = 2 + (int16)strlen(c->client_id);
  int16 len_user = (c->user_id? 2+(int16)strlen(c->user_id) : 0);
  int16 len_pass = (c->pass_id? 2+(int16)strlen(c->pass_id) : 0);

  int16 payload_len = len_id + len_user + len_pass;
  int16 var_len = 2+4 + 1 + 1 + 2; // proto name len(2) + "MQTT"(4) + level + flags + keepalive(2)
  int32 rem = var_len + payload_len;

  wr_u8(0x10);         // CONNECT
  wr_rem_len(rem);

  // Variable header
  wr_u8(0x00); wr_u8(0x04); wr_buf(&"MQTT",4); // Protocol Name
  wr_u8(0x04); // Level 4 (v3.1.1)

  uint8_t flags = 0x02; // CleanSession=1
  if (c->user_id) flags |= 0x80;
  if (c->pass_id) flags |= 0x40;
  wr_u8(flags);

  wr_u8((uint8_t)(c->keepalive_sec>>8));
  wr_u8((uint8_t)(c->keepalive_sec&0xFF));

  // Payload
  wr_utf8_str(c->client_id);
  if (c->user_id) wr_utf8_str(c->user_id);
  if (c->pass_id) wr_utf8_str(c->pass_id);

  c->state = MQTT_CONNECTING;
  c->last_tx_ms = platform_millis();
  return true;
}

int1 mqtt_publish_qos0(mqtt_client_t *c, char *topic,
                       int8 *payload, int32 len, int1 retain){
  if (c->state != MQTT_CONNECTED) return false;

  int16 tlen = (uint16_t)strlen(topic);
  int32 rem = 2 + tlen + len;

  int8 hdr = 0x30; // PUBLISH QoS0
  if (retain) hdr |= 0x01;

  wr_u8(hdr);
  wr_rem_len(rem);
  // Topic
  wr_u8((uint8_t)(tlen>>8)); wr_u8((uint8_t)(tlen&0xFF));
  wr_buf(topic, tlen);
  // Payload
  wr_buf(payload, (uint16_t)len);

  c->last_tx_ms = platform_millis();
  return true;
}

int1 mqtt_subscribe_qos0(mqtt_client_t *c, char *topic,mqtt_msg_cb_ptr cb){
  if (c->state != MQTT_CONNECTED) return false;

  // Registra callback
  int slot=-1;
  for (int i=0;i<MQTT_MAX_SUBS;i++) if (!c->subs[i].used){ slot=i; break; }
  if (slot<0) return false;
  memset(c->subs[slot].topic,0,MQTT_MAX_TOPIC);
  strncpy(c->subs[slot].topic, topic, MQTT_MAX_TOPIC-1);
  c->subs[slot].cb = cb; c->subs[slot].used = true;

  uint16_t pkt_id = next_id(c);
  uint16_t tlen   = (uint16_t)strlen(topic);
  uint32_t rem    = 2 /*pktid*/ + 2 + tlen + 1 /*QoS*/;

  wr_u8(0x82);             // SUBSCRIBE (QoS1 bit set en header fijo)
  wr_rem_len(rem);
  wr_u8((uint8_t)(pkt_id>>8)); wr_u8((uint8_t)(pkt_id&0xFF));
  wr_u8((uint8_t)(tlen>>8)); wr_u8((uint8_t)(tlen&0xFF));
  wr_buf(topic, tlen);
  wr_u8(0x00); // QoS0 solicitado

  c->last_tx_ms = platform_millis();
  return true;
}

// ===== Parser =====
static void handle_publish(mqtt_client_t *c, uint8_t hdr, uint32_t rem_len){
  // QoS = (hdr>>1)&0x03; asumimos QoS0
  // Topic
  int16_t b1 = mqtt_transport_read(); if (b1<0) return;
  int16_t b2 = mqtt_transport_read(); if (b2<0) return;
  uint16_t tlen = ((uint16_t)b1<<8) | (uint16_t)b2;
  char topic[MQTT_MAX_TOPIC];
  if (tlen >= MQTT_MAX_TOPIC) tlen = MQTT_MAX_TOPIC-1;

  for (uint16_t i=0;i<tlen;i++){ int16_t ch=mqtt_transport_read(); if (ch<0) return; topic[i]=(char)ch; }
  topic[tlen]=0;

  uint32_t header_sz = 2 + ((uint32_t)b1>=0 && (uint32_t)b2>=0 ? (uint32_t)(tlen) : 0);
  uint32_t payload_len = rem_len - (2 + (uint32_t)tlen);
  if (payload_len > MQTT_MAX_PACKET) payload_len = MQTT_MAX_PACKET;

  static uint8_t payload[MQTT_MAX_PACKET];
  for (uint32_t i=0;i<payload_len;i++){ int16_t ch=mqtt_transport_read(); if (ch<0) return; payload[i]=(uint8_t)ch; }

  // Dispatch a suscripciones exactas
  for (int i=0;i<MQTT_MAX_SUBS;i++){
     if (c->subs[i].used && (0==strcmp(topic, c->subs[i].topic)) && c->subs[i].cb){
        c->subs[i].cb(topic, payload, payload_len);
     }
  }
}

void mqtt_loop(mqtt_client_t *c, uint32_t now_ms){
  // Keepalive
  if (c->state == MQTT_CONNECTED && c->keepalive_sec){
     uint32_t KA = (uint32_t)c->keepalive_sec*1000u;
     if ((now_ms - c->last_tx_ms) > (KA/2)){ // ping a mitad del keepalive
        wr_u8(0xC0); wr_u8(0x00);             // PINGREQ
        c->last_tx_ms = now_ms;
     }
  }

  // Procesa RX (puede llegar fragmentado, hacemos lectura byte a byte)
  while (mqtt_transport_available() >= 2){
     int16_t fh = mqtt_transport_read(); if (fh<0) break;
     // remaining length varint
     uint32_t rem=0; if (!rd_rem_len(&rem)) break;

     c->last_rx_ms = now_ms;
     uint8_t pkt = (uint8_t)fh & 0xF0;

     switch(pkt){
       case 0x20: { // CONNACK
         (void)mqtt_transport_read(); // Acknowledge Flags
         int16_t rc = mqtt_transport_read();
         if (rc==0) c->state = MQTT_CONNECTED;
         else       c->state = MQTT_DISCONNECTED;
       } break;

       case 0x90: { // SUBACK
         // packet id(2) + [granted qos...]
         // Consumimos todo (no almacenamos)
         for (uint32_t i=0;i<rem;i++) (void)mqtt_transport_read();
       } break;

       case 0x30: // PUBLISH QoS0
       case 0x31: // retain=1
       case 0x32:
       case 0x33: {
         handle_publish(c, (uint8_t)fh, rem);
       } break;

       case 0xD0: { // PINGRESP
         // rem debe ser 0; nada que hacer
         for (uint32_t i=0;i<rem;i++) (void)mqtt_transport_read();
       } break;

       default: {
         // Descarta paquetes no soportados en este mini cliente
         for (uint32_t i=0;i<rem;i++) (void)mqtt_transport_read();
       } break;
     }
  }

  // Reintento básico si está CONNECTING mucho tiempo sin CONNACK (opcional)
  if (c->state == MQTT_CONNECTING){
     if ((now_ms - c->last_tx_ms) > 5000u){
        // reintenta CONNECT
        mqtt_connect(c);
     }
  }
}
