/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */




#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <SPIFFS.h>

#include <WebServer.h>
WebServer server(80);

// Eger baglanti basarili ise EEPROM a yazdir, birde eger baglanamadi ise aglari tekrar listelet ve baglanmasini iste.
 
#include "WiFi.h"
#include <EEPROM.h>

String value;
String networks, line, ssid;

#define EEPROM_SIZE 200
#define EEPROM_SSID 0
#define EEPROM_PASS 100
#define MAX_EEPROM_LEN 50 // Max length to read ssid/passwd

String savedSSID = "";
String savedPASS = "";
String message = "";


char ssidA[100];
char passA[100];

bool opened = false;
File root, file;

void setup(){
    Serial.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    
    
   // clearEEPROM();
    
    SPIFFS.begin();
    
    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    
    Serial.println("File Content:");
    
    File myFile = SPIFFS.open("/config.txt", FILE_READ);
    while (myFile.available()) {
      String ssidString = myFile.readStringUntil('\n');
      savedSSID = ssidString.c_str();
    }
    
    //savedSSID = file2.read(); //readStringEEPROM(EEPROM_SSID);
    myFile.close();
    Serial.print("savedSSID:");
    Serial.println(savedSSID);
    savedSSID.trim();
    if(savedSSID==""){
      networks = scanNetwork();
      Serial.println(networks);
      while(true){
        if(Serial.available() > 0){
          String incomingString = Serial.readString();
          incomingString.trim();
          line = getValue(networks, '\n', (incomingString.toInt()-1));
          ssid = midString(line, ": ", " (");
          if(ssid!=""){
            Serial.print("Input password for network ");
            Serial.print(ssid);
            Serial.println(":");
            while(true){
              if(Serial.available() > 0){
                  String incomingString = Serial.readString();
                  incomingString.trim();
                  Serial.println(incomingString);
                  String conn = wifiConnect((char *) ssid.c_str(), (char *) incomingString.c_str());
                  if(conn=="1"){
                    Serial.println("Connected");
                    //writeStringEEPROM(EEPROM_SSID, ssid+"|"+incomingString);
                    File configfile = SPIFFS.open("/config.txt", FILE_WRITE);
                    if(configfile.print(ssid+"|"+incomingString)) {
                        Serial.println("File was written");
                    }else {
                        Serial.println("File write failed");
                    }
                    configfile.close();
                    Serial.println(WiFi.localIP());
                    delay(1000);
                    ESP.restart(); 
                  }else{
                    Serial.println("WiFi Password is incorrect!");
                  }
              }
            }
          }
        }
        delay(50);
        }
      
    }
    else{
      String ssid = getValue(savedSSID, '|', 0);
      String password = getValue(savedSSID, '|', 1);
      String conn = wifiConnect((char *) ssid.c_str(), (char *) password.c_str());
      if(conn=="1"){
        Serial.println(WiFi.localIP());
        if(!SD.begin(5)){
          Serial.println("Card Mount Failed");
          return;
        }else{
          Serial.println("Card Mounted");
        }
        uint8_t cardType = SD.cardType();
      
        if(cardType == CARD_NONE){
          Serial.println("No SD card attached");
          return;
        }else{
          Serial.println("SD card attached");
        }
        
        server.on("/", handle_OnConnect);
        

        server.on("/sendCommand", sendCommand);
        server.on("/listDir", directoryList);
        server.on("/file_delete", file_delete);
        server.on("/file_read", file_read);

        server.on("/getssid", getSSID);
        server.on("/getrssi", getRSSI);
        server.on("/getlocalip", getlocalIP);
        server.on("/gettotalspace", getTotalSpace);
        server.on("/forgetnetwork", clearSSID);
        
        
        server.on("/update", HTTP_POST, [](){
          server.sendHeader("Connection", "close");
        },[](){
          HTTPUpload& upload = server.upload();
          if(opened == false){
            opened = true;
            root = SD.open((String("/") + upload.filename).c_str(), FILE_WRITE);  
            if(!root){
              Serial.println("- failed to open file for writing");
              return;
            }
          } 
          if(upload.status == UPLOAD_FILE_WRITE){
            if(root.write(upload.buf, upload.currentSize) != upload.currentSize){
              Serial.println("- failed to write");
              return;
            }
          } else if(upload.status == UPLOAD_FILE_END){
            root.close();
            Serial.println("UPLOAD_FILE_END");
            opened = false;
          }
        });

        server.on("/bg.png", []() {
            File f = SPIFFS.open("/img/bg.png", "r");
            server.streamFile(f, "image/png");
            int filesize = f.size();
            String WebString = "";
            WebString += "HTTP/1.1 200 OK\r\n";
            WebString += "Content-Type: image/png\r\n";
            WebString += "Content-Length: " + String(filesize) + "\r\n";
            WebString += "\r\n";
            //server.sendContent(WebString);
            server.send(200, "image/png", WebString);
            
            char buf[1024];
            int siz = f.size();
            while(siz > 0) {
              size_t len = std::min((int)(sizeof(buf) - 1), siz);
              f.read((uint8_t *)buf, len);
              //server.client().write((const char*)buf, len);
              server.sendContent((const char*)buf);
              siz -= len; 
              f.close();
            } 
      });

      server.on("/header.png", []() {
            File f = SPIFFS.open("/img/header.png", "r");
            server.streamFile(f, "image/png");
            int filesize = f.size();
            String WebString = "";
            WebString += "HTTP/1.1 200 OK\r\n";
            WebString += "Content-Type: image/png\r\n";
            WebString += "Content-Length: " + String(filesize) + "\r\n";
            WebString += "\r\n";
            //server.sendContent(WebString);
            server.send(200, "image/png", WebString);
            
            char buf[1024];
            int siz = f.size();
            while(siz > 0) {
              size_t len = std::min((int)(sizeof(buf) - 1), siz);
              f.read((uint8_t *)buf, len);
              //server.client().write((const char*)buf, len);
              server.sendContent((const char*)buf);
              siz -= len; 
              f.close();
            } 
      });

      server.on("/logo.png", []() {
            File f = SPIFFS.open("/img/logo.png", "r");
            server.streamFile(f, "image/png");
            int filesize = f.size();
            String WebString = "";
            WebString += "HTTP/1.1 200 OK\r\n";
            WebString += "Content-Type: image/png\r\n";
            WebString += "Content-Length: " + String(filesize) + "\r\n";
            WebString += "\r\n";
            //server.sendContent(WebString);
            server.send(200, "image/png", WebString);
            
            char buf[1024];
            int siz = f.size();
            while(siz > 0) {
              size_t len = std::min((int)(sizeof(buf) - 1), siz);
              f.read((uint8_t *)buf, len);
              //server.client().write((const char*)buf, len);
              server.sendContent((const char*)buf);
              siz -= len; 
              f.close();
            } 
      });

      server.on("/nozzle.png", []() {
            File f = SPIFFS.open("/img/nozzle.png", "r");
            server.streamFile(f, "image/png");
            int filesize = f.size();
            String WebString = "";
            WebString += "HTTP/1.1 200 OK\r\n";
            WebString += "Content-Type: image/png\r\n";
            WebString += "Content-Length: " + String(filesize) + "\r\n";
            WebString += "\r\n";
            //server.sendContent(WebString);
            server.send(200, "image/png", WebString);
            
            char buf[1024];
            int siz = f.size();
            while(siz > 0) {
              size_t len = std::min((int)(sizeof(buf) - 1), siz);
              f.read((uint8_t *)buf, len);
              //server.client().write((const char*)buf, len);
              server.sendContent((const char*)buf);
              siz -= len; 
              f.close();
            } 
      });
        
      server.on("/cube.png", []() {
            File f = SPIFFS.open("/img/cube.png", "r");
            server.streamFile(f, "image/png");
            int filesize = f.size();
            String WebString = "";
            WebString += "HTTP/1.1 200 OK\r\n";
            WebString += "Content-Type: image/png\r\n";
            WebString += "Content-Length: " + String(filesize) + "\r\n";
            WebString += "\r\n";
            //server.sendContent(WebString);
            server.send(200, "image/png", WebString);
            
            char buf[1024];
            int siz = f.size();
            while(siz > 0) {
              size_t len = std::min((int)(sizeof(buf) - 1), siz);
              f.read((uint8_t *)buf, len);
              //server.client().write((const char*)buf, len);
              server.sendContent((const char*)buf);
              siz -= len; 
              f.close();
            } 
      });

      server.on("/table.png", []() {
            File f = SPIFFS.open("/img/table.png", "r");
            server.streamFile(f, "image/png");
            int filesize = f.size();
            String WebString = "";
            WebString += "HTTP/1.1 200 OK\r\n";
            WebString += "Content-Type: image/png\r\n";
            WebString += "Content-Length: " + String(filesize) + "\r\n";
            WebString += "\r\n";
            //server.sendContent(WebString);
            server.send(200, "image/png", WebString);
            
            char buf[1024];
            int siz = f.size();
            while(siz > 0) {
              size_t len = std::min((int)(sizeof(buf) - 1), siz);
              f.read((uint8_t *)buf, len);
              //server.client().write((const char*)buf, len);
              server.sendContent((const char*)buf);
              siz -= len; 
              f.close();
            } 
      });

      server.on("/home.png", []() {
            File f = SPIFFS.open("/img/home.png", "r");
            server.streamFile(f, "image/png");
            int filesize = f.size();
            String WebString = "";
            WebString += "HTTP/1.1 200 OK\r\n";
            WebString += "Content-Type: image/png\r\n";
            WebString += "Content-Length: " + String(filesize) + "\r\n";
            WebString += "\r\n";
            //server.sendContent(WebString);
            server.send(200, "image/png", WebString);
            
            char buf[1024];
            int siz = f.size();
            while(siz > 0) {
              size_t len = std::min((int)(sizeof(buf) - 1), siz);
              f.read((uint8_t *)buf, len);
              //server.client().write((const char*)buf, len);
              server.sendContent((const char*)buf);
              siz -= len; 
              f.close();
            } 
      });

      

      /*server.on("/bootstrap-input-spinner.js", []() {
            File f = SPIFFS.open("/js/bootstrap-input-spinner.js", "r");
            server.streamFile(f, "text/javascript");
            int filesize = f.size();
            String WebString = "";
            WebString += "HTTP/1.1 200 OK\r\n";
            WebString += "Content-Type: text/javascript\r\n";
            WebString += "Content-Length: " + String(filesize) + "\r\n";
            WebString += "\r\n";
            //server.sendContent(WebString);
            server.send(200, "text/javascript", WebString);
            
            char buf[1024];
            int siz = f.size();
            while(siz > 0) {
              size_t len = std::min((int)(sizeof(buf) - 1), siz);
              f.read((uint8_t *)buf, len);
              //server.client().write((const char*)buf, len);
              server.sendContent((const char*)buf);
              siz -= len; 
              f.close();
            } 
      });*/
      
      server.on("/jquery.min.js", []() {
            File f = SPIFFS.open("/js/jquery.min.js", "r");
            server.streamFile(f, "text/javascript");
            int filesize = f.size();
            String WebString = "";
            WebString += "HTTP/1.1 200 OK\r\n";
            WebString += "Content-Type: text/javascript\r\n";
            WebString += "Content-Length: " + String(filesize) + "\r\n";
            WebString += "\r\n";
            //server.sendContent(WebString);
            server.send(200, "text/javascript", WebString);
            
            char buf[1024];
            int siz = f.size();
            while(siz > 0) {
              size_t len = std::min((int)(sizeof(buf) - 1), siz);
              f.read((uint8_t *)buf, len);
              //server.client().write((const char*)buf, len);
              server.sendContent((const char*)buf);
              siz -= len; 
              f.close();
            } 
      });
        
        
        
        server.begin();
      }
    }
    
}

