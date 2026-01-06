import time
import os
import Server.UDP_Controller
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
        rs.ControllerUpdate(udpServer.cState)
        rs.update()
        time.sleep(0.05)

