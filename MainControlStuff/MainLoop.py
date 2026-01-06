import sys
import XYZandANGs as za
sys.path.append("/home/dogo/robot")
import time
import math
import os
from Server.UDP_Controller import udpServer
import RobotState
rs = RobotState.State()


def startUP():
    udpServer.runUDPServer()
def startUpStuff():
    udpServer.runUDPServer()
def waitForController():
    print("waiting for controller")
    while udpServer.getControllerData() is None:
        time.sleep(0.1)
if __name__ == '__main__':
    startUpStuff()
    waitForController()
    rs.ControllerUpdate(udpServer.cState)
    rs.currentAction.start(0)
    print("starting actions")
    while True:
            rs.ControllerUpdate(udpServer.cState)
            rs.update()
            t = time.time()+0.01
            while t>time.time():
                pass