void sendCommand() 
{
 String cmd = server.arg("cmd");
 Serial.println(cmd);
 server.send(200, "text/plain", "1");
}

void file_delete() 
{
 
 String act_state = server.arg("fileName");
 Serial.println(act_state);
 deleteFile(SD, server.arg("fileName").c_str());
 server.send(200, "text/plain", "1");
}

void file_read() 
{
   String act_state = server.arg("fileName");
   Serial.println(act_state);
   String result = readFile(SD, server.arg("fileName").c_str());
   server.send(200, "text/plain", result);
}

void getSSID(){
  server.send(200, "text/plain", WiFi.SSID());
}

void getRSSI(){
  server.send(200, "text/plain", String(WiFi.RSSI()));
}

void getlocalIP(){
  server.send(200, "text/plain", ip2Str(WiFi.localIP()));
}

void getTotalSpace(){
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  server.send(200, "text/plain", genString(cardSize));
}


void handle_OnConnect() {
  file = SPIFFS.open("/index.html");
  message = "";
  while (file.available()){
    message += (char)file.read();
  }
  
  file.close();
  server.send(200, "text/html", message); //Send web page
  //server.send(200, "text/html", readFile(SPIFFS, "/index.html")); 
  //server.send(200, "text/html", SendHTML(listDir(SD, "/", 0))); 
}

