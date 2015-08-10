<h2><strong>AWS API for ESP8266 Development Software</strong></h2>

This project interface the ESP8266 with AWS DynamoDB using the Arduino IDE platform.

Setup:

1. Copy the ASW folder to your Arduino "../libraries" folder.
2. Copy the AWS_Get-SetDemo folder to your Arduino IDE sketchbook folder.

Operation:

The ESP8266 sketch:
1. setup() - connects to your local Wifi and initializes an AWS database client.
           - a web server is also started, but is not used for this demo sketch
           
2. loop()  - first gets the current RGB values from a dynamoDB table
           - then sets the dynamoDB table RGB values to an increment of the value from the 'get'

The current settings (change if needed in the sketch to match your configuration):

ESP8266 Static IP: 192.168.0.174
ESP8266 Server Port: 9703
Router IP: 192.168.0.1

Serial port baud: 74880 bps (Use ESPlorer to monitor ESP8266 serial port)

AWS Settings:

The AWS Dynamodb used in this example was based on the following:

https://github.com/awslabs/aws-sdk-arduino

See section "Table used by SparkGetItemSample and GalileoSample:"


The serial port output from this request will look similar to this:

Loop start time:  20150810210943 GMT<br>
Current Free Heap:21864

GetItem succeeded!<br>
Red value read:   125<br>
Green value read: 107<br>
Blue value read:  107<br>


PutItem succeeded!<br>
Red set to:       126<br>
Green set to:     108<br>
Blue set to:      108<br>

Loop start time:  20150810211000 GMT<br>
Current Free Heap:21864

GetItem succeeded!<br>
Red value read:   126<br>
Green value read: 108<br>
Blue value read:  108<br>


PutItem succeeded!<br>
Red set to:       127<br>
Green set to:     109<br>
Blue set to:      109<br>

Two iterations of the loop() function are shown above. Each iteration has-

1. A header with the loop start time in the format: yyyymmddhhmmss GMT
2. The header also shows the free heap, for troubleshooting in case a memory leak is detected.
3. The get values are displayed
4. The get values are incremented and then written (put) to the database

Note the lapse time between iterations is about 17 seconds.
