#include <Servo.h>
//DFRobot sd card module pins: MISO=50, SCK=52, SS=53, MOSI=51 (on arduino mega 2560)
//DFRobot sd card module uses SPI interface only!!!!
#include <SPI.h>
#include <SD.h>
//DS3231 RTC Module libraries:
#include <RTClib.h>
//DHT11 temprature & humidity module library
#include <dht.h>
//HC-12 RF Module library
#include <SoftwareSerial.h>

//variables for HC12 RF Module
SoftwareSerial HC12(10, 11); // HC-12 TX Pin, HC-12 RX Pin
/*
TO CONFIGURE THE HC12 MODULE: connect the “Set” pin of the module to Ground or any digital pin of the Arduino and set the pin to low logic level.!!!!
AT Commands:
1. AT – Test command.
Example: Send “AT” to module, and the module returns “OK”.
2. AT+Bxxxx – Change the serial port baud rate.
Available baud rates: 1200 bps, 2400 bps, 4800 bps, 9600 bps, 19200 bps, 38400 bps, 57600 bps, and 115200 bps. Default: 9600 bps.
Example: Send “AT+B38400” to module, and the module returns “OK+B19200”.
3. AT+Cxxxx – Change wireless communication channel, from 001 to 100.
Default: Channel 001, with working frequency of 433.4MHz. Each next channel is 400KHz higher.
Example: If we want to set the module to channel 006, we need to send “AT+C006” command to the module, and the module will return “OK+C006”. The new working frequency will be 435.4MHz.
*/

//variables for DHT11 temprature & humidity Module
dht DHT;
#define DHT11_PIN 46

//SD module variables
File distance_file;
const int chipSelect = 10;

//variables for DS3231 RTC Module
RTC_DS3231 rtc;
//String current_time;

//variables for sr-04 module
int trigPin = 25;
int echoPin = 27;
long duration, cm;
int distance;

//variables for sg-90 servo module
Servo sonar_servo;  //create servo object to control a servo --this is for the sonar with sr-04 module
// twelve servo objects can be created on most boards
int pos = 0;    // variable to store the servo position
int sonarServoPin = 49;

//variables for buzzer module
int buzzerPin = 3;

//H-Bridge module and motors variables
int E1 = 6;
int M1 = 7;
int E2 = 5;
int M2 = 4;

void motorsControl (String control)
{
  //H-Bridge code:
  //control = <value>,<b/f>  //when b = backward, f = forward
  int value = (control.substring(0,control.indexOf(',')).toInt());
  String d = control.substring(control.indexOf(',') + 1, control.length()); //direction
  //int value = control.toInt();
  /*

    E1,E2: Motor Enable Pin（PWM Control）
    M1,M2: Motor Signal Pin. Eg: M1 = 0,the motor rotates in forward direction. M1 = 1,the motor rotates in back direction.
    E      M          RUN
    LOW   LOW/HIGH    STOP
    HIGH  HIGH        Back Direction
    HIGH  LOW         Forward direction
    PWM   LOW/HIGH    Speed

Note: LOW = 0; HIGH = 1; PWM = 0~255
*/

   
   if (d.equals("f") && value <= 255 && value >= 0) //Forward direction
   {
    digitalWrite(M1,LOW);      //HIGH = Back Direction, LOW = Forward direction
    digitalWrite(M2,LOW);     //HIGH = Back Direction, LOW = Forward direction
    analogWrite(E1, value);   //PWM Speed Control
    analogWrite(E2, value);   //PWM Speed Control
   }
   else if (d.equals("b") && value <= 255 && value >= 0) //Back Direction
   {
    digitalWrite(M1,HIGH);      //HIGH = Back Direction, LOW = Forward direction
    digitalWrite(M2,HIGH);     //HIGH = Back Direction, LOW = Forward direction
    analogWrite(E1, value);   //PWM Speed Control
    analogWrite(E2, value);   //PWM Speed Control
   }
   else //bad data --> save to error report & print to serial
   {
    String error = "System Error: Motors control got bad direction. \r\nDirection data: " + d + "\r\nPWM Value: " + value ;
    Serial.println(error);
    print_to_error_file (error); //stops the system
   }
   
   
   
   //for debug:
   //Serial.println("value = " + String(value) + " ,string = " + control + " ,direction = " + d);
}

