//Source de libreria mqtt
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_serial.h"

#define DEFAULT_TIMEOUT_MS 2000 // DIVIDIDP/1000 =  250 mS

int1 DATA_AVAILABLE  = false;
//Activar o Desactivar con 1 o 0
#define LANT_DEBUG 0



char replybuffer[MAXBUFFERSIZE];
int16 BYTES_AVAILABLE;




#INT_RDA2
void rda_isr(){
  int16 timeout = 1000;

  int16 replyidx = 0;
  int16 timeoutBack = timeout;
  while (timeout--) {
      if (replyidx >= 499) {
          break;
      }
      if(kbhit(TCP)) {
          timeout = timeoutBack;
          char c =  fgetc(TCP);
          replybuffer[replyidx] = c;
          replyidx++;
      }
      if (timeout == 0) {
          break;
      }
      //delay_us(100);
  }
  replybuffer[replyidx] = 0;  // null term
  DATA_AVAILABLE = true;
  BYTES_AVAILABLE = replyidx;

  //return replyidx;

}

void printAvailableData(void){
  fprintf(pc, "[LANT_DEBUG] Recibido <-- " );
  for (int i = 0; i < BYTES_AVAILABLE; i++) {
    fprintf(pc, "%02X",replybuffer[i] );
  }
  fprintf(pc, "\r\n" );

}


void sendLantronix(uint8_t *buffer,int16 len){

  fprintf(pc, "[LANT_DEBUG] Lantronix Enviando...\r\n" );
  fprintf(pc, "[LANT_DEBUG] %ld bytes enviados...\r\n",len );
  for (int16 i = 0; i < len; i++) {
    fputc(buffer[i],TCP);
  }
}

boolean timeout_data(int16 milis_timeout){
  int16 timeout_ms = 0;
  while (!DATA_AVAILABLE && timeout_ms<milis_timeout  ) {
    timeout_ms++;
    delay_ms(1);
  }
  if (DATA_AVAILABLE) {

      fprintf(pc, "[LANT_DEBUG] Packet Available!\r\n" );
      printAvailableData();

  }else if (timeout_ms >= milis_timeout) {

      fprintf(pc, "[LANT_DEBUG] Timeout int serial\r\n" );

    return true;
  }
  return false;
}





void checkConnectionErrors(int error){
  switch (error) {
    case MQTT_CONNECTION_ACCEPTED:{
      mqtt.connected = true;

      fprintf(pc, "MQTT_CONNECTION_ACCEPTED \r\n" );

      break;
    }
    case MQTT_REFUSED_PROTOCOL_VERSION:{

        fprintf(pc, "MQTT_REFUSED_PROTOCOL_VERSION \r\n" );


      break;
    }
    case MQTT_REFUSED_ID_REJECTED:{

        fprintf(pc, "MQTT_REFUSED_ID_REJECTED \r\n" );


      break;
    }
    case MQTT_REFUSED_SERVER_UNAVAILABLE:{

        fprintf(pc, "MQTT_REFUSED_SERVER_UNAVAILABLE\r\n" );


      break;
    }
    case MQTT_REFUSED_BAD_USERNAME_PASS:{

        fprintf(pc, "MQTT_REFUSED_BAD_USERNAME_PASS\r\n" );


      break;
    }
    case MQTT_REFUSED_NOT_AUTHORIZED:{

        fprintf(pc, "MQTT_REFUSED_NOT_AUTHORIZED\r\n" );


      break;
    }
    default :

      fprintf(pc, "ERROR UNKNOW\r\n" );

    break;
  }
}

void mqttReadPacket(char *outTopic, char *outPayload, int *outLen) {
  // mqttPacket[0] = RL, mqttPacket[1..2] = tlen, mqttPacket[3..] = topic...
  uint8_t rl = mqttPacket[0];
  uint16_t tlen = make16(mqttPacket[1], mqttPacket[2]);

  // copia tópico
  memcpy(outTopic, &mqttPacket[3], tlen);
  outTopic[tlen] = '\0';

  // offset payload (QoS0): 3 + tlen
  int off = 3 + tlen;
  int pll = rl - (2 + tlen); // RL - (lenfield + topic)
  if (pll <= 0) pll = 0;
  memcpy(outPayload, &mqttPacket[off], pll);
  outPayload[pll] = '\0';
  *outLen = pll;
}




