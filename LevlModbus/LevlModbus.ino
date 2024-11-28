#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ModbusMaster.h>

// Configuração do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Configuração Modbus
#define RS485_RXD 16
#define RS485_TXD 17
#define RS485_DE_RE 4
ModbusMaster master;

// Configuração do relé
#define RELE_PIN 25
bool releAtivado = false;

// Configuração do registrador
const uint8_t radarID = 246;
const uint16_t enderecoNivel = 1302; // Endereço inicial do Input Register
float nivelProcesso = 0.0;

// Funções de controle do RS485
void preTransmission() {
  digitalWrite(RS485_DE_RE, HIGH);
}

void postTransmission() {
  digitalWrite(RS485_DE_RE, LOW);
}

// Combina dois registradores Modbus em um Float32
float combineRegisters(uint16_t highWord, uint16_t lowWord) {
  uint32_t combined = ((uint32_t)highWord << 16) | lowWord;
  return *(float*)&combined;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando...");

  // Inicialização do display OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(OLED_ADDRESS)) {
    Serial.println("Falha ao inicializar o display OLED!");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("NIVEL");
  display.display();

  // Configuração do relé
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW); // Relé desligado inicialmente

  // Configuração do Modbus Master
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  Serial2.begin(9600, SERIAL_8N1, RS485_RXD, RS485_TXD);
  master.begin(radarID, Serial2);
  master.preTransmission(preTransmission);
  master.postTransmission(postTransmission);

  Serial.println("Setup concluído!");
}

void loop() {
  uint8_t result;
  uint16_t dataHigh, dataLow;

  // Ler dois registradores consecutivos (1302 e 1303)
  Serial.println("Lendo Input Registers 1302 e 1303...");
  result = master.readInputRegisters(enderecoNivel, 2);

  if (result == master.ku8MBSuccess) {
    dataHigh = master.getResponseBuffer(0);
    dataLow = master.getResponseBuffer(1);
    nivelProcesso = combineRegisters(dataHigh, dataLow);

    Serial.print("Nivel lido: ");
    Serial.println(nivelProcesso);

    // Controle do relé
    if (nivelProcesso >= 30.0 && !releAtivado) {
      digitalWrite(RELE_PIN, HIGH); // Liga o relé
      releAtivado = true;
      Serial.println("Relé ativado!");
    } else if (nivelProcesso <= 14.0 && releAtivado) {
      digitalWrite(RELE_PIN, LOW); // Desliga o relé
      releAtivado = false;
      Serial.println("Relé desativado!");
    }

    // Atualizar display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("NIVEL");
    display.setCursor(0, 16);
    display.setTextSize(2);
    display.printf("%.2f %%", nivelProcesso);
    display.display();
  } else {
    // Exibir erro no Serial
    Serial.print("Erro ao ler registradores. Código: ");
    Serial.println(result);
    if (result == 2) {
      Serial.println("Endereço de registrador inválido (ILLEGAL DATA ADDRESS).");
    } else if (result == 226) {
      Serial.println("Timeout na comunicação Modbus.");
    } else if (result == 227) {
      Serial.println("Erro de resposta do escravo.");
    } else {
      Serial.println("Erro desconhecido.");
    }

    // Atualizar display com erro
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("NIVEL");
    display.setCursor(0, 16);
    display.setTextSize(1);
    display.println("Erro ao ler radar!");
    display.display();
  }

  delay(1000);
}

