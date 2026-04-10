#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
MFRC522 myRFID(SS_PIN, RST_PIN);

int ledGreen = 2; 
int ledRed = 7; 
int ledYellow = 4; 
int buzzer = 8; 
int relay = 5; 
int SolenoidValve = 3; 

// Authorized UIDs
String AUTH_UID_MAIN  = "F7 50 AE 63";
String AUTH_UID_SPARE = "0F 77 64 4B";

bool AlarmMode = false;
bool WarningMode = false; 
bool carLocked = true;
unsigned long previousMillis = 0;

const long intervalFast = 100; // For Intruder (Red)
const long intervalSlow = 300; // For Warning (Green)

bool blinkState = false;

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  myRFID.PCD_Init();
  Serial.println("System Ready. Please Scan Your Card...");

  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(SolenoidValve, INPUT_PULLUP); 
  
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledRed, HIGH);
  digitalWrite(ledYellow, LOW);
  digitalWrite(buzzer, LOW);
  digitalWrite(relay, LOW);
}

void loop()
{
  unsigned long currentMillis = millis();

  // Solenoid Valve Check - Performing Normal Close Door Operation Manually
  if (digitalRead(SolenoidValve) == LOW) 
  {
    if (!carLocked) {
       Serial.println("Manual Lock Detected by Solenoid Valve.");
       carLocked = true; 
       digitalWrite(ledGreen, LOW);
       digitalWrite(ledRed, HIGH);
       Serial.println();
    }
  }

  // Alarm Mode Logic
  if (AlarmMode)
  {
    digitalWrite(relay, LOW); 

    long activeInterval;
    // Warning Mode Logic Inside Alarm Mode
    // Determine The Blink/Buzz Rate Based on WarningMode
    if (WarningMode) {
      activeInterval = intervalSlow; // Slower Blink/Buzz Rate For Warning Mode
    } else {
      activeInterval = intervalFast; // Faster Blink/Buzz Rate For Alarm Mode
    }

    if (currentMillis - previousMillis >= activeInterval)
    {
      previousMillis = currentMillis;
      blinkState = !blinkState;

      digitalWrite(ledYellow, blinkState);
      digitalWrite(buzzer, blinkState);
      
      // Warning Mode Indication
      if (WarningMode) 
      {
         digitalWrite(ledGreen, HIGH);
         digitalWrite(ledRed, LOW);
      }
      // Alarm Mode Indication
      else
      {
         digitalWrite(ledRed, HIGH); 
         digitalWrite(ledGreen, LOW);
      }
    }
  }

  // RFID Check
  if (!myRFID.PICC_IsNewCardPresent()) return;
  if (!myRFID.PICC_ReadCardSerial()) return;

  String content = "";
  for (byte i = 0; i < myRFID.uid.size; i++)
  {
     content.concat(String(myRFID.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(myRFID.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  String cardID = content.substring(1);

  Serial.print("USER ID tag : ");
  Serial.println(cardID);

  // When True Access Card Scanned 
  bool isAuthorized = (cardID == AUTH_UID_MAIN) || (cardID == AUTH_UID_SPARE);

  if (isAuthorized)
  {
    // Alarm Mode Cancellation
    if (AlarmMode) 
    {
      Serial.println("Alarm Deactivated by Authorized User.");
      AlarmMode = false;
      
      digitalWrite(buzzer, LOW);
      digitalWrite(ledYellow, LOW);
      
      // Handle State After Cancelling (Either Locked or Unlocked)
      if (WarningMode)
      {
         digitalWrite(ledGreen, HIGH);
         digitalWrite(ledRed, LOW);
         carLocked = false; 
      }
      else
      {
         digitalWrite(ledGreen, LOW);
         digitalWrite(ledRed, HIGH);
         carLocked = true; 
      }
      
      WarningMode = false; 
      delay(1000); 
    }
    
    // Normal Operation of Car Door Lock System
    else 
    {
      if (carLocked)
      {
        // Lock to Unlock Sequence
        Serial.println("Access Granted! Unlocking for Authorized User...");
        digitalWrite(ledGreen, HIGH);
        digitalWrite(ledRed, LOW);
        
        digitalWrite(relay, HIGH); 
        delay(300); 
        digitalWrite(relay, LOW); 
        
        for (int i = 0; i < 2; i++)
        {
          digitalWrite(ledYellow, HIGH);
          digitalWrite(buzzer, HIGH);
          delay(300); 
          digitalWrite(ledYellow, LOW);
          digitalWrite(buzzer, LOW);
          delay(300); 
        }
        carLocked = false; 
      }
      else
      {
        // Warning Mode Activation
        Serial.println("Warning Mode Activated!");
        AlarmMode = true;
        WarningMode = true;
        
        carLocked = true; 
      }
    }
  }
  else
  {
    // When Wrong Access Card Detected
    Serial.println("Access Denied! Alarm Activated!");
    
    AlarmMode = true;
    WarningMode = false;
    carLocked = true; 
    digitalWrite(relay, LOW); 
  }

  myRFID.PICC_HaltA();
  myRFID.PCD_StopCrypto1();

  Serial.println(); 
}