int mqttCheckData(void){
    DATA_AVAILABLE =false;

    fprintf(pc, "[MQTT_DEBUG] Bytes Available: %ld\r\n", BYTES_AVAILABLE);

  char firstbyte = replybuffer[0];

  char CTRL_HEADER = firstbyte >> 4;
  switch (CTRL_HEADER) {
    case MQTT_CTRL_CONNECTACK:{

      fprintf(pc, "[MQTT_DEBUG] MQTT_CTRL_CONNECTACK: " );
 
      char connectionCode = replybuffer[3]; // posicion 3
      checkConnectionErrors(connectionCode);
      break;
    }
    case MQTT_CTRL_SUBACK: {
        uint16_t pid = make16(replybuffer[2], replybuffer[3]);
        uint8_t rc   = replybuffer[4]; // 0x00..0x02 ó 0x80
        fprintf(pc, "[MQTT_DEBUG] MQTT_CTRL_SUBACK pid=%lu rc=0x%02X\r\n", pid, rc);
        break;
    }

    case MQTT_CTRL_PUBACK:{

      fprintf(pc, "[MQTT_DEBUG] MQTT_CTRL_PUBACK\r\n" );
      uint16_t pid = make16(replybuffer[2], replybuffer[3]);
      fprintf(pc,"[MQTT_DEBUG] PUBACK pid=%lu\r\n", pid);

      break;
    }
    case MQTT_CTRL_UNSUBACK:{

      fprintf(pc, "[MQTT_DEBUG] MQTT_CTRL_UNSUBACK\r\n" );

      break;
    }
    case MQTT_CTRL_PINGRESP:{

      fprintf(pc, "[MQTT_DEBUG] MQTT_CTRL_PINGRESP\r\n" );

      break;
    }
    case MQTT_CTRL_PUBLISH:{
        
        // 1 Decodifica encabezado y longitudes
        uint8_t header = replybuffer[0];
        uint8_t qos = (header >> 1) & 0x03;
        uint8_t dup = (header >> 3) & 0x01;
        uint8_t retain = header & 0x01;

        uint8_t remainingLength = replybuffer[1];
        uint16_t topicLength = make16(replybuffer[2], replybuffer[3]);

        // 2 Extrae el tópico (sin tocar replybuffer original)
        char topic_rx[64] = {0};
        memcpy(topic_rx, &replybuffer[4], topicLength);
        topic_rx[topicLength] = '\0';

        // 3 Calcula offset al payload
        uint16_t offset = 4 + topicLength;
        if (qos > 0) offset += 2; // si tiene Packet Identifier

        uint16_t payloadLen = remainingLength - (offset - 2); // -2 porque RL empieza después del header

        char payload_rx[128] = {0};
        if (payloadLen > sizeof(payload_rx) - 1) payloadLen = sizeof(payload_rx) - 1;
        memcpy(payload_rx, &replybuffer[offset], payloadLen);
        payload_rx[payloadLen] = '\0';

        //   Debug completo
        fprintf(pc, "\r\n[MQTT_DEBUG] --- MQTT_CTRL_PUBLISH ---\r\n");
        fprintf(pc, "[MQTT_DEBUG] Header: 0x%02X (QoS=%u, DUP=%u, RETAIN=%u)\r\n", header, qos, dup, retain);
        fprintf(pc, "[MQTT_DEBUG] Remaining Length: %u\r\n", remainingLength);
        fprintf(pc, "[MQTT_DEBUG] Topic Length: %lu\r\n", topicLength);
        fprintf(pc, "[MQTT_DEBUG] Topic: %s\r\n", topic_rx);
        fprintf(pc, "[MQTT_DEBUG] Payload Length: %lu\r\n", payloadLen);
        fprintf(pc, "[MQTT_DEBUG] Payload (ASCII preview): %s\r\n", payload_rx);

        // 5 Hexdump
        fprintf(pc, "[MQTT_DEBUG] HEXDUMP (%lu bytes):\r\n", BYTES_AVAILABLE);
        for (int i = 0; i < BYTES_AVAILABLE; i++) {
            fprintf(pc, "%02X ", (uint8_t)replybuffer[i]);
            if ((i + 1) % 16 == 0) fprintf(pc, "\r\n");
        }
        fprintf(pc, "\r\n------------------------------\r\n");


      break;
    }
    default:{

    fprintf(pc, "[MQTT_DEBUG] Dato desconocido\r\n" );

    break;
    }
  }
return CTRL_HEADER;
}
char *stringprint(char *p,   char *s, int16 maxlen=0) {
	// If maxlen is specified (has a non-zero value) then use it as the maximum
	// length of the source string to write to the buffer.  Otherwise write
	// the entire source string.
	int16 len = strlen(s);
	if (maxlen > 0 && len > maxlen) {
		len = maxlen;
	}
	/*
	for (uint8_t i=0; i<len; i++) {
		Serial.write(pgm_read_byte(s+i));
	}
	*/
	p[0] = len >> 8; p++;
	p[0] = len & 0xFF; p++;
	strncpy((char *)p, s, len);
	return p+len;
}



