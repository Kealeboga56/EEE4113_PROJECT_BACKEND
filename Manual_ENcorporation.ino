#include <WiFi.h>
#include <ArduinoJson.h>
#include "esp_camera.h"  // ESP32 Camera library
#include <HardwareSerial.h>

// WiFi configuration
const char* ssid = "Kealeboga's S24";
const char* password = "Skhalo The Great";

// Server configuration (your PC's IP and port)
const char* serverIP = "196.42.97.5";
const int serverPort = 12345;



// Status LEDs
#define WIFI_LED 2       // Built-in LED (usually GPIO2 on ESP32)
#define COMM_LED 4       // Communication status LED
#define ERROR_LED 33     // Changed from 5 to avoid conflict with RFID

// RFID configuration
#define RFID_RX 5        // Keep RFID on GPIO 5
#define RFID_TX -1       // Not used
HardwareSerial RFID(2);  // Use UART2

// RFID variables
String text = "";
String CardNumber = "1100773C55";
String lastValidCardID = "";
bool cardProcessed = false;

// Timing
unsigned long lastSendTime = 0;
const long sendInterval = 1000;  // Check every 1 second
int mockCounter = 1;






#include <Arduino.h>
#include <HX711.h>

// --- Pin Definitions for 3 HX711 Scales ---
#define DOUT2_PIN       2   // Data pin for Scale 2 (now primary)
#define DOUT3_PIN       4   // Data pin for Scale 3
#define DOUT4_PIN       16  // Data pin for Scale 4
#define SCK_SHARED_PIN  17  // Shared clock pin

// --- HX711 Scale Objects ---
HX711 scale2;  // Now primary scale
HX711 scale3;
HX711 scale4;

// --- Calibration Factors ---
const float CALIBRATION_FACTOR_SCALE2 = 23.1358;
const float CALIBRATION_FACTOR_SCALE3 = 23.8812;
const float CALIBRATION_FACTOR_SCALE4 = 27.8928;

// --- Kalman Filter Parameters ---
double Q = 0.001;  // Process noise covariance
double R = 0.01;   // Measurement noise covariance
double x_est_combined_kg = 0.0;
double P_est_combined_kg = 1.0;




double getKalmanFilteredWeight(double meas_kg) {
    // Prediction
    double P_pred = P_est_combined_kg + Q;
    // Update
    double K = P_pred / (P_pred + R);
    x_est_combined_kg += K * (meas_kg - x_est_combined_kg);
    P_est_combined_kg = (1 - K) * P_pred;
    return x_est_combined_kg;
}











void calibrateScale(HX711 &scale_obj, const String& name) {
    Serial.printf("\n--- Calibrating %s ---\n", name.c_str());
    Serial.println("Remove all weight for tare...");
    delay(3000);
    scale_obj.tare(20);
    Serial.print("Tared. Raw offset = "); 
    Serial.println(scale_obj.get_value());

    float known_weights[] = {500, 2350.0, 4450.0};
    int n = sizeof(known_weights)/sizeof(known_weights[0]);

    for (int i = 0; i < n; i++) {
        Serial.printf("\nStep %d/%d: Place %.1fg. Press Enter.\n", i+1, n, known_weights[i]);
        while (!Serial.available()) { delay(500); }
        Serial.readStringUntil('\n'); 
        while (Serial.available()) Serial.read();
        delay(2000);
        float units = scale_obj.get_value(50);
        Serial.printf("Units with %.1fg: %.2f\n", known_weights[i], units);
        if (units != 0) {
            float f = units / known_weights[i];
            Serial.printf("Factor for this weight: %.4f\n", f);
        }
        Serial.println("Remove weight.");
        delay(2000);
    }
    Serial.println("\nAverage your 3 factors and update the sketch constants.");
    float last_units = scale_obj.get_value(20);
    if (last_units != 0) {
        scale_obj.set_scale(last_units / known_weights[n-1]);
        Serial.printf("Temp scale set to %.4f\n", last_units/known_weights[n-1]);
    } else {
        scale_obj.set_scale(1.0);
    }
    Serial.printf("--- Calibration complete for %s ---\n", name.c_str());
}





