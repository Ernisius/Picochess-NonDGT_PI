#ifndef SwitchMatrix_h
#define SwitchMatrix_h


#include <stdlib.h>
#include <Arduino.h>

#define  SR_Sin  2  //4021 #3
#define  PSCont  3  //4021 #9
#define  SR_SCLK  4  //4021 #10

#define  Cntr_CLK 5   //4017 #14
#define  Cntr_CLR 6   //4017 #15  //4017 #13 grnd


class SwitchMatrix
{
  public:
  SwitchMatrix();
  void fillMatrix();
  bool KeyChanged(void);
  bool GetKeyState(int key);
  int keychanged;
  int buttonmask;
  bool lifted;
  int boardmatrix[8][8];     //kb matrix

  private:
  
  long startTime = 0;  // the last time the output pin was toggled

  long debounceDelay = 50;    // the debounce time;
  int key;
  int row;
  int column;
  int state;
  
  bool checkmatrix(void);

};
#endif