void takeAction(String action) 
{
  if (action.equals("activate_alarm"))
  {
    activate_panic_alarm ();
  }
  if (action.equals("deactivate_alarm"))
  {
    deactivate_panic_alarm ();
  }
  if (action.equals("tx_dist_to_RF"))
  {
    tx_distance_to_RF ();
  }
  if (action.equals("tx_temperature_to_RF"))
  {
    tx_temperature_to_RF ();
  }
  if (action.equals("tx_GPS_to_RF"))
  {
    tx_GPS_to_RF ();
  }
  if (action.equals("scan_dist_to_SD"))
  {
    scan_distance_to_SD ();
  }
  if (action.equals("tx_RTC_to_RF"))
  {
    tx_RTC_to_RF ();
  }
  if (action.equals("tx_radius_file_data_over_RF")) 
  {
    tx_radius_file_data_over_RF ();
  }/*
  if (action.equals("print_SD_file_data_over_RS232 (String file_name)")) 
  {
    print_SD_file_data_over_RS232 (String file_name);
  }
  */
  if (action.equals("tx_temp_file_data_over_RF")) 
  {
    tx_temp_file_data_over_RF ();
  }
  if (action.equals("tx_error_file_data_over_RF")) 
  {
    tx_error_file_data_over_RF ();
  }
  if (action.equals("system_selfTest"))
  {
    system_selfTest ();
  }
  if (action.equals("tx_dist_to_serial"))
  {
    scan_distance_to_serial_port ();
  }
  if (action.equals("tx_temp_to_RF"))
  {
    tx_temperature_to_RF ();
  }
  if (action.equals("save_temp_to_SD"))
  {
    save_temperature_to_SD ();
  }
}




void tx_distance_to_RF ()
{
  HC12.println("Current GPS coordinates: " + get_gps_coordiantes_string());
  HC12.print(scan_radius_distance_to_string());
}

void tx_temperature_to_RF ()
{
  HC12.println("Current GPS coordinates: " + get_gps_coordiantes_string());
  HC12.print(get_temperature_string());
}

void tx_RTC_to_RF ()
{
  HC12.println(get_current_time_from_rtc ());
}

void tx_GPS_to_RF ()
{
  HC12.println("Current GPS coordinates: " + get_gps_coordiantes_string());
}

String get_temperature_string ()
{
  int chk = DHT.read11(DHT11_PIN);
  String str = "Temperature = " + String(DHT.temperature) + "°C\n" + "Humidity = " + String(DHT.humidity) + "%";
  return str;
}

String get_current_time_from_rtc ()
{
  if (!rtc.begin()) //for debug
  {
    Serial.println("debug message: Couldn't find RTC");
    HC12.write("debug message: Couldn't find RTC");
    print_to_error_file ("Error: Couldn't find RT");
  }
  DateTime dt = rtc.now();
  //String current_time = "Date: ";// + (dt.day(),DEC);// + "/" + (dt.month(),DEC) + "/" + (dt.year(),DEC) + "\n" + "Hour : " + (dt.hour(),DEC) + ":" + (dt.minute(),DEC) + "." + (dt.second(),DEC);
  char time_buf[] = "hh:mm:ss";
  char date_buf[] = "DD/MM/YYYY";
  String date = dt.toString(date_buf), timeb = dt.toString(time_buf);
  String current_time = "Date: " + date + " Time: " + timeb;

  return current_time;
}

int calculateDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  return distance;
}

void scan_distance_to_serial_port ()
{
  Serial.println("current GPS coordinates: " + get_gps_coordiantes_string());
  Serial.println(scan_radius_distance_to_string());
}

String scan_radius_distance_to_string ()
{
  String radius_distance = "";

  for (pos = 0; pos <= 180; pos += 1) // goes from 0 degrees to 180 degrees with 20 degrees steps
  {
    sonar_servo.write(pos);       // tell servo to go to position in variable 'pos'
    delay(15);                   // waits 15ms for the servo to reach the position
    if (pos % 20 == 0)
    {
      cm = calculateDistance(); //calculate distance in cm units
      radius_distance = radius_distance + "position of " + String(pos) + "° " + String(cm) + " cm \n";
      delay(20);
    }
  }
  sonar_servo.write(0); //return to start position
  return radius_distance;
}

void scan_distance_to_SD ()
{
  //file name in SD can be maximum 8 chars!!
  String distance_file_name = "radius.txt"; //distance data file name on SD card
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  distance_file = SD.open(distance_file_name, FILE_WRITE);
  // if the file opened okay, write to it:
  if (distance_file)
  {
    //Serial.println("Writing to " + distance_file_name); //for debug
    distance_file.println(get_current_time_from_rtc ());
    distance_file.println("GPS coordinates: " + get_gps_coordiantes_string() + " \n"); //first need to write GPS data
    distance_file.println(scan_radius_distance_to_string()); //scan distance to string and print it to the file
    distance_file.println();
    
    // close the file:
    distance_file.close();
    //for debug:
    //Serial.println("print to file on SD card done. \n file closed");
  }
  else 
  {
    // if the file didn't open, print an error:
    //for debug:
    //Serial.println("error opening file: " + distance_file_name);
    print_to_error_file ("error opening file: " + distance_file_name);
  }
}

