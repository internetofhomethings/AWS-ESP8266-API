#include <ESP8266WiFi.h>

#include <AmazonDynamoDBClient.h>
#include <AmazonSNSClient.h>
#include <AWSClient.h>
#include <AWSClient2.h>
#include <DeviceIndependentInterfaces.h>
#include <jsmn.h>
#include <sha256.h>
#include <Utils.h>

#include "AmazonDynamoDBClient.h"
#include "Esp8266AWSImplementations.h"
#include "AWSFoundationalTypes.h"
#include <stdlib.h>
#include "keys.h"

extern "C" {
#include "user_interface.h"
}

/*
 *
 *  This sample uses GetItem on a DynamoDB table to retrieve the RGB state
 *  and SetItem to set the RGB state.
 *
 * For this demo to work you must have keys.h/.ccp files that contain your AWS
 * access keys and define "awsSecKey" and "awsKeyID", a DynamoDB table with the
 * name defined by the constant TABLE_NAME with hash and range keys as defined
 * by constants HASH_KEY_NAME/RANGE_KEY_NAME, and and item in that table with
 * attributes as defined by HASH_KEY_VALUE/RANGE_KEY_VALUE and number
 * attributes R G and B.
 *
 */


//GPIO pin assignments
#define LED_IND 16      // LED used for initial code testing (not included in final hardware design)
                        // If present, the LED is initially ON and is set OFF during the setup() function 

// Serial baud and server port (customize to your liking)
#define SERBAUD 74880
#define SVRPORT 9703

/* Contants describing DynamoDB table and values being used. */
static const char* HASH_KEY_NAME = "DemoName";
static const char* HASH_KEY_VALUE = "Colors";
static const char* RANGE_KEY_NAME = "id";
static const char* RANGE_KEY_VALUE = "1";
static const char* TABLE_NAME = "AWSArduinoSDKDemo";
static const int KEY_SIZE = 2;
/* Constants for connecting to DynamoDB. */
static const char* AWS_REGION = "us-west-2";   //<<---CHANGE IF NECESSARY
static const char* AWS_ENDPOINT = "amazonaws.com";

/////////////////////////////////////////////////////////////////
//Network Parameters: You must customize for your network
/////////////////////////////////////////////////////////////////
const char* ssid = "YOUR SSID";                //<<---CHANGE AS NECESSARY
const char* password = "YOURWIFIPASSWORD";     //<<---CHANGE AS NECESSARY
const IPAddress ipadd(192,168,0,174);          //<<---CHANGE AS NECESSARY    
const IPAddress ipgat(192,168,0,1);            //<<---CHANGE AS NECESSARY      
const IPAddress ipsub(255,255,255,0);     

char szInfo[80];
char szT[40];

/* Device independent implementations required for AmazonDynamoDBClient to
 * function. */
Esp8266HttpClient httpClient;
Esp8266DateTimeProvider dateTimeProvider;

AmazonDynamoDBClient ddbClient;

PutItemInput putItemInput;    //Put Item
GetItemInput getItemInput;    //Get Item
AttributeValue hashKey;
AttributeValue rangeKey;
ActionError actionError;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(SVRPORT);
WiFiClient client;
WiFiClient client2;


