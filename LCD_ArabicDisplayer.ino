#include <Wire.h>
#include "LiquidCrystalArabic_I2C.h"

// تأكد من العنوان 0x27 أو 0x3F حسب الشاشة لديك
LiquidCrystalArabic lcd(0x27, 16, 2);

void setup() {
  lcd.init();
  lcd.backlight();
}

// دالة لعرض جملة كاملة
void displaySentence(String text) {
  lcd.clear();
  
  // في اللغة العربية نبدأ المؤشر من اليمين (العمود 15) ليكون العرض أصح
  // لكن بما أنك تستخدم setCursorArabic والمكتبة تضبط الاتجاه، سنستخدم طريقتك:
  lcd.setCursorArabic(0, 0); 
  
  // إرسال النص مدمجاً والمكتبة ستعالجه كجملة واحدة
  lcd.printArabic(text, false, true); 
}

void loop() {
  // --- البسملة ---
  displaySentence("بسم الله");
  delay(2000);
  
  displaySentence("الرحمن الرحيم");
  delay(2000);
  
  // --- سورة الإخلاص ---
  
  // الآية 1
  displaySentence("قل هو");
  delay(1500);
  
  displaySentence("الله احد");
  delay(2000);
  
  // الآية 2
  displaySentence("الله الصمد");
  delay(2000);
  
  // الآية 3
  displaySentence("لم يلد");
  delay(1500);
  
  displaySentence("ولم يولد");
  delay(2000);
  
  // الآية 4
  displaySentence("ولم يكن");
  delay(1500);
  
  // هنا تبقت 3 كلمات (له - كفوا - أحد)
  // تم تقسيمها لتعرض كلمتين ثم كلمة، أو يمكنك دمج الثلاثة لو الشاشة تكفي
  displaySentence("له كفوا");
  delay(1500);
  
  displaySentence("احد"); // كلمة وحيدة في النهاية
  delay(3000);
}