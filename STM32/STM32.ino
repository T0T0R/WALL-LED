/* Start : December 2020
 *  Interface Rapberry Pi / LED panels 8x8
 *  I2C Protocole
 */

#include <Wire.h>

#define LED PC13
#define DATA_PIN PA8
#define CLOCK_PIN PA10
#define LATCH_PIN PA9

#define SIZE_X 3
#define SIZE_Y 2
#define nbOfPWMcycles 8
#define dataSequence_size SIZE_X*(8*3)+8


byte RED_matrix [SIZE_Y*8][SIZE_X*8];
byte GREEN_matrix [SIZE_Y*8][SIZE_X*8];
byte BLUE_matrix [SIZE_Y*8][SIZE_X*8];

//RGB LED datas plus row control
byte dataLineA [SIZE_X*SIZE_Y*(8*3)+8];
byte dataLineB [SIZE_X*SIZE_Y*(8*3)+8];
byte dataLineC [SIZE_X*SIZE_Y*(8*3)+8];
byte dataLineD [SIZE_X*SIZE_Y*(8*3)+8];
byte dataLineE [SIZE_X*SIZE_Y*(8*3)+8];
byte dataLineF [SIZE_X*SIZE_Y*(8*3)+8];
byte dataLineG [SIZE_X*SIZE_Y*(8*3)+8];
byte dataLineH [SIZE_X*SIZE_Y*(8*3)+8];

byte RED_VALUES [nbOfPWMcycles];
byte GREEN_VALUES [nbOfPWMcycles];
byte BLUE_VALUES [nbOfPWMcycles];

bool displaySpectrum;
byte SPECTRUM [SIZE_X*8];




void setup() { 
  pinMode(LED, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  displaySpectrum = false;

  pinMode(PB5, OUTPUT);
  digitalWrite(PB5,HIGH);

  for (byte PWMlevel=0; PWMlevel<nbOfPWMcycles; PWMlevel++){
    RED_VALUES[PWMlevel]= (255/nbOfPWMcycles)*(PWMlevel+1);
    GREEN_VALUES[PWMlevel]= (255/nbOfPWMcycles)*(PWMlevel+1);
    BLUE_VALUES[PWMlevel]= (255/nbOfPWMcycles)*(PWMlevel+1);
  }
  
  Wire.setSCL(PB10);
  Wire.setSDA(PB11);
 
  Wire.begin(4);
  Wire.onReceive(receiveEvent); // register event
  initDATAS();
}




/* Functions used to debug the program with the on-board LED */

void debug(bool statement){
  /* If the statement is true, then light up the LED */
  if (statement){digitalWrite(LED,LOW);}else{digitalWrite(LED,HIGH);}
}
void debug(int value){
  /* LED blinks $value times */
  for(int i=0; i<value; i++){
    digitalWrite(LED,LOW);
    delay(100);
    digitalWrite(LED,HIGH);
    delay(300);
  }
  delay(150);
}





// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  debug(true);  // Light up LED when I2C datas are received

  byte color = Wire.read();
  int row = Wire.read();
  byte cell = Wire.read();

  int column = cell*8;
  
  switch(color){
    case 0: //RED
      displaySpectrum = false;
      while(0<Wire.available()){
        RED_matrix[row][column] = Wire.read();
        column++;
      }
    break;
    
    case 1: //GREEN
      displaySpectrum = false;
      while(0<Wire.available()){
        GREEN_matrix[row][column] = Wire.read();
        column++;
      }
    break;
    
    case 2: //BLUE
      displaySpectrum = false;
      while(0<Wire.available()){
        BLUE_matrix[row][column] = Wire.read();
        column++;
      }
    break;

    case 3: //Levels of red
      displaySpectrum = false;
      while(0<Wire.available()){
        RED_VALUES[column] = Wire.read();
        column++;
      }
    break;
    case 4: //Levels of green
      displaySpectrum = false;
      while(0<Wire.available()){
        GREEN_VALUES[column] = Wire.read();
        column++;
      }
    break;
    case 5: //Levels of blue
      displaySpectrum = false;
      while(0<Wire.available()){
          BLUE_VALUES[column] = Wire.read();
          column++;
        }
    break;
    
    case 7: //Spectrum datas
      displaySpectrum = true;
      while(0<Wire.available()){
        SPECTRUM[column] = Wire.read();
        column++;
      }
    break;
    
    default:
    break;
  }
    
  debug(false);
}



int resetPins() { //All output pins at LOW level
  digitalWrite(DATA_PIN, LOW);;
  digitalWrite(CLOCK_PIN, LOW);
  digitalWrite(LATCH_PIN, LOW);
  return 0;
}



