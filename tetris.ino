#include <MaxMatrix.h>
#include <Keypad.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

int DIN = 50;
int CLK = 51;
int CS = 53;
int maxInUse = 1;
byte buffer[20];
char text[] = "a";
MaxMatrix m(DIN, CS, CLK, maxInUse);
int dotX = 3;
int dotY = 4;

const byte ROWS = 4; 
const byte COLS = 3; 
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {49, 47, 45, 43}; 
byte colPins[COLS] = {41, 39, 37}; 
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

typedef struct BlockComponent{
  int x = -1;
  int y = -1;
}Point;
unsigned long startingTime, currentTime;
unsigned long secondsPassed = 1;
int currentBlockComponents = 4;
bool currentBlockReachedGround = false;
Point* currentBlock;
int occupiedSpots[8][8] = {0};
int blockType;
int rotationIndex;
int score=0;

void setup() {
 lcd.begin(16, 2);
 lcd.setCursor(0,0);
 lcd.print("YOUR SCORE: ");
 lcd.setCursor(0,1);
 lcd.print(score);
 m.init();
 m.setIntensity(8);
 keypad.addEventListener(keypadEvent);
 Serial.begin(9600);
 startingTime = millis();
 createBlock();
 setCurrentBlockCoordinates(3, -1);
}

void loop() {
 currentTime = millis();
 keypad.getKey(); 
 if(gameIsOver() == true) {
  for(int columnIndex = 0; columnIndex<8; columnIndex++){
    m.setColumn(columnIndex, B11111111);
  }
  return;
 }
 drawCurrentBlock();
 if((currentTime - startingTime) / 1000 > secondsPassed){
  secondsPassed++;
  if(currentBlockReachedGround){
    score+=10;
    checkIfAnyRowIsFullyOccupied();
    createBlock();
    setCurrentBlockCoordinates(3, -1);
    currentBlockReachedGround = false;
    lcd.setCursor(0,0);
    lcd.print("YOUR SCORE: ");
    lcd.setCursor(0,1);
    lcd.print(score);
  } else {
    fallOneRow();
  }
 }
 
} 

void drawCurrentBlock () {
  for(int i = 0; i<currentBlockComponents; i++) {
    m.setDot(currentBlock[i].x, currentBlock[i].y, true);
  }
}

void fallOneRow () {
  if(currentBlockReachedGround) {
    return;  
  }
  for(int i = 0; i<currentBlockComponents; i++) {
    m.setDot(currentBlock[i].x, currentBlock[i].y, false);
    currentBlock[i].y ++;
    if(occupiedSpots[currentBlock[i].y + 1][currentBlock[i].x] == 1 || currentBlock[i].y + 1 == 8){
      currentBlockReachedGround = true;
    }
  } 
  if(currentBlockReachedGround) {
    addCurrentBlockToOccupiedSpots();  
  }
}

void moveCurrentBlockRight(){
  if(!canGoRight() || currentBlockReachedGround){
    Serial.print(currentBlockReachedGround);
    
    return;
  }
  for(int i = 0; i<currentBlockComponents; i++) {
    m.setDot(currentBlock[i].x, currentBlock[i].y, false);
    currentBlock[i].x ++;
  }
}

bool canGoRight() {
  int xMax=0;
  int yForXMax = 0;
  for(int i = 0; i<currentBlockComponents; i++) {
    if(currentBlock[i].x > xMax){
      xMax = currentBlock[i].x;
      yForXMax = i;
    }
  }
  if (xMax == 7 || occupiedSpots[yForXMax][xMax+11] == 1){
    return false; 
  } else {
    return true;
  }
}

void moveCurrentBlockLeft(){
  if(!canGoLeft() || currentBlockReachedGround)
    return;
  for(int i = 0; i<currentBlockComponents; i++) {
    m.setDot(currentBlock[i].x, currentBlock[i].y, false);
    currentBlock[i].x --;
  }
}

bool canGoLeft() {
  for(int i = 0; i<currentBlockComponents; i++) {
    if (currentBlock[i].x == 0 || occupiedSpots[currentBlock[i].y][currentBlock[i].x-1] == 1)
      return false;
  }
  return true;
}

