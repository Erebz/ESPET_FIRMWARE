#include "ESPetWifiConfig.h"
#include "ESPet.h"
#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WebSocketsClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>

// URLS 
char * ws_host = "erebz.fr";
int ws_port = 9999;
char * api = "http://espet.erebz.fr";

// Wifi & Websocket
WebSocketsClient webSocketClient;
WebServer server(80);
String ssid="default", password="default"; 
const char* ssid_AP = "ESPet";
const char* password_AP = "ESPet";
bool WIFI_OK = false;
bool parameters_received = false;
String mac, token;
int last_espet = -1;
int loginAttempts = 0;
StaticJsonDocument<100> jsonDoc;

// Ecran TFT
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft); 

// ESPet
TaskHandle_t task_pet;
ESPet * pet;
bool GAME_ON = false;
#define DHTPIN 24
#define DHTTYPE 10

bool connectWifi(String ssid_, String password_, int attempts){
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid_.c_str(), password_.c_str());
  tft.print("Connecting to Wifi");
  for(int i=0; i < attempts; i++){ 
    if(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      tft.print(".");
    }else return true;
  }
  WiFi.disconnect();
  return false;
}

void initWifi(){
  WIFI_OK = false;
  while(!WIFI_OK){
    Serial.println("Tentative de connexion au WiFi("+ssid+")");
    tft.println("Saved WiFI ssid : ("+ssid+")");
    if(connectWifi(ssid, password, 10)){
      Serial.println("Connexion WiFi établie !");
      tft.println("");
      tft.println("Successfully connected to WiFi.");
      saveWifiParameters(ssid, password);
      WIFI_OK = true;
    }else{
      WiFi.mode(WIFI_MODE_AP);
      parameters_received = false;
      WiFi.softAP("ESPET");
      Serial.println("Pas de connexion wifi. Passage en mode Routeur.");
      tft.println("");
      tft.println("Can't connect to WiFi.");
      tft.println("Please open the ESPet app and setup the WiFi access.");
      
      server.on("/setWifi", HTTP_POST, []() {
        String postBody = server.arg("plain");
        Serial.println(postBody);
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, postBody);
        if(!error){
          JsonObject postObj = doc.as<JsonObject>();
          if (postObj.containsKey("ssid") && postObj.containsKey("password")) {
            ssid = postObj["ssid"].as<char*>();
            password = postObj["password"].as<char*>();
            Serial.println("SSID : " + ssid);
            Serial.println("PASSWORD : " + password);
            tft.println("Received Wifi ssid and password.");
            tft.println("Trying to reconnect.");
            server.send(200, F("text/html"), F("SSID AND PASSWORD SET. TRYING TO CONNECT..."));
            parameters_received = true;
          }
        }
      });
      server.begin();
      while(!parameters_received){
        server.handleClient();
      }
     }
  }
  mac = WiFi.macAddress();
}

void login(){
  if(WiFi.status() != WL_CONNECTED) initWifi();
  char url[50];
  strcpy(url,api);
  strcat(url,"/device/login");
  String json = "{\"mac\" : \"" + mac + "\"}";
  Serial.println("Tentative d'authentification...");
  Serial.println(json);
  tft.println("Connecting to the server...");
  String response = postJson(url, json.c_str());
  if(response == "REGISTER"){
      Serial.println("Connexion impossible. Attente de l'inscription de la part de l'utilisateur.");
      waitForRegister();
  }
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, response);
  if(!error){
    JsonObject postObj = doc.as<JsonObject>();
    if (postObj.containsKey("login_token")) {
      token = postObj["login_token"].as<char*>();
      if (postObj.containsKey("last_espet")) {
        last_espet = postObj["last_espet"].as<int>();
                    Serial.println(last_espet);
      }
    }else{
        tft.print("An error occurred while communicating with the server.");
        ESP.restart();
    }

  }else{
    Serial.println(response);
    loginAttempts++;
    Serial.println("Une erreur est survenue lors de la communication avec le serveur. Nouvelle tentative...");
    tft.println("An error occurred while communicating with the server.");
    delay(1000);
    if(loginAttempts > 5){
      tft.println("Too many attempts.");
      tft.println("Please check server status.");
      for(;;);
    }
    login(); 
  }
}

void waitForRegister(){
  // afficher adresse MAC sur l'écran
  // l'utilisateur devra enegristrer l'ESPET via l'appli
  // l'utilisateur devra ensuite redémarrer l'ESPET
  Serial.println("Veuillez inscrire l'appareil à partir de votre application.");
  Serial.println("Ensuite, rédémarrez l'appareil.");
  Serial.println("Adresse MAC : " + mac);
  tft.println("Couldn't authenticate.");
  tft.println("Please open the ESPet app and register"), 
  tft.println("this device.");
  tft.println("Please restart the ESPet when it's done.");
  tft.setTextSize(2);
  tft.println("");
  tft.println("MAC address :");
  tft.println(mac);
  
  delay(180000);
  ESP.restart();
}

String postJson(const char * url, const char * payload){
  HTTPClient http;
  String response = "{}";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  if(http.POST(payload) >= 0) response = http.getString();
  http.end();
  return response;
}