int sendPacket(byte PWMcycle) {
/* PWM ratio are stored with values V between 0 and 3.
  When a value V is not 0, it sets on an output, and this value is decreased.
  Therefore, after V passes in the loop, the output is turned off, simulating PWM */

  resetPins();
  unsigned int size = dataSequence_size;

  
  for (unsigned int i(0); i<size; i++) {  //LSB First, so datas are sent from the end of the table...

    if(dataLineA[size-i-1]!=0){  //If (last-i) bit != 0, send it
        digitalWrite(DATA_PIN, HIGH);
        dataLineA[size-i-1] = dataLineA[size-i-1]-1;  //Decrease the value, as explained in description of the function
    }    

        digitalWrite(CLOCK_PIN, HIGH);    //Clock up...
        digitalWrite(CLOCK_PIN, LOW);     //...Clock down.

        digitalWrite(DATA_PIN, LOW);     //Turn off the data line

  }

    digitalWrite(LATCH_PIN, HIGH);  //Outputs transmission, and draw dat line !
    digitalWrite(LATCH_PIN, LOW);

  // After treating 1st line, 2nd to 8th line are treated the same way :
  for (unsigned int i(0); i<size; i++) {
    if(dataLineB[size-i-1]!=0){
        digitalWrite(DATA_PIN, HIGH);
        dataLineB[size-i-1] = dataLineB[size-i-1]-1;
    }
        digitalWrite(CLOCK_PIN, HIGH);
        digitalWrite(CLOCK_PIN, LOW);
        digitalWrite(DATA_PIN, LOW);
  }
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
  for (unsigned int i(0); i<size; i++) {
    if(dataLineC[size-i-1]!=0){
        digitalWrite(DATA_PIN, HIGH);
        dataLineC[size-i-1] = dataLineC[size-i-1]-1;
    }
        digitalWrite(CLOCK_PIN, HIGH);
        digitalWrite(CLOCK_PIN, LOW);
        digitalWrite(DATA_PIN, LOW);
  }
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
  for (unsigned int i(0); i<size; i++) {
    if(dataLineD[size-i-1]!=0){
        digitalWrite(DATA_PIN, HIGH);
        dataLineD[size-i-1] = dataLineD[size-i-1]-1;
    }
        digitalWrite(CLOCK_PIN, HIGH);
        digitalWrite(CLOCK_PIN, LOW);
        digitalWrite(DATA_PIN, LOW);
  }
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
  for (unsigned int i(0); i<size; i++) {
    if(dataLineE[size-i-1]!=0){
        digitalWrite(DATA_PIN, HIGH);
        dataLineE[size-i-1] = dataLineE[size-i-1]-1;
    }
        digitalWrite(CLOCK_PIN, HIGH);
        digitalWrite(CLOCK_PIN, LOW);
        digitalWrite(DATA_PIN, LOW);
  }
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
  for (unsigned int i(0); i<size; i++) {
    if(dataLineF[size-i-1]!=0){
        digitalWrite(DATA_PIN, HIGH);
        dataLineF[size-i-1] = dataLineF[size-i-1]-1;
    }
        digitalWrite(CLOCK_PIN, HIGH);
        digitalWrite(CLOCK_PIN, LOW);
        digitalWrite(DATA_PIN, LOW);
  }
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
  for (unsigned int i(0); i<size; i++) {
    if(dataLineG[size-i-1]!=0){
        digitalWrite(DATA_PIN, HIGH);
        dataLineG[size-i-1] = dataLineG[size-i-1]-1;
    }
        digitalWrite(CLOCK_PIN, HIGH);
        digitalWrite(CLOCK_PIN, LOW);
        digitalWrite(DATA_PIN, LOW);
  }
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
  for (unsigned int i(0); i<size; i++) {
    if(dataLineH[size-i-1]!=0){
        digitalWrite(DATA_PIN, HIGH);
        dataLineH[size-i-1] = dataLineH[size-i-1]-1;
    }
        digitalWrite(CLOCK_PIN, HIGH);
        digitalWrite(CLOCK_PIN, LOW);
        digitalWrite(DATA_PIN, LOW);
  }
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);

  return 0;
}




byte convertValuePWM(byte const& value, byte const& color){
  /* Basically, converts values of color from 0-255 to 0-(nbOfPWMcycles-1).
    Conversion can be non-linear if configured by user in
    the "Calibration" menu.
    Linear by default.
  */
  switch (color){
    case 0: //RED

      for (byte PWMlevel=0; PWMlevel<nbOfPWMcycles; PWMlevel++){
        if (value<RED_VALUES[PWMlevel]) {return PWMlevel;}
      }
      return nbOfPWMcycles;
      
      break;
      
    case 1: //GREEN
      for (byte PWMlevel=0; PWMlevel<nbOfPWMcycles; PWMlevel++){
        if (value<GREEN_VALUES[PWMlevel]) {return PWMlevel;}
      }
      return nbOfPWMcycles;
      break;
      
    case 2: //BLUE
      for (byte PWMlevel=0; PWMlevel<nbOfPWMcycles; PWMlevel++){
        if (value<BLUE_VALUES[PWMlevel]) {return PWMlevel;}
      }
      return nbOfPWMcycles;
      break;
      
    default:
      return 0;
      break;
  }
  return 0;
}




