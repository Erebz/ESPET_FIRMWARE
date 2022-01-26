#ifndef ESPet_h
#define ESPet_h
#include <WString.h>
#include <DHT.h>

class ESPet{
  public:
    ESPet(String name, double happiness, double fatigue, double hunger, double age, double health, String type, int DHTPIN, int DHTTYPE);  
    void tick();

    // Actions
    bool feed(double bonus, double malus);
    bool sleep(double bonus, double malus);
    bool play(double bonus, double malus);
    bool treat(double bonus, double malus);
    
    bool canSleep();
    bool canPlay();
    bool canEat();
    void updateSensor();
    bool isSleeping();
    bool isPlaying();
    bool isEating();
    bool isTreating();
    bool available();
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    String exportJson();
    String exportJsonAndSensors();
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
    String getType();
          
  private:
    String _name;
    double _happiness;
    double _fatigue;
    double _hunger;
    double _age;
    double _health;
    bool _sleeping;
    bool _playing;
    bool _eating;
    bool _treating;
    String _type;
    DHT * _dht;
    float _humidity;
    float _temperature;
    int _ticks;
    int _ticksMax;
    int _playTime;
    int _sleepTime;
    int _eatTime;
    int _treatTime;
};

#endif // ESPet_h
