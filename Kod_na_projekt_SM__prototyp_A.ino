#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DS18B20.h>


#define thermometer 13
#define trigCupPin 12
#define echoCupPin 11
#define trigTankPin 10
#define echoTankPin 9
#define pumpPin 8
#define constPouringPin 7
#define setPouringPin 6
#define upVolumePin 6
#define menuPin 5
#define stopPin 3

#define tank 13


unsigned long distanceAlarmTimer = 0;
unsigned long stopPour = 15000UL;
unsigned long pouringTimer = 0;
unsigned long menuTimer = 0;
unsigned long menuExitTime = 10000UL;
unsigned long changingVolumeTimer = 0;
unsigned long holdButtonTimer = 0;
unsigned long exitTime = 0;
unsigned long aktualnyCzas = 0;
unsigned long zapamietanyCzas = 0;
unsigned long pomiarTemp = 0;

int upVolumeFlag = 0;
int filterSetPouring = 0;
int filterConstPouring = 0;
boolean distanceAlarmConstFlag = 0;
boolean distanceAlarmSetFlag = 0;
boolean setPouringFlag = 0;
boolean stopFlag = 0;
boolean constsetPouringFlag = 0;
boolean menuFlag = 0;
boolean enableExitFlag = 0;
boolean autoExitFlag = 0;
boolean displayConstPouring = 0;

unsigned int constPouringIteration = 0;
unsigned int lcdMemory = 0;
unsigned int lcdStatus = 0;

int distanceCup;
int distanceTank = 200;
int volume = 100;

float temperature = 0;

byte Volume100[] = {
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000
};

byte Volume80[] = {
  B01110,
  B11011,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000
};

byte Volume60[] = {
  B01110,
  B11011,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000
};

byte Volume40[] = {
  B01110,
  B11011,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111,
  B00000
};

byte Volume20[] = {
  B01110,
  B11011,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111,
  B00000
};

byte Volume0[] = {
  B01110,
  B11011,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111,
  B00000
};

enum FLAG {FALSE, TRUE};
enum HOLDSTATE {NOT_CLICKED, CLICKED_ONCE, HOLDING};

LiquidCrystal_I2C lcd(0x27,16,2);
DS18B20 sensors(thermometer);