void directoryList() {
  server.send(200, "text/html", listDir(SD, "/", 0));   
}



void clearSSID(){
    File configfile = SPIFFS.open("/config.txt", FILE_WRITE);
    if(configfile.print("")){
        Serial.println("File was written");
    }else {
        Serial.println("File write failed");
    }
    configfile.close();
    server.send(200, "text/html", "1");
    delay(5000);
    ESP.restart(); 
}

void clearEEPROM(){
    for (int i = 0; i < 512; i++) {
     EEPROM.write(i, 0);
     }
    EEPROM.commit();
    server.send(200, "text/html", "1");
    delay(10000);
    ESP.restart(); 
}

void writeStringEEPROM(char add, String data)
{
    int _size = data.length();
    if (_size > MAX_EEPROM_LEN)
        return;
    int i;
    for (i = 0; i < _size; i++)
    {
        EEPROM.write(add + i, data[i]);
    }
    EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
    EEPROM.commit();
}

String readStringEEPROM(char add)
{
    int i;
    char data[MAX_EEPROM_LEN + 1];
    int len = 0;
    unsigned char k;
    k = EEPROM.read(add);
    while (k != '\0' && len < MAX_EEPROM_LEN) //Read until null character
    {
        k = EEPROM.read(add + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}



void loop()
{
    if (Serial.available() > 0) {
      // read the incoming string:
      String incomingString = Serial.readString();
      Serial.print(incomingString);
      incomingString.trim();
      if(incomingString=="N" || incomingString=="n"){
        Serial.println(scanNetwork());
        while(true){
           if(incomingString=="M")
           delay(500);
         }
      }
      
    }
  server.handleClient();
  
  //delay(100);
    
}

/*String menu(){
  String menu = "";
  menu += "1. Show Wifi networks\n";
  menu += "2. Show IP address\n";
  menu += "3. WebServer address\n";
  menu += "4. Disconnect from connected network\n";
  return menu;
}*/

String wifiConnect(char* ssid, char* password){
    String conn = "";
    int trial = 0;
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
      
      if(trial==20){
        break;
      }else{
        trial = trial + 1;
      }
    }

    if(WiFi.status() == WL_CONNECTED){
      conn = "1"; 
    }else{
      conn = "0";
    }
    
    return conn;
}



String scanNetwork(){
  Serial.println("Setup done");
    Serial.println("scan start");
    String returnvalue = "";
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            returnvalue += String(i + 1);
            returnvalue += ": ";
            returnvalue += WiFi.SSID(i);
            returnvalue += " (";
            returnvalue += WiFi.RSSI(i);
            returnvalue += ")";
            returnvalue += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*\n";
            delay(10);
        }
    }
    returnvalue += "Input SSID number to choose:";
    return returnvalue;
}

