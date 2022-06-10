/*--- Includes ---*/
#include <Adafruit_ADS1X15.h>


/*--- Defines ---*/
#define ADS1115_SENSOR_1_ADDR   0x48     //ADS Address GND
#define ADS1115_SENSOR_2_ADDR   0x49     //ADS Address VDD
#define MEASURE                 ":MEAS?"        //SCPI
#define OPEN_CHANNEL            ":SET:OPEN:CH"
#define OPEN_PUMP               ":SET:PUMP:CH"
#define HEATER_ON               ":SET:HTR:ON"
#define HEATER_OFF              ":SET:HTR:OFF"
#define CONFIRM                 Serial.println("Ok");   //Usefull commands;
#define DONE                    Serial.println("Done");

/*--- Function declarations ---*/
String validateOpen(int channelInt,String timeAmmout);
String validatePump(int channelInt,String timeAmmout, int pwmInt);

/*--- Global variables ---*/
Adafruit_ADS1115 Ads1;                //ADS instance no.1
Adafruit_ADS1115 Ads2;                //ADS instance no.2
const int heaterPin = 2;
const int pwmPinsArr[6] = {3,5,6,9,10,11};  //PWM pins array
float resultAds1[4];                //Measurment arrays ADS1
float resultAds2[4];                //Measurment arrays ADS2
//Flags
bool cmdOpenChannel[6] = {false,false,false,false,false,false};
bool cmdOpenPump[6] = {false,false,false,false,false,false};
unsigned long timeOpenChannel[6] = {0,0,0,0,0,0};
unsigned long timeOpenPump[6] = {0,0,0,0,0,0};


void setup() {
  Serial.begin(115200);

  //Initialize PWM                          //Hard coded 6 should be number of PWM pins in array -> sizeof(pwmPinsArr)/sizeof(int) in global scope gives = 2 ???
  for(int i=0; i < 6 ;i++){
    pinMode(pwmPinsArr[i], OUTPUT); 
    analogWrite(pwmPinsArr[i], 0);
  }
  pinMode(heaterPin,OUTPUT); analogWrite(heaterPin,0);

  //Initialize sensors
  if(!Ads1.begin(ADS1115_SENSOR_1_ADDR)){
    Serial.println("Failed to initialize ADS1");
    while(1){
      Serial.println("ERR:ADS1:INIT");
      delay(5000);
    }
  }
  Ads1.setGain(GAIN_TWOTHIRDS);
  
  if(!Ads2.begin(ADS1115_SENSOR_2_ADDR)){
    Serial.println("Failed to initialize ADS2");
    while(1){
      Serial.println("ERR:ADS1:INIT");
      delay(5000);
    }
  }
  Ads2.setGain(GAIN_TWOTHIRDS);

}

void loop() {
  String command;
  if (Serial.available() > 0){
    //Get command
    command = Serial.readStringUntil('\n');
    
    if(command == MEASURE){
      CONFIRM;
      String results;
      results = "ADS1 CH1: " + String(resultAds1[0]) +
                 " ADS1 CH2: " + resultAds1[1] +
                 " ADS1 CH3: " + resultAds1[2] +
                 " ADS1 CH4: " + resultAds1[3] +
                 " ADS2 CH1: " + resultAds2[0] +
                 " ADS2 CH2: " + resultAds2[1] +
                 " ADS2 CH3: " + resultAds2[2] +
                 " ADS2 CH4: " + resultAds2[3];
      Serial.println(results);
      DONE;
    }

    if(command.startsWith(OPEN_CHANNEL)){
      CONFIRM;
      //Split command
      command = command.substring(12);
      int space = command.indexOf(' ');
      //Get channel parameter
      String channel = command.substring(0,space);
      int channelInt = channel.toInt();
      //Get time parameter
      String timeAmmount = command.substring(space);
      
      String validationResult = validateOpen(channelInt,timeAmmount);
      if(validationResult == "OK"){
          cmdOpenChannel[channelInt-1] = true;
          timeOpenChannel[channelInt-1] = millis() + timeAmmount.toInt() * 100;
        analogWrite(pwmPinsArr[channelInt-1], 255);
        //delay(timeAmmount.toInt() * 100);
        //analogWrite(pwmPinsArr[channelInt-1], 0);
        DONE;
      }else{
        Serial.println(validationResult);
      }
    }

    if(command.startsWith(OPEN_PUMP)){
      CONFIRM;
      //Split command
      command = command.substring(12);
      int space1 = command.indexOf(' ');
      int space2 = command.lastIndexOf(' ');
      //Get channel parameter
      String channel = command.substring(0,space1);
      int channelInt = channel.toInt();
      //Get time parameter
      String timeAmmount = command.substring(space1 + 1,space2);
      //Get PWM parameter
      String pwmAmmount = command.substring(space2 + 1);

      //Validate result
      String validationResult = validatePump(channelInt,timeAmmount,pwmAmmount.toInt());
      if(validationResult == "OK"){
          cmdOpenPump[channelInt-1] = true;
          timeOpenPump[channelInt-1] = millis() + timeAmmount.toInt() * 100;
        analogWrite(pwmPinsArr[channelInt-1], pwmAmmount.toInt());
        //delay(timeAmmount.toInt() * 100);
        //analogWrite(pwmPinsArr[channelInt-1], 0);
        DONE;
      }else{
        Serial.println(validationResult);
      }
    }

    if(command.startsWith(HEATER_ON)){
      DONE;
      analogWrite(heaterPin,255);
      CONFIRM;
    }

    if(command.startsWith(HEATER_OFF)){
      DONE;
      analogWrite(heaterPin,0);
      CONFIRM;
    }
  }

  //Gather info from ADS instances
  //First instance (ADS1)
  int16_t measurment;
  float volts;
  for(int i=0; i<4; i++){
    //First instance
    measurment = Ads1.readADC_SingleEnded(i);
    volts = Ads1.computeVolts(measurment);
    resultAds1[i] = volts;
    
    //Second instance
    measurment = Ads2.readADC_SingleEnded(i);
    volts = Ads2.computeVolts(measurment);
    resultAds2[i] = volts;
  }

  for(int i=0;i<6;i++){
    if(cmdOpenChannel[i] && millis() > timeOpenChannel[i]){
      analogWrite(pwmPinsArr[i], 0);
      Serial.println("CLOSE:CH:" + String(i+1));
      timeOpenChannel[i] = 0;
      cmdOpenChannel[i] = false;
    }
    if(cmdOpenPump[i] && millis() > timeOpenPump[i]){  
      analogWrite(pwmPinsArr[i], 0);
      Serial.println("CLOSE:PUMP:" + String(i+1));
      timeOpenPump[i] = 0;
      cmdOpenPump[i] = false;
    }
  }

  command = "";
}

String validateOpen(int channelInt,String timeAmmout){
   if(channelInt < 1 ||  channelInt > 6){
      return "ERR:CH:OUT_OF_RANGE";
   }
   return "OK";
}

String validatePump(int channelInt,String timeAmmout, int pwmInt){
   if(channelInt < 1 ||  channelInt > 6){
      return "ERR:CH:OUT_OF_RANGE";
   }
   if(pwmInt > 255 || pwmInt < 0){
      return "ERR:PWM:OUT_OF_RANGE";
   }
   return "OK";
}
