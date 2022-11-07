/*
  WiFi Web Server LED Blink

  A simple web server that lets you blink an LED via the web.
  This sketch will create a new access point (with no password).
  It will then launch a new server and print out the IP address
  to the Serial Monitor. From there, you can open that address in a web browser
  to turn on and off the LED on pin 13.

  If the IP address of your board is yourAddress:
    http://yourAddress/H turns the LED on
    http://yourAddress/L turns it off

  created 25 Nov 2012
  by Tom Igoe
  adapted to WiFi AP by Adafruit
 */
#include <Arduino.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <Servo.h>
#include <secret.h>
#include <Ultrasonic.h>
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "zenbook"; // your network SSID (name)
char pass[] = laurinsSecurePassword;  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;
int speed = 0;
int rotation = 0;

Servo speedServo;
Servo rotationServo;

int status = WL_IDLE_STATUS;
WiFiServer server(80);
Ultrasonic ultrasonicFront(13);
Ultrasonic ultrasonicBack(12);
int frontDistance;
int backDistance;

void setup()
{
  //attach speedservo to port 9
  speedServo.attach(9);
  //attach rotationservo to port 11
  rotationServo.attach(11);

  Serial.begin(9600); // initialize serial communication

  speedServo.write(97);
  rotationServo.write(97);
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }
  speedServo.write(91);

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid); // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  speedServo.write(0);
  rotationServo.write(0);
  Serial.println(WiFi.localIP());
  server.begin(); // start the web server on port 80
}

void loop()
{
  frontDistance = ultrasonicFront.read();
  backDistance = ultrasonicBack.read();

  if(frontDistance < 25){
    int minSpeed = 60;
    int maxSpeed = 92;

    speed = map(frontDistance, 0, 25, minSpeed, maxSpeed);
    Serial.println("going backwards");
    Serial.println(speed);
    speedServo.write(speed);

  } else if (backDistance < 25) {
    int minSpeed = 92;
    int maxSpeed = 135;

    speed = map(backDistance, 0, 25, minSpeed, maxSpeed);
    Serial.println("going forward");
    Serial.println(speed);
    speedServo.write(speed);
  }

  WiFiClient client = server.available(); // listen for incoming clients

  if (client)
  {
     // if you get a client,
    Serial.println("new client"); // print a message out the serial port
    String currentLine = "";      // make a String to hold incoming data from the client
    while (client.connected())
    {                        // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        if (c == '\n')
        { // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            client.print("<style>.vertigo { position:absolute; -webkit-transform: rotate(270deg); top:100px; } </style>");

            // the content of the HTTP response follows the header:
            client.print("LeftRightometer <input class='vertigo' type='range' min='85' max='100' value='94'></input><br>");
            client.print("Consumed Speed <input class='horizonto' type='range' min='60' max='135' value='92'></input><br>");

            char syncSpeedJS[] = "<script>async function syncSpeed(speed) { let response = await fetch('/speed/' + speed); if(!response.ok) console.error(' HTTP - Error : ' + response.status); }</script>";
            char syncRotationJS[] = "<script>async function syncRotation(rotation) { let response = await fetch('/rotation/' + rotation); if(!response.ok) console.error(' HTTP - Error : ' + response.status);}</script>";
            char speedEventListenerJS[] = "<script>document.querySelector('.vertigo').addEventListener('input', function(e) { syncSpeed(e.target.value); });</script>";
            char rotationEventListenerJS[] = "<script>document.querySelector('.horizonto').addEventListener('input', function(e) { syncRotation(e.target.value); });</script>";

            client.print(syncSpeedJS);
            client.print(syncRotationJS);
            client.print(speedEventListenerJS);
            client.print(rotationEventListenerJS);

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else
          { // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }

        String clientRequests = client.readStringUntil('\r');

        //Serial.print(clientRequests + "\n here we go");
        int speedIndex = clientRequests.indexOf("/speed");
        int rotationIndex = clientRequests.indexOf("/rotation/");
        //Serial.println("\n");
        //Serial.println(speedIndex);

        if (speedIndex != -1)
        {
          //Serial.println("got in here");
          String actualSpeedStringValue = clientRequests.substring(speedIndex + 7);
          //Serial.println(actualSpeedStringValue);
          actualSpeedStringValue = actualSpeedStringValue.substring(0, actualSpeedStringValue.indexOf(" "));

          //Serial.println(actualSpeedStringValue);
          int speedValue = actualSpeedStringValue.toInt();
          
          Serial.println("speed value is:");
          Serial.println(speedValue);

          if (speedValue >= -255 && speedValue <= 255)
          {
            speed = speedValue;
            speedServo.write(speedValue);
          }
        }

        if (rotationIndex != -1)
        {
          
          String stinger = clientRequests.substring(rotationIndex + 10);

          stinger = stinger.substring(0, stinger.indexOf(' '));

          int rotationValue = stinger.toInt();

          if (rotationValue >= -255 && rotationValue <= 255)
          {
            rotation = rotationValue;
            rotationServo.write(rotation);
          }
        }

      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}