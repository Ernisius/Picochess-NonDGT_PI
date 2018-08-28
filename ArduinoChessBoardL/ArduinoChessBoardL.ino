
#include "Adafruit_LEDBackpack.h"
#include <Wire.h>
#include "Adafruit_GFX.h"


Adafruit_LEDBackpack matrix = Adafruit_LEDBackpack();

#define White 0
#define Black 1;
#define  SR_Sin  2  //4021 #3
#define  PSCont  3  //4021 #9
#define  SR_SCLK  4  //4021 #10

#define  Cntr_CLK 5   //4017 #14
#define  Cntr_CLR 6   //4017 #15  //4017 #13 grnd


const unsigned char ttable[7][4] = {            //for encoders
  {0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x10},
  {0x3, 0x2, 0x0, 0x0}, {0x3, 0x2, 0x1, 0x0},
  {0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x20},
  {0x6, 0x5, 0x4, 0x0},
};
int speakerPin = 12;

int numTones = 10;
int tones[] = {261, 277, 294, 311, 330, 349, 370, 392, 415, 440};
//            mid C  C#   D    D#   E    F    F#   G    G#   A

char * boardsquare[] = {"A1", "B1", "C1", "D1", "E1", "F1", "G1", "H1",
                        "A2", "B2", "C2", "D2", "E2", "F2", "G2", "H2",
                        "A3", "B3", "C3", "D3", "E3", "F3", "G3", "H3",
                        "A4", "B4", "C4", "D4", "E4", "F4", "G4", "H4",
                        "A5", "B5", "C5", "D5", "E5", "F5", "G5", "H5",
                        "A6", "B6", "C6", "D6", "E6", "F6", "G6", "H6",
                        "A7", "B7", "C7", "D7", "E7", "F7", "G7", "H7",
                        "A8", "B8", "C8", "D8", "E8", "F8", "G8", "H8"
                       };
int move_to, move_from;
bool computermove = false;
bool gameinprogess = false;
int keychanged;
int buttonmask;
bool lifted;
int boardmatrix[8][8];     //kb matrix
long startTime = 0;  // the last time the output pin was toggled
long btnDebounce;

int key;
int row;
int column;
int state;
int rpState;

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
int buttonsave;
void setup()
{
  pinMode(SR_Sin , INPUT);
  pinMode(PSCont, OUTPUT);
  pinMode(SR_SCLK, OUTPUT);

  pinMode(SR_Sin , INPUT);
  pinMode(Cntr_CLK, OUTPUT);
  pinMode(Cntr_CLR, OUTPUT);

  Serial.begin(115200);
  Serial.setTimeout(50);
  matrix.begin(0x70);  // pass in the address
  matrix.clear();
  matrix.blinkRate(HT16K33_BLINK_2HZ);

  newgame(White);
  fillMatrix();

  
  matrix.writeDisplay();
}
void printmatrix()
{
  for (int row = 0; row < 8; row++)
  {

    for (int column = 7; column >= 0; column--)
    {

      Serial.print(boardmatrix[row][column]);

    }
    Serial.println();
  }
}
void loop()
{
  static bool enablebuttons;
  //bool buttonEnable = true;
  long buttonDelay;

  FromPython();

  if (GetButtonMask())
  {

    if (buttonmask == 0)
    {
      enablebuttons = true;
    }
    else if (enablebuttons)
    {
      enablebuttons = false;
      switch (buttonmask)
      {
        case 1:
          ToPython("B:0");
          break;
        case 2:
          ToPython("B:1");
          break;
        case 4:
          ToPython("B:2");
          break;
        case 8:
          ToPython("B:3");
          break;
        case 16:
          ToPython("B:4");
          break;

      }
    }
  }

  if (KeyChanged())
  {

    FlashSquare(keychanged, 100);
    if (computermove)
    {
      MakeComputerMove(keychanged);
    }
    else
    {
      MakeUserMove(keychanged);
    }
    if (!lifted)
    {
      CheckBoardPosition();
    }
  }

}
bool GetButtonMask(void)
{
  long debounceDelay = 200;
  if (buttonmask == buttonsave)
  {
    if ((millis() - startTime) > debounceDelay )
    {
      return true;
    }
  }
  else
  {
    startTime = millis();
    buttonsave = buttonmask;
  }
  return false;
}
void MakeComputerMove(int square)
{
  if (lifted)
  {
    if (square == move_from)
    {
      beep(450, 100);
      LightSquare(move_to, true);
      matrix.writeDisplay();
    }
    else if (square == move_to)  //remove taken piece
    {
      //do nothing
    }
    else
    {
      beep(550, 150);
      beep(450, 100); //wrong
    }
  }

  else   //dropped
  {
    if (square == move_to)
    {
      beep(350, 100);
      matrix.clear();
      matrix.writeDisplay();
      computermove = false;
      ToPython("Done");
    }
  }
}
void MakeUserMove(int square)
{
  static bool startsquare;
  if (lifted)
  {
    if (!startsquare)
    {
      beep(500, 100);
      LightSquare(square, true);
      matrix.writeDisplay();
      move_from = square;
      startsquare = true;
    }
    else
    {
      // remove oppenents piece
    }
  }
  else  //dropped
  {
    if (move_from != square)    //!put back on same square
    {
      matrix.clear();
      String move = boardsquare[move_from];
      ToPython(move + boardsquare[square]);
      FlashSquare(square, 100);
      beep(400, 100);
    }
    else
    {
      FlashSquare(square, 100);  //dropped on same square
      beep(300, 200);
    }
    startsquare = false;
  }
}

