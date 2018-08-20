___author__ = 'Brian'



import threading
import chess
import time
import logging
import serial
import chess.uci
from utilities import *
from timecontrol import *
from picochess import *

arduino = serial.Serial('COM4', 115200, timeout=.1)
time.sleep(2)
class SensorBoard(Observable, threading.Thread):
    def __init__(self):
        super(SensorBoard, self).__init__()
        self.flip_board = False
        #self.arduino = serial.Serial('COM3', 115200, timeout=.1)

    def Light_Square(self,sq,on):
        if on:
            sq = "L" + str(sq)
            arduino.write(str.encode(sq))
        else:
            sq = "C" + str(sq)
            arduino.write(str.encode(sq))
        arduino.flush()


    def run(self):
        while True:
            btxt = ""
            if arduino.inWaiting()>0:
                btxt = arduino.readline().strip()
                arduino.flush()
            if btxt:
                print(btxt)
                cmd = btxt.decode('utf-8').lower()

                #raw = input('PicoChess v' + version + ':>').strip()
                #cmd = raw.lower()
                print(cmd)
                try:
                    # commands like "newgame:<w|b>" or "setup:<legal_fen_string>"
                    # or "print:<legal_fen_string>"
                    #
                    # for simulating a dgt board use the following commands
                    # "fen:<legal_fen_string>" or "button:<0-4>"
                    #
                    # everything else is regarded as a move string
                    if cmd.startswith('newgame:'):
                        side = cmd.split(':')[1]
                        if side == 'w':
                            #self.flip_board = False
                            #self.fire(Event.FEN(fen='rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR'))
                            self.fire(Event.NEW_GAME(pos960=518))
                        elif side == 'b':
                            self.fire(Event.NEW_GAME)
                            #self.flip_board = True
                            #self.fire(Event.FEN(fen='RNBKQBNR/PPPPPPPP/8/8/8/8/pppppppp/rnbkqbnr'))
                        else:
                            raise ValueError(cmd)
                    else:

                        if cmd.startswith('FEN:'):
                            fen = cmd.split(':')[1]
                            self.fire(Event.FEN(fen=fen.split(' ')[0]))
                        elif cmd.startswith('b:'):
                            button = int(cmd.split(':')[1])
                            if button not in range(6):
                                raise ValueError(button)
                            if button==5:
                                button=0x40
                            self.fire(Event.KEYBOARD_BUTTON(button=button, dev = 'ser'))

                        elif cmd.startswith('l:'):
                            from_square = cmd.split(':')[1]
                            self.fire(Event.LIFT_PIECE(square=from_square))
                        #elif cmd.startswith('d:'):
                         #   to_square = cmd.split(':')[1]
                         #   self.fire(Event.DROP_PIECE(square=to_square))
                         #   print("Drop Piece")
                        elif cmd.startswith("tb:"):
                             self.fire(Event.TAKE_BACK())
                        elif cmd.startswith('shutdown'):
                            self.fire(Event.SHUTDOWN(dev='ser'))
                        elif cmd.startswith('reboot'):
                            self.fire(Event.REBOOT(dev='ser'))
                        elif cmd.startswith('done'):
                            print("ComputerMove done on board")
                            if keyboard_last_fen is not None:
                                self.fire(Event.KEYBOARD_FEN(fen=keyboard_last_fen))
                        else:
                            move = chess.Move.from_uci(cmd)
                            self.fire(Event.KEYBOARD_MOVE(move=move))
                            #print("command not recognised ", cmd)  #bl
                except ValueError as e:
                    logging.warning('Invalid user input [%s]', cmd)
            time.sleep(.2) # sleep .2s





