#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include "LiquidCrystalArabic_I2C.h"

// ---------------- إعدادات الشبكة ----------------
const char* ssid = "TE-WE";
const char* password = "A@hh01155582232";

IPAddress local_IP(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// ---------------- إعدادات الشاشة ----------------
LiquidCrystalArabic lcd(0x27, 16, 2);

// ---------------- متغيرات التشغيل ----------------
ESP8266WebServer server(80);

struct Message {
  String text;
  int duration; 
};

std::vector<Message> playlist;
int currentIndex = 0;
unsigned long previousMillis = 0;

// ---------------- HTML الصفحة (تم تعديل الـ CSS هنا) ----------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ar" dir="rtl">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>محرر الشاشة</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, sans-serif; text-align: center; background-color: #f0f2f5; padding: 20px; }
    .container { max-width: 500px; margin: auto; background: white; padding: 25px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); }
    
    /* التعديل هنا لدعم الكتابة من اليمين */
    textarea { 
      width: 100%; 
      height: 250px; 
      padding: 15px; 
      border: 2px solid #ddd; 
      border-radius: 8px; 
      font-size: 18px; 
      font-family: 'Courier New', monospace; 
      direction: rtl;       /* اتجاه من اليمين لليسار */
      text-align: right;    /* محاذاة النص لليمين */
      box-sizing: border-box;
    }
    
    textarea:focus { border-color: #007bff; outline: none; }
    button { width: 100%; padding: 12px; margin-top: 15px; border-radius: 8px; border: none; cursor: pointer; font-size: 18px; font-weight: bold; background-color: #007bff; color: white; }
    .note { font-size: 0.9em; color: #555; margin-bottom: 15px; text-align: right; background: #e9ecef; padding: 10px; border-radius: 5px; }
    .error { color: #dc3545; font-weight: bold; display: none; margin-top: 10px; }
  </style>
  <script>
    function validateAndSend() {
      var input = document.getElementById("content").value;
      var lines = input.split('\n');
      var errorDiv = document.getElementById("errorMsg");
      for (var i = 0; i < lines.length; i++) {
        var line = lines[i].trim();
        if (line === "") continue;
        var parts = line.split(',');
        if (parts.length < 2) {
          errorDiv.style.display = "block";
          errorDiv.innerText = "خطأ في السطر " + (i+1) + ": تأكد من وجود الفاصلة (،)";
          return false;
        }
        var txt = parts.slice(1).join(',').trim();
        if (txt.length > 16) {
          errorDiv.style.display = "block";
          errorDiv.innerText = "تنبيه السطر " + (i+1) + ": النص ('" + txt + "') أطول من 16 حرف.";
          return false;
        }
      }
      return true;
    }
  </script>
</head>
<body>
  <div class="container">
    <h2>تعديل محتوى الشاشة</h2>
    <div class="note">
      <strong>طريقة الكتابة:</strong><br>
      الثواني , النص العربي<br>
      مثال: 2.5 , بسم الله
    </div>
    <form action="/save" method="POST" onsubmit="return validateAndSend()">
      <textarea id="content" name="data" placeholder="اكتب هنا..">%CURRENT_DATA%</textarea>
      <div id="errorMsg" class="error"></div>
      <button type="submit">حفظ وتحديث الشاشة</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

// ---------------- دوال الملفات ----------------
void saveRawText(String data) {
  File file = LittleFS.open("/raw_data.txt", "w");
  if (file) { file.print(data); file.close(); }
}

String loadRawText() {
  if (LittleFS.exists("/raw_data.txt")) {
    File file = LittleFS.open("/raw_data.txt", "r");
    String s = file.readString();
    file.close();
    return s;
  }
  return "";
}

void parseAndLoadPlaylist(String rawData) {
  playlist.clear();
  int start = 0;
  int end = rawData.indexOf('\n');
  while (end != -1 || start < rawData.length()) {
    String line = (end == -1) ? rawData.substring(start) : rawData.substring(start, end);
    start = (end == -1) ? rawData.length() : end + 1;
    end = (end == -1) ? -1 : rawData.indexOf('\n', start);
    line.trim();
    if (line.length() > 0) {
      int commaIndex = line.indexOf(',');
      if (commaIndex > 0) {
        String timeStr = line.substring(0, commaIndex);
        String textStr = line.substring(commaIndex + 1);
        textStr.trim();
        int durationMs = (int)(timeStr.toFloat() * 1000);
        if (durationMs < 100) durationMs = 1000;
        playlist.push_back({textStr, durationMs});
      }
    }
  }
}

// ---------------- دوال السيرفر ----------------
void handleRoot() {
  String s = index_html;
  s.replace("%CURRENT_DATA%", loadRawText());
  server.send(200, "text/html", s);
}

void handleSave() {
  if (server.hasArg("data")) {
    String data = server.arg("data");
    saveRawText(data);
    parseAndLoadPlaylist(data);
    
    currentIndex = 0;
    if (!playlist.empty()) {
      lcd.clear();
      lcd.setCursorArabic(0, 0);
      lcd.printArabic(playlist[currentIndex].text, false, true);
    }
    previousMillis = millis(); 
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1); 
  lcd.init();
  lcd.backlight();
  lcd.clear();

  if(!LittleFS.begin()) Serial.println("LittleFS Error");

  String savedData = loadRawText();
  if (savedData.length() > 0) parseAndLoadPlaylist(savedData);

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  if (!playlist.empty()) {
    lcd.clear();
    lcd.setCursorArabic(0, 0);
    lcd.printArabic(playlist[0].text, false, true);
    previousMillis = millis();
  }

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

// ---------------- LOOP ----------------
void loop() {
  server.handleClient();

  if (!playlist.empty()) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= (unsigned long)playlist[currentIndex].duration) {
      previousMillis = currentMillis;
      currentIndex++;
      if (currentIndex >= (int)playlist.size()) currentIndex = 0;
      lcd.clear();
      lcd.setCursorArabic(0, 0);
      lcd.printArabic(playlist[currentIndex].text, false, true);
    }
  }
}
