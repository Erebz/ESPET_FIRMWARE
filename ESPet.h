#ifndef ESPet_h
#define ESPet_h
#include <WString.h>
#include <DHT.h>
#include "Arduino.h"

extern uint16_t baby1 [10000] PROGMEM;
extern uint16_t baby2 [10000] PROGMEM;

class ESPet{
  public:
    ESPet(String name, double happiness, double fatigue, double hunger, double age, double health, String type, int DHTPIN, int DHTTYPE);  
    void tick();

    // Actions
    void feed(double bonus, double malus);
    void sleep(double bonus, double malus);
    void play(double bonus, double malus);
    void treat(double bonus, double malus);
    
    bool canSleep();
    bool canPlay();
    void updateSensor();
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    String exportJson();
    String status();

    // Getters
    String getName();
    int getHappiness();
    int getHunger();
    int getFatigue();
    int getAge();
    int getHealth();
    int getTemperature();
    int getHumidity();
      
  private:
    String _name;
    double _happiness;
    double _fatigue;
    double _hunger;
    double _age;
    double _health;
    bool _sleeping;
    bool _playing;
    String _type;
    DHT * _dht;
    float _humidity;
    float _temperature;
    int _ticks;
    int _ticksMax;
    int _playTime;
    int _sleepTime;
};

#endif // ESPet_h