class BoardDisplay(DisplayMsg, threading.Thread):

    def __init__(self):
        super(BoardDisplay, self).__init__()
        #arduino = serial.Serial('COM3', 115200, timeout=.1)
        self.enginename = ''
        self.time_left = (0, 0, 0)
        self.time_right = (0, 0, 0)
        self.color = 255
        self.level = ''
        #self.timetext = ""
        self.info = 'Picochess'
        self.game = chess.Board()

        self.arduino = arduino

    def Light_Square(self,sq,on):
        if on:
            sq = "L" + str(sq)
            self.arduino.write(str.encode(sq))
        else:
            sq = "C" + str(sq)
            self.arduino.write(str.encode(sq))
        self.arduino.flush()

    def SendMove(self, frm, to):
        sq = "F" + str(frm)
        self.arduino.write(str.encode(sq))
        time.sleep(.5)
        self.arduino.flush()
        sq = "T" + str(to)
        self.arduino.write(str.encode(sq))

        self.arduino.flush()

    def SendText(self, txt, row):
        sq = "D" + row + txt
        a = sq.ljust(20)
        self.arduino.write(str.encode(a))
        time.sleep(.2)
        self.arduino.flush()


    def run(self):
        global keyboard_last_fen
        logging.info('msg_queue terminaldisplay ready')
        while True:
            # Check if we have something to display
            try:
                message = self.msg_queue.get()

                if not isinstance(message, Message.DGT_SERIAL_NR):
                    logging.debug('received message from msg_queue: %s', message)
                    #print(message)
                if isinstance(message, Message.COMPUTER_MOVE):
                    game_copy = message.game.copy()
                    game_copy.push(message.move)
                    self.game = game_copy
                    keyboard_last_fen = game_copy.fen().split(' ')[0]
                    #Observable.fire(Event.KEYBOARD_FEN(fen=keyboard_last_fen))
                    mov = message.move.from_square
                    to = message.move.to_square
                    #self.Light_Square(mov, True)
                    #self.Light_Square(to, True)
                    self.SendMove(mov,to)
                
                elif isinstance(message, Message.CLOCK_TIME):
                    time_u = message.time_white
                    time_c = message.time_black
                    l_hms = hms_time(time_u)
                    r_hms = hms_time(time_c)
                    self.time_left = '{}:{:02d}.{:02d}'.format(l_hms[0], l_hms[1], l_hms[2])
                    self.time_right = '{}:{:02d}.{:02d}'.format(r_hms[0], r_hms[1], r_hms[2])
                    clck = 'T{} - {}'.format(self.time_left,self.time_right)
                    print(clck)
                    self.SendText(clck,'2')
                    #self.lcd_string('{} - {}'.format(self.time_left,self.time_right), self.LCD_LINE_4)

                elif isinstance(message, Message.COMPUTER_MOVE_DONE):
                    print("ComputerMoveDone")

                elif isinstance(message, Message.ENGINE_STARTUP):
                    print("Level index ",message.level_index)

                    
                elif isinstance(message, Message.ENGINE_READY):
                    #print(message.engine_name)
                    self.enginename = message.engine_name
                    #self.lcd_string(self.enginename, self.LCD_LINE_1)
                    self.SendText(str(self.enginename), '1')

                elif isinstance(message, Message.SYSTEM_INFO):
                    engine_name = message.info['engine_name']

                    self.user_name = message.info['user_name']
                    self.user_elo = message.info['user_elo']
                    self.enginename = engine_name
                    #self.lcd_string(self.enginename, self.LCD_LINE_1)
                    self.SendText(str(self.enginename), '0')
                    
                elif isinstance(message, Message.STARTUP_INFO):
                    level = message.info['level_text'].l
                    #self.lcd_string(level, self.LCD_LINE_2)
                    timetext = message.info['time_text'].s
                    #self.lcd_string(timetext, self.LCD_LINE_3)
                    self.SendText(timetext, '2')
                    self.SendText(level, '1')
                                    
                elif(isinstance(message, Message.START_NEW_GAME)):                                    
                    #print(message.game)
                    self.game = message.game                    

                elif (isinstance(message, Message.USER_MOVE_DONE)):
                    self.game=message.game
                    print("User Move Done");
                    

                elif isinstance(message, Message.LEVEL):
                    print(message.level_text.m)
                    level_text= message.level_text.l
                    #self.lcd_string(level_text, self.LCD_LINE_2)
                    self.SendText(level_text, '2')
                    print(message.level_text.m)
                    
                elif isinstance(message, Message.TIME_CONTROL):
                    #print(message.level_text.m)
                    tcinit = message.tc_init
                    if tcinit['mode'] == TimeMode.BLITZ:
                        t = 'Blitz Game {:d} '.format(tcinit['blitz'])
                    elif tcinit['mode'] == TimeMode.FISCHER:
                        t= 'Fisher {:d} {:d}'.format(tcinit['blitz'], tcinit['fischer'])
                    elif tcinit['mode'] == TimeMode.FIXED:
                        t='Move Time {:d}'.format(tcinit['fixed'])
                    self.timetext= t
                    #self.lcd_string(self.timetext, self.LCD_LINE_3)
                    self.SendText(self.timetext, '2')

                elif isinstance(message, Message.DISPLAY_TEXT):
                    print(message.text['msg'])
                    self.info = message.text['msg']
                    #self.lcd_string(self.info, self.LCD_LINE_4)
                    self.SendText(self.info, '3')

                elif isinstance(message, Message.GAME_ENDS):
                    print(message.result.name)
                    self.info = message.result.name
                    #self.lcd_string(self.info, self.LCD_LINE_4)

                elif isinstance(message, Message.EXIT_MENU):
                    self.info = ''
                    

                elif isinstance(message, Message.TAKE_BACK):
                    print(message.game)

            except queue.Empty:
                pass
