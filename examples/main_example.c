#include <18F67K40.h>          // Ajusta tu MCU
#device ADC = 10
#fuses HS
#include "protolink.h"

#use delay(crystal=20mhz)
#use rs232(baud=115200,parity=N,UART2,bits=8,stream=TCP,errors)
#use rs232(baud=115200,parity=N,UART3,bits=8,stream=pc,errors)


#include "mqtt_serial.c"

 //PARAMETROS MQTT

//char broker[] = {"tuna-iot.com"};
char broker[] = {"lora.galio.dev"};
int16 port = 1883;
char username[] = {"galiomosquitto"};
char pass[] = {"g4L10mqtt$."};
char clientId [] = {"protolink-v1-test"};

char topicOut[50];
char topicIn[50] ;


// Tópico MQTT ? literal constante, no se modifica
//char topicTest[] = "devices/update/device1";
uint8_t topicTest[] = "devices/update/device1";
// Payload JSON ? buffer modificable (permite actualizar valores dinámicamente)
uint8_t payload[] = "{\"data\":\"hola\"}";




#define POLLING_TIME 20
int segundos = 0;
int cuenta = 0;
boolean SEND_DATA =true;

#INT_TIMER0
void  TIMER0_isr(void){
 cuenta++;
 if(cuenta >=5){
   cuenta =0;
   segundos++;
    if (segundos >=POLLING_TIME) {
      segundos = 0;
      SEND_DATA = true;
    }
 }
}
boolean connectBroker(){
  fprintf(pc, "Reiniciando LANTRONIX...\r\n" );
  /*if (!lantronixInit()) {
    fprintf(pc, "Fallo al inicializar lantronix\r\n" );
    fprintf(pc, "Revisa las configuraciones y reinica el dispositivo\r\n" );
    return false;
  }
  fprintf(pc, "Lantronix OK!\r\n" );
  delay_ms(2000);*/
  if (!mqttConnect(&broker,port)) {
    fprintf(pc, "Fallo conexion mqtt\r\n" );
    return false;
  }
  fprintf(pc, "Conectado a: %s\r\n",broker );
  delay_ms(5000);
  //sprintf(topicIn,"devices/receiveData/%s",mac);
  sprintf(topicIn,"test/topic/downlink");
  fprintf(pc, "Suscribiendo a: %s\r\n",topicIn );
  if (!mqttSubscribe(topicIn)) {
    fprintf(pc, "Sub error\r\n" );
    return false;
  }
  fprintf(pc, "OK\r\n" );
  return true;
}

void main(void){
  set_tris_a(0b00000000);
  set_tris_g(0b00000100);
  setup_timer_0(RTCC_INTERNAL|RTCC_DIV_16);		//210 ms overflow
  enable_interrupts(GLOBAL);
  enable_interrupts(INT_TIMER0);
  enable_interrupts(INT_RDA2);
  output_low(LED1);
  delay_ms(1000);
    memset(replybuffer, 0xA5, sizeof replybuffer);

  //while(1);
  fprintf(pc,">>>TunaBoard MQTT Example<<<\r\n");
  mqttCredentials(&username,&pass,&clientId); //Configura en variables los datos para conexion
  int1 response = connectBroker(); //<--Agregar las subscripciones
  delay_ms(5);
  /*if(!response){ while(1);
  }*/
  //sprintf(topicOut,"devices/update/%s",mac);
  sprintf(topicOut,"devices/update/device1");

 
 while(true){
    if (mqttClientConnected()) {
      if (mqttPacketAvailable()) {
        if (mqttCheckData() == MQTT_CTRL_PUBLISH) {
            delay_ms(1);
//          char reqTopic[50];
//          char reqPayload[50];
//          int payloadLength;
//          mqttReadPacket(&reqTopic,&reqPayload,&payloadLength);
//          fprintf(pc, "Topico recibido: %s\r\n",reqTopic );
//          fprintf(pc, "Payload recibido: %s\r\n",reqPayload );
//          fprintf(pc, "Length: %d\r\n",payloadLength );
//          if (!strcmp(reqPayload,(char*)"ON")) {
//            fprintf(pc, "Encender\r\n" );
//            output_high(LED1);
//          }
//          if (!strcmp(reqPayload,(char*)"OFF")) {
//            fprintf(pc, "Apagar\r\n" );
//            output_low(LED1);
//          }

        }
        //printAvailableData();
      }
      if (SEND_DATA) {

        SEND_DATA = false;
        /*if (mqttSendPing()) {
          fprintf(pc, "Sigo Online!\r\n" );
        }else{
          fprintf(pc, "Ping error!!\r\n" );
        }
        delay_ms(3000);*/
        fprintf(pc,"Publicando a: %s\r\n",topicOut);
        mqttPublish(&topicTest,&payload,strlen(payload));

      }
    }else{
      fprintf(pc, "SE PERDIO LA CONEXION AL BROKER!!\r\n" );
      delay_ms(5000);
      if (connectBroker()) {
        fprintf(pc, "RECONECTADO!\r\n" );
      }
    }


  }
}


