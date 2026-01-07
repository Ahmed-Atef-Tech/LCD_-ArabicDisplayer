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
  int duration; // بالمللي ثانية
};

std::vector<Message> playlist;
int currentIndex = 0;
unsigned long previousMillis = 0;

// ---------------- HTML الصفحة ----------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ar" dir="rtl">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>محرر الشاشة</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; background-color: #f0f2f5; padding: 20px; }
    .container { max-width: 500px; margin: auto; background: white; padding: 25px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); }
    textarea { 
      width: 100%; 
      height: 200px; 
      padding: 10px; 
      border: 1px solid #ccc; 
      border-radius: 8px; 
      font-size: 16px; 
      font-family: monospace;
      resize: vertical;
      direction: ltr; /* لسهولة كتابة الأرقام والفواصل */
      text-align: left;
    }
    button { width: 100%; padding: 12px; margin-top: 15px; border-radius: 8px; border: none; cursor: pointer; font-size: 18px; font-weight: bold; transition: 0.3s; }
    .btn-save { background-color: #007bff; color: white; }
    .btn-save:hover { background-color: #0056b3; }
    .note { font-size: 0.85em; color: #666; margin-bottom: 10px; text-align: right; }
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
          errorDiv.innerText = "خطأ في السطر " + (i+1) + ": يجب وضع فاصلة بين الوقت والنص";
          return false;
        }
        
        var txt = parts.slice(1).join(',').trim(); // تجميع النص لو فيه فواصل زيادة
        if (txt.length > 16) {
          errorDiv.style.display = "block";
          errorDiv.innerText = "تنبيه في السطر " + (i+1) + ": النص '" + txt + "' أطول من 16 حرف.";
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
      الوقت بالثانية , النص<br>
      مثال:<br>
      <code style="direction:ltr; display:block; background:#eee; padding:5px;">1.5, Hello World<br>3, مرحبا بكم</code>
    </div>
    
    <form action="/save" method="POST" onsubmit="return validateAndSend()">
      <textarea id="content" name="data" placeholder="1, نص الرسالة الأولى&#10;3.5, نص الرسالة الثانية">%CURRENT_DATA%</textarea>
      <div id="errorMsg" class="error"></div>
      <button type="submit" class="btn-save">حفظ وتحديث الشاشة</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

// ---------------- دوال الملفات ----------------
// حفظ النص الخام كما كتبه المستخدم بالضبط
void saveRawText(String data) {
  File file = LittleFS.open("/raw_data.txt", "w");
  if (file) {
    file.print(data);
    file.close();
  }
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

// دالة تفكيك النص وتحويله لقائمة التشغيل
void parseAndLoadPlaylist(String rawData) {
  playlist.clear();
  int start = 0;
  int end = rawData.indexOf('\n');
  
  // حلقة تكرارية لكل سطر
  while (end != -1 || start < rawData.length()) {
    String line;
    if (end == -1) {
      line = rawData.substring(start);
      start = rawData.length(); 
    } else {
      line = rawData.substring(start, end);
      start = end + 1;
      end = rawData.indexOf('\n', start);
    }
    
    line.trim();
    if (line.length() > 0) {
      int commaIndex = line.indexOf(',');
      if (commaIndex > 0) {
        // الجزء الأول هو الوقت (float)
        String timeStr = line.substring(0, commaIndex);
        // الجزء الثاني هو النص
        String textStr = line.substring(commaIndex + 1);
        textStr.trim();
        
        // تحويل الثواني لمللي ثانية (ضرب في 1000)
        int durationMs = (int)(timeStr.toFloat() * 1000);
        
        // منع القيم الصفرية أو السالبة
        if (durationMs < 100) durationMs = 1000; 

        playlist.push_back({textStr, durationMs});
      }
    }
  }
}

// ---------------- دوال السيرفر ----------------
void handleRoot() {
  String s = index_html;
  // وضع النص الحالي داخل مربع النص لكي يراه المستخدم ويعدل عليه
  String currentContent = loadRawText();
  s.replace("%CURRENT_DATA%", currentContent);
  server.send(200, "text/html", s);
}

void handleSave() {
  if (server.hasArg("data")) {
    String data = server.arg("data");
    
    // 1. حفظ النص الخام للمرة القادمة
    saveRawText(data);
    
    // 2. تحليل النص وتحديث التشغيل فوراً
    parseAndLoadPlaylist(data);
    
    // 3. إعادة تعيين العدادات للبدء من جديد
    currentIndex = 0;
    previousMillis = millis(); // لضمان العرض الفوري
    lcd.clear(); // تنظيف الشاشة لبدء العرض الجديد
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
  lcd.setCursorArabic(0, 0);
  lcd.printArabic("تهيئة النظام...", false, true);

  if(!LittleFS.begin()){
    Serial.println("LittleFS Error");
    return;
  }

  // تحميل آخر بيانات محفوظة عند التشغيل
  String savedData = loadRawText();
  if (savedData.length() > 0) {
    parseAndLoadPlaylist(savedData);
  } else {
    // بيانات افتراضية لو الملف فارغ
    playlist.push_back({"مرحبا بك", 2000});
  }

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 20) {
    delay(500);
    t++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.print("IP: 192.168.1.200");
    delay(2000);
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
    
    if (currentMillis - previousMillis >= playlist[currentIndex].duration) {
      previousMillis = currentMillis;
      
      // التجهيز للجملة التالية (يتم التنفيذ بعد انتهاء وقت الجملة الحالية)
      currentIndex++;
      if (currentIndex >= playlist.size()) {
        currentIndex = 0;
      }
      
      // عرض الجملة
      lcd.clear();
      lcd.setCursorArabic(0, 0);
      lcd.printArabic(playlist[currentIndex].text, false, true);
    }
  }
}