String getJson(const char * url){
  HTTPClient http;
  String response = "{}";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  if(http.GET() >= 0) response = http.getString();
  http.end();
  return response;
}

int postJsonGetCode(const char * url, const char * payload){
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  http.end();
  Serial.println("code http : " + code);
  return code;
}

int split(char * payload, char * delimiter, char * args[10]){
  char * arg = strtok(payload, delimiter);
  int n = 0;
  while (arg != NULL) {
    args[n] = arg;
    arg = strtok(NULL, " ");
    n++;
  }
  return n;
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
 int n, id_pet;
 String ans, load_pet, pet_json;
 bool error;
 switch(type) {
  case WStype_TEXT:
    Serial.println((char *) payload);
    // single arg commands :
    if(strcmp((char *) payload, "AUTH?")==0){
      webSocketClient.sendTXT("ESP32 " + mac + " " + token);
    }
    else if(strcmp((char *) payload, "GAME?")==0){
      ans = "";
      if(GAME_ON){
        ans = "YES";
      }else{
        ans = "NO";
      }
      webSocketClient.sendTXT("GAME? " + ans);
    }
    else if(strcmp((char *) payload, "SAVE")==0){
      ans = "ERROR";
      if(last_espet > 0){
        pet_json = "{" + pet->exportJson() + ",\"id_pet\":" + last_espet + "}";
        char url[50];
        strcpy(url, api);
        strcat(url, "/game/save");
        Serial.println(pet_json);
        if(postJsonGetCode(url, pet_json.c_str()) >= 0) ans = "OK";
      }
      webSocketClient.sendTXT("SAVE " + ans);
      printConsole("SAVING...");
    }
    else if(strcmp((char *) payload, "INFO")==0){
      pet_json = "{" + pet->exportJson() + "}";
      Serial.println(pet_json);
      char message[150];
      strcpy(message, "INFO ");
      strcat(message, pet_json.c_str());
      webSocketClient.sendTXT(message);
    }else{
      
      // multiple args commands :
      char * args[10];
      n = split((char *)payload, " ", args);

      if(n==2){
        // ACTION
        if(strcmp(args[0], "ACTION")==0){ 
          ans = "";
          if(doAction(args[1])) ans = "OK";
          else ans = "NO";
          char message[30];
          strcpy(message, "ACTION ");
          strcat(message, ans.c_str());
          webSocketClient.sendTXT(message);
        }
        // LOAD
        else if(strcmp(args[0], "LOAD")==0){
          Serial.println("GOT LOAD COMMAND");
          if(GAME_ON && last_espet > 0){
            Serial.println("SAVING BEFORE LOADING...");
            pet_json = "{" + pet->exportJson() + ",\"id_pet\":" + last_espet + "}";
            char url[50];
            strcpy(url, api);
            strcat(url, "/game/save");
            if(postJsonGetCode(url, pet_json.c_str()) > 0){
              Serial.println("SAVED BEFORE LOADING");
            }else{
              Serial.println("COULDN'T SAVE BEFORE LOADING");
            }
          }
  
          GAME_ON = false;
          char load_pet[20];
          strcpy(load_pet, args[1]);
          id_pet = atoi(load_pet);
          last_espet = id_pet;
          startGame();
          delay(500);
          if(GAME_ON){
            webSocketClient.sendTXT("LOAD OK");
            printConsole("Hello !");
          }else{
            webSocketClient.sendTXT("LOAD ERROR");
          }
        }
      }       
    }

    
  break;
  
  case WStype_CONNECTED:
      startGame();
  break;  
  case WStype_DISCONNECTED:
      printConsole("DISCONNECTED.");
      delay(5000);
      ESP.restart();
  break;
  default:break;
 }
}

void initWebsocket(){
  tft.println("Websocket initialization...");
  webSocketClient.begin(ws_host, ws_port, "/");
  webSocketClient.onEvent(webSocketEvent);
  webSocketClient.setReconnectInterval(5000);
}

bool loadEspet(int id_pet){
  char url[100];
  strcpy(url, api);
  strcat(url, "/game/load/");
  char buf[15];
  itoa(id_pet, buf, 10);
  strcat(url, buf);
  strcat(url, "/");
  strcat(url, mac.c_str());
  String json = getJson(url);
  Serial.println(url);
  Serial.println(json);
  //json = "{\"id_pet\": 1,\"name\": \"Mon ESPet\",\"happiness\": 0.0,\"hunger\": 0.0,\"fatigue\": 0.0,\"age\": 0.0,\"health\": 0.0,\"type\": \"adult\"}";
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, json);
  if(!error){
    JsonObject getObj = doc.as<JsonObject>();
    if (getObj.containsKey("name") && getObj.containsKey("happiness") 
     && getObj.containsKey("hunger") && getObj.containsKey("fatigue") 
     && getObj.containsKey("age") && getObj.containsKey("health") 
     && getObj.containsKey("type")) {
      double happiness = getObj["happiness"].as<double>();
      double hunger = getObj["hunger"].as<double>();
      double fatigue = getObj["fatigue"].as<double>();
      double age = getObj["age"].as<double>();
      double health = getObj["health"].as<double>();
      String name = getObj["name"].as<char*>();
      String type = getObj["type"].as<char*>();
      pet = new ESPet(name, happiness, fatigue, hunger, age, health, type, DHTPIN, DHTTYPE);
      return true;
    }else return false;
  }else return false;
}