// Requisitos:
// - MQTT_PROTOCOL_LEVEL == 4 (MQTT 3.1.1)
// - MQTT_CTRL_CONNECT, MQTT_CONN_CLEANSESSION, MQTT_CONN_USERNAMEFLAG,
//   MQTT_CONN_PASSWORDFLAG, MQTT_CONN_KEEPALIVE definidos en tu headers.
// - stringprint(char *p, char *s, int16 maxlen=0) existente (como la tuya).
// - struct mqtt_data mqtt; global (como lo mostraste).

int connectPacket(char *packet) {
  uint8_t *p = (uint8_t *)packet;
  uint8_t *start = p;

  // Fixed header: CONNECT (0x10) con flags 0
  *p++ = (MQTT_CTRL_CONNECT << 4) | 0x00;

  // Placeholder de Remaining Length (1 byte, simple)
  *p++ = 0x00;  // lo llenamos al final (asumimos RL < 128)

  // ---- Variable header ----
  // Protocol Name: "MQTT"
#if MQTT_PROTOCOL_LEVEL == 4
  p = (uint8_t*)stringprint((char*)p, (char*)"MQTT");
#else
  #error MQTT level not supported (usa 4 para MQTT 3.1.1)
#endif

  // Protocol Level
  *p++ = MQTT_PROTOCOL_LEVEL;  // 4 para 3.1.1

  // Connect Flags
  uint8_t flags = 0;
  flags |= MQTT_CONN_CLEANSESSION;
  if (((char*)mqtt.username)[0] != '\0') flags |= MQTT_CONN_USERNAMEFLAG;
  if (((char*)mqtt.password)[0] != '\0') flags |= MQTT_CONN_PASSWORDFLAG;
  *p++ = flags;

  // Keep Alive
  *p++ = (uint8_t)(MQTT_CONN_KEEPALIVE >> 8);
  *p++ = (uint8_t)(MQTT_CONN_KEEPALIVE & 0xFF);

  // ---- Payload ----
  // OJO: NO uses &mqtt.clientid — los arrays ya decaen a puntero
  p = (uint8_t*)stringprint((char*)p, (char*)mqtt.clientid);

  if (flags & MQTT_CONN_USERNAMEFLAG)
    p = (uint8_t*)stringprint((char*)p, (char*)mqtt.username);

  if (flags & MQTT_CONN_PASSWORDFLAG)
    p = (uint8_t*)stringprint((char*)p, (char*)mqtt.password);

  // ---- Remaining Length (1 byte) ----
  // RL = bytes después del RL => total - (1 byte header + 1 byte RL)
  int total = (int)(p - (uint8_t*)packet);
  int rl = total - 2;

  // Esta versión simple solo soporta RL < 128 (sin varint).
  if (rl >= 128) {
    // Si llegas aquí, o tu clientid/usuario/pass son muy largos o
    // el payload creció. Cambia a RL varint si lo necesitas.
    return -1;
  }

  packet[1] = (uint8_t)rl;
  return total;
}


void mqttCredentials(char *userName, char *passWord,char *clientId){
    strcpy(mqtt.username,userName);
    strcpy(mqtt.password,passWord);
    strcpy(mqtt.clientid,clientId);
}
boolean mqttConnect(char *server, int16 port){
  MQTT_SUB_COUNT = 0; // reseteamos el contador de subscripcion
  strcpy(mqtt.server,server);
  mqtt.port = port;
  /*if (!CONNECT_SERVER(&mqtt.server,mqtt.port)) {
    return false;
  }*/
  delay_ms(500);
  int bytes_to_send = connectPacket((char*)mqttPacket);
  sendLantronix(mqttPacket,bytes_to_send);
  if (timeout_data(5000)) {
    return false;
  }
  if (mqttCheckData() != MQTT_CTRL_CONNECTACK) {
    mqtt.connected = false;
    return false;
  }
  if (!mqtt.connected) {
    return false;
  }
  return true;

}

