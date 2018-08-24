//#include "ledmatrix.h"
#include "Switchmatrix.h"
#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include <LiquidCrystal_I2C.h>
//I2C pins declaration
LiquidCrystal_I2C lcd(0x3f, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 
SwitchMatrix sw;

int White=0;
int Black=1;
int buttonsave;
int move_to, move_from;

Adafruit_LEDBackpack matrix = Adafruit_LEDBackpack();

static long startTime = 0;  // the last time the output pin was toggled
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
bool computermove = false;
void setup()
{
  
  Serial.begin(115200);
  Serial.setTimeout(50);
  matrix.begin(0x70);  // pass in the address
  sw.fillMatrix();
  buttonsave=0;
  //newgame(White);
  lcd.begin(20,4);//Defining 20 columns and 4 rows of lcd display
  lcd.backlight();//To Power ON the back light
//lcd.backlight();// To Power OFF the back light

  lcd.setCursor(0,0); //Defining positon to write from first row,first column .
  lcd.print("PicoChess nN"); 
  lcd.setCursor(0,1);  
  lcd.print("Modified by BRL");
  lcd.setCursor(0,2);  
  lcd.print("");
  lcd.setCursor(0,3);  
  lcd.print("");

}

void loop() 
{
  static bool enablebuttons;
  
  FromPython();
  if(GetButtonMask())
  {
    if(sw.buttonmask==0)
    {
      enablebuttons=true;
    } 
    else if(enablebuttons)
    {
      enablebuttons=false;
      switch (sw.buttonmask)
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
        case 32:
          ToPython("B:5");
          //newgame(White);
        break;
        case 64:
          ToPython("B:6");
          // newgame(Black);
         break;
         case 128:
         //ToPython("B:7");
         ToPython("tb:");
        break;
      }
    }
  }
  if (sw.KeyChanged())
  {
    int key = sw.keychanged;
    Serial.println(key);
    bool lifted = sw.lifted;
    FlashSquare(key, 300);
//    if (computermove)
//    {
//      MakeComputerMove(key);
//    }
//    else
//    {
//      MakeUserMove(key);
//    }
//    
//    if (!lifted)
//    {
//      //CheckBoardPosition();
//    }
    
  }
}
void MakeComputerMove(int square)
{
  if (sw.lifted)
  {
    if (square == move_from)
    {
      // beep(450, 100);
      LightSquare(move_to);
      matrix.writeDisplay();
    }
    else if (square == move_to)  //remove taken piece
    {
      //do nothing
    }
    else
    {
      //beep(550, 150);
      //beep(450, 100); //wrong
    }
  }

  else   //dropped
  {
    if (square == move_to)
    {
      // beep(350, 100);
      matrix.clear();
      matrix.writeDisplay();
      computermove = false;
      ToPython("Done");
    }
  }
}
void MakeUserMove(int square)
{
  static bool moveStarted;
  if (sw.lifted)
  {
    if (!moveStarted)
    {
      //beep(500, 100);
      ToPython("l:" + String(square));
      LightSquare(square);
      matrix.writeDisplay();
      move_from = square;
      moveStarted = true;
    }
    else
    {
      // another piece lifted after move started so must be
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
      //beep(400, 100);
    }
    else
    {
      FlashSquare(square, 100);  //dropped on same square
      //beep(300, 200);
    }
    moveStarted = false;
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
     //lcd.setCursor(0,3); 
     // lcd.print(Serial.available()); 
    char data = Serial.read();
//    if (data == 'F')  //Flash
//    {
//      int sq = Serial.parseInt();
//      FlashSquare(sq, 50);
//    }
    if (data == 'L') //LED on
    {
      //beep(600,500);
      int sq = Serial.parseInt();
      LightSquare(sq);
      matrix.writeDisplay();
    }
    if (data == 'C')  //LED Off
    {
       int sq = Serial.parseInt();
       ClearLeds();
    }
    if (data == 'B')  //LED Off
    {
       int sq = Serial.parseInt();
       beep(sq);
    }
    if (data == 'D') //LCD Disply
    {
      int sq = Serial.parseInt();
      lcd.setCursor(0,sq);
      String s  = Serial.readString();
      lcd.print(s); 
    }
    if (data == 'F')    //from computer 'from square'
    {
      computermove = true;
      move_from = Serial.parseInt();
      beep(350);
      LightSquare(move_from);
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


bool GetButtonMask(void)
{
  long debounceDelay = 200;
  if (sw.buttonmask== buttonsave)
  {
    if((millis() - startTime) > debounceDelay )
     {
       return true;
     }
  }
  else 
  {
    startTime = millis();
    buttonsave=sw.buttonmask;
  }
  return false;
}



void newgame(int color)
{
  int brdsetup=0;
  
  while(brdsetup<32)
  {
    sw.fillMatrix(); //piece positions to matrix
    ClearLeds();
    
    brdsetup=32;
    for(int i=0;i<16;i++)
    {
      if(!sw.GetKeyState(i))  
      {
        brdsetup--;
        LightSquare(i);
      }
    }
     for(int i=48;i<64;i++)
    {
      if(!sw.GetKeyState(i))
     
      {
        brdsetup--;
        LightSquare(i);
      }
    }
    matrix.writeDisplay();
  }
  if(color)   ToPython("newgame:b");
  else ToPython("newgame:w");
}


void  lightled(int row,int column)
{
 
  if(column<8)
  {
    matrix.displaybuffer[column] |= (1<<row);
  }
  else if(row<8)  //column 8
  {
     matrix.displaybuffer[row] |= (1<<9);
  }
  else  //column and row 8
  {
    matrix.displaybuffer[0] |= (1<<10);
  }
}

void ClearLeds()
{
  for(int i=0;i<8;i++)
  {
    matrix.displaybuffer[i] =0;
  }
   matrix.writeDisplay();
}

void FlashSquare(int square, int milliseconds)
{
  ClearLeds();
  LightSquare(square);
  beep(350);
  matrix.writeDisplay();
  delay(milliseconds);
  ClearLeds();
}

void  LightSquare(int square)
{
    int row =int (square/8);
    int column = square & 7;
    lightled(row,column);
    lightled(row+1, column);
    lightled(row,column+1);
    lightled(row+1,column+1);
   
}
void CheckBoardPosition()
{
  sw.fillMatrix(); //piece positions to matrix
  int squarecount = 0;
  for (int row = 0; row < 8; row++)
  {
    for (int column = 0; column < 8; column++)
    {
      if (row == 0 || row == 1 || row == 6 || row == 7)
      {
        if (sw.boardmatrix[row][column] == 1) //occupied
        {
          squarecount++;
          //LightSquare(row * 8 + column, true);
        }
      }
    }
  }
   if (squarecount == 32)
  {
      computermove = false;
      ToPython("newgame:w");
    }

}
void beep(int note1)
{
  tone(speakerPin, note1);
  delay(50);
  noTone(speakerPin);
  //delay(100);
  //tone(speakerPin, note2);
  //delay(50);
  //noTone(speakerPin);

}

