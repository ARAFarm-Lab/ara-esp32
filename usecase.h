#ifndef USECASE_H
#define USECASE_H

#include <String.h>

#include <map>

#include "constants.h"
#include "repository.h"

class Usecase {
private:
  Repository* repository;

  // dispatcherMap is map of topic to the executor function that will be
  std::map<String, void (Usecase::*)(DynamicJsonDocument*)> dispatcherMap;

  // actuatorMap is a map of actuator ID to the actuator pin number on the devince.
  // the ID is used to translate the action from server.
  std::map<int, int> actuatorMap;

  // configurationActionsMap is map of action type to the executor function
  // that will be invoked. The json payload will be on form of [[actionType, configurationCategory, configurationValue]]
  std::map<int, void (Usecase::*)(DynamicJsonDocument*)> configurationActionsMap;

public:
  virtual ~Usecase(){};
  Usecase(){};
  Usecase(Repository* repository)
    : repository(repository) {
    // Push a request to the server to get the initial state of the device
    // The server will return with another message to update the device
    char topic[] = "device-initial-state-request/" DEVICE_ID;
    this->repository->PushJSON(topic, NULL);

    // Define the dispatcher map based on the incoming topic
    this->dispatcherMap["action/" DEVICE_ID] = &Usecase::doAction;
    this->dispatcherMap["device-initial-state-response/" DEVICE_ID] = &Usecase::doInitDeviceCurrentState;
    this->dispatcherMap["heartbeat-request/" DEVICE_ID] = &Usecase::doSendHeartbeatResponse;
    this->dispatcherMap["restart-device/" DEVICE_ID] = &Usecase::doRestartDevice;
  };

  // doAction will be the entry point of the usecase. It will invoke the
  // executor function based on the given actionType sent from the server.
  void doAction(DynamicJsonDocument* json) {
    int actuatorID = (*json)[0].as<int>();
    if (actuatorMap.find(actuatorID) == actuatorMap.end()) {
      return;
    }

    String value = (*json)[1].as<String>();
    digitalWrite(this->actuatorMap[actuatorID], value == "true" ? LOW : HIGH);
    blinkBuiltInLED(4);
  }

  // this function will be invoked when the device is initialized. It will
  // update the current state of the device based on the given json payload
  // from the server.
  void doInitDeviceCurrentState(DynamicJsonDocument* json) {
    if (json == NULL) {
      return;
    }

    // Unset any previously configured pins
    for (auto& entry : this->actuatorMap) {
      int pinNumber = entry.second;
      pinMode(pinNumber, INPUT);  // Reset pin to default state
    }
    this->actuatorMap.clear();

    // Register available actuator
    JsonArray actuators = (*json)["a"].as<JsonArray>();
    for (JsonArray item : actuators) {
      int pinNumber = item[1];
      this->actuatorMap[item[0]] = pinNumber;
      pinMode(pinNumber, OUTPUT);
    }

    // Initialize actuator state
    JsonArray states = (*json)["v"].as<JsonArray>();
    for (JsonArray config : states) {
      digitalWrite(this->actuatorMap[config[0]], config[1] == "true" ? LOW : HIGH);
    }
  }

  // doSendHeartbeatResponse will send the heartbeat response to server, indicating the IoT device is running healthy
  void doSendHeartbeatResponse(DynamicJsonDocument* json) {
    char topic[] = "heartbeat-response/" DEVICE_ID;
    this->repository->PushJSON(topic, NULL);
    blinkBuiltInLED(2);
  }

  // doRestartDevice will restart the IoT device in case of configuration change or remote restart
  void doRestartDevice(DynamicJsonDocument* json) {
    ESP.restart();
  }

  void dispatchMQMessage(String topic, DynamicJsonDocument* json) {
    if (dispatcherMap.find(topic) == dispatcherMap.end()) {
      return;
    }

    (this->*dispatcherMap[topic])(json);
  }

  void blinkBuiltInLED(uint times) {
    for (int i = 0; i < times; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
  }
};

#endif