void startWIFI(void) {
  //set IP if not correct
  IPAddress ip = WiFi.localIP();
  //if( (ip[0]!=ipadd[0]) || (ip[1]!=ipadd[1]) || (ip[2]!=ipadd[2]) || (ip[3]!=ipadd[3]) ) { 
  if( ip!= ipadd) { 
      WiFi.config(ipadd, ipgat, ipsub);  
      Serial.println();
      delay(10);
      Serial.print("ESP8266 IP:");
      delay(10);
      Serial.println(ip);
      delay(10);
      Serial.print("Fixed   IP:");
      delay(10);
      Serial.println(ipadd);
      delay(10);
      Serial.print("IP now set to: ");
      delay(10);
      Serial.println(WiFi.localIP());
      delay(10);
  }
  // Connect to WiFi network
  Serial.println();
  delay(10);
  Serial.println();
  delay(10);
  Serial.print("Connecting to ");
  delay(10);
  Serial.println(ssid);
  delay(10); 
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("ESP8266 IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("ESP8266 WebServer Port: ");
  Serial.println(SVRPORT);
  delay(300);

}

void setup() {
    /* Begin serial communication. */
    Serial.begin(SERBAUD);
    Serial.println("Starting Setup");

    startWIFI();
 
    /* Initialize ddbClient. */
    ddbClient.setAWSRegion(AWS_REGION);
    ddbClient.setAWSEndpoint(AWS_ENDPOINT);
    ddbClient.setAWSSecretKey(awsSecKey);
    ddbClient.setAWSKeyID(awsKeyID);
    ddbClient.setHttpClient(&httpClient);
    ddbClient.setDateTimeProvider(&dateTimeProvider);

    /* Use GPIO16 for LED Indicator */
    pinMode(LED_IND , OUTPUT);
    digitalWrite(LED_IND, 0); //Turn LED OFF (Setup completed)
}

void yieldEspCPU(void) {
    delay(100);
    ESP.wdtFeed(); 
    yield();
}

void getColors(int *R, int *G, int *B, GetItemOutput getItemOutput) {
        char szC[6];
        Serial.println("GetItem succeeded!");
        yieldEspCPU(); 

        /* Get the "item" from the getItem output. */
        MinimalMap < AttributeValue > attributeMap = getItemOutput.getItem();
        AttributeValue av;
        
        // Get the rgb values and set the led with them. //
        attributeMap.get("R", av);
        *R = atoi(av.getS().getCStr());
        Serial.print("Red value read:   "); 
        Serial.println(*R);

        attributeMap.get("G", av);
        *G = atoi(av.getS().getCStr());
        Serial.print("Green value read: "); 
        Serial.println(*G);

        attributeMap.get("B", av);
        *B = atoi(av.getS().getCStr());
        Serial.print("Blue value read:  "); 
        Serial.println(*B);

        delay(10);
        Serial.print("\n\n");
}

void awsGetRGB(int *R, int *G, int *B) {
    
    // Set the string and number values for the range and hash Keys, respectively.
    hashKey.setS(HASH_KEY_VALUE);
    rangeKey.setN(RANGE_KEY_VALUE);

    //////////////////////////////////////////////////////////////////////////////////////////
    // Create key-value pairs out of the hash and range keys, and create a map out off them, 
    // which is the key. 
    //////////////////////////////////////////////////////////////////////////////////////////
    MinimalKeyValuePair < MinimalString, AttributeValue > pair1(HASH_KEY_NAME, hashKey);
    MinimalKeyValuePair < MinimalString, AttributeValue > pair2(RANGE_KEY_NAME, rangeKey);
    MinimalKeyValuePair<MinimalString, AttributeValue> keyArray[] = { pair1, pair2 };
    getItemInput.setKey(MinimalMap < AttributeValue > (keyArray, KEY_SIZE));

    // Looking to get the R G and B values 
    MinimalString attributesToGet[] = { "R", "G", "B" };
    getItemInput.setAttributesToGet(MinimalList < MinimalString > (attributesToGet, 3));

    // Set Table Name
    getItemInput.setTableName(TABLE_NAME);

    // Perform getItem and check for errors. 
    GetItemOutput getItemOutput = ddbClient.getItem(getItemInput, actionError);

    //////////////////////////////////////////////////////////
    // AWS DynamoDB get/set serial message header
    //////////////////////////////////////////////////////////

    ddbClient.getESPtime(szInfo);  //debug getTime
    Serial.print("------------------------------------------\n");
    Serial.print("Loop start time:  ");
    Serial.print(szInfo);
    Serial.print(" GMT\n");
    
    Serial.print("Current Free Heap:");
    Serial.println(system_get_free_heap_size());
    Serial.println("------------------------------------------\n");
    
    //////////////////////////////////////////////////////////
    // AWS HTTP Request/Response (uncomment for debug)
    //////////////////////////////////////////////////////////
    /************************************************
    Serial.println("AWS Request:\n--------------\n");
    Serial.println(ddbClient.szR); //HTTP Request

    Serial.println("AWS Reply:\n--------------\n");
    Serial.println(ddbClient.szResponse); //HTTP Response
    *************************************************/
    
    switch (actionError) {
    case NONE_ACTIONERROR:
        getColors(R,G,B,getItemOutput);
        break;
        
    case INVALID_REQUEST_ACTIONERROR:
        Serial.print("ERROR: ");
        Serial.println(getItemOutput.getErrorMessage().getCStr());
        break;
    case MISSING_REQUIRED_ARGS_ACTIONERROR:
        Serial.println("ERROR: Required arguments were not set for GetItemInput");
        break;
    case RESPONSE_PARSING_ACTIONERROR:
        Serial.println("ERROR: Problem parsing http response of GetItem\n");
        break;
    case CONNECTION_ACTIONERROR:
        Serial.println("ERROR: Connection problem");
        break;
    }
}

void awsSetRGB(int *R, int *G, int *B) {
    //Now lets change the RGB color values and put them to DynamoDB
    // Create an Item. //
    AttributeValue deviceValue;
    deviceValue.setS(HASH_KEY_VALUE);
    AttributeValue rValue;
    AttributeValue gValue;
    AttributeValue bValue;
    //Increment and set color
    String(++*R).toCharArray(szT,10);
    rValue.setS(szT);
    String(++*G).toCharArray(szT,10);
    gValue.setS(szT);
    String(++*B).toCharArray(szT,10);
    bValue.setS(szT);

    MinimalKeyValuePair < MinimalString, AttributeValue > att1(HASH_KEY_NAME, hashKey);
    MinimalKeyValuePair < MinimalString, AttributeValue > att2(RANGE_KEY_NAME, rangeKey);
    MinimalKeyValuePair < MinimalString, AttributeValue > att3("R", rValue);
    MinimalKeyValuePair < MinimalString, AttributeValue > att4("G", gValue);
    MinimalKeyValuePair < MinimalString, AttributeValue > att5("B", bValue);
    MinimalKeyValuePair < MinimalString, AttributeValue > itemArray[] = { att1, att2, att3, att4, att5};

    // Set values for putItemInput. //
    putItemInput.setItem(MinimalMap < AttributeValue > (itemArray, 5));
    putItemInput.setTableName(TABLE_NAME);

    // perform putItem and check for errors. //
    PutItemOutput putItemOutput = ddbClient.putItem(putItemInput, actionError);
    switch (actionError) {
        case NONE_ACTIONERROR:
            Serial.println("PutItem succeeded!");
            Serial.print("Red set to:       "); 
            Serial.println(*R);
            Serial.print("Green set to:     "); 
            Serial.println(*G);
            Serial.print("Blue set to:      "); 
            Serial.println(*B);
            Serial.print("\n"); 
=ojhv             break;
        case INVALID_REQUEST_ACTIONERROR:
            Serial.print("ERROR: ");
            Serial.println(putItemOutput.getErrorMessage().getCStr());
            break;
        case MISSING_REQUIRED_ARGS_ACTIONERROR:
            Serial.println(
                    "ERROR: Required arguments were not set for PutItemInput");
            break;
        case RESPONSE_PARSING_ACTIONERROR:
            Serial.println("ERROR: Problem parsing http response of PutItem");
            break;
        case CONNECTION_ACTIONERROR:
            Serial.println("ERROR: Connection problem");
            break;
    }
}
/////////////////////////////////////////////////////////////////////////////////
// Sketch loop(): 
// 1. Gets current RGB values and output to serial port 
// 2. Sets RGB values (current value + 1) and output set value to  serial port 
// 3. yieldEspCPU() prevents ESP timeout resets
/////////////////////////////////////////////////////////////////////////////////
void loop() {
    static int R,G,B;
    yieldEspCPU();  
    awsGetRGB(&R,&G,&B);
    yieldEspCPU(); 
    awsSetRGB(&R,&G,&B);
    yieldEspCPU();
}