void save_temperature_to_SD ()
{
  //file name in SD can be maximum 8 chars!!
  String temperature_file_name = "temp.txt"; //distance data file name on SD card
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File temperature_file = SD.open(temperature_file_name, FILE_WRITE);
  // if the file opened okay, write to it:
  if (temperature_file)
  {
    //Serial.println("Writing to " + temperature_file_name); //for debug
    temperature_file.println(get_current_time_from_rtc ());
    temperature_file.println("GPS coordinates: " + get_gps_coordiantes_string() + " \n"); //first need to write GPS data
    temperature_file.println(get_temperature_string ()); //get temerature data string and print it to the file
    temperature_file.println();
    
    // close the file:
    temperature_file.close();
    //for debug:
    //Serial.println("print to file on SD card done. \n file closed");
  }
  else 
  {
    // if the file didn't open, print an error:
    //for debug:
    //Serial.println("error opening file: " + temperature_file_name);
    print_to_error_file ("error opening file: " + temperature_file_name);
  }
}

String get_gps_coordiantes_string()
{
  String gps_coordiantes = "";
  //need to add code to interface with the GPS module

  return gps_coordiantes;
}

void print_error_file_data_over_RS232 ()
{
  String file_name = "errors.txt";
  File error_file = SD.open(file_name);
  if (error_file)
  {
    Serial.println("file name:" + file_name);
    // read from the file until there's nothing else in it:
    while (error_file.available())
    {
      Serial.write(error_file.read());
    }
    // close the file:
    error_file.close();
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("error opening the file: " + file_name + " from SD card.");
  }
}

void print_radius_file_data_over_RS232 ()
{
  String file_name = "radius.txt";
  File error_file = SD.open(file_name);
  if (error_file)
  {
    Serial.println("file name: " + file_name);
    // read from the file until there's nothing else in it:
    while (error_file.available())
    {
      Serial.write(error_file.read());
    }
    // close the file:
    error_file.close();
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("error opening the file: " + file_name + " from SD card.");
  }
}

void tx_radius_file_data_over_RF ()
{
  String file_name = "radius.txt";
  File send_file = SD.open(file_name);
  if (send_file)
  {
    HC12.println("file name: " + file_name);
    // read from the file until there's nothing else in it:
    while (send_file.available())
    {
      HC12.write(send_file.read());
    }
    // close the file:
    send_file.close();
  }
  else
  {
    // if the file didn't open, print an error:
    HC12.println("error opening the file: " + file_name + " from SD card.");
  }
}

void tx_error_file_data_over_RF ()
{
  String file_name = "errors.txt";
  File send_file = SD.open(file_name);
  if (send_file)
  {
    HC12.println("file name: " + file_name);
    // read from the file until there's nothing else in it:
    while (send_file.available())
    {
      HC12.write(send_file.read());
    }
    // close the file:
    send_file.close();
  }
  else
  {
    // if the file didn't open, print an error:
    HC12.println("error opening the file: " + file_name + " from SD card.");
  }
}

void tx_temp_file_data_over_RF ()
{
  String file_name = "temp.txt";
  File send_file = SD.open(file_name);
  if (send_file)
  {
    HC12.println("file name: " + file_name);
    // read from the file until there's nothing else in it:
    while (send_file.available())
    {
      HC12.write(send_file.read());
    }
    // close the file:
    send_file.close();
  }
  else
  {
    // if the file didn't open, print an error:
    HC12.println("error opening the file: " + file_name + " from SD card.");
  }
}

void print_to_error_file (String error_str)
{
  //get error string and print it in the error file on SD card (include time stamp of error)
  String error_file_name = "errors.txt"; //distance data file name on SD card

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File error_file = SD.open(error_file_name, FILE_WRITE);

  // if the file opened okay, write to it:
  if (error_file)
  {
    //Serial.println("Writing to " + error_file_name); //for debug
    error_file.println(get_current_time_from_rtc()); //first need to write current time
    error_file.println(error_str); //print the error string to the file
    error_file.println();
    
    // close the file:
    error_file.close();
    //Serial.println("print to file on SD card done. \n file closed"); //for debug
  }
  else 
  {
    // if the file didn't open, print an error:
    Serial.println("error opening file: " + error_file_name);
  }
}

