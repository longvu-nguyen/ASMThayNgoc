#define BLYNK_TEMPLATE_ID "TMPL6D3Dlca3R"
#define BLYNK_TEMPLATE_NAME "Mở cửa"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPClient.h>

// Cấu hình chân kết nối
#define SS_PIN 5
#define RST_PIN 22
#define RELAY_PIN 4
#define BUZZER_PIN 26

// Thông tin kết nối
char auth[] = "j7HH7xWYChMNCImomXi6To9dqnI8qbN8";  // Blynk auth token
char ssid[] = "NhinDauBuoi";  // Tên WiFi
char pass[] = "longvu10";  // Mật khẩu WiFi

// Thông tin Telegram
const String telegramToken = "7983060026:AAGHUh5SqQ3K7DYERDR1sjHAOX9S9LEzAP0";
const String chatID = "7833498338";

MFRC522 rfid(SS_PIN, RST_PIN);
bool buzzerEnabled = false;
std::vector<String> validTags = {"CBF2FB03", "A1B2C3D4"};
int invalidAttempts = 0;
unsigned long lockTime = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.begin(ssid, pass);
  Blynk.begin(auth, ssid, pass);

  Serial.println("🚀 Hệ thống sẵn sàng! Hãy quẹt thẻ để mở cửa.");
}

void loop() {
  Blynk.run();

  if (millis() - lockTime < 10000 && lockTime != 0) {
    Serial.println("⏳ Hệ thống đang bị khóa, vui lòng chờ...");
    return;
  }
  lockTime = 0;

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String rfidTag = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      rfidTag += (rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      rfidTag += String(rfid.uid.uidByte[i], HEX);
    }

    rfidTag.toUpperCase();
    Serial.println("🔹 Đã nhận thẻ: " + rfidTag);

    if (isValidTag(rfidTag)) {
      openDoor();
      invalidAttempts = 0;
    } else {
      invalidTag();
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

void openDoor() {
  Serial.println("✅ Cửa đang mở...");
  digitalWrite(RELAY_PIN, HIGH);
  if (buzzerEnabled) digitalWrite(BUZZER_PIN, HIGH);

  sendTelegramMessage("🔓 Cửa đã được mở thành công!");

  delay(3000);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("🔒 Cửa đã tự động khóa lại.");

  sendTelegramMessage("🔒 Cửa đã đóng an toàn.");
}

void invalidTag() {
  Serial.println("❌ Cảnh báo! Thẻ không hợp lệ.");
  invalidAttempts++;
  sendTelegramMessage("🚨 Phát hiện thẻ không hợp lệ! Hãy kiểm tra lại.");

  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);

  if (invalidAttempts >= 3) {
    Serial.println("🚫 Quá nhiều lần nhập sai! Hệ thống bị khóa trong 10 giây.");
    sendTelegramMessage("⏳ Hệ thống bị khóa tạm thời do nhập sai quá nhiều lần!");
    lockTime = millis();
    invalidAttempts = 0;
  }
}

bool isValidTag(String tag) {
  for (String validTag : validTags) {
    if (tag == validTag) return true;
  }
  return false;
}

// Hàm mã hóa URL (message)
String urlEncode(String str) {
  String encoded = "";
  char c;
  char code0, code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encoded += '%';
      encoded += char(code0 > 9 ? code0 - 10 + 'A' : code0 + '0');
      encoded += char(code1 > 9 ? code1 - 10 + 'A' : code1 + '0');
    }
  }
  return encoded;
}

void sendTelegramMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⛔️ Không thể gửi tin nhắn do mất kết nối WiFi!");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();  // Bỏ qua xác thực SSL

  HTTPClient http;
  String encodedMessage = urlEncode(message);
  String url = "https://api.telegram.org/bot" + telegramToken +
               "/sendMessage?chat_id=" + chatID + "&text=" + encodedMessage;

  http.begin(client, url);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.println("📨 Đã gửi thông báo: " + message);
  } else {
    Serial.print("⚠️ Lỗi khi gửi tin nhắn! Mã lỗi: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

// Hàm bật/tắt cảnh báo từ Blynk (V1)
BLYNK_WRITE(V1) {
  buzzerEnabled = param.asInt();
  Serial.print("🔔 Chế độ cảnh báo: ");
  Serial.println(buzzerEnabled ? "KÍCH HOẠT" : "TẮT");
}

// Hàm mở cửa từ Blynk (V0)
BLYNK_WRITE(V0) {
  int doorAction = param.asInt();
  if (doorAction == 1) {
    openDoor();
  }
}