void convertMatrixesToLED() {
  /* Each lineSet is build by getting the values for the pixels in this very line set (SIZE_X*SIZE_Y*8*3 bits)
    and also the byte read by the shift register (row byte) that chooses the row/line to display (+8 bits).
  */

  byte rowByteSize (8);  //Not so hard-coded to not forget to leave space at the beginning of
          //a lineset to store there the data for the shift register that drives the rows.


  for (byte lineSet=0; lineSet<8; lineSet++){  //Line set of the picture

    for (byte cellLine(0); cellLine < SIZE_Y; cellLine++){  //Yeah, we got the line set, but which line of the image in this line set ? cellLine*8 + lineSet !
      for (byte cell(0); cell < SIZE_X; cell++) { //Cell in the cellLine (one cell is composed of 8 pixels).
        for (byte noPixel(0); noPixel<8; noPixel++) { //Pixel in the cell

          switch(lineSet){
            case 0:
              //Row byte
              dataLineA[0] = nbOfPWMcycles+1; dataLineA[1] = 0; dataLineA[2] = 0; dataLineA[3] = 0;
              dataLineA[4] = 0;               dataLineA[5] = 0; dataLineA[6] = 0; dataLineA[7] = 0;
            
              //in red:
              dataLineA[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel] = convertValuePWM(RED_matrix[cellLine*8 + lineSet][cell*8 + noPixel],0);

              //in green:
              dataLineA[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+8] = convertValuePWM(GREEN_matrix[cellLine*8 + lineSet][cell*8 + noPixel],1);

              //in blue:
              dataLineA[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+16] = convertValuePWM(BLUE_matrix[cellLine*8 + lineSet][cell*8 + noPixel],2);
            break;
            
            case 1:
              dataLineB[0] = 0; dataLineB[1] = nbOfPWMcycles+1; dataLineB[2] = 0; dataLineB[3] = 0;
              dataLineB[4] = 0; dataLineB[5] = 0;               dataLineB[6] = 0; dataLineB[7] = 0;
              dataLineB[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel] = convertValuePWM(RED_matrix[cellLine*8 + lineSet][cell*8 + noPixel],0);
              dataLineB[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+8] = convertValuePWM(GREEN_matrix[cellLine*8 + lineSet][cell*8 + noPixel],1);
              dataLineB[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+16] = convertValuePWM(BLUE_matrix[cellLine*8 + lineSet][cell*8 + noPixel],2);
            break;
            case 2:
              dataLineC[0] = 0; dataLineC[1] = 0; dataLineC[2] = nbOfPWMcycles+1; dataLineC[3] = 0;
              dataLineC[4] = 0; dataLineC[5] = 0; dataLineC[6] = 0;               dataLineC[7] = 0;
              dataLineC[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel] = convertValuePWM(RED_matrix[cellLine*8 + lineSet][cell*8 + noPixel],0);
              dataLineC[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+8] = convertValuePWM(GREEN_matrix[cellLine*8 + lineSet][cell*8 + noPixel],1);
              dataLineC[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+16] = convertValuePWM(BLUE_matrix[cellLine*8 + lineSet][cell*8 + noPixel],2);
            break;
            case 3:
              dataLineD[0] = 0; dataLineD[1] = 0; dataLineD[2] = 0; dataLineD[3] = nbOfPWMcycles+1;
              dataLineD[4] = 0; dataLineD[5] = 0; dataLineD[6] = 0; dataLineD[7] = 0;
              dataLineD[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel] = convertValuePWM(RED_matrix[cellLine*8 + lineSet][cell*8 + noPixel],0);
              dataLineD[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+8] = convertValuePWM(GREEN_matrix[cellLine*8 + lineSet][cell*8 + noPixel],1);
              dataLineD[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+16] = convertValuePWM(BLUE_matrix[cellLine*8 + lineSet][cell*8 + noPixel],2);
            break;
            case 4:
              dataLineE[0] = 0;               dataLineE[1] = 0; dataLineE[2] = 0; dataLineE[3] = 0;
              dataLineE[4] = nbOfPWMcycles+1; dataLineE[5] = 0; dataLineE[6] = 0; dataLineE[7] = 0;
              dataLineE[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel] = convertValuePWM(RED_matrix[cellLine*8 + lineSet][cell*8 + noPixel],0);
              dataLineE[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+8] = convertValuePWM(GREEN_matrix[cellLine*8 + lineSet][cell*8 + noPixel],1);
              dataLineE[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+16] = convertValuePWM(BLUE_matrix[cellLine*8 + lineSet][cell*8 + noPixel],2);
            break;
            case 5:
              dataLineF[0] = 0; dataLineF[1] = 0;               dataLineF[2] = 0; dataLineF[3] = 0;
              dataLineF[4] = 0; dataLineF[5] = nbOfPWMcycles+1; dataLineF[6] = 0; dataLineF[7] = 0;
              dataLineF[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel] = convertValuePWM(RED_matrix[cellLine*8 + lineSet][cell*8 + noPixel],0);
              dataLineF[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+8] = convertValuePWM(GREEN_matrix[cellLine*8 + lineSet][cell*8 + noPixel],1);
              dataLineF[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+16] = convertValuePWM(BLUE_matrix[cellLine*8 + lineSet][cell*8 + noPixel],2);
            break;
            case 6:
              dataLineG[0] = 0; dataLineG[1] = 0; dataLineG[2] = 0;               dataLineG[3] = 0;
              dataLineG[4] = 0; dataLineG[5] = 0; dataLineG[6] = nbOfPWMcycles+1; dataLineG[7] = 0;
              dataLineG[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel] = convertValuePWM(RED_matrix[cellLine*8 + lineSet][cell*8 + noPixel],0);
              dataLineG[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+8] = convertValuePWM(GREEN_matrix[cellLine*8 + lineSet][cell*8 + noPixel],1);
              dataLineG[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+16] = convertValuePWM(BLUE_matrix[cellLine*8 + lineSet][cell*8 + noPixel],2);
            break;
            case 7:
              dataLineH[0] = 0; dataLineH[1] = 0; dataLineH[2] = 0; dataLineH[3] = 0;
              dataLineH[4] = 0; dataLineH[5] = 0; dataLineH[6] = 0; dataLineH[7] = nbOfPWMcycles+1;
              dataLineH[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel] = convertValuePWM(RED_matrix[cellLine*8 + lineSet][cell*8 + noPixel],0);
              dataLineH[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+8] = convertValuePWM(GREEN_matrix[cellLine*8 + lineSet][cell*8 + noPixel],1);
              dataLineH[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+16] = convertValuePWM(BLUE_matrix[cellLine*8 + lineSet][cell*8 + noPixel],2);
            break;

            default:
            break;
          }
        }
      }
    }
  }
}




