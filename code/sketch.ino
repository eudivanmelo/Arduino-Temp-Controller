/*
 * Projeto: Arduino Temp Controller
 * Descrição: Controlador de temperatura utilizando Arduino, sensor de temperatura DS18B20, display OLED e módulo relé.
 * Autor: Eudivan de Melo e Silva Junior
 * Email: eudivan44@gmail.com
 * Versão: 1.0
 * Licença: MIT
 */

#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Defina a largura e a altura do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Defina o endereço I2C do display OLED (geralmente é 0x3C)
#define OLED_ADDR 0x3C

// Inicializa o display OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define ONE_WIRE_BUS 11 // Pino de dados do sensor DS18B20

// Inicializa o sensor DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Pino do relé
#define RELAY_PIN 13

// Pinos dos botões
#define SET_BUTTON_PIN 3
#define UP_BUTTON_PIN 2
#define DOWN_BUTTON_PIN 4

float minTemp = -6.0; // Temperatura mínima
float maxTemp = 0.0;  // Temperatura máxima
float currentTemp = -127.0;

int selected = 0;

// Endereços de memória EEPROM para as temperaturas mínima e máxima
#define EEPROM_MIN_TEMP_ADDRESS 0
#define EEPROM_MAX_TEMP_ADDRESS (EEPROM_MIN_TEMP_ADDRESS + sizeof(float))

void setup() {
  Serial.begin(9600);

  // Inicializa o sensor DS18B20
  sensors.begin();
  Serial.println(sensors.getDeviceCount());

  // Configura o pino do relé como saída
  pinMode(RELAY_PIN, OUTPUT);

  // Configura os botões como entradas com pull-up interno
  pinMode(SET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);

  // Inicializa o display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("Falha ao alocar a memória do display OLED"));
    for (;;);
  }

  // Define a rotação do display para de cabeça para baixo
  display.setRotation(2);

  // Limpa o buffer do display
  display.clearDisplay();
  display.display();

  updateDisplay();
}

void loop() {
  // Lê a temperatura atual do sensor DS18B20
  sensors.requestTemperatures();

  if (sensors.getDeviceCount() > 0){
    float temp = 0.0;

    // Calcula a temperatura média se houver mais de um sensor
    for (int i = 0; i < sensors.getDeviceCount(); i++){
      temp += sensors.getTempCByIndex(i);
    }
    
    temp = temp / sensors.getDeviceCount();

    // Atualiza a temperatura atual se houver uma mudança
    if (temp != currentTemp){
      currentTemp = temp;
      // Atualiza o display OLED
      updateDisplay();
      Serial.println(temp);
    }
  }

  // Verifica se os botões foram pressionados para ajustar as temperaturas
  checkButtons();

  // Controla o compressor com base nas temperaturas configuradas
  controlCompressor();
}

// Função que verifica o estado dos botões e ajusta as temperaturas
void checkButtons() {
  // Botão SET: Seleciona qual alteração será feita
  if (digitalRead(SET_BUTTON_PIN) == LOW) {
    selected++;

    // Cicla entre as opções de ajuste
    if (selected > 2){
      selected = 0;
    }

    Serial.print("Menu changed to: ");
    Serial.print(selected);
    Serial.print("\n");

    updateDisplay();

    while(digitalRead(SET_BUTTON_PIN) == LOW)
      delay(200); // Delay de debounce
  }

  // Ajusta as temperaturas mínima e máxima com os botões UP e DOWN
  if (selected != 0){
    // Botão UP: aumenta as temperaturas mínima ou máxima
    if (digitalRead(UP_BUTTON_PIN) == LOW) {
      if (selected == 1){
        minTemp += 0.1;

        Serial.print("Min temperature changed to: ");
        Serial.print(minTemp);
        Serial.print("\n");
      }
      else if (selected == 2){
        maxTemp += 0.1;

        Serial.print("Max temperature changed to: ");
        Serial.print(maxTemp);
        Serial.print("\n");
      }

      updateDisplay();
      delay(200); // Delay de debounce
    }

    // Botão DOWN: diminui as temperaturas mínima ou máxima
    if (digitalRead(DOWN_BUTTON_PIN) == LOW) {
      if (selected == 1){
        minTemp -= 0.1;

        Serial.print("Min temperature changed to: ");
        Serial.print(minTemp);
        Serial.print("\n");
      }
      else if (selected == 2){
        maxTemp -= 0.1;

        Serial.print("Max temperature changed to: ");
        Serial.print(maxTemp);
        Serial.print("\n");
      }

      updateDisplay();
      delay(200); // Delay de debounce
    }
  }
}

// Função que controla o estado do compressor baseado nas temperaturas configuradas
void controlCompressor() {
  // Liga o compressor se a temperatura atual for maior que a máxima
  if (currentTemp >= maxTemp) {
    digitalWrite(RELAY_PIN, HIGH);
  }
  // Desliga o compressor se a temperatura atual for menor que a mínima
  else if (currentTemp <= minTemp) {
    digitalWrite(RELAY_PIN, LOW);
  }
}

// Função que atualiza o display OLED com as informações atuais
void updateDisplay() {
  display.clearDisplay();

  // Coluna 1: Temperatura atual
  display.setTextSize(3);             // Tamanho do texto grande
  display.setTextColor(SSD1306_WHITE); // Cor do texto
  
  if (currentTemp == -127.0){ // Caso a temperatura seja -127, indica que não há sensores 
                              // conectados, logo exibirá uma mensagem de error
                              
    display.setCursor(8, 22); // Define a posição do cursor
    display.print("NaN");
  } else {
    if (currentTemp < 0 || currentTemp >= 10)
      display.setCursor(8, 22); // Define a posição do cursor
    else
      display.setCursor(14, 22); // Define a posição do cursor

    display.print(currentTemp, 1); // Exibe a temperatura atual com uma casa decimal

    display.setTextSize(1); // Tamanho do texto normal
    display.setCursor(70, 16);
    display.print("C");
  }

  display.setTextSize(1); // Tamanho do texto normal

  // Coluna 2: Temperaturas mínima e máxima
  if (selected == 1){
    display.drawRect(86, 4, 40, 24, SSD1306_WHITE);
  }

  display.setCursor(90, 8);
  display.print("Min:");
  display.setCursor(90, 18);
  display.print(minTemp, 1);

  if (selected == 2){
    display.drawRect(86, 36, 40, 24, SSD1306_WHITE);
  }

  display.setCursor(90, 40);
  display.print("Max:");
  display.setCursor(90, 50);
  display.print(maxTemp, 1);

  display.display(); // Atualiza o display com o conteúdo desenhado
}