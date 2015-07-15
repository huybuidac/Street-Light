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
const byte JSON_SIZE = 1;
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

//Read String from Serial 2
String readStringFromSerial2() {
    while (!Serial2.available()); //wait for user input
    //there is something in the buffer now
    String str = "";
    while (Serial2.available()) {
        str += (char) Serial2.read();
        delay(1); //wait for the next byte, if after this nothing has arrived it means the text was not part of the same stream entered by the user
    }
    return str;
}

//Check if SERVER is connected with AT+CIPSTART
boolean serverConnected(){
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += SERVER;
    cmd += "\",80";
    Serial2.println(cmd);
    Serial2.find("AT+CIPSTART=\"TCP\",\"lightoflife.azure-mobile.net\",80");
    delay(500);
    String s = readStringFromSerial2();
    Serial.println("[Start]"+s+"[End]");
    Serial.println("- ready index:" + String(s.indexOf("ALREADY CONNECTED\r\n\r\nERROR")));
    Serial.println("- connect index:" + String(s.indexOf("CONNECT\r\n\r\nOK")));
    if (s.indexOf("ALREADY CONNECTED\r\n\r\nERROR")>=0) {
        Serial.println("Server already connected");
        s="";
        return true;
    } else if(s.indexOf("CONNECT\r\n\r\nOK")>=0){
        Serial.println("Server connected");
        s="";
        return true;
    } else {
        Serial.println("Cannot connect to server");
        s="";
        return false;
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
        while (!Serial2.available());
        if(Serial2.find("OK\r\n>")){
            Serial2.print(cmd);
            while (!Serial2.available());
            Serial.println("Insert Command sent");
            if(Serial2.find("201")){
                Serial.println("Respond OK");
            }
            //clear response in serial
            while (Serial2.available()) {
                Serial2.read();
            }
        } else {
            Serial.println("CIPSEND error");
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
        if(Serial2.find("OK\r\n>")){
            Serial2.print(cmd);
            while (!Serial2.available());
            //clear response in serial
            while (Serial2.available()) {
                Serial2.read();
            }
        }else{
            Serial.println("CIPSEND error");
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

//Update data to Azure
void updateData(String itemID, String JSON_DATA){
    if(serverConnected()){
        String cmd = "PATCH https://" + SERVER;
        cmd += "/tables/"+ TABLE_NAME;
        cmd += "/" + itemID;
        cmd += " HTTP/1.1\r\n";
        cmd += "X-ZUMO-APPLICATION: " + APPLICATION_KEY + "\r\n";
        cmd += "Host: " + SERVER + "\r\n";
        cmd += "Content-Type: application/json\r\n";
        cmd += "Content-Length: " + String(JSON_DATA.length()) + "\r\n\r\n";
        cmd += JSON_DATA + "\r\n\r\n";
        
        Serial2.println("AT+CIPSEND=" + String(cmd.length()));
        while (!Serial2.available());
        if(Serial2.find("OK\r\n>")){
            Serial2.print(cmd);
            while (!Serial2.available());
            Serial.println("Insert Command sent");
            if(Serial2.find("200 OK")){
                Serial.println("Respond OK---------");
            }
            //clear response in serial
            while (Serial2.available()) {
                Serial2.read();
            }
        } else {
            Serial.println("CIPSEND error");
        }
    }
}

//Check if connected to wifi
void connectWifi(){
    boolean connected = false;
    Serial2.println("AT+CWMODE=1");
    delay(500);
    while (!connected) {
        Serial2.println("AT+CWJAP?");
        if(Serial2.find("No AP\r\n\r\nOK")){
            String cmd="AT+CWJAP=\"";
            cmd+=SSID;
            cmd+="\",\"";
            cmd+=PASS;
            cmd+="\"";
            Serial2.println(cmd);
            delay(5000);
            if(Serial2.find("WIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK")){
                Serial.println("Connected to Wifi");
                connected = true;
            } else{
                Serial.println("Fail to connect wifi, trying again...");
                while (!Serial2.available());
            }
        } else{
            Serial.println("Already connected to wifi...");
            connected = true;
        }
    }
}



void setup() {
    
    Serial2.begin(115200);
    Serial.begin(9600);
    pinMode(LED, OUTPUT);
    
    boolean wifiReady = false;
    while (!wifiReady) {
        Serial2.println("AT");
        while (!Serial2.available());
        if(Serial2.find("OK")){
            wifiReady=true;
            connectWifi();
        }else{
            Serial.println("Cannot connect to Wifi Module, trying again...");
            delay(3000);
        }
    }
}

void loop() {
//    int LDR_value1 = analogRead(LDR1);
//    String data = String(LDR_value1);
    // Check INSERT
//    jsonData[0]("id","1");
//    jsonData[1]("status","off");
//    insertData(jsonToString(jsonData));
    
    // Check UPDATE
    jsonData[0]("status","update");
    updateData("1", jsonToString(jsonData));
    
//    // Check GET
//    String status = getData("1");
//    Serial.println("Status: " + status);
//    if(status=="on"){
//        Serial.println("true on");
//        digitalWrite(LED, HIGH);
//    } else {
//        digitalWrite(LED, LOW);
//    }
    
    delay(5000);
}