bool doAction(char * action){
  bool reaction = false;
  if(strcmp(action, "PLAY")==0){
    reaction = pet->play(10,10);
  }else if(strcmp(action, "SLEEP")==0){
    reaction = pet->sleep(10,10);    
  }else if(strcmp(action, "FEED")==0){
    reaction = pet->feed(10,10);    
  }else if(strcmp(action, "TREAT")==0){
    reaction = pet->treat(10,10);    
  }
  return reaction;
}

void startGame(){
  tft.fillScreen(TFT_BLACK);

  if(last_espet > 0){
    Serial.printf("LOADING THE ESPET #%d", last_espet);
    if(loadEspet(last_espet)){
      drawHUD();
      drawPet();
      GAME_ON = true;  
    }
  }
  if(!GAME_ON){
    tft.println("No espet loaded.");
    tft.println("Please select a gamedata from the app.");
  }
}

void runEspet( void * pvParameters ){
  for(;;){
    if(GAME_ON){
      pet->tick();
      clearValues();
      printValues();
      //Serial.println(pet->status()); 
      if(pet->isSleeping()){
        printConsole("zZzZZ...");
      }else if(pet->isPlaying()){
        printConsole("I'm having fun!");
      }else{
        printConsole(" ");
      }
    }
    vTaskDelay( pdMS_TO_TICKS( 1000 ) );
  }
}

void drawPet(){
  uint16_t * image;
  image = baby1;
  uint16_t transparent = TFT_TRANSPARENT;
  uint16_t color = TFT_WHITE; 
  sprite.setColorDepth(8);         
  sprite.createSprite(100, 100);                  
  sprite.fillSprite(transparent);
  for(int i = 0; i < 100; i++){
    for(int j = 0; j < 100; j++){
      sprite.drawPixel(i, j, image[j*100 + i]);  
    }
  }
  sprite.setBitmapColor(color, transparent);
  sprite.pushSprite(70, 110, transparent);
  sprite.deleteSprite();
}

void drawHUD(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  
  tft.setCursor(5, 10);
  tft.println("HAP");
  tft.setCursor(5, 30);
  tft.println("HUN");
  tft.setCursor(130, 10);
  tft.println("FAT");
  tft.setCursor(130, 30);
  tft.println("AGE");
  tft.setCursor(185, 65);
  tft.println("HUM");
  tft.drawLine(0, 55, 240, 55, TFT_WHITE);
  tft.drawRect(5, 250, 230, 65, TFT_WHITE);
  String name = pet->getName();
  tft.setCursor(120 - (int) (name.length()*5), 220);
  tft.print(pet->getName());
}

void clearValues(){
  tft.setTextSize(2);
  tft.setCursor(60, 10);
  tft.print("    ");
  tft.setCursor(60, 30);
  tft.print("    ");
  tft.setCursor(185, 10);
  tft.print("    ");
  tft.setCursor(185, 30);
  tft.print("    ");
  tft.setCursor(5, 65);
  tft.printf("    ", pet->getTemperature()); 
  tft.setCursor(185, 65);
  tft.printf("    ", pet->getHumidity()); 
}

void printValues(){
  tft.setTextSize(2);
  tft.setCursor(60, 10);
  tft.printf("%d", pet->getHappiness());
  tft.setCursor(60, 30);
  tft.printf("%d", pet->getHunger());
  tft.setCursor(185, 10);
  tft.printf("%d", pet->getFatigue());
  tft.setCursor(185, 30);
  tft.printf("%d", pet->getAge());  
  
  tft.setCursor(5, 65);
  tft.printf("%d\367C", pet->getTemperature()); 
  tft.setCursor(185, 65);
  tft.printf("%d", pet->getHumidity());
  tft.print("%"); 
}

void printConsole(String message){
  tft.setTextSize(2);
  tft.setCursor(15, 275);
  tft.print("                 ");
  tft.setCursor(15, 275);
  tft.print(message);
}

void setup() {
  Serial.begin(9600);
  Serial.println("ESPet V0 - booting up...");
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(0);
  tft.setTextSize(4);
  tft.setCursor(55, 10);
  tft.println("ESPet");
  tft.setCursor(0, 50);
  tft.setTextSize(1);
  tft.println("By Yacine HAMDI and Louis PONT.");
  
  initEEPROM();
  readWifiParameters(ssid, password);
  initWifi();
  login();
  initWebsocket();
  xTaskCreatePinnedToCore(runEspet, "ESPet game", 10000, NULL, 10, &task_pet, 0);
  delay(1000);
}

void loop() {
  if(WiFi.status() != WL_CONNECTED){
    tft.println("Connection lost.");
    delay(5000);
    ESP.restart();
  }
  webSocketClient.loop();
}