void rotateCurrentBlock() {
  for(int i = 0; i<currentBlockComponents; i++) {
    m.setDot(currentBlock[i].x, currentBlock[i].y, false);
  }
  rotationIndex++;
  setCurrentBlockCoordinates(currentBlock[0].x, currentBlock[0].y);
}

void addCurrentBlockToOccupiedSpots(){
  for(int i=0; i<currentBlockComponents; i++){
    occupiedSpots[currentBlock[i].y][currentBlock[i].x] = 1;
  }
}

void checkIfAnyRowIsFullyOccupied() {
  for(int i=0; i<8; i++){
    int nrLitLedsOnOneRow = 0;
    for(int j=0; j<8; j++){
      if (occupiedSpots[i][j] == 1)
        nrLitLedsOnOneRow++;
    }
    if(nrLitLedsOnOneRow == 8) {
      score+=100;
      for(int blinkTime=0; blinkTime<5; blinkTime++){//the full row blinks 5 times
        for(int dot=0; dot<8; dot++){
          m.setDot(dot, i, true);
        }
        delay(100); 
        for(int dot=0; dot<8; dot++){
          m.setDot(dot, i, false);
        }
        delay(100); 
      }
      for(int litDot=0; litDot<8; litDot++){
        //m.setDot(litDot, i, false);
        occupiedSpots[i][litDot] = 0;
      }
      for(int i2=i-1; i2>=0; i2--){
        for(int j=0; j<8; j++){
          if(occupiedSpots[i2][j] == 1){
            occupiedSpots[i2+1][j] = 1;
            occupiedSpots[i2][j] = 0;
          }
        }
      }
      reDrawOccupiedSpots();
    }
  }
}

void reDrawOccupiedSpots() {
  m.clear();
  for(int i=0; i<8; i++){
    for(int j=0; j<8; j++){
      if(occupiedSpots[i][j] == 1)
        m.setDot(j, i, true);  
    }
  }
}

void keypadEvent(KeypadEvent key) {
  if(keypad.getState()==PRESSED){
    if (key == '6') {
      moveCurrentBlockRight();
    }  
    if (key == '4') {
      moveCurrentBlockLeft();
    }
    if (key == '8') {
      fallOneRow();
    }
    if (key == '5') {
      rotateCurrentBlock();
    }
  } else {
    if(keypad.getState()==HOLD && key == '8') {
      while(!currentBlockReachedGround){
        fallOneRow();
      }
    }
  }
}

bool gameIsOver(){
  for(int i=0; i<currentBlockComponents; i++){
    if(currentBlock[i].y <= 0 && currentBlockReachedGround){
      return true;
    }
  }
  return false;
}

void createBlock() {
  currentBlockComponents = 4;
  currentBlock = (Point*)malloc(currentBlockComponents*sizeof(Point));
  blockType = rand() % 7;
  rotationIndex = 0;
}

