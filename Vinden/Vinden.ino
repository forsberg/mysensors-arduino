 
#include <SPI.h>
#include <MySensor.h>  
#include <MySigningAtsha204Soft.h>
#include <DHT.h>  

#define CHILD_ID_HUM0 0
#define CHILD_ID_TEMP0 1
#define CHILD_ID_HUM1 2
#define CHILD_ID_TEMP1 3
#define CHILD_ID_BATTERYVOLTAGE 4

#define DHTTYPE DHT22
#define HUMIDITY_SENSOR0_DIGITAL_PIN 5 // Inside the box
#define HUMIDITY_SENSOR1_DIGITAL_PIN 6 // Outside the box
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

MyTransportNRF24 radio;  // NRFRF24L01 radio driver
MyHwATMega328 hw; // Select AtMega328 hardware profile
MySigningAtsha204Soft signer;

MySensor gw(radio, hw, signer); 
DHT dht0(HUMIDITY_SENSOR0_DIGITAL_PIN, DHTTYPE);
DHT dht1(HUMIDITY_SENSOR1_DIGITAL_PIN, DHTTYPE);
float lastTemp0;
float lastHum0;
float lastTemp1;
float lastHum1;

boolean metric = true; 
boolean sentValue = false;
MyMessage msgHum(CHILD_ID_HUM0, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP0, V_TEMP);
MyMessage msgBatteryVoltage(CHILD_ID_BATTERYVOLTAGE, V_VOLTAGE);

int oldBatteryPcnt = 0;

void setup()  
{ 
  analogReference(DEFAULT); // Use 3.3V reference
 
  gw.begin();
  dht0.setup(HUMIDITY_SENSOR0_DIGITAL_PIN); 
  dht1.setup(HUMIDITY_SENSOR1_DIGITAL_PIN); 

  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)

  gw.present(CHILD_ID_BATTERYVOLTAGE, S_MULTIMETER);
  gw.present(CHILD_ID_HUM0, S_HUM);
  gw.present(CHILD_ID_TEMP0, S_TEMP);
  gw.present(CHILD_ID_HUM1, S_HUM);
  gw.present(CHILD_ID_TEMP1, S_TEMP); 
  
  metric = gw.getConfig().isMetric;
}

void loop()      
{  
  delay(2000);

  // Read Temperature from sensor 0
  float temperature = dht0.readTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT0");
  } else if (temperature != lastTemp0) {
    sentValue = true;
    lastTemp0 = temperature;
    gw.send(msgTemp.setSensor(CHILD_ID_TEMP0).set(temperature, 1));
    Serial.print("T0 (inside box): ");
    Serial.println(temperature);
  }

  // Read humidity from sensor 0
  float humidity = dht0.readHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT0");
  } else if (humidity != lastHum0) {
      sentValue = true;    
      lastHum0 = humidity;
      gw.send(msgHum.setSensor(CHILD_ID_HUM0).set(humidity, 1));
      Serial.print("H0 (inside box): ");
      Serial.println(humidity);
  }

  // Read Temperature from sensor 1
  temperature = dht1.readTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT1");
  } else if (temperature != lastTemp1) {
    sentValue = true;    
    lastTemp1 = temperature;
    gw.send(msgTemp.setSensor(CHILD_ID_TEMP1).set(temperature, 1));
    Serial.print("T1 (outside box): ");
    Serial.println(temperature);
  }

  // Read humidity from sensor 1
  humidity = dht1.readHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT1");
  } else if (humidity != lastHum1) {
      sentValue = true; 
      lastHum1 = humidity;
      gw.send(msgHum.setSensor(CHILD_ID_HUM1).set(humidity, 1));
      Serial.print("H1 (outside box): ");
      Serial.println(humidity);
  }

  int batterySensorValue = analogRead(A0);
  Serial.print("Battery sensor value: ");
  Serial.println(batterySensorValue);

   // 1M, 470K divider across battery and using internal ADC ref of 3.3V
   // ((1e6+470e3)/470e3)*3.3 = Vmax = 10.32 Volts
   // 10.32/1023 = Volts per bit = 0.010089224433768015
   float batteryV  = batterySensorValue * 0.010089224433768015;
   
   float batteryVremaining = batteryV - 3.3;
   int batteryPcnt = 100 * batteryVremaining / (4.2-3.3);   

   Serial.print("Battery Voltage: ");
   Serial.print(batteryV);
   Serial.println(" V");

   Serial.print("Battery percent: ");
   Serial.print(batteryPcnt);
   Serial.println(" %");
   

   if (oldBatteryPcnt != batteryPcnt && sentValue) { // Only send battery value if we already powered up radio 
     // Power up radio after sleep
     gw.sendBatteryLevel(batteryPcnt);
     oldBatteryPcnt = batteryPcnt;
     gw.send(msgBatteryVoltage.set(batteryV, 2));
   }  

  gw.sleep(SLEEP_TIME); //sleep a bit
  sentValue = false;
}