void setup() {
  // Initialize Serial - ONLY ONCE!
  Serial.begin(115200);
  
  // Initialize LEDs
  pinMode(WIFI_LED, OUTPUT);
  pinMode(COMM_LED, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);
  digitalWrite(WIFI_LED, LOW);
  digitalWrite(COMM_LED, LOW);
  digitalWrite(ERROR_LED, LOW);
  
  // Initialize RFID
  RFID.begin(9600, SERIAL_8N1, RFID_RX, RFID_TX);
  Serial.println("RFID Reader initialized on GPIO 5");
  Serial.println("Bring your RFID Card Closer...");
  
  // Initialize WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(WIFI_LED, !digitalRead(WIFI_LED));  // Blink while connecting
  }
  
  digitalWrite(WIFI_LED, HIGH);  // Solid on when connected
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  delay(1000);
















  //weigh set up 
    delay(300);
    Serial.println("\n--- 3-Scale HX711 System (Scales 2,3,4) ---");

    // Initialize scales
    scale2.begin(DOUT2_PIN, SCK_SHARED_PIN);
    scale3.begin(DOUT3_PIN, SCK_SHARED_PIN);
    scale4.begin(DOUT4_PIN, SCK_SHARED_PIN);
    delay(2000);

    // Apply calibration
    scale2.set_scale(CALIBRATION_FACTOR_SCALE2);
    scale3.set_scale(CALIBRATION_FACTOR_SCALE3);
    scale4.set_scale(CALIBRATION_FACTOR_SCALE4);

    Serial.println("Calibration factors set:");
    Serial.printf("  S2=%.4f, S3=%.4f, S4=%.4f\n",
                  CALIBRATION_FACTOR_SCALE2,
                  CALIBRATION_FACTOR_SCALE3,
                  CALIBRATION_FACTOR_SCALE4);

    // Tare all
    scale2.tare(10);
    scale3.tare(10);
    scale4.tare(10);

    // Reset Kalman
    x_est_combined_kg = 0;
    P_est_combined_kg = 1;

    Serial.println("Ready! Commands: calibrate2, calibrate3, calibrate4");
    Serial.println("Output: Raw2 W2(g)  Raw3 W3(g)  Raw4 W4(g)  Filtered(kg)");
}





























void loop() {
  // Continuously read RFID
  readRFID();
  
  unsigned long currentTime = millis();
  
  // Send data at intervals or when new card is detected
  if (currentTime - lastSendTime >= sendInterval) {
    lastSendTime = currentTime;
    
    // Prepare data to send
    String rfidToSend = "NO_CARD";
    String status = "WAITING";
    
    if (lastValidCardID != "") {
      rfidToSend = lastValidCardID;
      status = checkAccess(lastValidCardID) ? "ACCESS_GRANTED" : "ACCESS_DENIED";
      
      // Clear the card ID after processing to avoid repeated sends
      lastValidCardID = "";
    }
    





//===============
    double filt_kg = 0;
    static uint32_t last = 0;
    if (millis() - last >= 100) {
        if (scale2.is_ready() && scale3.is_ready() && scale4.is_ready()) {

            long r2 = scale2.read();
            long r3 = scale3.read();
            long r4 = scale4.read();
            float w2 = scale2.get_units(1);
            float w3 = scale3.get_units(1);
            float w4 = scale4.get_units(1);

            // Combine all three scales
            float comb_g = 0.9 * (abs(w2) + abs(w3) + abs(w4)) / 3.0;
            double meas_kg = comb_g / 1000.0;
            filt_kg = getKalmanFilteredWeight(meas_kg);

            Serial.printf("R2:%ld W2:%.1fg  R3:%ld W3:%.1fg  R4:%ld W4:%.1fg  F:%.2fkg\n",
                          r2, w2, r3, w3, r4, w4, filt_kg);

            last = millis();
        }
    }

    // Handle calibration commands
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd == "calibrate2") {
            calibrateScale(scale2, "Scale 2");
        } else if (cmd == "calibrate3") {
            calibrateScale(scale3, "Scale 3");
        } else if (cmd == "calibrate4") {
            calibrateScale(scale4, "Scale 4");
        }
        // Re-tare after calibration
        scale2.tare(10);
        scale3.tare(10);
        scale4.tare(10);
        x_est_combined_kg = 0;
        P_est_combined_kg = 1;
        while (Serial.available()) Serial.read();
    }