void setCurrentBlockCoordinates(int xCenter, int yCenter){
  switch(blockType) {
    case 0:
      if(rotationIndex % 4 == 0){
        currentBlock[0].x = xCenter;         // X
        currentBlock[0].y = yCenter;         // X 
        currentBlock[1].x = xCenter;         // X
        currentBlock[1].y = yCenter-1;       // X
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter+1;
        currentBlock[3].x = xCenter;
        currentBlock[3].y = yCenter+2;
      }
      if(rotationIndex % 4 == 1){
        currentBlock[0].x = xCenter;         // X X X X        
        currentBlock[0].y = yCenter;       
        currentBlock[1].x = xCenter+1;        
        currentBlock[1].y = yCenter;        
        currentBlock[2].x = xCenter-1;
        currentBlock[2].y = yCenter;
        currentBlock[3].x = xCenter-2;
        currentBlock[3].y = yCenter;
      }
      if(rotationIndex % 4 == 2){
        currentBlock[0].x = xCenter;         // X
        currentBlock[0].y = yCenter;         // X 
        currentBlock[1].x = xCenter;         // X
        currentBlock[1].y = yCenter-1;       // X
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter-2;
        currentBlock[3].x = xCenter;
        currentBlock[3].y = yCenter+1;
      }
      if(rotationIndex % 4 == 3){
        currentBlock[0].x = xCenter;         // X X X X        
        currentBlock[0].y = yCenter;       
        currentBlock[1].x = xCenter-1;        
        currentBlock[1].y = yCenter;        
        currentBlock[2].x = xCenter+1;
        currentBlock[2].y = yCenter;
        currentBlock[3].x = xCenter+2;
        currentBlock[3].y = yCenter;
      }      
      break;
    case 1:
      currentBlock[0].x = xCenter;         // X X
      currentBlock[0].y = yCenter;         // X X
      currentBlock[1].x = xCenter+1;
      currentBlock[1].y = yCenter;
      currentBlock[2].x = xCenter;
      currentBlock[2].y = yCenter+1;
      currentBlock[3].x = xCenter+1;
      currentBlock[3].y = yCenter+1;
      break;
    case 2:
      if(rotationIndex % 4 == 0){
        currentBlock[0].x = xCenter;        //   X
        currentBlock[0].y = yCenter;        // X X X
        currentBlock[1].x = xCenter-1;
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter+1;
        currentBlock[2].y = yCenter;
        currentBlock[3].x = xCenter;
        currentBlock[3].y = yCenter-1;
      }
      if(rotationIndex % 4 == 1){
        currentBlock[0].x = xCenter;        // X  
        currentBlock[0].y = yCenter;        // X X
        currentBlock[1].x = xCenter+1;      // X  
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter+1;
        currentBlock[3].x = xCenter;
        currentBlock[3].y = yCenter-1;
      }
      if(rotationIndex % 4 == 2){
        currentBlock[0].x = xCenter;        // X X X  
        currentBlock[0].y = yCenter;        //   X
        currentBlock[1].x = xCenter+1;      
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter-1;
        currentBlock[2].y = yCenter;
        currentBlock[3].x = xCenter;
        currentBlock[3].y = yCenter+1;
      }
      if(rotationIndex % 4 == 3){
        currentBlock[0].x = xCenter;        //   X  
        currentBlock[0].y = yCenter;        // X X
        currentBlock[1].x = xCenter;        //   X  
        currentBlock[1].y = yCenter+1;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter-1;
        currentBlock[3].x = xCenter-1;
        currentBlock[3].y = yCenter;
      }
      break;
    case 3:
      if(rotationIndex % 4 == 0) {
        currentBlock[0].x = xCenter;         //   X
        currentBlock[0].y = yCenter;         // X X
        currentBlock[1].x = xCenter-1;       // X
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter-1;
        currentBlock[3].x = xCenter-1;
        currentBlock[3].y = yCenter+1;
      }
      if(rotationIndex % 4 == 1) {
        currentBlock[0].x = xCenter;         // X X
        currentBlock[0].y = yCenter;         //   X X
        currentBlock[1].x = xCenter+1;      
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter-1;
        currentBlock[3].x = xCenter-1;
        currentBlock[3].y = yCenter-1;
      }
      if(rotationIndex % 4 == 2) {
        currentBlock[0].x = xCenter;         //   X
        currentBlock[0].y = yCenter;         // X X
        currentBlock[1].x = xCenter+1;       // X
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter+1;
        currentBlock[3].x = xCenter+1;
        currentBlock[3].y = yCenter-1;
      }
      if(rotationIndex % 4 == 3) {
        currentBlock[0].x = xCenter;         // X X
        currentBlock[0].y = yCenter;         //   X X
        currentBlock[1].x = xCenter-1;      
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter+1;
        currentBlock[3].x = xCenter+1;
        currentBlock[3].y = yCenter+1;
      }
      break;
    case 4:
      if(rotationIndex % 4 == 0) {
        currentBlock[0].x = xCenter;         // X
        currentBlock[0].y = yCenter;         // X X
        currentBlock[1].x = xCenter;         //   X
        currentBlock[1].y = yCenter-1;
        currentBlock[2].x = xCenter+1;
        currentBlock[2].y = yCenter;
        currentBlock[3].x = xCenter+1;
        currentBlock[3].y = yCenter+1;
      }
      if(rotationIndex % 4 == 1) {
        currentBlock[0].x = xCenter;         //   X X
        currentBlock[0].y = yCenter;         // X X
        currentBlock[1].x = xCenter+1;         
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter+1;
        currentBlock[3].x = xCenter-1;
        currentBlock[3].y = yCenter+1;
      }
      if(rotationIndex % 4 == 2) {
        currentBlock[0].x = xCenter;         // X
        currentBlock[0].y = yCenter;         // X X
        currentBlock[1].x = xCenter;         //   X
        currentBlock[1].y = yCenter+1;
        currentBlock[2].x = xCenter-1;
        currentBlock[2].y = yCenter;
        currentBlock[3].x = xCenter-1;
        currentBlock[3].y = yCenter-1;
      }
      if(rotationIndex % 4 == 3) {
        currentBlock[0].x = xCenter;         //   X X
        currentBlock[0].y = yCenter;         // X X
        currentBlock[1].x = xCenter-1;         
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter-1;
        currentBlock[3].x = xCenter+1;
        currentBlock[3].y = yCenter-1;
      }
      break;
    case 5:
      if(rotationIndex % 4 == 0) {
        currentBlock[0].x = xCenter;         // X
        currentBlock[0].y = yCenter;         // X
        currentBlock[1].x = xCenter;         // X X
        currentBlock[1].y = yCenter-1;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter+1;
        currentBlock[3].x = xCenter+1;
        currentBlock[3].y = yCenter+1;
      }
      if(rotationIndex % 4 == 1) {
        currentBlock[0].x = xCenter;         // X X X
        currentBlock[0].y = yCenter;         // X
        currentBlock[1].x = xCenter+1;         
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter-1;
        currentBlock[2].y = yCenter;
        currentBlock[3].x = xCenter-1;
        currentBlock[3].y = yCenter+1;
      }
      if(rotationIndex % 4 == 2) {
        currentBlock[0].x = xCenter;         // X X
        currentBlock[0].y = yCenter;         //   X
        currentBlock[1].x = xCenter;         //   X
        currentBlock[1].y = yCenter-1;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter+1;
        currentBlock[3].x = xCenter-1;
        currentBlock[3].y = yCenter-1;
      }
      if(rotationIndex % 4 == 3) {
        currentBlock[0].x = xCenter;         //     X
        currentBlock[0].y = yCenter;         // X X X
        currentBlock[1].x = xCenter+1;         
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter-1;
        currentBlock[2].y = yCenter;
        currentBlock[3].x = xCenter+1;
        currentBlock[3].y = yCenter-1;
      }
      break;
    case 6:
      if(rotationIndex % 4 == 0) {
        currentBlock[0].x = xCenter;         //   X
        currentBlock[0].y = yCenter;         //   X
        currentBlock[1].x = xCenter;         // X X
        currentBlock[1].y = yCenter-1;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter+1;
        currentBlock[3].x = xCenter-1;
        currentBlock[3].y = yCenter+1;
      }
      if(rotationIndex % 4 == 1) {
        currentBlock[0].x = xCenter;         // X 
        currentBlock[0].y = yCenter;         // X X X
        currentBlock[1].x = xCenter-1;        
        currentBlock[1].y = yCenter-1;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter-1;
        currentBlock[3].x = xCenter+1;
        currentBlock[3].y = yCenter;
      }
      if(rotationIndex % 4 == 2) {
        currentBlock[0].x = xCenter;         // X X
        currentBlock[0].y = yCenter;         // X 
        currentBlock[1].x = xCenter;         // X 
        currentBlock[1].y = yCenter-1;
        currentBlock[2].x = xCenter+1;
        currentBlock[2].y = yCenter-1;
        currentBlock[3].x = xCenter;
        currentBlock[3].y = yCenter+1;
      }
      if(rotationIndex % 4 == 3) {
        currentBlock[0].x = xCenter;         // X X X 
        currentBlock[0].y = yCenter;         //     X 
        currentBlock[1].x = xCenter-1;        
        currentBlock[1].y = yCenter;
        currentBlock[2].x = xCenter;
        currentBlock[2].y = yCenter+1;
        currentBlock[3].x = xCenter+1;
        currentBlock[3].y = yCenter+1;
      }
      break;
  }
}
