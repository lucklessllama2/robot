import sys
import XYZandANGs as za
sys.path.append("/home/dogo/robot")
import time
import math
import os
from Server.UDP_Controller import udpServer
import RobotState
rs = RobotState.State()


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
        if False:
            rs.ControllerUpdate(udpServer.cState)
            rs.update()
            t = time.time()+0.10
            while t>time.time():
                pass
        else:
            if False:
                d = 180
                l = 0
                h = 180
                r = (h-l)/d
                for i in range(d):
                    dg = r*i+l
                    print("dg:",dg)
                    j  = dg/180*math.pi
                    rs.body.getLeg(1).GoToANG(za.MOTOR_ANG(1,math.pi/2,math.pi/2,j))
                    time.sleep(2/d)
                for i in range(d):
                    dg=r*(d-i)+l
                    print("dg:",dg)
                    j/180*math.pi
                    rs.body.getLeg(1).GoToANG(za.MOTOR_ANG(1,math.pi/2,math.pi/2,j))
                    time.sleep(2/d)
            else:
                dg = 0
                j = dg/180*math.pi
                rs.body.getLeg(1).GoToANG(za.MOTOR_ANG(1,math.pi/2,math.pi/2,j))
