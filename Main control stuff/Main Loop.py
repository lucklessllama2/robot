import time

import Server.UDP_Controller
from Server.UDP_Controller import udpServer
import RobotState
rs = RobotState.State()


def startUpStuff():
    udpServer.runUDPServer()
def waitForController():
    while udpServer.cState is None:
        time.sleep(0.1)
if __name__ == '__main__':
    startUpStuff()
    waitForController()
    rs.currentAction.start(True)
    while True:
        rs.ControllerUpdate(udpServer.cState)
        rs.update()

