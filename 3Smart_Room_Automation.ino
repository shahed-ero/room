/*
 ╔═══════════════════════════════════════════════════════════════════════════╗
 ║          স্মার্ট রুম অটোমেশন - এসপি32-ক্যাম ফার্মওয়্যার                  ║
 ║                         বাংলাদেশ বিজ্ঞান প্রকল্প                         ║
 ║                                                                           ║
 ║  বৈশিষ্ট্য:                                                              ║
 ║  • মুখ সনাক্তকরণ (সিমুলেশন + ক্যামেরা-প্রস্তুত)                          ║
 ║  • DHT11 তাপমাত্রা সেন্সর                                              ║
 ║  • স্মার্ট AC নিয়ন্ত্রণ (তাপমাত্রা-ভিত্তিক PWM)                         ║
 ║  • SG90 সার্ভো দরজা নিয়ন্ত্রণ (স্বয়ংক্রিয় বন্ধ ৫ সেকেন্ডে)            ║
 ║  • LED সূচক (সবুজ = খোলা, লাল = AC অবস্থা)                            ║
 ║                                                                           ║
 ║  GPIO নির্ধারণ:                                                        ║
 ║  → GPIO 4  : সবুজ এলইডি (দরজা খোলা সূচক)                            ║
 ║  → GPIO 12 : লাল এলইডি (AC স্থিতি, PWM উজ্জ্বলতা)                    ║
 ║  → GPIO 13 : SG90 সার্ভো (PWM, 50Hz)                                  ║
 ║  → GPIO 14 : DHT11 ডেটা                                               ║
 ║                                                                           ║
 ║  ব্যবহার:                                                              ║
 ║  • সিরিয়াল মনিটর খুলুন (115200 বোড)                                 ║
 ║  • 'f' টাইপ করুন এবং এন্টার দিন → দরজা খুলবে                         ║
 ║  • তাপমাত্রা পরিবর্তন দেখবেন → এলইডি উজ্জ্বলতা পরিবর্তিত হবে         ║
 ║                                                                           ║
 ╚═══════════════════════════════════════════════════════════════════════════╝
*/

#include "DHT.h"
#include <ESP32Servo.h>

// ═══════════════════════════════════════════════════════════════
// ১. পিন ডেফিনিশন
// ═══════════════════════════════════════════════════════════════
#define DHTPIN 14              // DHT11 ডেটা পিন
#define GREENLEDPIN 4          // দরজা খোলা সূচক এলইডি
#define REDLEDPIN 12           // AC স্থিতি এলইডি (PWM নিয়ন্ত্রিত)
#define SERVOPPIN 13           // SG90 সার্ভো সিগন্যাল পিন
#define DHTTYPE DHT11          // DHT সেন্সর ধরন

// ═══════════════════════════════════════════════════════════════
// २. গ্লোবাল ভেরিয়েবল এবং অবজেক্ট
// ═══════════════════════════════════════════════════════════════
DHT dht(DHTPIN, DHTTYPE);      // DHT11 অবজেক্ট
Servo doorServo;                // সার্ভো অবজেক্ট

// স্থিতি ট্র্যাকিং
bool roomOccupied = false;
unsigned long doorOpenTime = 0;
const unsigned long DOOR_OPEN_DURATION = 5000;  // 5 সেকেন্ড

// ডিবাগিং ফ্ল্যাগ
const bool DEBUG_MODE = true;  // সিরিয়াল আউটপুট সক্ষম করতে

