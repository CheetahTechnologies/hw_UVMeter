from tkinter import *
import tkinter.ttk as ttk
import serial
import time
import subprocess
import os
from termcolor import colored
import serial.tools.list_ports
from colorama import Fore, Back, Style
from colorama import init
init()

global LightMeterID, PacketLength, ErrorThreshold, fetchFlag
LightMeterID = 0x03
PacketLength = 3
ErrorThreshold = 5
fetchFlag = FALSE

def serial_ports():
   return [p.device for p in serial.tools.list_ports.comports()]

def on_select(event=None):
   global ser
   ser= serial.Serial(
      port=serPort.get(),
      baudrate=9600,
      parity=serial.PARITY_NONE,
      stopbits=serial.STOPBITS_ONE,
      bytesize=serial.EIGHTBITS
   )
   ser.close()




def clearALL ():
   cls = lambda: os.system('cls')
   cls()

def stop():
   fetchFlag = FALSE
   ser.open()
   ser.flush()
   ser.write(b'r')
   ser.flush()
   ser.close()

def fetch():
   fetchFlag = TRUE
   errorFlag = 0
   while (fetchFlag):
      ser.open()
      ser.flush()
      ret = list(ser.readline(6))
      ser.flush()
      ser.close()

      if len(ret) == PacketLength and ret[0] == LightMeterID:
         UVI = ret[1]/10
         print("UVI:", UVI)
      elif errorFlag >= ErrorThreshold:
         ser.open()
         ser.flush()
         ser.write(b'r')
         ser.flush()
         ser.close()
         errorFlag = 0
         print("Device is reset!")
      else:
         errorFlag +=1




window=Tk()
window.title('Cheetah LightMeter GUI')
window.geometry("250x300+900+100")

serText = Text(window, height=1, width=23)
serText.place(x=20, y=50)
serText.insert(END, "USB-Serial Port:")
serText.configure(state="disabled")

serPort = ttk.Combobox(window, text="Port for USB-Serial #1", values=serial_ports())
serPort.place(x=20, y=80)
serPort.bind('<<ComboboxSelected>>', on_select)

butCheckAll = Button(window, text=" Fetch Data", bg='light grey', command=fetch)
butCheckAll.place(x=20, y=190)

butCheckAll = Button(window, text=" Stop", bg='light grey', command=stop)
butCheckAll.place(x=140, y=190)

butCheckAll = Button(window, text=" Clear Terminal ", bg='light grey', command=clearALL)
butCheckAll.place(x=20, y=250)

window.mainloop()