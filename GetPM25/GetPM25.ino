#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

#define BAUD_RATE 9600
// baud 9600, 33 mS to send 32 bytes
// set timeOut to 100mS

#define ALLOWED_CONNECT_CYCLES 40   // how many esp try to connect wifi, if no wifi, it will sleep after 40 cycles
#define ALLOWED_READ_CYCLES 10      // how long will esp wait for measure temperature, mostly nothing
#define SLEEP_TIME_SECONDS 30       // 900 for 15 minutes, but sometimes it depend on internal resonator

WiFiClient espClient;
// Update these with values suitable for your network.
const char* ssid = "ADB";
const char* password = "0f58775867";
const char* mqtt_domoticz = "192.168.0.210";

int idx1 = 18; // aqi

PubSubClient mqtt_client(espClient);
char msg[256];
char msg1[100];
//char msg2[100];
//char msg3[100];




struct PARAMS {
  float T;//显示温度
  float H;//显示湿度
  unsigned int PM25;//PM2.5
} _params;

//G5 相关变量
static unsigned char ucRxBuffer[256];
static unsigned char ucRxCnt = 0;

//循环计数器
unsigned char loopCnt = 0;

unsigned char PM25_flag = 0;

    unsigned long sum = 0;
    unsigned char avg_ok = 0;
    unsigned char counter = 0;

SoftwareSerial swSer(12, 12, false, 256);

void setup() {
  unsigned char i;
  
  Serial.begin(BAUD_RATE);
  swSer.begin(BAUD_RATE);

  Serial.println("\nSoftware serial test started");
  
  WiFi.disconnect();
  
  setup_wifi();

  sum = 0;
  counter = 0;
}

void loop() {

    while (swSer.available()) {
        getPM25(swSer.read());
    }
    
    if( PM25_flag == 1 ) {
        PM25_flag = 0;
        
        sum = sum + _params.PM25;
        counter ++;
        if( counter == 8 ) {
            counter = 0;
            sum = sum / 8;
            avg_ok = 1;
            Serial.println("PM！！！");
        }
    }
    
    if( avg_ok == 1 ) {
        avg_ok = 0;
        
        //------------domoticz mqtt-------------------------------------------------------------------------------- 
        String mqtt_st1 = "{\"idx\":" + String(idx1) +  ",\"nvalue\":0,\"svalue\":\"" + String(sum) + "\"}";
        
        strncpy(msg1, mqtt_st1.c_str(), 100);
        
        mqtt_client.setServer(mqtt_domoticz, 1883);
        
        if ( mqtt_client.connect("esp") ) {
            Serial.println("domoticz connected");
            Serial.println(mqtt_st1);
            mqtt_client.publish("domoticz/in", msg1);     
        }

        mqtt_client.disconnect(); //disconnect to connect next mqtt server

        delay(1000);
        
        Serial.print("sleep");
        ESP.deepSleep(SLEEP_TIME_SECONDS * 1000000);
    }
}

void getPM25(unsigned char ucData) {
    unsigned int sum;
    unsigned char i;
    
    ucRxBuffer[ucRxCnt] = ucData;
    ucRxCnt ++;

    if (ucRxBuffer[0] != 0x42 && ucRxBuffer[1] != 0x4D)  {                          //Check Head
        ucRxCnt = 0;
    }
    if (ucRxCnt > 31) {
        sum = 0;
        for( i=0; i<30; i++ ) {
            sum = sum + ucRxBuffer[i];
        }
        if( ( (sum / 256) == ucRxBuffer[30] ) && (sum % 256) == ucRxBuffer[31] ) {  //Check Sum
        //_params.PM25 = (float)ucRxBuffer[6] * 256 + (float)ucRxBuffer[7];         //美国标准 标准颗粒物
          _params.PM25 = (unsigned int)ucRxBuffer[12] * 256 + ucRxBuffer[13] ;       //中国标准 大气标准
          Serial.println( _params.PM25 );

          ucRxCnt = 0;
          PM25_flag = 1;
        }
    }
}


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  uint8_t counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter > ALLOWED_CONNECT_CYCLES) {
      //Serial.print("sleep");
      //ESP.deepSleep(SLEEP_TIME_SECONDS * 1000000); //sleeping
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    //digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    //if (mqtt_client.connect("ESP8266Client")) {
    if (mqtt_client.connect("336c43aa4f_02")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqtt_client.publish("outTopic", "hello world");
      // ... and resubscribe
      mqtt_client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


