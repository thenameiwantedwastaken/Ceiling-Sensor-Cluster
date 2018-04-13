#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define LedPin 13

//Todo
// Add functions to class, add loop to class

// MotionSensor class
class MotionSensor{
  // Setup variables
  int msPin;                        //The pin to read input from
  bool msMotionState;               //Whether in a motion detected state
  unsigned long msLastChangedTime;  //The time of the last state change or last motion detected
  int msDelayPeriod;                //Period (ms) during which an on state will be maintained
  const char *msName;               //Name of the sensor, used as MQTT sub-topic

  //MotionSensor constructor
  public:

    MotionSensor(int pin, int delay, String name)
    {
      // Read in arguments
      msPin = pin;
      msDelayPeriod = delay;
      String topic = "MotionSensor/";
      topic = topic+name;
      msName=topic.c_str();

      // Set defaults for other variables
      msMotionState=0;
      msLastChangedTime=0;

      // Set-up read-in pin
      pinMode(msPin, INPUT);
    } // end constructor

    int Update(){
      // Read the motion state of this sensor right now
      int motionNow = 0;
      int toreturn=3;
      motionNow=digitalRead(msPin);
      //Serial.println(": MotionNow=" + String(motionNow));
      unsigned long currentMillis = millis();

      //Step through each of the sensors
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
      } else if (msMotionState==1 && motionNow==LOW && currentMillis - msLastChangedTime >= msDelayPeriod){
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

// Establish a MAC address
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };

//Set up sensors
//MotionSensor (pin,ms delay,"name")
MotionSensor Toilet(2,10000,"Toilet");
//MotionSensor Hallway(3,10000,"Hallway");


EthernetClient ethClient;
PubSubClient client(ethClient);

//Called if a subscribed MQTT topic is received
void callback(char* topic, byte* payload, unsigned int length) {
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
    if (client.connect("CeilingSensorHub")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("testtopic","hello world from Ceiling Sensor Hub");
      // ... and resubscribe
      // client.subscribe("inTopic"); We don't need to subscribe to anything
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
    reconnect();
  } else {
  client.loop();
  }

int result=Toilet.Update();
Serial.println(result);
if (result==1){
  client.publish("MotionSensor/Toilet","1");
} else if (result==0){
  client.publish("MotionSensor/Toilet","0");
}

delay(200);
}