int drawScreen() {
  /* One frame composed of nbOfPWMcycles cycles of PWM. */

  convertMatrixesToLED();
  
    for (byte i=0; i<nbOfPWMcycles; i++){  //Draws 8 frames to allow for PWM.
        sendPacket(i);
    }
  return 0;
}





void initDATAS(){
  /* Set a test pattern as the frame */
  
  for (byte row=0; row<SIZE_Y*8; row+=2){
    for (byte column=0; column<SIZE_X*8; column+=2){

      RED_matrix[row][column]=255/nbOfPWMcycles*column;
      RED_matrix[row][column+1]=0;
      GREEN_matrix[row][column]=0;
      GREEN_matrix[row][column+1]=255/nbOfPWMcycles*(column+1);
      BLUE_matrix[row][column]=0;
      BLUE_matrix[row][column+1]=0;

      RED_matrix[row+1][column]=0;
      RED_matrix[row+1][column+1]=0;
      GREEN_matrix[row+1][column]=255/nbOfPWMcycles*column;
      GREEN_matrix[row+1][column+1]=0;
      BLUE_matrix[row+1][column]=0;
      BLUE_matrix[row+1][column+1]=255/nbOfPWMcycles*(column+1);
    }
  }
}



void loop() {
  int r (0);
  int g (0);
  
  if (displaySpectrum){
    for (byte column (0); column < SIZE_X*8; column++){ // For each band
      
      for (byte i (0); i < SIZE_Y*8; i++){  // ...draw a vertical line (from the bottom)...
        r = (byte)(i*255.0/(SIZE_Y*8-1));     // ...with its hue going from green to red.
        g = 255-r;
        if (i >= map(SPECTRUM[column],0,127,0,SIZE_Y*8+1)){
          r = 0; g = 0;         // complete the top of the line with black pixels.
        }
        
        RED_matrix[SIZE_Y*8-1-i][column] = r;
        GREEN_matrix[SIZE_Y*8-1-i][column] = g;
        BLUE_matrix[SIZE_Y*8-1-i][column] = 0;
      }
    }
  }else{  // Reseting spectrum datas
    for (byte i (0); i < SIZE_X*8; i++){
      SPECTRUM[i]=0;
    }
  }
  
  drawScreen();
  debug(false);
}
