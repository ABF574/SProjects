#include <mcp2515_can.h>  
#include <SPI.h>          

// broches
#define LED_PIN 6       
#define KNOB_PIN A0    
#define CAN_CS_PIN 9   

// CANopen
#define NODE_ID 0x01                      // ID du nœud CANopen
#define SDO_SERVER_ID (0x600 + NODE_ID)   // ID pour les requêtes SDO serveur
#define SDO_CLIENT_ID (0x580 + NODE_ID)   // ID pour les réponses SDO client
#define TPDO1_ID (0x180 + NODE_ID)        // transmission PDO
#define RPDO1_ID (0x210 + NODE_ID)        // reception PDO
#define NMT_ID 0x000                      // gestion des états des nœuds

// Index de l'objet dans le dictionnaire d'objets
#define OD_KNOB_ANGLE 0x2100     // index pour l'angle du potentiomètre
#define OD_LED_CONTROL 0x2101    // nndex pour le contrôle de la LED

mcp2515_can CAN(CAN_CS_PIN); 

// Variables globales
volatile uint8_t ledState = 0;        // état de la LED (0 = éteinte, 1 = allumée)
volatile bool nmtStateGood = false;   // état du nœud (true si en bon état)

// afficher les données en hexadécimal
void printHexBuffer(unsigned char *buffer, unsigned char length) {
  for (int i = 0; i < length; i++) {
    Serial.print(buffer[i] < 0x10 ? "0x0" : "0x");
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// allumer/éteindre la LED
void setLED(uint8_t state) {
  ledState = state & 0x01; 
  digitalWrite(LED_PIN, ledState); 
  Serial.print("LED State Changed to: ");
  Serial.println(ledState); 
}

void setup() {
  Serial.begin(115200); 
  pinMode(LED_PIN, OUTPUT); 
  setLED(0); 
  
  // Initialisation du CAN
  int canInitAttempts = 0;
  while (CAN.begin(CAN_125KBPS) != CAN_OK && canInitAttempts < 5) {
    Serial.print("CAN Init Attempt ");
    Serial.println(canInitAttempts + 1);
    delay(500);
    canInitAttempts++;
  }

  if (canInitAttempts == 5) {
    Serial.println("CAN Initialization Failed!");
    while(1);
  }

  while(CAN.checkReceive() == CAN_MSGAVAIL) {
    unsigned char temp[8];
    unsigned char len;
    CAN.readMsgBuf(&len, temp);
  }

  Serial.println("CANopen Node Initialized");
}

// Traitement des messages SDO reçus
void processSDO(unsigned long canId, unsigned char len, unsigned char *data) {
  if (canId != SDO_SERVER_ID) return;  // Vérifie si le message est bien destiné à ce nœud
  
  Serial.print("SDO Received: ");
  printHexBuffer(data, len);  // Affiche les données reçues en hexadécimal
  
  uint8_t command = data[0];                // Commande SDO (lecture, écriture)
  uint16_t index = (data[2] << 8) | data[1]; // Index de l'objet 
  uint8_t subIndex = data[3];               // Sous-index 

  unsigned char response[8] = {0x60, data[1], data[2], data[3], 0, 0, 0, 0};  // Réponse par défaut
  bool success = false;

  // Gestion des différents objets SDO
  switch (index) {
    case 0x1000:  // Type de périphérique
      Serial.println("Device Type");
      if (command == 0x40) {  // Requête de lecture
        response[0] = 0x43;  // Réponse de lecture de 4 octets
        response[4] = 0x00;  // Valeur par défaut
        response[5] = 0x00;
        response[6] = 0x00;
        response[7] = 0x00;
        success = true;
      }
      break;

    case OD_LED_CONTROL: // Contrôle de la LED (0x2101)
      if (command == 0x2F || command == 0x2B) {  // Requête d'écriture
        Serial.print("LED Control SDO received: ");
        Serial.println(data[4], HEX);
        setLED(data[4]);  // Met à jour l'état de la LED
        success = true;
      } else if (command == 0x40) {  // Requête de lecture
        response[0] = 0x4F;  // Réponse de lecture d'1 octet
        response[4] = ledState;  // Retourne l'état actuel de la LED
        success = true;
      }
      break;

    default:
      Serial.println("SDO Index Not Supported"); 
      response[0] = 0x80;
      break;
  }
  if (success) {
    CAN.sendMsgBuf(SDO_CLIENT_ID, 0, 8, response);
  } else {
    Serial.println("Error Processing SDO");
  }
}

// Lire l'angle du potentiomètre
uint16_t readKnobAngle() {
  return map(analogRead(KNOB_PIN), 0, 1023, 0, 255);
}

// Envoyer un PDO avec l'angle du potentiomètre
void sendPDO() {
  static unsigned long lastPDOTime = 0;
  static uint16_t lastAngle = 0xFFFF; 
  const unsigned long PDO_INTERVAL = 100; 
  
  if (millis() - lastPDOTime >= PDO_INTERVAL) {
    uint16_t currentAngle = readKnobAngle(); 
    
    if (currentAngle != lastAngle) {
      unsigned char pdo[2];
      pdo[0] = currentAngle & 0xFF;         
      pdo[1] = (currentAngle >> 8) & 0xFF; 

      if (CAN.sendMsgBuf(TPDO1_ID, 0, 2, pdo) == CAN_OK) {  // Envoie le PDO
        Serial.print("Sent PDO Angle: ");
        Serial.println(currentAngle);
        lastAngle = currentAngle;  // Màj de la dernière valeur envoyée
      } else {
        Serial.println("PDO Send Failed"); 
      }
    }
    lastPDOTime = millis();  // Màj du temps de la dernière transmission
  }
}

// Traitement des messages RPDO reçus
void processRPDO(unsigned long canId, unsigned char len, unsigned char *data) {
  if (canId == RPDO1_ID && len >= 1) {
    Serial.print("RPDO Received - Data: 0x");
    Serial.println(data[0], HEX);
    setLED(data[0]);  // Màj de la LED en fonction des données reçues
  }
}

void loop() {
  unsigned char len = 0;
  unsigned char buf[8];
  unsigned long canId;
  
  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&len, buf); 
    canId = CAN.getCanId();    
    
    if (canId == NMT_ID) { 
      if (buf[0] == 0x01 && buf[1] == NODE_ID) {
        nmtStateGood = true;
      }
    } else if (canId == RPDO1_ID) {
      processRPDO(canId, len, buf);
    } else if (canId == SDO_SERVER_ID) { 
      processSDO(canId, len, buf);
    }
  }
  sendPDO();  // Envoie périodique de l'angle du potentiomètre
}