void ToPython(String s)
{
  Serial.println(s);// write a string
}

void FromPython()
{
  if (Serial.available() > 2)
  {
    char data = Serial.read();
    //    if (data == 'F')  //Flash
    //    {
    //      int sq = Serial.parseInt();
    //      FlashSquare(sq, 50);
    //    }
    //    if (data == 'L') //LED on
    //    {
    //      beep(600);
    //      int sq = Serial.parseInt();
    //      LightSquare(sq,true);
    //      matrix.writeDisplay();
    //    }
    //    if (data == 'C')  //LED Off
    //    {
    //       int sq = Serial.parseInt();
    //       ClearLeds();
    //    }
    //    if (data == 'B')
    //    {
    //       int sq = Serial.parseInt();
    //       beep(sq);
    //    }
    if (data == 'F')    //from computer 'from square'
    {
      gameinprogess = true;
      computermove = true;
      move_from = Serial.parseInt();
      beep(350, 50);
      LightSquare(move_from, true);
      matrix.writeDisplay();
    }
    if (data == 'T')  //from computer 'to square'
    {
      move_to = Serial.parseInt();
      computermove = true;
      //LightSquare(sq,true);
      // beep(450,50);
      // matrix.writeDisplay();
    }
  }
}

bool KeyChanged()
{
  static int lastkey;

  if (checkmatrix() != 99)
  { //key changed

    boardmatrix[row][column] = state;
    keychanged = key;
    if (state == 1) lifted = false;
    else lifted = true;
    return true;
  }
  return false;
}


int checkmatrix()
{
  //Serial.println("CheckMatrix");
  static int lastkey;
  digitalWrite(Cntr_CLR, HIGH); //reset 4017
  digitalWrite(Cntr_CLR, LOW);

  for (row = 0; row < 8; row++)
  {
    digitalWrite(Cntr_CLK, HIGH);  //clock 4017 -use outputs 1-9 ; not 0
    digitalWrite(Cntr_CLK, LOW);

    digitalWrite(PSCont, HIGH);   //latch paralel data
    digitalWrite(PSCont, LOW);
    for (column = 7; column >= 0; column--)
    {
      int data = digitalRead(SR_Sin);
      if (data != boardmatrix[row][column])
      {
        state = data;
        key = (row * 8 + column);
        if (key == lastkey)
        {
          lastkey = 0;
          return (key);
        }
        else
        {
          lastkey = key;
        }
      }
      digitalWrite(SR_SCLK, HIGH);            //next bit
      digitalWrite(SR_SCLK, LOW);
    }
  }
  buttonmask = 0;
  //last row for buttons
  digitalWrite(Cntr_CLK, HIGH);  //clock 4017 -use outputs 1-9 ; not 0
  digitalWrite(Cntr_CLK, LOW);

  digitalWrite(PSCont, HIGH);   //latch paralel data
  digitalWrite(PSCont, LOW);
  for (column = 7; column >= 0; column--)
  {
    int data = digitalRead(SR_Sin);
    buttonmask = buttonmask | data << column;
    digitalWrite(SR_SCLK, HIGH);            //next bit
    digitalWrite(SR_SCLK, LOW);
  }
  return (99);
}

