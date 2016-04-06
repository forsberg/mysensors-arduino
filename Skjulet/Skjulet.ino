#include <SPI.h>
#include <MySensor.h>  
#include <MySigningAtsha204Soft.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <OneWire.h>


#define CHILD_ID_HUM0 0
#define CHILD_ID_TEMP0 1
#define CHILD_ID_BATTERYVOLTAGE 2
#define BASE_TEMP_CHILD_ID 3

#define HUMIDITY_SENSOR0_DIGITAL_PIN 6 
unsigned long SLEEP_TIME = 60000; // Sleep time between reads (in milliseconds)

MyTransportNRF24 radio;  // NRFRF24L01 radio driver
MyHwATMega328 hw; // Select AtMega328 hardware profile
MySigningAtsha204Soft signer;

MySensor gw(radio, hw, signer); 
DHT dht0;
float lastTemp0;
float lastHum0;

#define ONE_WIRE_BUS A1 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 1

boolean metric = true; 
boolean sentValue = false;
MyMessage msgHum(CHILD_ID_HUM0, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP0, V_TEMP);
MyMessage msgBatteryVoltage(CHILD_ID_BATTERYVOLTAGE, V_VOLTAGE);

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices 
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.

float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;

int oldBatteryPcnt = 0;

void setup()  
{
    Serial.begin(115200);
    Serial.println("Start");
    analogReference(INTERNAL); // Use 1.1V reference
 
    gw.begin();
    Serial.println("Init dht0");
    dht0.setup(HUMIDITY_SENSOR0_DIGITAL_PIN);
    Serial.println("Done init dht0");

    // Startup up the OneWire library
  sensors.begin();
  Serial.println("Done sensors.begin()");
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);

  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();

  Serial.print("Found ");
  Serial.print(numSensors);
  Serial.println(" temp sensors");

  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Humidity", "1.0");

  // Register all sensors to gw (they will be created as child devices)

  gw.present(CHILD_ID_BATTERYVOLTAGE, S_MULTIMETER);
  gw.present(CHILD_ID_HUM0, S_HUM);
  gw.present(CHILD_ID_TEMP0, S_TEMP);

  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
      gw.present(BASE_TEMP_CHILD_ID+i, S_TEMP);
  }
  
      metric = gw.getConfig().isMetric;
}

void loop()      
{  
  delay(2000);

  // Read Temperature from sensor 0
  float temperature = dht0.getTemperature();
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
  float humidity = dht0.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT0");
  } else if (humidity != lastHum0) {
      sentValue = true;    
      lastHum0 = humidity;
      gw.send(msgHum.setSensor(CHILD_ID_HUM0).set(humidity, 1));
      Serial.print("H0 (inside box): ");
      Serial.println(humidity);
  }

  // Fetch temperatures from Dallas sensors


  int batterySensorValue = analogRead(A0);
  Serial.print("Battery sensor value: ");
  Serial.println(batterySensorValue);

   // 1M, 470K divider across battery and using internal ADC ref of 3.3V
   // ((1.5e6+470e3)/470e3)*1.1 = Vmax = 4.61V
   // 4.61/1023 = Volts per bit = 0.00451 volts per bit
  float batteryV  = batterySensorValue * 0.004506977808281858;
   
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
     //gw.sendBatteryLevel(batteryPcnt);
     oldBatteryPcnt = batteryPcnt;
     //gw.send(msgBatteryVoltage.set(batteryV, 2));
   }  

   sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  Serial.print("Sleeping ");
  Serial.print(conversionTime);
  Serial.println("ms waiting for temperature readout");

  delay(conversionTime);  
  // Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
 
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((sensors.getTempCByIndex(i)) * 10.)) / 10.;
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print("temp ");
    Serial.println(temperature);

    if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) {
        // Send in the new temperature
        gw.send(msgTemp.setSensor(BASE_TEMP_CHILD_ID+i).set(temperature,1));
        // Save new temperatures for next compare
        lastTemperature[i]=temperature;
        sentValue = true;        
    }

  }
  

  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  gw.sleep(SLEEP_TIME);
  sentValue = false;
}




