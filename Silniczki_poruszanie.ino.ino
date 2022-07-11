#include <AFMotor.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#include <SoftwareSerial.h> 

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *myMotor_L = AFMS.getMotor(2);
Adafruit_DCMotor *myMotor_R = AFMS.getMotor(1);
SoftwareSerial MyBlue(2, 3); // piny służące do komunikacji bluetooth
int Trig = 12 ;  //pin 2 Arduino połączony z pinem Trigger czujnika
int Echo = 13;  //pin 3 Arduino połączony z pinem Echo czujnika
int CM;        //odległość w cm
long TIME;     //długość powrotnego impulsu w uS
bool proximityAlertSent = false;
bool delayRunning=true;
int long delayStart = millis();
int measurement[3] = {99,99,99};
int meanMeasurement = 99;
int robotSpeed = 150;
bool autoBypassing = false;

void setup(){ 
  AFMS.begin(9600); // połączenie 
  MyBlue.begin(9600); 
  Serial.begin(9600);
  pinMode(Trig, OUTPUT);  //ustawienie pinu 2 w Arduino jako wyjście
  pinMode(Echo, INPUT);  //ustawienie pinu 3 w Arduino jako wejście
  Serial.println(">> START<<");
  myMotor_L->setSpeed(robotSpeed);
  myMotor_R->setSpeed(robotSpeed);  //ustawienie prędkości silnika 
}

void forward(){
  myMotor_L->run(FORWARD); // do przodu  
  myMotor_R->run(FORWARD); 
  delay(500); //zatrzymanie programu na 500 milisec czyli 0.5 sek
  myMotor_R->run(RELEASE);  
  myMotor_L->run(RELEASE); // zatrzymywanie silnika 
}

void backward(){
  myMotor_L->run(BACKWARD); // do tyłu
  myMotor_R->run(BACKWARD);  
  delay(500);
  myMotor_R->run(RELEASE); 
  myMotor_L->run(RELEASE); // zatrzymywanie silnika 
}

void left(){
  myMotor_R->run(BACKWARD); 
  delay(750);
  myMotor_R->run(RELEASE);
 
}
void right(){
  myMotor_L->run(BACKWARD); 
  delay(750);
  myMotor_L->run(RELEASE); 
}
// dodany kod 
void speedChange(){
 if (robotSpeed >= 250)
  {
   robotSpeed=250;
  }
  if (robotSpeed <= 0)
  {
    robotSpeed=0;
  }
  myMotor_R->setSpeed(robotSpeed);
  myMotor_L->setSpeed(robotSpeed);
  Serial.println("NOWA PREDKOSC " + robotSpeed);
 
}

void sendInfo()
{
   String message = "INFO:PREDKOSC<"+String(robotSpeed)+">";
   if (autoBypassing == true)
   {
    message = message + "auto";
   }
   else if( autoBypassing == false)
   {
      message = message + "manual";
   } //TODO test
   Serial.println("###############################");
   Serial.println(message);
   char charMessage[message.length()+1];
   message.toCharArray(charMessage, sizeof(charMessage));
   Serial.println("Wysylanie aktualnej predkosci: " + robotSpeed);
   MyBlue.write(charMessage);

}


int proximityMeasurement ()
{
  //ustawienie stanu wysokiego na 2 uS - impuls inicjalizujacy
  digitalWrite(Trig, LOW);        
  delayMicroseconds(2);
  //ustawienie stanu wysokiego na 10 uS - impuls inicjalizujący
  digitalWrite(Trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trig, LOW);
  digitalWrite(Echo, HIGH); 
  TIME = pulseIn(Echo, HIGH);
  //szerokość odbitego impulsu w uS podzielone przez 58 to odleglosc w cm
  CM = TIME / 58; 
  return CM;
}

void loop(){
  if (MyBlue.available()>0){
   char command = MyBlue.read();
   switch ( command )
   {
    case 'w':
      Serial.println("forward");
      forward();
      break;
    case 's':
      Serial.println("backward");
      backward();
      break;
    case 'd':
      Serial.println("right");
      right();
      break;
    case 'a':
      Serial.println("left");
      left();
      break;
    case 'r':
      Serial.println("ZWIEKSZEIE PREDKOSCI");
      robotSpeed = robotSpeed + 10;
      speedChange();
      sendInfo();
      break;
    case 'y':
      Serial.println("ZMNIEJSZENIE PREDKOSCI");
      robotSpeed = robotSpeed - 10;
      speedChange();
      sendInfo();
      break;
    case 'i':
      sendInfo();
      break;
     case 'o':
      autoBypassing = !autoBypassing;
      sendInfo();
    default:
      break;
   }
  }

  // timer 
  if (delayRunning && ((millis() - delayStart) >= 2000)) { 
    // usrednienie pomiarów
    if (measurement[0] == 99) 
    {
      measurement[0]=proximityMeasurement();
    }
    else if (measurement[1] == 99)
    {
      measurement[1]=proximityMeasurement();
    }
    else
    {
      measurement[2]=proximityMeasurement();
      meanMeasurement = (measurement[0]+measurement[1]+measurement[2])/3;
      measurement[0]=99;  // przywrocenie tablicy do wartosci poczatkowych
      measurement[1]=99;
    }

    if (proximityAlertSent==false && meanMeasurement<10 ) 
    {
      MyBlue.write("PRZESZKODA"); // wyslanie komunikatu o przeszkodzie
      proximityAlertSent = true; // proximity_alert zapobiega wysylaniu wielu komunikatow 
      delayStart = millis(); // wyzerowanie timera
      delayRunning = true;  // j.w.
      if (autoBypassing == true)
      {
        left();
      }
    }
    else if (proximityAlertSent==true && meanMeasurement>=15)
    {
      MyBlue.write("KONIEC"); //wyslanie komunikatu o zniknieciu przeszkody z pola widzenia
      proximityAlertSent=false; 
    } 
  }
}
 
