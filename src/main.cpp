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
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "VROOM VROOM FOR FREE NOW!"; // your network SSID (name)
char pass[] = "VROOMVROOMTHISISYOURDOOM";  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;
int speed = 0;
int rotation = 0;

Servo speedServo;
Servo rotationServo;

int status = WL_IDLE_STATUS;
WiFiServer server(80);
void printWiFiStatus();

void setup()
{
  speedServo.attach(9);
  rotationServo.attach(10);
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Access Point Web Server");

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

  // by default the local IP address will be 192.168.4.1
  // you can override it with the following:
  // WiFi.config(IPAddress(10, 0, 0, 1));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING)
  {
    Serial.println("Creating access point failed");
    // don't continue
    while (true)
      ;
  }

  // wait 10 seconds for connection:
  delay(5000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();
}

void loop()
{
  // compare the previous status to the current status
  if (status != WiFi.status())
  {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED)
    {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    }
    else
    {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }

  WiFiClient client = server.available(); // listen for incoming clients

  if (client)
  {
     // if you get a client,
    Serial.println("new client"); // print a message out the serial port
    String currentLine = "";      // make a String to hold incoming data from the client
    while (client.connected())
    {                        // loop while the client's connected
      delayMicroseconds(10); // This is required for the Arduino Nano RP2040 Connect - otherwise it will loop so fast that SPI will never be served.
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
            client.print("LeftRightometer <input class='vertigo' type='range' min='85' max='100' value='95'></input><br>");
            client.print("Consumed Speed <input class='horizonto' type='range' min='85' max='100' value='95'></input><br>");

            char syncSpeedJS[] = "<script>async function syncSpeed(speed) { let response = await fetch('http://192.168.4.1/speed/' + speed); if(!response.ok) console.error(' HTTP - Error : ' + response.status); }</script>";
            char syncRotationJS[] = "<script>async function syncRotation(rotation) { let response = await fetch('http://192.168.4.1/rotation/' + rotation); if(!response.ok) console.error(' HTTP - Error : ' + response.status);}</script>";
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

        Serial.print(clientRequests);
        int speedIndex = clientRequests.indexOf("/speed/");
        int rotationIndex = clientRequests.indexOf("/rotation/");

        Serial.println(speedIndex);

        if (speedIndex != -1)
        {
          String stringer = clientRequests.substring(speedIndex + 7);

          for (int i = 0; i < stringer.length(); i++)
          {
            if (stringer[i] == '/')
            {
              stringer = stringer.substring(0, i);
              break;
            }
          }

          int speedValue = clientRequests.substring(speedIndex + 7, speedIndex + 7 + stringer.length()).toInt();

          Serial.println(speedValue);

          if (speedValue >= -255 && speedValue <= 255)
          {
            speed = speedValue;
            speedServo.write(speed);
          }
        }
        if (rotationIndex != -1)
        {
          String stringer = clientRequests.substring(rotationIndex + 7);

          for (int i = 0; i < stringer.length(); i++)
          {
            if (stringer[i] == '/')
            {
              stringer = stringer.substring(0, i);
              break;
            }
          }

          int rotationValue = currentLine.substring(rotationIndex + 7, rotationIndex + 7 + stringer.length()).toInt();

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
void printWiFiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}