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
        if True:
            rs.ControllerUpdate(udpServer.cState)
            rs.update()
            t = time.time()+0.01
            while t>time.time():
                pass
        else:
            rs.body.setLegPos(
                za.Leg_XYZ(1,0,150,0),
                za.Leg_XYZ(2,0,150,0),
                za.Leg_XYZ(3,0,150,0),
                za.Leg_XYZ(4,0,150,0)
            )