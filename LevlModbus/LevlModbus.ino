#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <ModbusMaster.h>
#include <ModbusRTU.h>

// Configuração do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Configuração Modbus Master (Radar)
#define RS485_RXD 16
#define RS485_TXD 17
#define RS485_DE_RE 4
ModbusMaster master;

// Configuração Modbus Slave
#define RS485_SLAVE_RXD 26
#define RS485_SLAVE_TXD 27
#define RS485_SLAVE_DE_RE 15
ModbusRTU slave;

// Configuração do relé
#define RELE_PIN 25
bool releAtivado = false;

// Variáveis Modbus Master
const uint8_t radarID = 246;
const uint16_t enderecoNivel = 0x089A;
float nivelProcesso = 0.0;

// Registradores Modbus Slave
uint16_t regNivel = 0; // Valor do nível armazenado no registrador do slave

void preTransmissionMaster() {
  digitalWrite(RS485_DE_RE, HIGH); // Habilita transmissão
}

void postTransmissionMaster() {
  digitalWrite(RS485_DE_RE, LOW); // Habilita recepção
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
  digitalWrite(RELE_PIN, LOW);

  // Configuração do Master Modbus
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  Serial2.begin(9600, SERIAL_8N1, RS485_RXD, RS485_TXD);
  master.begin(radarID, Serial2);
  master.preTransmission(preTransmissionMaster);
  master.postTransmission(postTransmissionMaster);

  // Configuração do Slave Modbus
  pinMode(RS485_SLAVE_DE_RE, OUTPUT);
  digitalWrite(RS485_SLAVE_DE_RE, LOW);
  Serial1.begin(9600, SERIAL_8N1, RS485_SLAVE_RXD, RS485_SLAVE_TXD);
  slave.begin(&Serial1, RS485_SLAVE_DE_RE);
  slave.slave(1); // ID do Slave: 1
  slave.addHreg(1, regNivel); // Registrador 1 contém o nível

  Serial.println("Setup concluído!");
}

void loop() {
  uint8_t result;
  uint16_t data;

  // Leitura do Radar (Master)
  result = master.readHoldingRegisters(enderecoNivel, 1);
  if (result == master.ku8MBSuccess) {
    data = master.getResponseBuffer(0);
    nivelProcesso = data / 1.0;
    regNivel = (uint16_t)nivelProcesso; // Atualiza o registrador do Slave

    // Atualiza display
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("NIVEL");
    display.setCursor(0, 16);
    display.setTextSize(2);
    display.printf("%.2f %%", nivelProcesso);
    display.display();

    // Controle do relé
    if (nivelProcesso >= 30.0 && !releAtivado) {
      digitalWrite(RELE_PIN, HIGH);
      releAtivado = true;
      Serial.println("Relé ativado!");
    } else if (nivelProcesso <= 14.0 && releAtivado) {
      digitalWrite(RELE_PIN, LOW);
      releAtivado = false;
      Serial.println("Relé desativado!");
    }
  } else {
    Serial.print("Erro ao ler registrador. Código: ");
    Serial.println(result);
  }

  // Processa o Slave
  slave.task();

  delay(1000); // Intervalo entre requisições
}
