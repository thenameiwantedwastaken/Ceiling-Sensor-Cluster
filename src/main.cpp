#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define LedPin 13

// Establish a MAC address
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };

//Setup arrays for microwave sensors
int SensorPins[] = {2,3}; // A list of the sensor pins to which motion sensors are attached
const byte SensorCount = sizeof(SensorPins) / sizeof(int);
bool MotionState[SensorCount]; // 1=on, 0=off
unsigned long LastTime[SensorCount]; //Last time a change was made to motionstate
int DelayPeriod[SensorCount];
const char *SensorNames[] = { "Toilet", "Hallway",  "Bathroom"};



EthernetClient ethClient;
PubSubClient client(ethClient);

//Called if a subscribed MQTT topic is received
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
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
  // Setup input pin, motion state and timers for each sensor
  for (int x=0;x<SensorCount;x++){
    pinMode(SensorPins[x], INPUT);
    MotionState[x]=0;
    LastTime[x]=0;
    DelayPeriod[x]=10000; // Milliseconds to ignore input after each change of state
  }

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

for (int x=0; x<SensorCount; x++){



    // Sort out the MQTT Topic string as a char constant
    String topic = "MotionSensor/";
    topic = topic+SensorNames[x];
    const char* chartopic=topic.c_str();

    // Read the motion state of this sensor right now
    int motionNow = 0;
    motionNow=digitalRead(SensorPins[x]);
    //Serial.println(": MotionNow=" + String(motionNow));
    unsigned long currentMillis = millis();

    //Step through each of the sensors
    //if there is movement, we can update the timer regardless of the previous state
    if (motionNow==HIGH) {
      LastTime[x]=currentMillis;
    }
    if (MotionState[x]==0){
      // Motion state is set to false. Set it to true if there is movement (regardless of timer)
      if (motionNow==HIGH){
        Serial.println(topic +" motion event triggered");
        client.publish(chartopic,"1"); // Announce motion to MQTT
        MotionState[x]=1;
        LastTime[x]=currentMillis;
      }
    } else if (MotionState[x]==1 && motionNow==LOW && currentMillis - LastTime[x] >= DelayPeriod[x]){
      Serial.println(topic + " motion event ended");
      client.publish(chartopic,"0"); // Announce (lack of) motion to MQTT
      MotionState[x]=0;
      LastTime[x]=currentMillis;
    }//end if motion state was true and no motion and timedout
}
delay(200);
}