// ═══════════════════════════════════════════════════════════════
// ३. প্রধান সেটআপ ফাংশন
// ═══════════════════════════════════════════════════════════════
void setup() {
  // সিরিয়াল যোগাযোগ শুরু করুন
  Serial.begin(115200);
  delay(1000);
  
  if (DEBUG_MODE) {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  🚀 স্মার্ট রুম অটোমেশন শুরু হচ্ছে  ║");
    Serial.println("╚════════════════════════════════════════╝\n");
  }
  
  // ডিজিটাল পিন কনফিগারেশন
  pinMode(GREENLEDPIN, OUTPUT);
  pinMode(REDLEDPIN, OUTPUT);
  digitalWrite(GREENLEDPIN, LOW);
  analogWrite(REDLEDPIN, 0);
  
  if (DEBUG_MODE) Serial.println("[१] ✓ এলইডি পিন কনফিগার করা হয়েছে");
  
  // সার্ভো মোটর সেটআপ
  doorServo.attach(SERVOPPIN, 1000, 2000);
  doorServo.write(0);  // প্রাথমিক অবস্থান: বন্ধ
  delay(500);
  
  if (DEBUG_MODE) Serial.println("[२] ✓ সার্ভো মোটর সেটআপ সম্পন্ন");
  
  // DHT11 সেন্সর শুরু করুন
  dht.begin();
  delay(2000);  // DHT11 স্থিতিশীল হতে সময় প্রয়োজন
  
  if (DEBUG_MODE) Serial.println("[३] ✓ DHT11 সেন্সর শুরু করা হয়েছে");
  
  Serial.println("\n✅ সিস্টেম প্রস্তুত!");
  Serial.println("═══════════════════════════════════════");
  Serial.println("নির্দেশ: সিরিয়াল মনিটরে 'f' টাইপ করুন দরজা খোলার জন্য");
  Serial.println("═══════════════════════════════════════\n");
}

// ═══════════════════════════════════════════════════════════════
// ४. প্রধান লুপ ফাংশন
// ═══════════════════════════════════════════════════════════════
void loop() {
  // १. তাপমাত্রা এবং আর্দ্রতা পড়ুন
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // २. সেন্সর ত্রুটি যাচাই করুন
  if (isnan(humidity) || isnan(temperature)) {
    if (DEBUG_MODE) Serial.println("⚠️  DHT11 পড়তে ব্যর্থ - পরবর্তী চেষ্টা...");
    delay(2000);
    return;
  }
  
  // ३. তাপমাত্রা এবং আর্দ্রতা প্রদর্শন করুন
  if (DEBUG_MODE) {
    Serial.print("🌡️  তাপমাত্রা: ");
    Serial.print(temperature, 1);
    Serial.print("°C  |  💧 আর্দ্রতা: ");
    Serial.print(humidity, 1);
    Serial.println("%");
  }
  
  // ४. দরজা বন্ধ করার সময় যাচাই করুন
  if (roomOccupied && (millis() - doorOpenTime) > DOOR_OPEN_DURATION) {
    if (DEBUG_MODE) Serial.println("\n⏱️  ५ সেকেন্ড পরে দরজা স্বয়ংক্রিয়ভাবে বন্ধ হচ্ছে...");
    
    doorServo.write(0);              // সার্ভো বন্ধ অবস্থানে ফেরত আনুন
    digitalWrite(GREENLEDPIN, LOW);  // সবুজ এলইডি বন্ধ করুন
    roomOccupied = false;
    
    if (DEBUG_MODE) Serial.println("✓ দরজা বন্ধ, সবুজ এলীডি বন্ধ\n");
  }
  
  // ५. মুখ সনাক্তকরণ চেক করুন (ব্যবহারকারী ইনপুট)
  if (checkFaceDetection()) {
    handleRoomEntry();
  }
  
  // ६. AC নিয়ন্ত্রণ লজিক (শুধুমাত্র যদি কক্ষ দখল করা)
  if (roomOccupied) {
    controlACIndicator(temperature);
  } else {
    // কক্ষ খালি → লাল এলীডি বন্ধ রাখুন
    analogWrite(REDLEDPIN, 0);
    if (DEBUG_MODE) Serial.println("📍 কক্ষ খালি - AC এলীডি বন্ধ\n");
  }
  
  // প্রতি २ সেকেন্ডে নতুন পড়া করুন
  delay(2000);
}

// ═══════════════════════════════════════════════════════════════
// ५. AC সূচক নিয়ন্ত্রণ ফাংশন (PWM উজ্জ্বলতা)
// ═══════════════════════════════════════════════════════════════
void controlACIndicator(float temp) {
  int brightness;
  const char* status;
  
  if (temp > 32.0) {
    // ছেদ १: খুব গরম → AC সম্পূর্ণ শক্তি
    brightness = 255;  // 100%
    status = "100% (গরম)";
  } 
  else if (temp >= 25.0 && temp <= 32.0) {
    // ছেদ २: মধ্যম → AC অর্ধ শক্তি
    brightness = 76;   // ~30% (0.3 * 255 ≈ 76)
    status = "30% (মধ্যম)";
  } 
  else {
    // ছেদ ३: শীতল → AC বন্ধ
    brightness = 0;    // 0%
    status = "0% (শীতল)";
  }
  
  // PWM উজ্জ্বলতা সেট করুন
  analogWrite(REDLEDPIN, brightness);
  
  if (DEBUG_MODE) {
    Serial.print("🔴 লাল এলীডি (AC স্থিতি): ");
    Serial.println(status);
  }
}