// reads the current switch states into matrix
void fillMatrix()
{
  digitalWrite(Cntr_CLR, HIGH); //reset 4017
  digitalWrite(Cntr_CLR, LOW);

  for (int row = 0; row < 8; row++)
  {
    digitalWrite(Cntr_CLK, HIGH);  //clock 4017 -use outputs 1-9 ; not 0
    digitalWrite(Cntr_CLK, LOW);

    digitalWrite(PSCont, HIGH);   //latch paralel data
    digitalWrite(PSCont, LOW);
    for (int column = 7; column >= 0; column--)
    {
      int data = digitalRead(SR_Sin);
      boardmatrix[row][column] = data;

      digitalWrite(SR_SCLK, HIGH);            //next bit
      digitalWrite(SR_SCLK, LOW);
    }
  }
  //printmatrix();
}
void  lightled(int row, int column, bool on)
{
  if (column < 8)
  {
    if (on) matrix.displaybuffer[column] |= (1 << row);
    else matrix.displaybuffer[column] &= ~(1 << row);
  }
  else if (row < 8) //column 8
  {
    if (on) matrix.displaybuffer[row] |= (1 << 9);
    else matrix.displaybuffer[row] &= ~(1 << 9);
  }
  else  //column and row 8
  {
    if (on) matrix.displaybuffer[0] |= (1 << 10);
    else matrix.displaybuffer[0] &= ~(1 << 10);
  }
}

void FlashSquare(int square, int milliseconds)
{
  matrix.clear();
  LightSquare(square, true);
  matrix.writeDisplay();
  //beep(350,milliseconds);
  delay(milliseconds);
  LightSquare(square, false);
  matrix.writeDisplay();
}

void  LightSquare(int square, bool on)
{
  int row = int (square / 8);
  int column = square & 7;
  lightled(row, column, on);
  lightled(row + 1, column, on);
  lightled(row, column + 1, on);
  lightled(row + 1, column + 1, on);
  matrix.writeDisplay();
}

void beep(int note1, int lengh)
{
  tone(speakerPin, note1);
  delay(lengh);
  noTone(speakerPin);
  //delay(100);
  //tone(speakerPin, note2);
  //delay(50);
  //noTone(speakerPin);

}


void newgame(int color)
{
  int squarecount = 0;
  while (squarecount < 32)
  {
    fillMatrix(); //piece positions to matrix
    squarecount = 0;
    for (row = 0; row < 8; row++)
    {
      for (column = 0; column < 8; column++)
      {
        if (row == 0 || row == 1 || row == 6 || row == 7)
        {
          if (boardmatrix[row][column] == 1) //occupied
          {
            squarecount++;
            
          }
          else
          {
            LightSquare(row * 8 + column, true);
          }
        }
      }
    }
    matrix.clear();
  }
   computermove = false;
   if(color)   ToPython("newgame:b");
   else ToPython("newgame:w");
}

//'RNBKQBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbqqbnr' = REBOOT


void CheckBoardPosition()
{
  fillMatrix(); //piece positions to matrix
  int squarecount = 0;
  for (row = 0; row < 8; row++)
  {
    for (column = 0; column < 8; column++)
    {
      if (row == 0 || row == 1 || row == 6 || row == 7)
      {
        if (boardmatrix[row][column] == 1) //occupied
        {
          squarecount++;
          //LightSquare(row * 8 + column, true);
        }
      }
    }
  }
  if (squarecount == 32)
  {
    if (boardmatrix[2][0] == 1)
    {
      ToPython("shutdown");  //reboot
    }
    else if (boardmatrix[2][1] == 1)
    {
      ToPython("reboot");  //reboot
    }
    else ToPython("newgame:w");

  }
}