void setup() {
  lcd.init();
  lcd.backlight();
  
  Serial.begin(9600);
  pinMode(trigCupPin, OUTPUT);
  pinMode(trigTankPin, OUTPUT);
  pinMode(echoCupPin, INPUT);
  pinMode(echoTankPin, INPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(constPouringPin, INPUT_PULLUP);
  pinMode(setPouringPin, INPUT_PULLUP);
  pinMode(stopPin, INPUT_PULLUP);
  pinMode(menuPin, INPUT_PULLUP);
  digitalWrite(pumpPin, LOW);

  lcd.createChar(0, Volume100);
  lcd.createChar(1, Volume80);
  lcd.createChar(2, Volume60);
  lcd.createChar(3, Volume40);
  lcd.createChar(4, Volume20);
  lcd.createChar(5, Volume0);
  
}

void loop() {

  aktualnyCzas = millis();                                  //Ile czasu minelo od podlaczenia zasilania
  if (aktualnyCzas - zapamietanyCzas >= 75UL) {             //Co 75 milisekund kod programu jest wykonywany
    zapamietanyCzas = aktualnyCzas;
    distanceCup = calculateDistance(trigCupPin, echoCupPin);                                     //Obliczanie dystansu z czujnika odleglosci poziomu cieczy w kubku

;

    




    

    ////////////////////////  TUTAJ IMPLEMENTOWANY JEST STOP NALEWANIA  ///////////////////////////
    

    if (digitalRead(stopPin) == LOW)          //Nacisnieto guzik zatrzymujacy 
      stopHandling();                          //Funkcja obsługi guzika stop


      

    ///////////////////////  FRAGMENT KODU ODPOWIADAJĄCY ZA CIĄGŁE LANIE PŁYNU  /////////////////////////////



    else {                                                //Nie nacisnieto guzika zatrzymujacego nalewanie
      stopFlag = FALSE;
      if ((digitalRead(constPouringPin) == LOW) && (setPouringFlag == FALSE) && (menuFlag == FALSE))               //Nacisnieto guzik ciaglego nalewania
      {
        if (!constPouringIteration)
          displayConstPouring = TRUE;
        else
          displayConstPouring = FALSE;
        constPouringIteration++;
        if (distanceCup <= 8) {                                      //Dystans miedzy czujnikiem a plynem mniejszy niz ustalony, zadzialalo zabezpieczenie przed przelaniem
          filterConstPouring++;
          if (filterConstPouring > 1){
            digitalWrite(pumpPin, LOW);
            distanceAlarmConstFlag = TRUE;                                    //Informacja, ze zadzialal czujnik zabezpieczajacy przelanie
            constsetPouringFlag = FALSE;                                 //Informacja, ze zakonczono ciagle nalewanie
            distanceAlarmTimer = aktualnyCzas;
            constPouringIteration = 0;
            displayConstPouring = FALSE;
          }
        }
        else if ((distanceCup > 8 && distanceAlarmConstFlag == FALSE) || (distanceCup >= 18 && distanceAlarmConstFlag == TRUE) && (aktualnyCzas - distanceAlarmTimer >= 2000UL)) {    //Dystans miedzy czujnikiem a plynem jest odpowiedni i nie zadzialal poki co czujnik lub zadzialal i wtedy dystans sie odpowiednio zwieksza
          digitalWrite(pumpPin, HIGH);
          distanceAlarmConstFlag = FALSE;                                    //Zerujemy informacje o odpaleniu sie zabezpieczenia przed przelaniem
          constsetPouringFlag = TRUE;                                 //Informacja, ze rozpoczeto ciagle nalewanie
          filterConstPouring = 0;
          Serial.println("Nalewam w trybie ciągłym");
        }
      }
      else if (digitalRead(constPouringPin) == HIGH) {       //Nie nacisnieto guzika ciaglego nalewania
        distanceAlarmConstFlag = FALSE;                                    //Zerujemy informacje o odpaleniu sie zabezpieczenia przed przelaniem
        constsetPouringFlag = FALSE;                                 //Informacja, ze zakonczono ciagle nalewanie
        constPouringIteration = 0;
        filterConstPouring = 0;
        digitalWrite(pumpPin, LOW);
      }




      

      ////////////////////////  IMPLEMENTACJA NALEWANIA ODPOWIEDNIEJ ILOŚCI PŁYNU  /////////////////////////////////

      

      if ((digitalRead(setPouringPin) == LOW) && (setPouringFlag == FALSE) && (constsetPouringFlag == FALSE) && (menuFlag == FALSE) && (aktualnyCzas - distanceAlarmTimer >= 2000)) {   //Nacisnieto guzik od nalania konkretnej objetosci i obecnie nie nalewa sie zaden plyn
        pouringTimer = aktualnyCzas;                                  //Zapamietanie aktualnego czasu
        setPouringFlag = TRUE;                                           //Informacja, ze rozpoczeto nalewanie plynu
      }
      if ((setPouringFlag == TRUE) || (distanceAlarmSetFlag == TRUE)) {                                        //Jesli wlasnie nalewany jest plyn
        if (distanceCup <= 8) {                                                 //Dystans miedzy czujnikiem a plynem mniejszy niz ustalony, zadzialalo zabezpieczenie przed przelaniem
          filterSetPouring++;
          if (filterSetPouring > 1){
            digitalWrite(pumpPin, LOW);
            pouringTimer = aktualnyCzas - stopPour;
            Serial.println("Awaryjnie zatrzymano nalewanie w trybie objetosciowym");
            distanceAlarmSetFlag = TRUE;
            distanceAlarmTimer = aktualnyCzas;
            Serial.println(aktualnyCzas - pouringTimer);
          }
        }
        else if ((distanceCup > 8) && (aktualnyCzas - distanceAlarmTimer >= 2000UL)){
          filterSetPouring = 0;
          digitalWrite(pumpPin, HIGH);
          Serial.println("Rozpoczeto nalewanie");
          distanceAlarmSetFlag = FALSE;
          Serial.println(aktualnyCzas - pouringTimer);
        }
      }
      if ((aktualnyCzas - pouringTimer >= stopPour) && (setPouringFlag == TRUE)) {   //Jesli minal czas na nalewanie odpowiedniej objetosci i jest informacja, ze wczesniej rozpoczeto nalewanie
        filterSetPouring = 0;
        digitalWrite(pumpPin, LOW);
        Serial.println("Napój nalany");
        distanceAlarmSetFlag = FALSE;
        setPouringFlag = FALSE;                                              //Zerujemy informacje o nalewaniu
      }




      

      ////////////////////////  TUTAJ IMPLEMENTOWANA JEST ZMIANA OBJETOSCI NALEWANEGO PLYNU  ///////////////////////////////

      

      if ((digitalRead(menuPin) == LOW) && (setPouringFlag == FALSE) && (constsetPouringFlag == FALSE) && (aktualnyCzas - exitTime >= 1000UL)){     
        if (enableExitFlag == TRUE){
          menuExitTime = 10000UL;
          menuFlag = FALSE;
          enableExitFlag = FALSE;
          exitTime = aktualnyCzas;
          Serial.println("Wyszedlem z menu!");
        }
        else if (menuFlag == FALSE){
          menuTimer = aktualnyCzas;
          menuFlag = TRUE;
          Serial.println("Jestem w menu!");
        }     
      }
      else if ((digitalRead(menuPin) == HIGH) && (menuFlag == TRUE)){
        if (upVolumeFlag == NOT_CLICKED)
          enableExitFlag = TRUE;
          
        if ((aktualnyCzas - menuTimer >= menuExitTime) && ((upVolumeFlag == CLICKED_ONCE) || (upVolumeFlag == HOLDING) || (aktualnyCzas - holdButtonTimer <= 5000))){
            menuExitTime = menuExitTime + 10000UL;
            Serial.println("Wydluzono czas w menu!");
        }     
        else if ((aktualnyCzas - menuTimer >= menuExitTime) && (upVolumeFlag == NOT_CLICKED)){
          menuExitTime = 10000UL;
          menuFlag = FALSE;
          enableExitFlag = FALSE;
          exitTime = aktualnyCzas;
          Serial.println("Zakonczono czas w menu!");
        }
        
        if(aktualnyCzas - menuTimer < menuExitTime){
          upVolumeHandling();
        }
      }
    }  

    if (lcdMemory != lcdStatus){
      lcd.clear();
      lcdMemory = lcdStatus;
    }
    if ((constsetPouringFlag == TRUE) || (setPouringFlag == TRUE)){
      lcd.setCursor(2, 0);
      lcd.print("Nalewanie...");
      lcdStatus = 1;
    }
    else if ((distanceAlarmConstFlag == TRUE) || (distanceAlarmSetFlag == TRUE)){
      lcd.setCursor(1, 0);
      lcd.print("Zabesp. przed");
      lcd.setCursor(3, 1);
      lcd.print("przelaniem");
      lcdStatus = 2;
    }
    else if (menuFlag == TRUE){
      lcd.setCursor(0, 0);
      lcd.print("Nalewana obj:");
      lcd.setCursor(0, 1);
      lcd.print(stopPour/60000.0);
      lcd.print(" L");
      lcdStatus = 3;
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature);
      lcd.print(" C");
      lcd.setCursor(0, 1);
      lcd.print("Obj.: ");
      lcd.write(volumeDisplay());
      if (distanceTank <= 200){
        lcd.print(" ");
        lcd.print(volume);
        lcd.print(" %  ");
      }
      Measurement();
      lcdStatus = 4;
    }
  }
}  



                                                  ///////////////////////////  NAPISANE FUNKCJE  /////////////////////////////

                                                  

