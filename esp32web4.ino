#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_PN532.h>

#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (5)
#define PN532_MISO (19)
char auth[] = "beabc42e94ed447db00737a53c108545";

const char* ssid = "SM-G930W89689";
const char* password = "libw5328";
char ssid2[] = "SM-G930W89689";
char pass2[] = "libw5328";
String readRFID;
bool flag = false;
int LED = 4;

WidgetLCD lcd(V5);

uint8_t success;
uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

WiFiServer server(80);
char linebuf[80];
int charcount = 0;
int current_spi = -1; // -1 - not started, 0 - rfid, 1 - else

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  nfc.begin();
  nfc.SAMConfig();
  Blynk.begin(auth, ssid2, pass2);
  connectToWiFi();


}

void connectToWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wi-Fi Connected");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void webServer() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client");
    memset(linebuf, 0, sizeof(linebuf));
    charcount = 0;
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        linebuf[charcount] = c;
        if (charcount < sizeof(linebuf) - 1) charcount++;

        if (c == '\n' && currentLineIsBlank) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text.html");
          client.println("Connection: Closed");
          client.println( );
          client.println("<!DOCTYPE HTML><html><head>");
          client.println("<meta http-equiv=\"refresh\" content=\"3\"></head>");
          client.println(" <meta charset=\"utf-8\"> " );
          client.println(" <meta http-equic=\"X-UA-Compatible\" content=\"IE=edge\"> ");
          client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" ) ;
          client.println("<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.2.1/css/bootstrap.min.css\"");
          client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">" );
          client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js\"></script>");
          client.println("<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js\"></script>");
          client.println("<body style=\"background-color:black;\"> ");
          client.println("<div class=\"jumbotron\">");
          client.println("<center><font size=\"24\"><bold> ESP32 Web Server</font></bold></center>");
          client.println("</div>");
          client.println("<br>");
          client.println("<br>");
          client.println("<br>");
          client.println("<br>");
          client.println("<center><h2 class=\"text-white\">RFID ID</h2>");
          client.println("<h2 class=\"text-white\">");
          client.println(readRFID);
          client.println("</h2></center>");
          client.println("<center>");
          client.println("<br>");
          client.println("<br>");
          client.println("<br>");
          client.println("<br>");
          client.println("<h2 class=\"text-white\">LED</h2>");
          client.println("<a href=\"on1\"><button class=\"btn btn-primary btn-lg\">ON</button></a>&nbsp;<a href=\"off1\"><button class=\"btn btn-primary btn-lg\">OFF</button></a>");
          client.println("</center>");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
          if (strstr(linebuf, "GET /on1") > 0) {
            Serial.println("Turn on LED");
            digitalWrite(LED, HIGH);
          }
          else if (strstr(linebuf, "GET /off1") > 0) {
            Serial.println("Turn off LED");
            digitalWrite(LED, LOW);
          }
          currentLineIsBlank = true;
          memset(linebuf, 0, sizeof(linebuf));
          charcount = 0;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
  }
}

void loop() {
  webServer();
  delay(50);
  RFID_check();
  if (flag == true) {
    RFID_edit();
  }
  delay(50);
  Blynk.run();
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  readRFID = "";
  for (byte i = 0; i < bufferSize; i++) {
    readRFID = readRFID + String(buffer[i], HEX);
  }
}

void RFID_edit() {
  // Display some basic information about the card
  Serial.println("Found an ISO14443A card");
  Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
  Serial.print("  UID Value: ");
  nfc.PrintHex(uid, uidLength);
  dump_byte_array(uid, uidLength);
  lcd.print(0, 0, readRFID);

 

  if (uidLength == 4)
  {
    // We probably have a Mifare Classic card ...
    uint32_t cardid = uid[0];
    cardid <<= 8;
    cardid |= uid[1];
    cardid <<= 8;
    cardid |= uid[2];
    cardid <<= 8;
    cardid |= uid[3];
    Serial.print("Seems to be a Mifare Classic card #");
    Serial.println(cardid);
  }
  Serial.println("");

}

void RFID_check() {
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  flag = true;
}