int countArray(String data, char separator){
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found;
}

String getValue(String data, char separator, int index){
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String midString(String str, String start, String finish){
  int locStart = str.indexOf(start);
  if (locStart==-1) return "";
  locStart += start.length();
  int locFinish = str.indexOf(finish, locStart);
  if (locFinish==-1) return "";
  return str.substring(locStart, locFinish);
}

/******************** FILE SYSTEM ********************************/

String listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  String files = "";
  //Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return "";
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return "";
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      //Serial.print("  DIR : ");
      //Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      //Serial.print("  FILE: ");
      //Serial.print(file.name());
      //Serial.print("  SIZE: ");
      //Serial.println(file.size());
      files += String(file.name())+"|"+String(file.size())+"\n";
    }
    file = root.openNextFile();
  }
  return files;
}

void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path){
  Serial.printf("Removing Dir: %s\n", path);
  if(fs.rmdir(path)){
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

String readFile(fs::FS &fs, const char * path){
  String str = "";
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    //return;
  }

  //Serial.print("Read from file: ");

      
  while(file.available()){
    str += (char)file.read();
    //Serial.write(file.read());
  }
  //Serial.print(str);
  //return String(file.read());
  return str;
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char * path){
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if(file){
    len = file.size();
    size_t flen = len;
    start = millis();
    while(len){
      size_t toRead = len;
      if(toRead > 512){
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for(i=0; i<2048; i++){
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

/******************** WEB SERVER ********************************/

String genString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c +='0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}

String ip2Str(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}