void activate_panic_alarm ()
{
  tone(buzzerPin, 3000); // Send 3KHz sound signal
}

void deactivate_panic_alarm ()
{
  noTone(buzzerPin); // Stop sound signal
  digitalWrite(buzzerPin, HIGH);
}

void system_selfTest ()
{
  //RTC
  if (!rtc.begin())
  {
    Serial.println("Error on SelfTest: RTC is NOT running!");
    print_to_error_file ("Error on SelfTest: RTC is NOT running!");
  }
  //SD
  if (!SD.begin())  //debug SD module status
  {
    Serial.println("Error on POST: SD initialization failed!");
  }
  else
  {
    Serial.println("SD initialization done.");
  }
}

void delete_old_files_from_sd ()
{
  if (!SD.begin())  //debug SD module status
  {
    Serial.println("Error on POST: SD initialization failed!");
  }
  else
  {
    if (SD.exists("errors.txt"))
    {
      SD.remove("errors.txt");
    }
    if (SD.exists("radius.txt"))
    {
      SD.remove("radius.txt");
    }
    if (SD.exists("temp.txt"))
    {
      SD.remove("temp.txt");
    }
  }
}

void setup() 
{
  // put your setup code here, to run once:

  // Set sketch compiling time to RTC DS3231 module
    rtc.begin();
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //serial port begin
  Serial.begin(9600);

  //set i/o pins mode for modules
  pinMode(trigPin, OUTPUT); //for sr-04 module
  pinMode(echoPin, INPUT); //for sr-04 module
  sonar_servo.attach(sonarServoPin);  // attaches the servo on pin 9 to the servo object
  pinMode(buzzerPin, OUTPUT); // Set buzzer - pin 9 as an output
  digitalWrite(buzzerPin, HIGH); //silent the buzzer
  pinMode(M1, OUTPUT); //for H-Bridge module
  pinMode(M2, OUTPUT); //for H-Bridge module

  //SD module start
  SD.begin();
  delete_old_files_from_sd (); //if old logs exist on SD card --> delete them

  // Serial port to HC12
  HC12.begin(9600);
          
  system_selfTest ();
  delay(2000);
}

void loop() 
{
  // put your main code here, to run repeatedly:
  //Serial.println(get_current_time_from_rtc());
  delay(250);
  //Serial.println(String(__DATE__) + " , " + String(__TIME__));

  String HC12_readBuffer = "";
  boolean startAction = false; //flag for actions
  boolean isAction = false; //flag to identify if data is action (true) or motors control (false)
  boolean startMotors = false; //flag for motors control
  boolean gotPause = false; //flag to get zero value for motors only 1 time
  // Reads the incoming data
  while (HC12.available()) // If HC-12 has data
  {             
    byte incomingByte = HC12.read();          // Store each icoming byte from HC-12
    delay(5);

    //read actions protocol:
    // Reads the data between the start "(" and end markers ")"
    if (startAction == true) 
    {
      if (incomingByte != ')') 
      {
        HC12_readBuffer += char(incomingByte);    // Add each byte to ReadBuffer string variable
      }
      else 
      {
        startAction = false;
      }
    }
    // Checks whether the received message statrs with the start marker "("
    else if ( incomingByte == '(') 
    {
      startAction = true; // If true start reading the message
      isAction = true;
    }

    //read motor controls protocol:
    //// Reads the data between the start "<" and end markers ">"
    if (startMotors == true) 
    {
      if (incomingByte != '>') 
      {
        HC12_readBuffer += char(incomingByte);    // Add each byte to ReadBuffer string variable
      }
      else 
      {
        startMotors = false;
      }
    }
    // Checks whether the received message statrs with the start marker "<"
    else if ( incomingByte == '<') 
    {
      startMotors = true; // If true start reading the message
      isAction = false;
    }
  }

  if(!HC12_readBuffer.equals("")) //send data over RF just if there is data
  {
    if (isAction)
    {
      takeAction(HC12_readBuffer);
    }
    else
    {
      if (HC12_readBuffer.toInt() == 0 && !gotPause) //Pause
      {
        gotPause = true;
        motorsControl(HC12_readBuffer);
      }
      if (HC12_readBuffer.toInt() != 0) //Control speed
      {
        motorsControl(HC12_readBuffer);
        gotPause = false;
      }
    }
  }
  
  
  


}
