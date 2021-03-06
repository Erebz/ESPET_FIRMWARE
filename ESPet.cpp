#include "ESPet.h"

ESPet::ESPet(String name, double happiness, double fatigue, double hunger, double age, double health, String type, int DHTPIN, int DHTTYPE){
  _ticks = 0;
  _ticksMax = 10;
  _name = name;
  _happiness = happiness;
  _fatigue = fatigue;
  _hunger = hunger;
  _age = age;
  _health = health;
  _sleeping = false;
  _playing = false;
  _eating = false;
  _treating = false;
  _playTime = 0;
  _sleepTime = 0;
  _eatTime = 0;
  _treatTime = 0;
  _type = type;
  _dht = new DHT(DHTPIN, DHTTYPE);
  _dht->begin();
  _humidity = 50;
  _temperature = 30;
}

void ESPet::tick(){
  _ticks = (_ticks + 1) % _ticksMax;
  updateSensor();
  if(_ticks == 0){
    if(_sleeping){
      _sleepTime = (_sleepTime + 1) % 10; 
      if(_sleepTime == 0) _sleeping = false;
    }else if(_playing){
      _playTime = (_playTime + 1) % 2; 
      if(_playTime == 0) _playing = false;
    }else if (_eating){
      _eatTime = (_eatTime + 1) % 2; 
      if(_eatTime == 0) _eating = false;
    }else if(_treating){
      _treatTime = (_treatTime + 1) % 2; 
      if(_treatTime == 0) _treating = false;
    }else{
      double ratio_t = mapFloat(_temperature, 0, 60, 0, 2);
      double ratio_h = mapFloat(_humidity, 0, 100, 0, 2);
      _fatigue += 0.5 + ratio_t + ratio_h;;
      _age += 0.1;
      _hunger += 1 + ratio_t + ratio_h;
      _happiness -= 1;
      _health -= 0.05;      
    }
  }
  if(_fatigue > 100) _fatigue = 100;
  if(_hunger > 100) _hunger = 100;
  if(_happiness < 0) _happiness = 0;
  if(_health < 0) _health = 0;
}

bool ESPet::feed(double bonus, double malus){
  if(canEat()){
    _hunger -= bonus;  
    _eating = true;
    return true;
  }else{
    return false;
  }
}

bool ESPet::sleep(double bonus, double malus){
  if(canSleep()){
    _fatigue = 0;
    _health += bonus;
    _hunger += malus;
    _sleeping = true;
      return true;
  }else{
    return false;
  }
}

bool ESPet::play(double bonus, double malus){
  if(canPlay()){
    _happiness += bonus;
    _fatigue += malus;
    _hunger += malus;
    _playing = true;
    return true;
  }else{
    return false;
  }
}

bool ESPet::treat(double bonus, double malus){
  _happiness += bonus;
  _health -= malus;
  _treating = true;
  return true;
}

bool ESPet::canSleep(){
  return _fatigue >= 50;
}

bool ESPet::canPlay(){
  return _fatigue <= 50 && _hunger <= 50;
}

bool ESPet::canEat(){
  return _hunger >= 40 && _happiness >= 50;   
}

bool ESPet::isSleeping(){
  return _sleeping;
}

bool ESPet::isPlaying(){
  return _playing;
}

bool ESPet::isEating(){
  return _eating;
}

bool ESPet::isTreating(){
  return _treating;
}

bool ESPet::available(){
  return !_sleeping && !_playing && !_treating && !_eating;  
}

void ESPet::updateSensor(){
  _humidity = _dht->readHumidity();
  _temperature = _dht->readTemperature();
  if(_humidity > 100) _humidity = 100;
  if(_humidity < 0) _humidity = 0;
  if (isnan(_humidity)) _humidity = 0;
  if(_temperature > 999) _temperature = 999; 
  if(_temperature < -99) _temperature = -99;
  if (isnan(_temperature)) _temperature = 0;
}

float ESPet::mapFloat(float x, float in_min, float in_max, float out_min, float out_max){
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}

String ESPet::exportJson(){
  String json = "";
  json += "\"happiness\":\"" + String(_happiness, 2) + "\",";
  json += "\"fatigue\":\"" + String(_fatigue, 2) + "\",";
  json += "\"hunger\":\"" + String(_hunger, 2) + "\",";
  json += "\"age\":\"" + String(_age, 2) + "\",";
  json += "\"health\":\"" + String(_health, 2) + "\",";
  json += "\"type\":\"" + _type + "\",";
  json += "\"name\":\"" + _name + "\"";
  return json;
}

String ESPet::exportJsonAndSensors(){
  String json = "";
  json += "\"happiness\":\"" + String(_happiness, 2) + "\",";
  json += "\"fatigue\":\"" + String(_fatigue, 2) + "\",";
  json += "\"hunger\":\"" + String(_hunger, 2) + "\",";
  json += "\"age\":\"" + String(_age, 2) + "\",";
  json += "\"health\":\"" + String(_health, 2) + "\",";
  json += "\"temperature\":\"" + String((int)_temperature) + "\",";
  json += "\"humidity\":\"" + String((int)_humidity) + "\"";
  return json;
}

String ESPet::status(){
  return "HAPPINESS : " + String(_happiness, 2) + "  FATIGUE : " + String(_hunger, 2) + "  AGE : " + String(_age, 2);    
}

String ESPet::getName(){
  return _name; 
}

String ESPet::getType(){
  return _type; 
}

int ESPet::getHappiness(){
  return (int) (_happiness + 0.5 - (_happiness<0)); 
}
  
int ESPet::getHunger(){
  return (int) (_hunger + 0.5 - (_hunger<0)); 
}

int ESPet::getFatigue(){
  return (int) (_fatigue + 0.5 - (_fatigue<0));  
}

int ESPet::getAge(){
  return (int) (_age + 0.5 - (_age<0)); 
}

int ESPet::getHealth(){
  return (int) (_health + 0.5 - (_health<0));  
}

int ESPet::getTemperature(){
  return (int) (_temperature + 0.5 - (_temperature<0));  
}

int ESPet::getHumidity(){
  return (int) (_humidity + 0.5 - (_humidity<0));  
}
