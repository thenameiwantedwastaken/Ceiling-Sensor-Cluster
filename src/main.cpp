#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>

#define LedPin 13

int pinDHT22 = 8; // Pin for the temp/humidity sensor
SimpleDHT22 dht22; // Object for the temp/humidity sensor
int THcounter=999;
float LastTemp=0;
float LastHum=0;

// MotionSensor class
class MotionSensor {
  // Setup variables
  int msPin;                        //The pin to read input from
  bool msMotionState;               //Whether in a motion detected state
  unsigned long msLastChangedTime;  //The time of the last state change or last motion detected
  unsigned long msDelayPeriod;      //Period (secs) during which an on state will be maintained
  const char *msName;               //Name of the sensor, used as MQTT topic

  //MotionSensor constructor
  public:

    MotionSensor(int pin, unsigned long delay, char* name)
    {
      // Read in arguments
      msPin = pin;
      msDelayPeriod = delay;
      msName = name;

      // Set defaults for other variables
      msMotionState=0;
      msLastChangedTime=0;

      // Set-up read-in pin
      pinMode(msPin, INPUT);
    } // end constructor

    int Update(){
      // Read the motion state of this sensor right now
      int motionNow = LOW;
      int toreturn=3;
      motionNow=digitalRead(msPin);
      unsigned long currentMillis = millis();

      //if there is movement, we can update the timer regardless of the previous state
      if (motionNow==HIGH) {
        msLastChangedTime=currentMillis;
      }

      if (msMotionState==0){
        // Motion state is set to false. Set it to true if there is movement (regardless of timer)
        if (motionNow==HIGH){
          msMotionState=1;
          msLastChangedTime=currentMillis;
          toreturn=1;
        }
      } else if (msMotionState==1 && motionNow==LOW && currentMillis - msLastChangedTime >= (msDelayPeriod*1000)){
        msMotionState=0;
        msLastChangedTime=currentMillis;
        toreturn=0;
      }//end if motion state was true and no motion and timedout
    return(toreturn);
  } // end update routine

  const char* Name(){
    return msName;
  }
}; //end MotionSensor class
//---------------------------- End Class Definitions -------------------------//


// Establish a MAC address
// byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
byte mac[]    = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

//Set up sensors
//MotionSensor (pin,delay (secs),"name")
//MotionSensor Toilet(2,300,"Sensors/Toilet");
//Below is an array of MotionSensor objects
MotionSensor MotionSensorArray[1] = {
 MotionSensor(2,250,(char*)"Sensors/Pin2")//,
// MotionSensor(3,50,(char*)"Sensors/Pin3")
};

EthernetClient ethClient;
PubSubClient client(ethClient);

//Called if a subscribed MQTT topic is received
void callback(char* topic, byte* payload, unsigned int length) {
  client.publish("Sensors/",payload);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
} //End callback - MQTT message received handler

// Called if the MQTT connection is lost
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("CeilingSensorHub2")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("testtopic","hello world from Ceiling Sensor Hub2");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} //End MQTT reconnect function

void setup()
{
  pinMode(LedPin, OUTPUT);
  Serial.begin(57600);

  client.setServer("openhabianpi.fritz.box", 1883);
  client.setCallback(callback);
  Ethernet.begin(mac); // IP will be set by DHCP
  // Allow the hardware to sort itself out
  delay(1500);

} // end setup

void loop()
{
  if (!client.connected()) {
          Serial.println("Connection lost - in void loop");
    reconnect();
  } else {
  client.loop();
  }

// Step through object arrays
for(auto &item : MotionSensorArray){
  switch (item.Update()) {
     case 0:
       client.publish(item.Name(),"0");
       break;
     case 1:
       client.publish(item.Name(),"1");
       break;
     default:
       // do nothing
       Serial.print("Nothing happening on ");
       Serial.println(item.Name());
       break;
  }
}

//Check and publish temp and humidity
THcounter++;
if (THcounter>6000){
  THcounter=0;
  float temperature = 0;
  float humidity = 0;
  byte data[40] = {0};
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(pinDHT22, &temperature, &humidity, data)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT22 failed, err="); Serial.println(err);delay(2000);
    return;
  }

  if (LastTemp!=temperature){
    LastTemp=temperature;
    static char outstr[15];
    dtostrf(temperature,4, 1, outstr);
    client.publish("Sensors/Temp",outstr);
  }

  if (LastHum!=humidity){
    LastHum=humidity;
    static char outstr[15];
    dtostrf(humidity,4, 1, outstr);
    client.publish("Sensors/Humidity",outstr);
  }
}
delay(200);
}
