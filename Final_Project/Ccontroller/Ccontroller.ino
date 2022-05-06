#include <Wire.h>
#include "SparkFun_Qwiic_Joystick_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_joystick

#include <Servo.h>


uint8_t Address = 0x20; //Start address (Default 0x20)

JOYSTICK joystick; //Create instance of this object

Servo myservo;
int pos = 90;

void setup() {
  Serial.begin(9600);
  Serial.println("Qwiic Joystick Example");

  if(joystick.begin(Wire, Address) == false)
  {
    Serial.println("Joystick does not appear to be connected. Please check wiring. Freezing...");
    while(1);
  }

  myservo.attach(40);
}

void loop() 
{
    int X = joystick.getHorizontal();
    int Y = joystick.getVertical();
    int B = joystick.getButton();
    String control = "<";

    int mapX = 0;
    int mapY = 0;

    if  (Y > 504)
    {
        mapY = map(Y, 505, 1023, 0, 255); //forward - H-Bridge module
        control = control + String(mapY) + ",f,";
    }
    else if (Y < 504)
    {
        mapY = map(Y, 503, 0, 0, 255); //backward - H-Bridge module
        control = control + String(mapY) + ",b,";
    }
    else if (Y == 504)
    {
      mapY = 0;
      control = control + String(mapY) + ",f,";
    }
    
    if  (X > 514)
    {
        mapX = map(X, 515, 1023, 90, 180); //right - servo angle
    }
    else if (X < 514)
    {
        mapX = map(X, 0, 513, 0, 90); //left - servo angle
    }
    else if(X == 514)
    {
      mapX = 90;
    }
    control = control + String(mapX) + ">";
    

    if (B == 0)
    {
        Serial.println("Button");
    }

    //Serial.println("X = " + String(X) + " ,Y = " + String(Y));
    //Serial.println("mapX = " + String(mapX) + " ,mapY = " + String(mapY));

    //Serial.println("X = " + String(X) + " ,mapX = " + String(mapX));
    
Serial.println(control);
myservo.write(mapX);
  delay(15);
}
