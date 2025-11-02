#ifndef _mqtt_serial_H
#define _mqtt_serial_H

//Desactivar con 0
#define MQTT_DEBUG 1
// Usar 3 (MQTT 3.0) o 4 (MQTT 3.1.1) Libreria implementada para 3.1.1
#define MQTT_PROTOCOL_LEVEL   4

#define MQTT_CTRL_CONNECT     0x1
#define MQTT_CTRL_CONNECTACK  0x2
#define MQTT_CTRL_PUBLISH     0x3
#define MQTT_CTRL_PUBACK      0x4
#define MQTT_CTRL_PUBREC      0x5
#define MQTT_CTRL_PUBREL      0x6
#define MQTT_CTRL_PUBCOMP     0x7
#define MQTT_CTRL_SUBSCRIBE   0x8
#define MQTT_CTRL_SUBACK      0x9
#define MQTT_CTRL_UNSUBSCRIBE 0xA
#define MQTT_CTRL_UNSUBACK    0xB
#define MQTT_CTRL_PINGREQ     0xC
#define MQTT_CTRL_PINGRESP    0xD
#define MQTT_CTRL_DISCONNECT  0xE

#define MQTT_QOS_1 0x1
#define MQTT_QOS_0 0x0

#define CONNECT_TIMEOUT_MS 6000
#define PUBLISH_TIMEOUT_MS 500
#define PING_TIMEOUT_MS    500
#define SUBACK_TIMEOUT_MS  500

// Adjust as necessary, in seconds.  Default to 5 minutes.
#define MQTT_CONN_KEEPALIVE 300

// Largest full packet we're able to send.
// Need to be able to store at least ~90 chars for a connect packet with full
// 23 char client ID.
#define MAXBUFFERSIZE 256

#define MQTT_CONN_USERNAMEFLAG    0x80
#define MQTT_CONN_PASSWORDFLAG    0x40
#define MQTT_CONN_WILLRETAIN      0x20
#define MQTT_CONN_WILLQOS_1       0x08
#define MQTT_CONN_WILLQOS_2       0x18
#define MQTT_CONN_WILLFLAG        0x04
#define MQTT_CONN_CLEANSESSION    0x02


//CODIGO DE CONEXION A SERVER
#define MQTT_CONNECTION_ACCEPTED        0x00
#define MQTT_REFUSED_PROTOCOL_VERSION   0x01
#define MQTT_REFUSED_ID_REJECTED        0x02
#define MQTT_REFUSED_SERVER_UNAVAILABLE 0x03
#define MQTT_REFUSED_BAD_USERNAME_PASS  0x04
#define MQTT_REFUSED_NOT_AUTHORIZED     0x05

int MQTT_SUB_COUNT = 1;


struct mqtt_data{
  uint8_t clientid[50];
  uint8_t username[50];
  uint8_t password[50];
  uint8_t server[50];
  uint16_t port;
  boolean connected;
  boolean available;
}mqtt;

uint8_t mqttPacket[MAXBUFFERSIZE];


int connectPacket(char *packet); 
boolean mqttConnect(char *server, int16 port);
void mqttCredentials(char *userName, char *passWord,char *userId);
void mqttReadPacket(char *receivedTopic,char *receivedPayload);


uint8_t write_utf8_str(uint8_t *p,  char *s);
uint8_t encode_rl(uint32_t rl, uint8_t *out);

// --- Estado del Packet Identifier ---
uint16_t MQTT_PID = 1;  // ¡nunca 0! 1..65535, luego vuelve a 1
#endif