int calculateDistance(int trigPin, int echoPin) {         //Funkcja obliczajaca dystans miedzy czujnikiem odleglosci a plynem w kubku
  long distanceTime, distance;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  distanceTime = pulseIn(echoPin, HIGH);
  distance = distanceTime / 58;

  return distance;
}

void resetFlags(){
  upVolumeFlag = NOT_CLICKED;
  menuFlag = FALSE;
  constsetPouringFlag = FALSE;
  enableExitFlag = FALSE;
  setPouringFlag = FALSE;                                      //Informacja, ze teraz nie nalewamy napoju
  distanceAlarmConstFlag = FALSE;                                    //Zerujemy informacje o odpaleniu sie zabezpieczenia przed przelaniem
}

void stopHandling(){
  Serial.println("Użyto przycisku STOP");
  digitalWrite(pumpPin, LOW);                               //Wylaczenie pompy
  resetFlags();
  filterSetPouring = 0;
  filterConstPouring = 0;
  menuExitTime = 10000UL;
  stopFlag = TRUE;
  displayStop();
  delay(2000);                                              //Awaryjne zatrzymanie programu na 2 sekundy
}

void upVolumeHandling(){
  
  switch(upVolumeFlag){  
    case(NOT_CLICKED):
      if (digitalRead(upVolumePin) == LOW){
        upVolumeFlag = CLICKED_ONCE;
        holdButtonTimer = aktualnyCzas;
        if (stopPour < 30000UL)
            stopPour = stopPour + 600UL;                 
          else
            stopPour = 12000UL;
        Serial.print("Czas nalewania w ms: ");
        Serial.println(stopPour);
      }
    break;
    
    case(CLICKED_ONCE):
      if (digitalRead(upVolumePin) == LOW){
        if (aktualnyCzas - holdButtonTimer >= 600){
          upVolumeFlag = HOLDING;
          if (stopPour < 30000UL)
            stopPour = stopPour + 600UL;                 
          else
            stopPour = 12000UL;
          Serial.print("Czas nalewania w ms: ");
          Serial.println(stopPour);
        }
      }
      else
        upVolumeFlag = NOT_CLICKED;
    break;

    case(HOLDING):
      if (digitalRead(upVolumePin) == LOW){
        if (aktualnyCzas - holdButtonTimer >= 300){
          if (stopPour < 30000UL)
            stopPour = stopPour + 600UL;                 
          else
            stopPour = 12000UL;
          Serial.print("Czas nalewania w ms: ");
          Serial.println(stopPour);
        }
      }
      else
        upVolumeFlag = NOT_CLICKED;
    break;

    default:
    break;
  }
}

