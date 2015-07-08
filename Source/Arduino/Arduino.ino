#include "Arduino.h"
#include "ArduinoJson.h"
#include "HashMap.h"


int LED = 52;
#define SSID "HoangTuan"
#define PASS "tuan21193"

const String SERVER = "lightoflife.azure-mobile.net"; // azure
const String TABLE_NAME = "bongden";
const String APPLICATION_KEY = "sOTRpZFpcmFQpjeKFLrMqGAvvuoTIf85";

char httpResponse[150];

//Setting JSON data
const byte JSON_SIZE = 2;
HashType<char*,char*> hashRawArray[JSON_SIZE];
HashMap<char*,char*> jsonData = HashMap<char*,char*>(hashRawArray,JSON_SIZE);

//Convert JSON to String
String jsonToString(HashMap<char*,char*> jsonData){
    String data = "{";
    for (int i = 0; i < JSON_SIZE; i++) {
        if(i == JSON_SIZE - 1){
            data += "\"" + String(jsonData[i].getHash()) + "\":\"" + String(jsonData[i].getValue()) + "\"}";
        } else {
            data += "\"" + String(jsonData[i].getHash()) + "\":\"" + String(jsonData[i].getValue()) + "\",";
        }
    }
    return data;
}

//Check if SERVER is connected with AT+CIPSTART
boolean serverConnected(){
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += SERVER;
    cmd += "\",80";
    Serial2.println(cmd);
    //delay(3000);
    if(Serial2.find("Error")){
        Serial.println("Connect Server Fail...");
        return false;
    } else {
        Serial.println("Server connected...");
        return true;
    }
}

//Send POST http to Azure server to insert record
void insertData(String JSON_DATA){
    if(serverConnected()){
        String cmd = "POST https://" + SERVER;
        cmd += "/tables/"+ TABLE_NAME;
        cmd += " HTTP/1.1\r\n";
        cmd += "X-ZUMO-APPLICATION: " + APPLICATION_KEY + "\r\n";
        cmd += "Host: " + SERVER + "\r\n";
        cmd += "Content-Type: application/json\r\n";
        cmd += "Content-Length: " + String(JSON_DATA.length()) + "\r\n\r\n";
        cmd += JSON_DATA + "\r\n\r\n";
        
        Serial2.println("AT+CIPSEND=" + String(cmd.length()));
        if(Serial2.find(">")){
            Serial2.print(cmd);
            Serial.print(cmd);
            Serial.println("Insert Command sent...");
            delay(6000);
            if(Serial2.find("201")){
                Serial.println("Respond OK");
            }
        } else {
            Serial2.println("AT+CIPCLOSE");
            Serial.println("CIPSEND error! Connection closed...");
        }
    }
}

//Send GET http to Azure server to get JSON data from server
String getData(String ID){
    if (serverConnected()) {
        String cmd = "GET https://" + SERVER;
        cmd += "/tables/" + TABLE_NAME;
        cmd += "?$filter=(id%20eq%20"+ ID +")";
        cmd += " HTTP/1.1\r\n";
        cmd += "Host: " + SERVER + "\r\n\r\n";
        
        Serial2.println("AT+CIPSEND=" + String(cmd.length()));
        if(Serial2.find(">")){
            Serial2.print(cmd);
            Serial.print(cmd);
        }else{
            Serial.println("CIPSEND error! Connection closed...");
        }
        
        
        bool bodyStarted = false;
        int charIndex = 0;
        bool readDone = false;
        long int time = millis();
        
        while ((time + 50000 > millis())) {
            while (Serial2.available()) {
                char c = Serial2.read();
                Serial.write(c);
                if (c == ']' && bodyStarted){
                    bodyStarted = false;
                    readDone = true;
                }
                if (bodyStarted){
                    delay(30);
                    httpResponse[charIndex] = c;
                    charIndex++;
                    Serial.print(String(charIndex));
                }
                if (c == '['){
                    bodyStarted = true;
                    Serial.println("found [");
                }
            }
            if (readDone) {
                break;
            }
        }
       
        Serial.println("httpResponse:");
        Serial.println(httpResponse);
        StaticJsonBuffer<150> jsonBuffer;
        
        JsonObject& root = jsonBuffer.parseObject(httpResponse);
        
        if (root.success()){
            const char* id = root["id"];
            const char* status = root["status"];
            Serial.println("Get OK: " + String(id) + " : " + String(status));
            return String(status);
        } else {
            Serial.println("Fail to parse JSON object");
            return "fail";
        }
    }
}

//Check if connected to wifi
boolean connectWifi(){
    Serial2.println("AT+CWMODE=1");

    delay(1000);
    Serial2.println("AT+CWJAP?");
    delay(2000);
    if(Serial2.find("No AP")){
        String cmd="AT+CWJAP=\"";
        cmd+=SSID;
        cmd+="\",\"";
        cmd+=PASS;
        cmd+="\"";
        Serial2.println(cmd);
        
        delay(5000);
        if(Serial2.find("OK")){
            Serial.println("Connected to Wifi");
            return true;
        }else{
            Serial.println("Fail to connect Wifi");
            return false;
        }
    } else{
        Serial.println("Already connected to wifi...");
        return true;
    }
}

void setup() {
    
    Serial2.begin(115200);
    Serial.begin(9600);
    pinMode(LED, OUTPUT);
    Serial2.println("AT");
    delay(1000);
    if(Serial2.find("OK")){
        Serial.println("Connecting...");
        boolean wifiConnected = connectWifi();
        while(!wifiConnected){
            Serial.println("Try to connect Wifi again!");
            connectWifi();
        }
    }else{
        Serial.println("Cannot connect to Wifi Module");
        delay(10000);
    }
}

void loop() {
//    int LDR_value1 = analogRead(LDR1);
//    String data = String(LDR_value1);
    
    jsonData[0]("id","1");
    jsonData[1]("status","off");
    insertData(jsonToString(jsonData));
    
    // Get data
    String status = getData("1");
    Serial.println("Status: " + status);
    if(status=="on"){
        Serial.println("true on");
        digitalWrite(LED, HIGH);
    } else {
        digitalWrite(LED, LOW);
    }
        
    delay(5000);
}


