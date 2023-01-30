/**
 * Alireza Zare Zeynabadi 9931022
 * Amirfazel Kozeh Gar Kaleji 9931099
 */
#include   <SPI.h>

/**
 * Initialize SPI Registers
 */
#define CS 10
#define DECODE_MODE   9 
#define RATE 0x0A
#define SCAN_LIMIT 0x0B 
#define GAMEDELAY 500
#define SHUTDOWN 0x0C
#define SCREEN_TEST 0x0F

/**
 * When a player scores a goal we need restart the game so we must hold an extra array for this
 */
byte primary_state [8]={B00111000,
                    B00000000,
                    B00000000,
                    B00010000,
                    B00000000,
                    B00000000,
                    B00000000,
                    B00111000};



/**
 * Data To send and handle for leds during the game loop
 */
                                  /*IN CIRCUIT WE WILL SEE:*/ 
byte Current_state [8]={B00111000,/*     00000000          */
                      B00000000,/*       00000000          */
                      B00000000,/*       00000000          */
                      B00010000,/*       10000001          */
                      B00000000,/*       10010001          */
                      B00000000,/*       10000001          */
                      B00000000,/*       00000000          */
                      B00111000};/*      00000000          */



/**
 * 
 * Paddle
 */
class Paddle {
  public : 
    /**
     * position of center of paddle
     */
    int def = 0;
    /**
     * ROW OF PADDLE
     */
    int row;
    /**
     * Method for move paddle on the matrix
     */
    void setDEF(int n){
      def = n;
    }
    void setROW(int n){
      row = n;
    }
    void tick(int velocity){
      if (
        velocity == 1 && def != -2 /*if paddel want go up and reaches wall he can't */
        ) {
        Current_state[row] <<= 1;
        setDEF(def - 1);
      }
      else if (
        velocity == -1 && def != 3 /*if paddel want go down and reaches wall he can't */
        ){
        Current_state[row] >>= 1;
        setDEF(def + 1);
      }
    }
};

/**
 * 
 * Ball
 */
class Ball {
  public :
    /** 
     *  Direction => movement of ball 
     *  Position => current position of ball
     */
    int directionY = 0;
    int directionX = 0;
    int positionX = 3;
    int positionY = 4;


    /**
     * when ball reachs end of the matrix we need restart the game
     */
    void Reset_Screen(){
      if(positionX == 0 || positionX == 7)
      {
        for(int i=0; i < 8; i++)
        {
          Current_state[i] = primary_state[i];
        }
        directionY = 0;
        directionX = 1;
        positionX = 3;
        positionY = 4;
      }
    }

    /**
     * Method for move ball on matrix
     */
    int tick() {
      int X_Coords  = positionX + directionX;
      int Y_coords = positionY + directionY;
      
      bitWrite(Current_state[positionX], positionY, 0); /*send new position to SPI*/

      positionX= X_Coords ;
      positionY= Y_coords;

      /**
       * update movment of ball
       */
      if ((X_Coords  == 0) || (X_Coords  == 7) || (Y_coords == 0) || (Y_coords == 7)) {
          if (borderIsTouched(X_Coords ,Y_coords) == 0){
            return 0;
          }
      } else {
        X_Coords  += directionX;
        if (bitRead(Current_state[X_Coords], Y_coords) == 1) {
          directionX = - directionX;
          if (bitRead(Current_state[X_Coords], Y_coords + 1) != 1) { 
            directionY = 1;              
          } else if (bitRead(Current_state[X_Coords ], Y_coords - 1) != 1) {
            directionY = - 1;
          }
        }
      }
      bitWrite(Current_state[positionX], positionY, 1);
      return 1;
    }

    /**
     * method for ball touching the borders (walls and goal lines)
     */
    int borderIsTouched(int X_Coords ,int Y_coords) {
      if ((X_Coords  == 0) || (X_Coords  == 7)) {
        Reset_Screen();
        return 0;
      } else {
        directionY = - directionY;
        return 1;
      }
    }
};





/**
 * initialize pins for buttons
 */
