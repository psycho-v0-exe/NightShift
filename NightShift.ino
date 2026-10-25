#include "SevSeg.h"
#include "RTClib.h"
SevSeg Display;
RTC_DS3231 rtc;
DateTime time;

#define d1 12
#define d2 9
#define d3 8
#define d4 6
#define a 11
#define b 7
#define c 4
#define d 2
#define e A1
#define f 10
#define g 5
#define p 3 //deimal point

#define rly 1
#define cs A0 //current sensor
#define sw 13 //switch
#define swl A2 //switch light

//A4=SDA, A5=SCL


int hr, min, dpFlg; //Time values
int cdMin, cdSec; //Cooldown time values
unsigned long prev=0; //Millis counter for wait
const int minThr=440, maxThr=580; //Current Threshold min and max value
const long wait=10*60*1000, cdt=15*60*1000; //Current wait time, Battery cooldown time
bool rlyTrg=true; //Relay trigger value
bool swFlg=false, cdtFlg=false; //Switch Flag, Cooldown time Flag

void segTime(){
  time=rtc.now();
  hr=time.hour();
  min=time.minute();
  dpFlg=(time.second()%2==0)? 2:-1;
  Display.setNumber((hr*100)+min,dpFlg);
  Display.refreshDisplay();
}

void setup() {
  if (!rtc.begin()) {
    while (1);
  }

  byte hardwareConfiguration=COMMON_ANODE, numDigits=4;
  byte digitPins[]={d1,d2,d3,d4}, segmentPins[]={a,b,c,d,e,f,g,p};
  bool resistorOnSegments=true, updateWithDelays=false, leadingZeros=false, disableDecPoint=false;
  Display.begin(hardwareConfiguration,numDigits,digitPins,segmentPins,resistorOnSegments,updateWithDelays,leadingZeros,disableDecPoint);
  Display.setBrightness(100);

  pinMode(rly,OUTPUT);
  pinMode(cs,INPUT);
  pinMode(sw,INPUT);
  pinMode(swl,OUTPUT);
}

void loop() {
  //display time
  segTime();

  //automatic power from 10pm to 6am
  if(hr>=22 || hr<6){
    digitalWrite(rly,rlyTrg);
  }else if((analogRead(cs)>minThr) && (analogRead(cs)<maxThr)){
    if(prev==0){
      prev=millis();
    }else if(millis()-prev>wait){
      digitalWrite(rly,!rlyTrg);
      delay(2000);
      prev=0;
    }
  }else if(prev!=0){
    prev=0;
  }

  //manual power after 15min cooldown
  if(digitalRead(sw) && (prev==0 || (millis()-prev>5000))){
    swFlg=true;
    cdtFlg=true;
    prev=millis();
    digitalWrite(rly,!rlyTrg);
    Display.blank();
    Display.refreshDisplay();
    delay(2000);
  }
  while(swFlg){
    if((cdtFlg) && (millis()-prev<cdt)){
      cdSec=((cdt+prev-millis())/1000)%60;
      cdMin=(cdt+prev-millis())/60000;
      digitalWrite(swl,cdSec%2);
      Display.setNumber((cdMin*100)+cdSec,2);
      Display.refreshDisplay();
    }else if((cdtFlg) || (analogRead(cs)<minThr) || (analogRead(cs)>maxThr)){
      Display.blank();
      Display.refreshDisplay();
      digitalWrite(rly,rlyTrg);
      digitalWrite(swl,HIGH);
      cdtFlg=false;
      delay(2000);
      if(prev!=0){
        prev=0;
      }
      while((analogRead(cs)<minThr) || (analogRead(cs)>maxThr)){
        segTime();
      }
    }
    if((!cdtFlg) && (analogRead(cs)>minThr) && (analogRead(cs)<maxThr)){
      if(prev==0){
        prev=millis();
      }else if(millis()-prev>wait){
        digitalWrite(rly,!rlyTrg);
        digitalWrite(swl,LOW);
        delay(2000);
        swFlg=false;
        break;
      }
    }
  }
}