___author__ = 'Brian'



import threading
import chess
import time
import logging
import serial
import chess.uci
import smbus
from utilities import *
from timecontrol import *
from picochess import *




class SensorBoard(Observable, threading.Thread):
    def __init__(self):
        super(SensorBoard, self).__init__()
        self.flip_board = False
        self.arduino = serial.Serial('/dev/ttyUSB0', 115200, timeout=.1)

    def Light_Square(self,sq,on):
        if on:
            sq = "L" + str(sq)
            self.arduino.write(str.encode(sq))
        else:
            sq = "C" + str(sq)
            self.arduino.write(str.encode(sq))
        self.arduino.flush()


    def run(self):
        global playersturn
        while True:
            btxt = ""
            if self.arduino.inWaiting()>0:
                btxt = self.arduino.readline().strip()
                self.arduino.flush()
            if btxt:
                #print(btxt)
                cmd = btxt.decode('utf-8').lower()

                #raw = input('PicoChess v' + version + ':>').strip()
                #cmd = raw.lower()
                #print(cmd)
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
                        # if cmd.startswith('print:'):
                        #     fen = cmd.split(':')[1]
                        #     print(chess.Board(fen))
                        # elif cmd.startswith('setup:'):
                        #     fen = cmd.split(':')[1]
                        #     uci960 = False  # make it easy for the moment
                        #     bit_board = chess.Board(fen, uci960)
                        #     if bit_board.is_valid():
                        #         self.fire(Event.SETUP_POSITION(fen=bit_board.fen(), uci960=uci960))
                        #     else:
                        #         raise ValueError(fen)
                        # Here starts the simulation of a dgt-board!
                        # Let the user send events like the board would do
                        if cmd.startswith('FEN:'):
                            fen = cmd.split(':')[1]
                            # dgt board only sends the basic fen => be sure
                            # it's same no matter what fen the user entered
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
                        elif cmd.startswith('d:'):
                            to_square = cmd.split(':')[1]
                            self.fire(Event.DROP_PIECE(square=to_square))
                            #print("Drop Piece")
                        #elif cmd.startswith("tb:"):
                        #   self.fire(Event.TAKE_BACK())
                        elif cmd.startswith('shutdown'):
                            self.fire(Event.SHUTDOWN(dev='ser'))
                        elif cmd.startswith('reboot'):
                            self.fire(Event.REBOOT(dev='ser'))
                        elif cmd.startswith('done'):
                            #print("ComputerMove done on board")
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

    I2C_ADDR = 0x3f  # I2C device address
    LCD_WIDTH = 20  # Maximum characters per line

    # Define some device constants
    LCD_CHR = 1  # Mode - Sending data
    LCD_CMD = 0  # Mode - Sending command

    LCD_LINE_1 = 0x80  # LCD RAM address for the 1st line
    LCD_LINE_2 = 0xC0  # LCD RAM address for the 2nd line
    LCD_LINE_3 = 0x94  # LCD RAM address for the 3rd line
    LCD_LINE_4 = 0xD4  # LCD RAM address for the 4th line

    LCD_BACKLIGHT = 0x08  # On
    # LCD_BACKLIGHT = 0x00  # Off

    ENABLE = 0b00000100  # Enable bit

    # Timing constants
    E_PULSE = 0.0005
    E_DELAY = 0.0005

    # Open I2C interface
    # bus = smbus.SMBus(0)  # Rev 1 Pi uses 0
    bus = smbus.SMBus(1)  # Rev 2 Pi uses 1


    def __init__(self):
        super(BoardDisplay, self).__init__()
        self.arduino = serial.Serial('/dev/ttyUSB0', 115200, timeout=.1)
        self.enginename = ''
        self.time_left = (0, 0, 0)
        self.time_right = (0, 0, 0)
        self.color = 255
        self.level = ''
        #self.timetext = ""
        self.info = 'Picochess'
        self.game = chess.Board()

        self.lcd_init()
        self.lcd_string(">Raspberry pi chess<", self.LCD_LINE_1)
        self.lcd_string(">based on picochess<", self.LCD_LINE_2)
        self.lcd_string(">Modified by<", self.LCD_LINE_3)
        self.lcd_string(">Brian Loxton<", self.LCD_LINE_4)


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
    def lcd_init(self):
        # Initialise display
        self.lcd_byte(0x33, self.LCD_CMD)  # 110011 Initialise
        self.lcd_byte(0x32, self.LCD_CMD)  # 110010 Initialise
        self.lcd_byte(0x06, self.LCD_CMD)  # 000110 Cursor move direction
        self.lcd_byte(0x0C, self.LCD_CMD)  # 001100 Display On,Cursor Off, Blink Off
        self.lcd_byte(0x28, self.LCD_CMD)  # 101000 Data length, number of lines, font size
        self.lcd_byte(0x01, self.LCD_CMD)  # 000001 Clear display
        time.sleep(self.E_DELAY)

    def lcd_byte(self,bits, mode):
        # Send byte to data pins
        # bits = the data
        # mode = 1 for data
        #        0 for command

        bits_high = mode | (bits & 0xF0) | self.LCD_BACKLIGHT
        bits_low = mode | ((bits << 4) & 0xF0) |self.LCD_BACKLIGHT

        # High bits
        self.bus.write_byte(self.I2C_ADDR, bits_high)
        self.lcd_toggle_enable(bits_high)

        # Low bits
        self.bus.write_byte(self.I2C_ADDR, bits_low)
        self.lcd_toggle_enable(bits_low)

    def lcd_toggle_enable(self,bits):
        # Toggle enable
        time.sleep(self.E_DELAY)
        self.bus.write_byte(self.I2C_ADDR, (bits | self.ENABLE))
        time.sleep(self.E_PULSE)
        self.bus.write_byte(self.I2C_ADDR, (bits & ~self.ENABLE))
        time.sleep(self.E_DELAY)

    def lcd_string(self,message, line):
        # Send string to display

        message = message.ljust(self.LCD_WIDTH, " ")

        self.lcd_byte(line, self.LCD_CMD)

        for i in range(self.LCD_WIDTH):
            self.lcd_byte(ord(message[i]),self. LCD_CHR)


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
                    self.Light_Square(mov, True)
                    self.Light_Square(to, True)
                    self.SendMove(mov,to);
                
                elif isinstance(message, Message.CLOCK_TIME):
                    time_u = message.time_white
                    time_c = message.time_black
                    l_hms = hms_time(time_u)
                    r_hms = hms_time(time_c)
                    self.time_left = '{}:{:02d}.{:02d}'.format(l_hms[0], l_hms[1], l_hms[2])
                    self.time_right = '{}:{:02d}.{:02d}'.format(r_hms[0], r_hms[1], r_hms[2])
                    #print('Clock time: {} - {}'.format(self.time_left,self.time_right))
                    self.lcd_string('{} - {}'.format(self.time_left,self.time_right), self.LCD_LINE_4)

                #elif isinstance(message, Message.COMPUTER_MOVE_DONE):
                   # print("ComputerMoveDone")

                    

                #elif isinstance(message, Message.ENGINE_STARTUP):
                   # print(message.level_index)
                    
                elif isinstance(message, Message.ENGINE_READY):
                    #print(message.engine_name)
                    self.enginename = message.engine_name
                    self.lcd_string(self.enginename, self.LCD_LINE_1)

                elif isinstance(message, Message.SYSTEM_INFO):
                    engine_name = message.info['engine_name']

                    self.user_name = message.info['user_name']
                    self.user_elo = message.info['user_elo']
                    self.enginename = engine_name
                    self.lcd_string(self.enginename, self.LCD_LINE_1)
                    
                elif isinstance(message, Message.STARTUP_INFO):
                    level = message.info['level_text'].l
                    self.lcd_string(level, self.LCD_LINE_2)
                    timetext = message.info['time_text'].l
                    self.lcd_string(timetext, self.LCD_LINE_3)
                                    
                elif(isinstance(message, Message.START_NEW_GAME)):                                    
                    #print(message.game)
                    self.game = message.game                    

                elif (isinstance(message, Message.USER_MOVE_DONE)):
                    self.game=message.game
                    #print("User Move Done");
                    

                elif isinstance(message, Message.LEVEL):
                    #print(message.level_text.m)
                    level_text= message.level_text.l
                    self.lcd_string(level_text, self.LCD_LINE_2)
                    #print(message.level_text.m)
                    
                elif isinstance(message, Message.TIME_CONTROL):
                    #print(message.level_text.m)
                    tcinit = message.tc_init
                    if tcinit['mode'] == TimeMode.BLITZ:
                        t = 'Blitz Game {:d} '.format(tcinit['blitz'])
                    elif tcinit['mode'] == TimeMode.FISCHER:
                        t= 'Fisher {:d} {:d}'.format(tcinit['blitz'], tcinit['fischer'])
                    elif tcinit['mode'] == TimeMode.FIXED:
                        t='Move Time{:d}'.format(tcinit['fixed'])
                    self.timetext= t
                    self.lcd_string(self.timetext, self.LCD_LINE_3)

                elif isinstance(message, Message.DISPLAY_TEXT):
                    #print(message.text['msg'])
                    self.info = message.text['msg']
                    self.lcd_string(self.info, self.LCD_LINE_4)

                elif isinstance(message, Message.GAME_ENDS):
                    #print(message.result.name)
                    self.info = message.result.name
                    self.lcd_string(self.info, self.LCD_LINE_4)

                elif isinstance(message, Message.EXIT_MENU):
                    self.info = ''
                    

                #elif isinstance(message, Message.TAKE_BACK):
                    #print(message.game)

            except queue.Empty:
                pass
