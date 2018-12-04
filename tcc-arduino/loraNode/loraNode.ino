// CENTRO FEDERAL DE EDUCAÇÃO TECNOLÓGICA DE MINAS GERAIS
// TRABALHO DE CONCLUSÃO DE CURSO
// GUSTAVO JORDÃO NUNES
// gjordaonunes@gmail.com
// Este código corresponde ao nó de um sistema de detecção de incêndios florestais.

// Diagrama de Conexões da Placa
// https://user-images.githubusercontent.com/40477400/42231041-270317d0-7ef3-11e8-9a44-6cc9bf57fdb8.jpg

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 30 /* Time ESP32 will go to sleep (in seconds) */
#define MQ9_HEATING_TIME 180

#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <SSD1306.h>
#include <OLEDDisplayUi.h>
#include <SimpleDHT.h>

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

// Sensors Constants -----------------------------------

// MQ-9 Pins
const int MQ9_A0_PIN = 13;
const int MQ9_D0_PIN = 37;

// DHT22 Pin
const int DHT22_DATA_PIN = 2;//38;
const int DHT22_NC_PIN = 39;

// 5V Pin
const int _5V_PIN = 12;

// ---------------------------------------------

SSD1306 display(_OLED_ADDRESS, _OLED_SDA, _OLED_SCL);
OLEDDisplayUi ui(&display);
SimpleDHT22 dht22(DHT22_DATA_PIN);

float temperatura = 0.0f;
float umidade = 0.0f;
float gases = 0.0f;

RTC_DATA_ATTR bool MQ9aquecido = false;

void setup() {
  if(!MQ9aquecido){
      initAquecimento(MQ9_HEATING_TIME);
      MQ9aquecido = true;
  }
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
  //Serial.begin(115200);                   // initialize serial
  
  SPI.begin(_LORA_SCK, _LORA_MISO, _LORA_MOSI, _LORA_CS);
//  SPI.begin(5, 19, 27, 18);
  LoRa.setPins(_LORA_CS, _LORA_RST, _LORA_IRQ);
//  LoRa.setPins(18, 14, 26);

  if (!LoRa.begin(_LORA_FREQUENCY)) {
    //Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  /*Serial.println("LoRa init succeeded.");
  Serial.println();
  Serial.println("LoRa Simple Node");
  Serial.println("Only receive messages from gateways");
  Serial.println("Tx: invertIQ disable");
  Serial.println("Rx: invertIQ enable");
  Serial.println();*/

  LoRa.onReceive(onReceive);
  LoRa_rxMode();

  pinMode(_5V_PIN, OUTPUT);
  //ledcAttachPin(_5V_PIN, 0);
  //ledcSetup(0, 1000, 10);
  //ledcWrite(0, 1023);
  digitalWrite(_5V_PIN, HIGH);
  
  //-------------------------------------------------------
  
  pinMode(MQ9_A0_PIN, INPUT);
   
  //-------------------------------------------------------
  
  // initialize and clear display
  pinMode(_OLED_RST, OUTPUT);
  digitalWrite(_OLED_RST, LOW);
  delay(50);
  digitalWrite(_OLED_RST, HIGH);
  
  display.init();
  display.clear();
  display.drawString(0, 0, "LoRa Node");
  display.display();

  tarefas();

  //ledcWrite(0, 0);
  digitalWrite(_5V_PIN, LOW);

  esp_deep_sleep_start(); 
}

int packet = 0;

String accessToken = "af71bd7f-ac19-43d5-bc8f-865cc12666b5";

void loop() {}

void tarefas(){
  //Serial.print("\n");
  //Serial.print(": Retrieving information from sensor: ");
  //Serial.print("Read sensor: ");

  // Internamente possui delay de 2.5s para efetuar leitura adequada dos sensores
  efetuarLeituras();

  // Formato da Mensagem: AccessToken|Temperatura|Umidade|Gases
  String message = accessToken + "|" + String(temperatura) + "|" + String(umidade) + "|" + String(gases);

  // Envia mensagem ao Gateway
  LoRa_sendMessage(message);
  
  //Serial.println("Send Message!");
  atualizaDisplay();

  delay(5000);
}

void LoRa_rxMode(){
  //LoRa.enableInvertIQ();                // active invert I and Q signals
  LoRa.receive();                       // set receive mode
}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  //LoRa.disableInvertIQ();               // normal mode
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket();                     // finish packet and send it
  LoRa_rxMode();                        // set rx mode
}

void onReceive(int packetSize) { }

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

void atualizaDisplay(){
  display.clear();
  display.drawString(0, 0, "LoRa Node");
  
  String strTemp = "Temperatura: "+String(temperatura)+"ºC";
  String strUmid = "Umidade: "+String(umidade)+"%";
  String strGas = "Gases: "+String(gases)+"ppm";
  
  display.setColor(WHITE);
  display.drawString(0, 20, strTemp);
  display.drawString(0, 30, strUmid);
  display.drawString(0, 40, strGas);
  display.display();

  /*Serial.println(strTemp);
  Serial.println(strUmid);
  Serial.println(strGas);
  Serial.println();*/
  
}

void initAquecimento(int tempoEmSegundos){
  // initialize and clear display
  pinMode(_OLED_RST, OUTPUT);
  digitalWrite(_OLED_RST, LOW);
  delay(50);
  digitalWrite(_OLED_RST, HIGH);

  pinMode(_5V_PIN, OUTPUT);
  digitalWrite(_5V_PIN, HIGH);
  
  display.init();

  while(tempoEmSegundos >= 0){
      display.clear();
      display.drawString(0, 0, "LoRa Node");
      display.drawString(0, 20, "Aquecimento do MQ-9");
      display.drawString(0, 30, "Aguarde "+String(tempoEmSegundos)+" segundos...");
      display.display();  
    
      delay(1000);
      tempoEmSegundos--;
  }

}

bool efetuarLeituras(){
  gases = 0;
  for(int x = 0 ; x < 100 ; x++)
  {
    gases = gases + analogRead(MQ9_A0_PIN);
  }
  gases = gases/100.0;

  //– Faixa de detecção CO: 10ppm à 1000ppm
  //– Faixa de detecção gás combustível: 100ppm à 10000ppm
  gases = gases/1023*(10000-100)+100;
  
  int err = dht22.read2(&temperatura, &umidade, NULL);

  delay(2500);
  
  return err == SimpleDHTErrSuccess;
}

