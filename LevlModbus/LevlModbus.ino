#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ModbusMaster.h>

// Configuração do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDRESS 0x3C // Endereço I2C padrão do display

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Configuração Modbus
#define RS485_RXD 16 // Pino RX do ESP32
#define RS485_TXD 17 // Pino TX do ESP32
#define RS485_DE_RE 4 // Pino para habilitar transmissão/recepção no MAX485
ModbusMaster node;

// Configuração do relé (saída digital)
#define RELE_PIN 25 // GPIO para acionar o relé
bool releAtivado = false; // Estado do relé

// Variáveis de controle
const uint8_t radarID = 246;      // ID do dispositivo Modbus
const uint16_t enderecoNivel = 0x089A; // Endereço do nível no radar
float nivelProcesso = 0.0;        // Valor do nível do radar

void preTransmission() {
  digitalWrite(RS485_DE_RE, HIGH); // Habilita transmissão
}

void postTransmission() {
  digitalWrite(RS485_DE_RE, LOW); // Habilita recepção
}

void setup() {
  // Inicialização do Serial para debug
  Serial.begin(115200);
  Serial.println("Inicializando...");

  // Inicialização da interface I2C para o display
  Wire.begin(OLED_SDA, OLED_SCL);

  // Inicialização do display OLED
  if (!display.begin(OLED_ADDRESS)) {
    Serial.println("Falha ao inicializar o display OLED!");
    while (true); // Travar o programa se o display falhar
  }
  display.clearDisplay();

  // Exibir palavra fixa "NÍVEL" no topo do display
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("NIVEL");
  display.display();

  // Configuração do pino do relé
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, LOW); // Relé desligado inicialmente

  // Configuração do pino DE/RE do MAX485
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW); // Começa em recepção

  // Inicialização da comunicação Modbus na Serial2
  Serial2.begin(9600, SERIAL_8N1, RS485_RXD, RS485_TXD);
  node.begin(radarID, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  Serial.println("Setup concluído!");
}

void loop() {
  uint8_t result;
  uint16_t data;

  // Solicitar leitura Modbus
  result = node.readHoldingRegisters(enderecoNivel, 1); // Ler um registrador
  if (result == node.ku8MBSuccess) {
    data = node.getResponseBuffer(0);
    nivelProcesso = data / 1.0; // Ajustar conforme a escala. Os dispositivos podem utilizar uma escala de 0 a 65535

    Serial.print("Nivel lido: ");
    Serial.println(nivelProcesso);

    // Controle do relé
    if (nivelProcesso >= 30.0 && !releAtivado) {
      digitalWrite(RELE_PIN, HIGH); // Liga o relé
      releAtivado = true;
      Serial.println("Relé ativado!");
    } else if (nivelProcesso <= 14 && releAtivado) {
      digitalWrite(RELE_PIN, LOW); // Desliga o relé
      releAtivado = false;
      Serial.println("Relé desativado!");
    }

    // Atualizar display com o nível
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("NIVEL"); // Palavra fixa no topo
    display.setCursor(0, 16);
    display.setTextSize(2);
    display.printf("%.2f %%", nivelProcesso);
    display.display();
  } else {
    Serial.println("Erro na leitura Modbus.");

    // Atualizar display com mensagem de erro
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("NIVEL");
    display.setCursor(0, 16);
    display.setTextSize(1);
    display.println("Erro na leitura!");
    display.display();
  }

  delay(1000); // Intervalo entre leituras
}
