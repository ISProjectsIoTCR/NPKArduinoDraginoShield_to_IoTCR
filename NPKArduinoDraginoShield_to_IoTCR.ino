#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include "LowPower.h"

#define RE 4
#define DE 5

RH_RF95 rf95;
SoftwareSerial modbus(3, 6);// RX, TX

const byte nitro_inquiry_frame[] = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos_inquiry_frame[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota_inquiry_frame[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

byte nitrogen_val, phosphorus_val, potassium_val, node_id;
long timeSend = 0;

byte values[11];

byte nitrogen();
byte phosphorous();
byte potassium();
void readNPKsensor();
void sendData();
void printdata();

//__________________________________________SET_UP______________________________
void setup()
{

  Serial.begin(9600);
  modbus.begin(9600);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  while (!Serial)
    ;
  if (!rf95.init())
    Serial.println("init failed");
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  //  driver.setTxPower(23, false);
  // If you are using Modtronix inAir4 or inAir9,or any other module which uses the
  // transmitter RFO pins and not the PA_BOOST pins
  // then you can configure the power transmitter power for -1 to 14 dBm and with useRFO true.
  // Failure to do that will result in extremely low transmit powers.
  //  driver.setTxPower(14, true);
  timeSend = millis();
  delay(2000);
  readNPKsensor();
}

//__________________________________________LOOP_______________________________
void loop()
{

  for (int i = 0 ;  i  <  200 ; i++)
  {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }

  Serial.println(".");
  Serial.println("Hello IOTCR");
  readNPKsensor();

}

//_________________________________________FUNCIONES_______________________________
//READ_SENSOR
void readNPKsensor()
{
  byte NPKclear = nitrogen();  //La primera siempre da 255.
  delay(250);
  byte Nval, Pval, Kval;
  for (int i = 0; i < 5; i++)
  {
    nitrogen_val = nitrogen();
    delay(250);
    phosphorus_val = phosphorous();
    delay(250);
    potassium_val = potassium();
    delay(250);
  }
  //nitrogen_val =Nval/5;
  //phosphorus_val = Pval/5;
  //potassium_val = Kval/5;

  node_id = byte(1234);
  sendData();
  printdata();
}

//LEER_NITROGENO
byte nitrogen()
{
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
  if (modbus.write(nitro_inquiry_frame, sizeof(nitro_inquiry_frame)) == 8)
  {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    // When we send the inquiry frame to the NPK sensor, then it replies with the response frame
    // now we will read the response frame, and store the values in the values[] arrary, we will be using a for loop.
    for (byte i = 0; i < 7; i++)
    {
      //Serial.print(modbus.read(),HEX);
      values[i] = modbus.read();
      // Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4]; // returns the Nigtrogen value only, which is stored at location 4 in the array
}

//LEER_FOSFORO
byte phosphorous()
{
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
  if (modbus.write(phos_inquiry_frame, sizeof(phos_inquiry_frame)) == 8)
  {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    for (byte i = 0; i < 7; i++)
    {
      //Serial.print(modbus.read(),HEX);
      values[i] = modbus.read();
      // Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}

//LEER_POTASIO
byte potassium()
{
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
  if (modbus.write(pota_inquiry_frame, sizeof(pota_inquiry_frame)) == 8)
  {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    for (byte i = 0; i < 7; i++)
    {
      //Serial.print(modbus.read(),HEX);
      values[i] = modbus.read();
      //Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}

//ENVIAR_DATA
void sendData()
{
  Serial.println("Sending to LoRaIoTCR_server");
  // Send a message to rf95_server
  uint8_t data[4] = {nitrogen_val, phosphorus_val, potassium_val, node_id};
  rf95.send(data, sizeof(data));

  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(6000))
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len))
    {
      Serial.print("got reply: ");
      Serial.println((char *)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    }
    else
    {
      Serial.println("recv failed");
    }
  }
  else
  {
    Serial.println("No reply, is rf95_server running?");
  }
  delay(400);
}

//IMPRIMIR_DATOS
void printdata()
{
  Serial.print("Nitrogen_Val: ");
  Serial.print(nitrogen_val);
  Serial.println(" mg/kg");
  Serial.print("Phosphorous_Val: ");
  Serial.print(phosphorus_val);
  Serial.println(" mg/kg");
  Serial.print("Potassium_Val: ");
  Serial.print(potassium_val);
  Serial.println(" mg/kg");
  delay(2000);
}
