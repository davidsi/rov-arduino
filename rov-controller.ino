/**
 * ESC motors driven by a webserver
 *
 * Circuit:
 *      Ethernet shield attached to pins 10, 11, 12, 13
 *      Analog inputs attached to pins A0 through A5 (optional)
 */
#include <SPI.h>
#include <Ethernet.h>
#include <Servo.h>

#define DEBUG

/**
 * Enter a MAC address and IP address for your controller below.
 * The IP address will be dependent on your local network:
 */
byte     mac[]     = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte     subnet[]  = { 255, 255, 255, 0 };
byte     gateway[] = { 192, 168, 42, 1 };
byte     ip[]      = { 192, 168, 42, 177 };
unsigned port      = 8888;

// #define RESPONSE_MAX 100
// char response[RESPONSE_MAX];
// int  responseLen;

/**
 * pins
 */
#define SD_CARD_PIN      4
#define ETHERNET_PIN1   10
#define ETHERNET_PIN2   11
#define ETHERNET_PIN3   12
#define ETHERNET_PIN4   13
#define SERVO1           9
#define SERVO2           5
#define SERVO3           6
#define SERVO4           3  // also interrupt 1

/**
 * motor pins : array 
 */
#define NUM_MOTORS 4

byte   servoPins[] = { SERVO1, SERVO2, SERVO3, SERVO4  };
Servo *servos[NUM_MOTORS];
int    currentSpeed[NUM_MOTORS];

/**
 * Initialize the Ethernet UDP library
 */
EthernetUDP udp;

/**
 * convert 0..99 to a motor speed
 */
int percentToMotorSpeed( int percent ) {

    if( percent < 0 ) {
        return -1;
    }
    else if( percent > 45 && percent < 55 ) {
        return 1500;
    }
    else if( percent < 50 ) {
        return 1100 + (int)((float)((float)percent * (float)7.5));
    }
    else if( percent < 101 ) {
        return 1526 + (int)((float)((float)(percent-50) * (float)7.5));
    }
    else {
        return -1;
    }
}

/**
 * set the motor speed
 */
void motorSpeed( int motorIdx, int newValue ) {

    if( newValue < 1100 ) {
        // value to low!
        //
        return;
    }
    else if( newValue < 1475 ) {
  
        // reverse. 
        // if we were going forward before, stop
        //
        if( currentSpeed[motorIdx] > 1500 ) {
            newValue = 1500;
        }
    }
    else if( newValue < 1526 ) {
  
        // stopping
        //
        newValue = 1500;
    }
    else if( newValue < 1901 ) {
  
        // forward
        // if going backward before, stop
        //
        if( currentSpeed[motorIdx] < 1500 ) {
            newValue = 1500;
        }
    }
    else {
        // value to high!
        //
        return;
    }

#ifdef DEBUG
    Serial.print( "Setting motor " );
    Serial.print( motorIdx );
    Serial.print( " speed to " );
    Serial.println(  newValue );
#endif

    currentSpeed[motorIdx] = newValue;
    servos[motorIdx]->writeMicroseconds(newValue); // Send signal to ESC.    
}

/**
 * arduino setup
 */
void setup() {

    // set up the serial port
    //
    Serial.begin( 57600 );
#ifdef DEBUG    
    Serial.println( "ROV test start" );
#endif    
    // set up the servos
    //
    for( int idx = 0; idx < NUM_MOTORS; idx ++ ) {
        servos[idx] = new Servo();

        servos[idx]->attach( servoPins[idx] );
        currentSpeed[idx] = 1500;               // preset to good value
        motorSpeed( idx, 1500 );                // send "stop" signal to ESC.
    }

    // delay to allow the ESC to recognize the stopped signal
    //
    delay(3000); 

    // Send signal to ESC.    
    //
    for( int idx = 0; idx < NUM_MOTORS; idx ++ ) {
        motorSpeed( idx, 1700 ); 
    }

    // and turn off again
    //
    for( int idx = 0; idx < NUM_MOTORS; idx ++ ) {
        motorSpeed( idx, 1500 ); 
    }

    // block SD card just in case
    //
    pinMode( SD_CARD_PIN, OUTPUT );
    digitalWrite( SD_CARD_PIN, HIGH );

    // set up the ethernet shield
    //  
    Ethernet.begin( mac, ip, gateway, subnet );
    udp.begin( port );

    // print your local IP address:
    //
#ifdef DEBUG    
    Serial.print("server is at ");
    Serial.println( Ethernet.localIP() );
#endif    
    // server.begin();
}


char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,        

/**
 * http://192.168.15.177/camera?move=up
 */
void loop() {

    int packetSize = udp.parsePacket();

    if( packetSize ) {
//        Serial.print( "received packet of size " ); Serial.print( packetSize );
//
        // read the packet into packetBufffer
        //
        udp.read( packetBuffer, UDP_TX_PACKET_MAX_SIZE );
//        Serial.print( ". Contents:" );
//        for( int idx = 0; idx <packetSize; idx ++ ) {
//          Serial.print( packetBuffer[idx] );
//        }

        int motor = packetBuffer[1] - '0';
        int speed = (packetBuffer[3] - '0') * 10 + (packetBuffer[5] - '0');
        Serial.print( "motor = " ); Serial.print( motor ); Serial.print( ", speed = " ); Serial.println( speed );
        motorSpeed( motor, percentToMotorSpeed( speed ) );
    }
}