//===============



















    int count = 0;
    while(count < 5){
      delay(1000);
      count++;
    }
    
    //float mockWeight = random(1, 50) / 10.0;  // Random weight between 1.0-50.0 kg
    float mockWeight = filt_kg;
    // Create JSON payload
    DynamicJsonDocument doc(512);
    doc["rfid"] = rfidToSend;
    doc["weight"] = mockWeight;
    doc["imageurl"] = "https://drive.google.com/file/d/1HNX2AHT-P-N_5J7Y3iFb8TWuacCzBD_g/view?usp=drive_link";
    doc["counter"] = mockCounter;
    doc["status"] = status;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    // Only send if we have meaningful data (not just waiting status)
    if (rfidToSend != "NO_CARD") {
      sendToServer(payload);
      mockCounter++;
    }
  }
  
  delay(50);  // Small delay to prevent overwhelming the system

















}

void readRFID() {
  char c;
  
  // Read available RFID data
  while (RFID.available() > 0) {
    delay(5);
    c = RFID.read();
    text += c;
  }
  
  // Process when we have enough data
  if (text.length() > 20) {
    processCard();
    text = "";  //Clear for next reading
  }
}

void processCard() {
  // Extract card ID (same logic as your check function)
  String cardID = text.substring(1, 11);
  
  Serial.println("\n=== CARD DETECTED ===");
  Serial.println("Raw data length: " + String(text.length()));
  Serial.println("Card ID: " + cardID);
  Serial.println("Authorized ID: " + CardNumber);
  
  // Store the card ID for sending to server
  lastValidCardID = cardID;
  
  // Provide immediate feedback
  if (checkAccess(cardID)) {
    Serial.println("✓ Access accepted");
    digitalWrite(COMM_LED, HIGH);
    delay(500);
    digitalWrite(COMM_LED, LOW);
  } else {
    Serial.println("✗ Access denied");
    digitalWrite(ERROR_LED, HIGH);
    delay(1000);
    digitalWrite(ERROR_LED, LOW);
  }
  
  Serial.println("Bring your RFID card closer for next reading...");
}

bool checkAccess(String cardID) {
  return (CardNumber.indexOf(cardID) >= 0);
}

void sendToServer(String payload) {
  WiFiClient client;
  
  Serial.println("\n--- SENDING TO SERVER ---");
  Serial.println("Payload: " + payload);
  
  digitalWrite(COMM_LED, HIGH);  // Turn on comm LED during transmission
  
  if (!client.connect(serverIP, serverPort)) {
    Serial.println("❌ Connection to server failed!");
    digitalWrite(ERROR_LED, HIGH);
    delay(500);
    digitalWrite(ERROR_LED, LOW);
    digitalWrite(COMM_LED, LOW);
    return;
  }
  
  Serial.println("✓ Connected to server");
  
  client.println(payload);
  
  // Wait for response with timeout
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("⏰ Client Timeout!");
      client.stop();
      digitalWrite(ERROR_LED, HIGH);
      delay(500);
      digitalWrite(ERROR_LED, LOW);
      digitalWrite(COMM_LED, LOW);
      return;
    }
  }
  
  // Read response
  Serial.println("Server responses:");
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.println("  → " + line);
  }
  
  client.stop();
  digitalWrite(COMM_LED, LOW);
  Serial.println("✓ Data sent successfully!");
}