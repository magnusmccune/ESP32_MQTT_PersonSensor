#include <Wire.h>
#include "person_sensor.h"
#include <ArduinoJson.h>
#include "EspMQTTClient.h"
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password
char mqtt_server[] = SECRET_MQTT_SERVER; // your mqtt server ip
char client_name[] = "esp32"; // your client name
char mqtt_port[] = "1883"; // your mqtt port

EspMQTTClient client(
  ssid,
  pass,
  mqtt_server,       // MQTT Broker server ip
  client_name,            // Client name that uniquely identify your device
  mqtt_port                // The MQTT port, default to 1883. this line can be omitted
);

const int32_t SAMPLE_DELAY_MS = 1000;
StaticJsonDocument<512> doc;
person_sensor_results_t oldResult ={};

void setup() {
  Wire.begin();
  Serial.begin(9600);

  client.enableLastWillMessage("esp32/state", "offline", true);
  client.setMaxPacketSize(512);
}

void onConnectionEstablished(){
client.publish("esp32/state", "online", true);
}

bool personDataAreTheSame(const person_sensor_results_t& data1, const person_sensor_results_t& data2) {
  if (data1.num_faces != data2.num_faces) {
      return false;
  }

  for (int i = 0; i < data1.num_faces; ++i) {
    const person_sensor_face_t& face1 = data1.faces[i];
    const person_sensor_face_t& face2 = data2.faces[i];

    if (face1.box_confidence != face2.box_confidence ||
        face1.box_left != face2.box_left ||
        face1.box_top != face2.box_top ||
        face1.box_right != face2.box_right ||
        face1.box_bottom != face2.box_bottom ||
        face1.is_facing != face2.is_facing) {
      return false;
    }
  }

  return true;
}

void loop() {
  client.loop();
  person_sensor_results_t results = {};
  JsonObject object = doc.to<JsonObject>();
  char buffer[512];

  if (!person_sensor_read(&results)) {
    Serial.println("No person sensor results found on the i2c bus");
    delay(SAMPLE_DELAY_MS);
    return;
  }
 if (! personDataAreTheSame(oldResult, results)){
    oldResult = results;
    Serial.println("********************");
    String num_faces_string = String(results.num_faces);
    client.publish("esp32/insight", num_faces_string);
  
    object["faces"] = results.num_faces;
  
    for (int i = 0; i < results.num_faces; ++i) {
      const person_sensor_face_t* face = &results.faces[i];
      JsonObject FACE = object.createNestedObject("Face" + String(i));
      FACE["confidence"] = face->box_confidence;
      FACE["left"] = face->box_left;
      FACE["top"] = face->box_top;
      FACE["right"] = face->box_right;
      FACE["bottom"] = face->box_bottom;
      FACE["facing"] = face->is_facing;
    }
    serializeJsonPretty(doc, buffer);
    client.publish("esp32/faces", buffer);
    serializeJsonPretty(object, Serial);
    Serial.println("");
    Serial.println("********************");
    delay(SAMPLE_DELAY_MS);
 }
}