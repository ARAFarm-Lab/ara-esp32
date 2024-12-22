#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>

#include "config.h"

class Repository {
  MqttClient* mq;

public:
  virtual ~Repository(){};
  Repository(){};
  Repository(MqttClient* mq)
    : mq(mq){};

  void PushJSON(char* topic, DynamicJsonDocument* json) {
    String jsonStr;
    if (json != NULL) {
      serializeJson(*json, jsonStr);
    }
    this->mq->beginMessage(topic);
    this->mq->print(jsonStr);
    this->mq->endMessage();
  };
};

#endif