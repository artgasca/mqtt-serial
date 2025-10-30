#include <18F67K40.h>          // Ajusta tu MCU
#device ADC = 10
#fuses HS
#include "protolink.h"

#use delay(crystal=20mhz)
#use rs232(baud=115200, parity=N, UART2, bits=8, stop=1,stream=TCP,errors)


#include "../mqtt.serial.h"
#include"../uart_ring.h"

#include "../"

// ====== Tick en ms con Timer1 ======
volatile uint32_t g_ms = 0;

#int_timer1
void TMR1_ISR(void){
   // Config: Timer1 a 1ms
   g_ms++;
}

uint32_t platform_millis(void){ return g_ms; }

static void systick_init(void){
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_8); // 16 MHz / 8 = 2 MHz => 2000 ticks/ms si PR cargado
   // CCS: usa set_timer1() para precarga si buscas 1ms exacto; aproximamos:
   set_timer1(65535 - 2000);  // ~1ms a 2MHz
   enable_interrupts(INT_TIMER1);
   enable_interrupts(GLOBAL);
}

// ====== Callbacks de MQTT ======
void on_cmd_cb(const char *topic, const uint8_t *payload, uint32_t len){
   // Ejemplo: comando recibido
   // Ojo: payload no está null-terminated
   // Haz tu lógica (toggle, parse JSON, etc.)
}

void on_conf_cb(const char *topic, const uint8_t *payload, uint32_t len){
   // Ejemplo: configuración
}

// ====== App ======
mqtt_client_t mq;

void mqtt_bootstrap(void){
   uart_ring_init();
   systick_init();
   enable_interrupts(INT_RDA);    // Habilita ISR UART
   enable_interrupts(GLOBAL);

   mqtt_init(&mq,
             "protolink-001",   // ClientID
             "user",            // user (o NULL)
             "pass",            // pass (o NULL)
             30);               // keepalive s

   // Asegúrate de que tu conversor ya haya abierto el TCP al broker
   // (algunos usan AT o config por web; aquí asumimos ya conectado)
   mqtt_connect(&mq);
}

void publish_telemetry(void){
   const char *topic = "galio/pl001/telemetry";
   char buf[64];
   int n = sprintf(buf, "{\"uptime\":%lu}", (unsigned long)platform_millis());
   mqtt_publish_qos0(&mq, topic, (const uint8_t*)buf, (uint32_t)n, false);
}

void main(){
   mqtt_bootstrap();

   // tras conectarnos, nos suscribimos
   // (puede que el SUB se dispare unos ms después del CONNACK; no pasa nada)
   mqtt_subscribe_qos0(&mq, "galio/pl001/cmd",  on_cmd_cb);
   mqtt_subscribe_qos0(&mq, "galio/pl001/conf", on_conf_cb);

   uint32_t tPub = platform_millis();

   while(TRUE){
      uint32_t now = platform_millis();

      // Bucle MQTT: parsea RX y maneja keepalive/ping
      mqtt_loop(&mq, now);

      // Publica cada 1000 ms
      if ((now - tPub) >= 1000u){
         tPub = now;
         if (mq.state == MQTT_CONNECTED){
            publish_telemetry();
         } else if (mq.state == MQTT_DISCONNECTED){
            mqtt_connect(&mq);
         }
      }
   }
}