// ═══════════════════════════════════════════════════════════════
// ६. মুখ সনাক্তকরণ ফাংশন (সিমুলেশন)
// ═══════════════════════════════════════════════════════════════
bool checkFaceDetection() {
  // সিমুলেশন: সিরিয়াল ইনপুট থেকে 'f' চেক করুন
  if (Serial.available()) {
    char cmd = Serial.read();
    
    if (cmd == 'f' || cmd == 'F') {
      return true;  // মুখ সনাক্তকরণ সফল
    }
    else if (cmd == 'h' || cmd == 'H') {
      printHelp();  // সহায়তা মেনু
    }
  }
  
  return false;
}

// ═══════════════════════════════════════════════════════════════
// ७. রুম প্রবেশ হ্যান্ডলার
// ═══════════════════════════════════════════════════════════════
void handleRoomEntry() {
  if (!roomOccupied) {  // দরজা এখনো খোলা নেই তাহলে
    if (DEBUG_MODE) {
      Serial.println("\n╔════════════════════════════════════╗");
      Serial.println("║  ✅ মুখ সনাক্ত হয়েছে!           ║");
      Serial.println("║  🟢 সবুজ এলীডি চালু              ║");
      Serial.println("║  🚪 দরজা খুলছে...                ║");
      Serial.println("╚════════════════════════════════════╝\n");
    }
    
    // সবুজ এলীডি চালু করুন
    digitalWrite(GREENLEDPIN, HIGH);
    
    // সার্ভো খোলার কোণে নিয়ে যান (90°)
    doorServo.write(90);
    
    // অবস্থা আপডেট করুন
    roomOccupied = true;
    doorOpenTime = millis();
  }
}

// ═══════════════════════════════════════════════════════════════
// ८. সহায়তা মেনু
// ═══════════════════════════════════════════════════════════════
void printHelp() {
  Serial.println("\n╔═══════════════════════════════════════════╗");
  Serial.println("║         সিরিয়াল কমান্ড তালিকা           ║");
  Serial.println("╠═══════════════════════════════════════════╣");
  Serial.println("║ 'f' বা 'F' → দরজা খুলুন (মুখ সনাক্ত)    ║");
  Serial.println("║ 'h' বা 'H' → এই সহায়তা মেনু             ║");
  Serial.println("╚═══════════════════════════════════════════╝\n");
}

// ═══════════════════════════════════════════════════════════════
// ९. বর্ধিত বৈশিষ্ট্য: WiFi সংযোগ (ঐচ্ছিক)
// ═══════════════════════════════════════════════════════════════
/*
// আনকমেন্ট করলে WiFi সক্ষম হবে:

#include <WiFi.h>

const char* ssid = "আপনার_WiFi_নাম";
const char* password = "আপনার_পাসওয়ার্ড";

void setupWiFi() {
  Serial.print("WiFi সংযোগ করছি: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi সংযুক্ত!");
    Serial.print("IP ঠিকানা: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi সংযোগ ব্যর্থ");
  }
}

// setup() এ যোগ করুন:
// setupWiFi();
*/

// ═══════════════════════════════════════════════════════════════
// রূপান্তর টাইমিং তথ্য
// ═══════════════════════════════════════════════════════════════
/*
  PWM মূল্য রেফারেন্স (লাল এলীডি উজ্জ্বলতা):
  
  তাপমাত্রা > 32°C  →  brightness = 255  (100% উজ্জ্বল)
  তাপমাত্রা 25-32°C →  brightness = 76   (~30% উজ্জ্বল)
  তাপমাত্রা < 25°C  →  brightness = 0    (বন্ধ)
  
  সার্ভো কোণ:
  0°   → দরজা বন্ধ
  90°  → দরজা খোলা
  দ্রুত সেবা সময়: ~1 সেকেন্ড
*/