int publishPacket(uint8_t *packet,
                  uint8_t *topic,
                  uint8_t *payload, uint16_t payload_len,
                  uint8_t qos, uint8_t retain, uint8_t dup) {
  uint8_t *p = (uint8_t *)packet;

  // 1) Calcula RL: 2 (len topic) + strlen(topic) + (qos?2:0) + payload_len
  uint16_t tlen = (uint16_t)strlen(topic);
  uint32_t vh   = 2u + tlen + ((qos & 0x03u) ? 2u : 0u);
  uint32_t rl   = vh + payload_len;

  // Codifica RL (varint)
  uint8_t rl_bytes[4]; uint8_t rl_len = encode_rl(rl, rl_bytes);

  // 2) Fixed header (tipo 0x3 + flags)
  uint8_t hdr = (0x03u << 4);
  if (dup)    hdr |= 0x08;
  hdr |= (qos & 0x03u) << 1;
  if (retain) hdr |= 0x01;
  *p++ = hdr;

  // 3) Remaining Length
  for (uint8_t i=0;i<rl_len;i++) *p++ = rl_bytes[i];

  // 4) Variable header: Topic
  *p++ = (uint8_t)(tlen >> 8);
  *p++ = (uint8_t)(tlen & 0xFF);
  memcpy(p, topic, tlen); p += tlen;

  // 5) PID si QoS>0
  if ((qos & 0x03u) > 0) {
    uint16_t pid = MQTT_PID++;
    if (MQTT_PID == 0) MQTT_PID = 1;
    *p++ = (uint8_t)(pid >> 8);
    *p++ = (uint8_t)(pid & 0xFF);
  }

  // 6) Payload
  if (payload_len && payload) { memcpy(p, payload, payload_len); p += payload_len; }
  
  
  
  // === DEBUG MQTT PUBLISH PACKET ===

    int n = (int)(p - (uint8_t*)packet);
    fprintf(pc,
      "\r\n[DBG] PUB n=%u tlen=%lu rl=%lu rl_len=%u payload_len=%lu qos=%u retain=%u dup=%u\r\n",
      n, tlen, (unsigned long)rl, rl_len, payload_len, qos, retain, dup);

    // Fixed header byte
    fprintf(pc, "[DBG] HDR=0x%02X (Type=%u QoS=%u RET=%u DUP=%u)\r\n",
            packet[0],
            (packet[0] >> 4) & 0x0F,
            (packet[0] >> 1) & 0x03,
            packet[0] & 0x01,
            (packet[0] >> 3) & 0x01);

    // Topic
    fprintf(pc, "[DBG] Topic='%s' (len=%lu)\r\n", topic, tlen);

    // Payload (solo si es texto)
    fprintf(pc, "[DBG] Payload (ascii preview): ");
    for (int i = 0; i < payload_len; i++) {
        uint8_t c = payload[i];
        if (c >= 32 && c <= 126) fputc(c, pc);
        else fputc('.', pc);
    }
    fprintf(pc, "\r\n");

    // Hexdump completo
    fprintf(pc, "[DBG] HEXDUMP:\r\n");
    for (int i = 0; i < n; i++) {
        fprintf(pc, "%02X ", packet[i]);
        if ((i+1)%16 == 0) fprintf(pc, "\r\n");
    }
    fprintf(pc, "\r\n");


  return (int)(p - (uint8_t *)packet);
}






