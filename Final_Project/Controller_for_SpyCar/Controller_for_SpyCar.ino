#include <Wire.h>
#include "SparkFun_Qwiic_Joystick_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_joystick

//HC-12 RF Module library
#include <SoftwareSerial.h>

//variables for HC12 RF Module
SoftwareSerial HC12(10, 11); // HC-12 TX Pin, HC-12 RX Pin

//variables for Joystick module
uint8_t Address = 0x20; //Start address (Default 0x20)
JOYSTICK joystick; //Create instance of this object

String lastControlWord = ""; //variable for check if last control is the same to not send data in loop

void setup() 
{
  Serial.begin(9600);

  HC12.begin(9600);

  if(joystick.begin(Wire, Address) == false)
  {
    Serial.println("Joystick does not appear to be connected. Please check wiring. Freezing...");
    while(1);
  }
}

void loop() 
{
    int X = joystick.getHorizontal();
    int Y = joystick.getVertical();
    int B = joystick.getButton();
    String control = "<"; //control = <motors_speed,motors_direction.front_wheel_angle>
    int value;
    

    int mapX = 0;
    int mapY = 0;

    if  (Y > 504)
    {
        mapY = 255;//map(Y, 505, 1023, 0, 255); //forward - H-Bridge module
        control = control + String(mapY) + ",f.";
    }
    else if (Y < 504)
    {
        mapY = 255;//map(Y, 503, 0, 0, 255); //backward - H-Bridge module
        control = control + String(mapY) + ",b.";
    }
    else if (Y == 504)
    {
      mapY = 0;
      control = control + String(mapY) + ",f.";
    }
    
    if  (X > 514)
    {
        mapX = 45;//map(X, 515, 1023, 90, 180); //right - servo angle
    }
    else if (X < 514)
    {
        mapX = 135;//map(X, 0, 513, 0, 90); //left - servo angle
    }
    else if(X == 514)
    {
      mapX = 85;//90;
    }
    control = control + String(mapX) + ">";
    

    if (B == 0)
    {
        control = "<0,f.90>"; //barkes and servo angle = 90 degrees
    }

//send data control to spyCar just if joyStic changes values
    if (!lastControlWord.equals(control))
    {
      HC12.println(control);
      lastControlWord = control;
    }

//send and recieve commands from PC app Serial to RF
  while (HC12.available()) // If HC-12 has data
  {        
    Serial.write(HC12.read());      // Send the data to Serial monitor
  }
  
  while (Serial.available()) // If Serial monitor has data
  {      
    HC12.write(Serial.read());      // Send that data to HC-12
  }

    //for debug:
    //Serial.println(control);

}
