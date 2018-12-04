// CENTRO FEDERAL DE EDUCAÇÃO TECNOLÓGICA DE MINAS GERAIS
// TRABALHO DE CONCLUSÃO DE CURSO
// GUSTAVO JORDÃO NUNES
// gjordaonunes@gmail.com
// Este código corresponde ao gateway de um sistema de detecção de incêndios florestais.

// Diagrama de Conexões da Placa
// https://user-images.githubusercontent.com/40477400/42231041-270317d0-7ef3-11e8-9a44-6cc9bf57fdb8.jpg

#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <QList.h>

// SPI Constants -----------------------------------

// Mater Out Slave In
const int _LORA_MOSI = 27;
// Master In Slave Out
const int _LORA_MISO = 19; // No mapeamento aparece erroneamente como LORA_MOSI
// Serial Clock
const int _LORA_SCK = 5;

// LoRa Constants -----------------------------------

// Frequency
const long _LORA_FREQUENCY = 433E6;  // LoRa Frequency
// Chip Select
const int _LORA_CS = 18;//10;          // LoRa radio chip select
// Radio Reset
const int _LORA_RST = 14;//9;        // LoRa radio reset
// Interrupt
const int _LORA_IRQ = 26;//2;          // change for your board; must be a hardware interrupt pin

// OLED Constants -----------------------------------

// Address
const int _OLED_ADDRESS = 0x3c;
// Serial Data
const int _OLED_SDA = 4;
// Serial Clock
const int _OLED_SCL = 15;
// Reset
const int _OLED_RST = 16;

const long frequency = 433E6;  // LoRa Frequency

const int csPin = 18;//10;          // LoRa radio chip select
const int resetPin = 14;//9;        // LoRa radio reset
const int irqPin = 26;//2;          // change for your board; must be a hardware interrupt pin

const char* ssid     = "SSID";
const char* password = "SENHA";

SSD1306 display(_OLED_ADDRESS, _OLED_SDA, _OLED_SCL);
OLEDDisplayUi ui(&display);

String messageOut = "";

QList<String> messages;

void setup() {
  Serial.begin(115200);                   // initialize serial
  //while (!Serial);

  // LoRa Connection

  SPI.begin(_LORA_SCK, _LORA_MISO, _LORA_MOSI, _LORA_CS);
  //SPI.begin(5, 19, 27, 18);
  LoRa.setPins(_LORA_CS, _LORA_RST, _LORA_IRQ);
  //LoRa.setPins(18, 14, 26);
  //LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
  Serial.println();
  Serial.println("LoRa Simple Gateway");
  Serial.println("Only receive messages from nodes");
  Serial.println("Tx: invertIQ enable");
  Serial.println("Rx: invertIQ disable");
  Serial.println();

  LoRa.onReceive(onReceive);
  LoRa_rxMode();

  //---------------------------------------------------

  connectToServer(30000);
  
  //---------------------------------------------------

  // initialize and clear display
  pinMode(_OLED_RST, OUTPUT);
  digitalWrite(_OLED_RST, LOW);
  delay(50);
  digitalWrite(_OLED_RST, HIGH);

  display.init();
  display.clear();
  display.drawString(0, 0, "LoRa Gateway");
  display.display();
}

int last = 0;

void loop() {
  
  while(messages.size() != 0 && !runEvery(1000)){

      delay(100);
      bool sent = sendToServer(messages[0]);
    
      if(sent)
        messages.pop_front();

  }
  
  atualizaDisplay(messageOut);

  delay(100);
  
}

void LoRa_rxMode(){
  //LoRa.disableInvertIQ();               // normal mode
  LoRa.receive();                       // set receive mode
}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  //LoRa.enableInvertIQ();                // active invert I and Q signals
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket();                     // finish packet and send it
  LoRa_rxMode();                        // set rx mode
}

void onReceive(int packetSize) {

  String message = "";
 
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }

  Serial.print("Gateway Receive: ");
  Serial.println(message);

  // Adiciona mensagem recebida na fila
  messages.push_back(message);
  messageOut = message;

  last = millis();
}

boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

void atualizaDisplay(String message){
  display.clear();
  display.drawString(0, 0, "LoRa Gateway");
  
  display.setColor(WHITE);
  
  display.drawString(0, 10, "Tam. fila: "+String(messages.size()));
  display.drawString(0, 20, "Última mensagem recebida: ");
  display.drawString(0, 30, message);
  display.drawString(0, 40, "Last Millis: "+String(last/1000));
  display.drawString(0, 50, "Cur. Millis: "+String(millis()/1000));
  
  display.display();
  
}

bool sendToServer(String message){

    int separadores[3] = { -1, -1, -1 };
    int numSeparadores = 0;

    for(int i=0; i<message.length(); i++){
        if(numSeparadores > 3)
            break;

        if(message.charAt(i) == '|'){
            separadores[numSeparadores] = i;
            numSeparadores++;
        }
    }

    Serial.println(String(separadores[0]) + " " + String(separadores[1]) + " " + String(separadores[2]));

    if(numSeparadores != 3){
        // Mensagem com formato errado (será removida da fila)
        return true;
    }
    
    String accessToken = message.substring(0, separadores[0]);
    String temperatura = message.substring(separadores[0]+1, separadores[1]);
    String umidade = message.substring(separadores[1]+1, separadores[2]);
    String gases = message.substring(separadores[2]+1);

    Serial.println(accessToken);
    Serial.println(temperatura);
    Serial.println(umidade);
    Serial.println(gases);
    
    
    String urlPost = "https://cefetincendios.herokuapp.com/leituras/novo?temperatura="+temperatura+"&umidade="+umidade+"&gases="+gases;

    HTTPClient http;
 
    http.begin(urlPost); //Specify the URL and certificate
    http.addHeader("Content-Type", "application/json");
    http.addHeader("access-token", accessToken);
    http.addHeader("Connection", "close");
    int httpCode = http.POST("");                                                  //Make the request
    http.writeToStream(&Serial);
        
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources

    return httpCode > 0;
    
}


bool connectToServer(unsigned long timeout){
  
  // WiFi Connection

  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED && !timeoutWiFi(timeout)) {
    Serial.print(".");
    delay(100);
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  else{
    return false;
  }
}

boolean timeoutWiFi(unsigned long interval)
{
  static unsigned long previousMillisWiFi = 0;
  unsigned long currentMillisWiFi = millis();
  if (currentMillisWiFi - previousMillisWiFi >= interval)
  {
    previousMillisWiFi = currentMillisWiFi;
    return true;
  }
  return false;
}