// --- Helpers MQTT ---
 uint8_t encode_rl(uint32_t rl, uint8_t *out) {
  uint8_t n = 0;
  do {
    uint8_t enc = rl % 128;
    rl /= 128;
    if (rl > 0) enc |= 0x80;
    out[n++] = enc;
  } while (rl > 0);
  return n;
}

 uint8_t write_utf8_str(uint8_t *p,  char *s) {
  uint16_t n = strlen(s);
  *p++ = (uint8_t)(n >> 8);
  *p++ = (uint8_t)(n & 0xFF);
  memcpy(p, s, n);
  return p + n;
}
int subscribePacket(char *packet, char *topic, uint8_t qos) {
  uint8_t *p = (uint8_t *)packet;
  uint16_t tlen = (uint16_t)strlen(topic);
  uint32_t rl   = 2u /*PID*/ + 2u /*tlen*/ + tlen + 1u /*QoS*/;

  uint8_t rl_bytes[4]; uint8_t rl_len = encode_rl(rl, rl_bytes);

  // Fixed header
  *p++ = (0x08u << 4) | 0x02u;        // 0x82
  for (uint8_t i=0;i<rl_len;i++) *p++ = rl_bytes[i];

  // PID (≠0)
  uint16_t pid = MQTT_PID++; if (MQTT_PID == 0) MQTT_PID = 1;
  *p++ = (uint8_t)(pid >> 8);
  *p++ = (uint8_t)(pid & 0xFF);

  // Topic (len + bytes)
  *p++ = (uint8_t)(tlen >> 8);
  *p++ = (uint8_t)(tlen & 0xFF);
  memcpy(p, topic, tlen); p += tlen;

  // QoS (¡este es el byte que estaba quedando en A5!)
  *p++ = (uint8_t)(qos & 0x03u);

  // DEBUG inmediato (antes de que nada pueda pisarlo)
  {
    int n = (int)(p - (uint8_t*)packet);
    // QoS debe ser el último byte (n-1)
//    fprintf(pc, "[DBG] SUB n=%d tlen=%u rl=%lu rl_len=%u pid=%u qos=0x%02X\n",
//            n, tlen, (unsigned long)rl, rl_len, pid, (unsigned)packet[n-1]);
    
    fprintf(pc, "[DBG] SUB n=%d tlen=%lu rl=%lu rl_len=%u pid=%lu qos=0x%02X\n",n, tlen,(unsigned long)rl,rl_len, pid, (unsigned)packet[n-1]);
    
    for (int i=0;i<n;i++) fprintf(pc, "%02X ", (unsigned)packet[i]);
    fprintf(pc, "\r\n");
  }
  

  return (int)(p - (uint8_t*)packet);
}

boolean mqttPublish(uint8_t *topic_mqtt, uint8_t *payload_mqtt, int16 payload_len) {
    memset(mqttPacket, 0xA5, sizeof mqttPacket);
    fprintf(pc, "[MQTT_DEBUG] mqttPublish()\r\n");
    fprintf(pc, "[MQTT_DEBUG] Topic: %s\r\n", topic_mqtt);
    fprintf(pc, "[MQTT_DEBUG] payload: ");
    int i;
    for (i = 0; i < payload_len; i++) {
        fputc(payload_mqtt[i],pc);

    }
    fprintf(pc,"\r\n");

    // OJO: si el payload es binario, NO lo imprimas con %s (puede cortar en 0x00)
    fprintf(pc, "[MQTT_DEBUG] Payload Len: %ld\r\n", (signed long)payload_len);
    delay_ms(10);

    // Buffer de salida (ajusta tamaño a tu máximo de paquete)
    //static uint8_t mqttPacket[256];

    // Firma esperada:
    // int publishPacket(char *packet, char *topic, const uint8_t *payload,
    //                   uint16_t payload_len, uint8_t qos, uint8_t retain, uint8_t dup);
    int bytes_to_send = publishPacket((uint8_t*)mqttPacket,
                                      (uint8_t*)topic_mqtt,
                                      (uint8_t*)payload_mqtt,
                                      (uint16_t)payload_len,
                                      1, 0, 0);

    sendLantronix((uint8_t*)mqttPacket, bytes_to_send);

    if (timeout_data(5000)) return false;
    mqttCheckData();
    return true;
}

boolean mqttSubscribe(char *topic){
    memset(mqttPacket, 0xA5, sizeof mqttPacket);
    fprintf(pc, "[MQTT_DEBUG] mqttSubscribe()\r\n" );
    delay_ms(1);
  int bytes_to_send = subscribePacket((char*)mqttPacket,topic,0x00);
  fprintf(pc,"[DBG] SUB bytes=%d\r\n", bytes_to_send);
  for (int i=0;i<bytes_to_send;i++){ fprintf(pc,"%02X ", (uint8_t)mqttPacket[i]); } fprintf(pc,"\r\n");
  sendLantronix(mqttPacket,bytes_to_send);
  if (timeout_data(5000)) {
    return false;
  }
  if(mqttCheckData() != MQTT_CTRL_SUBACK){
      return false;
  }
  return true;
}

boolean mqttSendPing(void){
    fprintf(pc, "[MQTT_DEBUG] mqttSendPing()\r\n" );
  mqttPacket[0] = (MQTT_CTRL_PINGREQ << 4)| 0x00;
  mqttPacket[1] = 0x00; //remain length

  sendLantronix(mqttPacket,2);
  if (timeout_data(5000)) {
    mqtt.connected = false;
    return false;
  }
  if (mqttCheckData() != MQTT_CTRL_PINGRESP) {
    mqtt.connected = false;
    return false;
  }
  return true;

}
boolean mqttPacketAvailable(void){
    return DATA_AVAILABLE;
}


boolean mqttClientConnected(void){
    return mqtt.connected;

}
