import sys
import XYZandANGs as za
sys.path.append("/home/dogo/robot")
import time
import math
import os
from StateSystem.State import State
st = State()
from Server.BlueServer import BLUETOOTH_Server
import RobotState
rs = RobotState.State()

def startUpStuff():
    BLU = BLUETOOTH_Server(st)
    BLU.runServer()
def waitForController():
    print("waiting for controller")
    #while udpServer.getControllerData() is None:
     #   time.sleep(0.1)

if __name__ == '__main__':
    startUpStuff()
    waitForController()
    rs.ControllerUpdate(st.controller)
    rs.currentAction.start(0)
    print("starting actions")
    while True:
        if True:
            rs.ControllerUpdate(st.controller)
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