const int P1_Pin_Down = 3;
const int P1_Pin_Up = 4;
const int P2_Pin_Down = 5;
const int P2_Pin_Up = 6;

/**
 * Buttons when untouched need to be initialized LOW
 */
int P1_Old_Down_State = LOW;
int P1_Old_Up_State = LOW;
int P2_Old_Down_State = LOW;
int P2_Old_Up_State = LOW;

/**
 * SPI DATA ADDRESSES
 */
byte transferDataArr[9];


/**
 * Game Objects
 */
Paddle p1;
Paddle p2;
Ball ball;


void movePaddle(int buttonState, int oldButtonState, int direction, int index){
  if (buttonState != oldButtonState &&
      buttonState == HIGH) /* check if button is pushed and who pushes*/
  {
    switch (index) {
      case 1:
        p1.tick(direction);
        break;
      case 2:
        p2.tick(direction);
        break;
      default:
          return;
    }
    delay(50);
  }
}

/*
 * for reset paddles to start we just need set def = 0
 */
void resetpaddle() {
  p1.setDEF(0);
  p2.setDEF(0);
}

void sendData(uint8_t address, uint8_t value) {  
  digitalWrite(CS, LOW);  /*SPI READY TO READ*/ 
  SPI.transfer(address);  
  SPI.transfer(value);  
  digitalWrite(CS, HIGH); /*SPI Turn of READ DATA*/
}


void setup()   {
  Serial.begin(9600);
  /**
   * initialize game
   */
  ball.directionX = 1;
  p1.setROW(7);
  p2.setROW(0);

  
  pinMode(CS, OUTPUT);  
  SPI.setBitOrder(MSBFIRST);   // initialize SPI Most Significant Bit First for read data 
  SPI.begin();                 
  sendData(SCREEN_TEST,   0x01);       // at system start we on all leds
  delay(1000);
  sendData(SCREEN_TEST,   0x00);           // turn of all lights
  sendData(DECODE_MODE, 0x00);            //   SPI SPECIAL !? 
  sendData(RATE, 0x0e);              // set intensity of leds (SPI SPECIAL)
  sendData(SCAN_LIMIT, 0x0f);             // SPI SPECIAL
  sendData(SHUTDOWN,   0x01);               // SPI SPECIAL
}

void loop()  {
  /**
   * READ BUTTONS
   */
  int buttonDOWNState_P1 = digitalRead(P1_Pin_Down);
  int buttonUPState_P1 = digitalRead(P1_Pin_Up);
  int buttonDOWNState_P2 = digitalRead(P2_Pin_Down);
  int buttonUPState_P2 = digitalRead(P2_Pin_Up);


  /**
   * SHOW ROW BY ROW ON MATRIX BY SENDING THEM TO SPI
   */
  for (int i = 0 ; i < 8; i++){
    sendData(i + 1, Current_state[i]);
  }


  /**
   * MOVE THE PADDLE FOR PLAYER ONE WHEN BUTTON PUSHED
   */
  movePaddle(buttonDOWNState_P2, P2_Old_Down_State, 1, 2);
  P2_Old_Down_State = buttonDOWNState_P2;

  movePaddle(buttonUPState_P2, P2_Old_Up_State, -1, 2);
  P2_Old_Up_State = buttonUPState_P2;


  /**
   * MOVE THE PADDLE FOR PLAYER TWO WHEN BUTTON PUSHED
   */
  movePaddle(buttonDOWNState_P1, P1_Old_Down_State, 1, 1);
  P1_Old_Down_State = buttonDOWNState_P1;

  movePaddle(buttonUPState_P1, P1_Old_Up_State, -1, 1);
  P1_Old_Up_State = buttonUPState_P1;

  /**
   * AT RESTART OF GAME WE NEED PUT PADDLE ON MIDDLE
   */
  if (ball.tick() == 0)
  {
    resetpaddle();
  }

  /**
   * SEND DATA TO SPI
   */
  transferDataArr[0] = 0x44;

  for(int i = 0 ; i < 8; i++){
        transferDataArr[i + 1] = Current_state[i];
    }
    
  Serial.write(transferDataArr, 9);

  delay(GAMEDELAY);

}