void displayStop(){
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("STOP");
}

void Measurement(){
  if (((aktualnyCzas - pomiarTemp >= 5000UL) && (lcdStatus == 4)) || (lcdStatus == 0)){
    temperature = sensors.getTempC();
    distanceTank = calculateDistance(trigTankPin, echoTankPin) - 11;
    pomiarTemp = aktualnyCzas;
  }
}

int volumeDisplay(){
  if (((distanceTank >= 0) && (distanceTank <= 0.05 * tank) && (volume != 80)) || ((distanceTank >= 0) && (distanceTank <= 0.075 * tank) && (volume == 80))){
    volume = 100;
    return 0;
  }
  else if (((distanceTank >= 0.05 * tank) && (distanceTank <= 0.2 * tank) && (volume != 100) && (volume != 60)) || ((distanceTank >= 0.1 * tank) && (distanceTank <= 0.2 * tank) && ((volume == 100) || (volume == 60)))){
    volume = 80;
    return 1;
  }
  else if (((distanceTank >= 0.2 * tank) && (distanceTank <= 0.4 * tank) && (volume != 80) && (volume != 40)) || ((distanceTank >= 0.25 * tank) && (distanceTank <= 0.35 * tank) && ((volume == 80) || (volume == 40)))){
    volume = 60;
    return 2;
  }
  else if (((distanceTank >= 0.4 * tank) && (distanceTank <= 0.6 * tank) && (volume != 60) && (volume != 20)) || ((distanceTank >= 0.45 * tank) && (distanceTank <= 0.55 * tank) && ((volume == 60) || (volume == 20)))){
    volume = 40;
    return 3;
  }
  else if (((distanceTank >= 0.6 * tank) && (distanceTank <= 0.8 * tank) && (volume != 40) && (volume != 0)) || ((distanceTank >= 0.65 * tank) && (distanceTank <= 0.75 * tank) && ((volume == 40) || (volume == 0)))){
    volume = 20;
    return 4;
  }
  else if (((distanceTank >= 0.8 * tank) && (distanceTank <= tank) && (volume != 20)) || ((distanceTank >= 0.85 * tank) && (distanceTank <= tank) && (volume == 20))){
    volume = 0;
    return 5;
  }
}
