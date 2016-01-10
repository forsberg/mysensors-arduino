/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 * 
 * DESCRIPTION
 * This sketch provides an example how to implement a humidity/temperature
 * sensor using DHT11/DHT-22 
 * http://www.mysensors.org/build/humidity
 */
 
#include <SPI.h>
#include <MySensor.h>  
#include <MySigningAtsha204Soft.h>
#include <DHT.h>  

#define CHILD_ID_HUM0 0
#define CHILD_ID_TEMP0 1
#define CHILD_ID_HUM1 2
#define CHILD_ID_TEMP1 3

#define HUMIDITY_SENSOR0_DIGITAL_PIN 5 // Inside the box
#define HUMIDITY_SENSOR1_DIGITAL_PIN 6 // Outside the box
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

MyTransportNRF24 radio;  // NRFRF24L01 radio driver
MyHwATMega328 hw; // Select AtMega328 hardware profile
MySigningAtsha204Soft signer;

MySensor gw(radio, hw, signer); 
DHT dht0;
DHT dht1;
float lastTemp0;
float lastHum0;
float lastTemp1;
float lastHum1;

boolean metric = true; 
MyMessage msgHum0(CHILD_ID_HUM0, V_HUM);
MyMessage msgTemp0(CHILD_ID_TEMP0, V_TEMP);

MyMessage msgHum1(CHILD_ID_HUM1, V_HUM);
MyMessage msgTemp1(CHILD_ID_TEMP1, V_TEMP);



void setup()  
{ 
  gw.begin();
  dht0.setup(HUMIDITY_SENSOR0_DIGITAL_PIN); 
  dht1.setup(HUMIDITY_SENSOR1_DIGITAL_PIN); 

  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)

  gw.present(CHILD_ID_HUM0, S_HUM);
  gw.present(CHILD_ID_TEMP0, S_TEMP);
  gw.present(CHILD_ID_HUM1, S_HUM);
  gw.present(CHILD_ID_TEMP1, S_TEMP); 
  
  metric = gw.getConfig().isMetric;
}

void loop()      
{  
  delay(dht0.getMinimumSamplingPeriod());

  // Read Temperature from sensor 0
  float temperature = dht0.getTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT0");
  } else if (temperature != lastTemp0) {
    lastTemp0 = temperature;
    gw.send(msgTemp0.set(temperature, 1));
    Serial.print("T0 (inside box): ");
    Serial.println(temperature);
  }

  // Read humidity from sensor 0
  float humidity = dht0.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT0");
  } else if (humidity != lastHum0) {
      lastHum0 = humidity;
      gw.send(msgHum0.set(humidity, 1));
      Serial.print("H0 (inside box): ");
      Serial.println(humidity);
  }

  // Read Temperature from sensor 1
  temperature = dht1.getTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT1");
  } else if (temperature != lastTemp1) {
    lastTemp1 = temperature;
    gw.send(msgTemp1.set(temperature, 1));
    Serial.print("T1 (outside box): ");
    Serial.println(temperature);
  }

  // Read humidity from sensor 1
  humidity = dht1.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT1");
  } else if (humidity != lastHum1) {
      lastHum1 = humidity;
      gw.send(msgHum1.set(humidity, 1));
      Serial.print("H1 (outside box): ");
      Serial.println(humidity);
  }

  

  gw.sleep(SLEEP_TIME); //sleep a bit
}

