#include <ESP32Servo.h>
//Fingerprint সেন্সর না থাকাই Pushbutton কে fingerprint sensor ধরে নিয়ে ডেমো
//slider কে গ্যাস ডিটেক্টর ধরে নিয়ে ডেমো
// পিন ডেফিনিশন
#define FINGER_BUTTON 16 
#define SERVO_PIN 18
#define GREEN_LIGHT 19   
#define RED_LIGHT 25     
#define GAS_SWITCH 26    
#define BUZZER_PIN 27

Servo myServo;

// স্টেট ট্র্যাকিং ভ্যারিয়েবল (অপ্টিমাইজড মেমোরি)
bool lightState = false; 
bool doorOpen = false;
bool lastButtonState = HIGH; // বাটনের আগের অবস্থা সংরক্ষণের জন্য

unsigned long doorOpenStartTime = 0; 
const unsigned long doorOpenDuration = 5000; // ৫ সেকেন্ড

void setup() {
  Serial.begin(115200);
  
  // পিন মোড সেটআপ
  pinMode(FINGER_BUTTON, INPUT_PULLUP); 
  pinMode(GREEN_LIGHT, OUTPUT);
  pinMode(RED_LIGHT, OUTPUT);
  pinMode(GAS_SWITCH, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // শুরুতে সব আউটপুট বন্ধ
  digitalWrite(GREEN_LIGHT, LOW);
  digitalWrite(RED_LIGHT, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  // সার্ভো ইনিশিয়ালাইজেশন
  myServo.attach(SERVO_PIN);
  myServo.write(0); 

  Serial.println("System Ready! Pure Non-Blocking Mode Active.");
}

void loop() {
  // ========================================================
  // ১. গ্যাস লিকেজ অ্যালার্ম (১০০% নন-ব্লকিং এবং রিয়েল-টাইম)
  // ========================================================
  int gasStatus = digitalRead(GAS_SWITCH);
  digitalWrite(BUZZER_PIN, gasStatus); 
  digitalWrite(RED_LIGHT, gasStatus); 
  
  // সিরিয়াল মনিটর ওভারফ্লো রোধে কেবল স্টেট চেঞ্জ হলে প্রিন্ট করবে
  static int lastGasStatus = LOW;
  if (gasStatus != lastGasStatus) {
    if (gasStatus == HIGH) Serial.println("WARNING: Gas Leakage Detected!");
    lastGasStatus = gasStatus;
  }

  // ========================================================
  // ২. বাটনের স্টেট চেঞ্জ ডিটেকশন (কোনো while লুপ ছাড়া)
  // ========================================================
  bool currentButtonState = digitalRead(FINGER_BUTTON);

  // বাটন কেবল প্রেস করা হয়েছে (HIGH থেকে LOW হয়েছে) এবং দরজা বর্তমানে বন্ধ
  if (currentButtonState == LOW && lastButtonState == HIGH && !doorOpen) {
    Serial.println("Fingerprint Matched! Door Opening...");
    
    // লাইট টগল
    lightState = !lightState; 
    digitalWrite(GREEN_LIGHT, lightState); 
    Serial.println(lightState ? "Room Light: ON" : "Room Light: OFF");

    // দরজা খোলা
    myServo.write(90);   
    doorOpen = true;                 
    doorOpenStartTime = millis();    
  }
  lastButtonState = currentButtonState; // কারেন্ট স্টেট সেভ করে রাখা

  // ========================================================
  // ৩. অটোমেটিক ডোর টাইমার (মিলিস ভিত্তিক)
  // ========================================================
  if (doorOpen) {
    if (millis() - doorOpenStartTime >= doorOpenDuration) {
      myServo.write(0);              
      doorOpen = false;              
      Serial.println("Door Closed automatically.");
    }
  }
}