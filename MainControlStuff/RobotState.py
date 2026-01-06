from __future__ import annotations

import socket
import time
from datetime import timedelta

from settings import bodyData

import math
from XYZandANGs import *

class State:
    def __init__(self):
        self.on:bool = False
        self.imuOn:bool = False
        self.currentAction:Action = Standing(self)
        self.Speed:float = 50
        self.bodyHeight:float = 150

        self.controllerState = None

        self.body:Body = Body()
    def ControllerUpdate(self,c):
        self.controllerState = c
    def update(self):
        self.currentAction.update()
class Action:
    def __init__(self,s):
        self.timeOffset = 0
        self.defaultState = []
        self.state:State = s
        self.DeltaTime = 0
        self.lastTime = 0
        self.running = False
    def start(self, timeOffSet):
        self.timeOffset = timeOffSet
        self.running = True
        self.lastTime = time.time()
        self.update()
    def update(self):
        self.DeltaTime = time.time() - self.lastTime
        if self.running:
            self._update()
        else:
            print("running = False")
        self.lastTime = time.time()
    def _update(self):
        pass

class inBetween(Action):
    def __init__(self, s, Next:Action):
        super().__init__(s)
        self.next:Action = Next
        self.startPos:[Leg_XYZ,Leg_XYZ,Leg_XYZ,Leg_XYZ] = self.state.body.getLegPos()
        endPos = self.next.getDefaultPose()
        self.d0 = XYZ(self.startPos[0].x - endPos[0].x, self.startPos[0].y - endPos[0].y, self.startPos[0].z - endPos[0].z)
        self.d1 = XYZ(self.startPos[1].x - endPos[1].x, self.startPos[1].y - endPos[1].y, self.startPos[1].z - endPos[1].z)
        self.d2 = XYZ(self.startPos[2].x - endPos[2].x, self.startPos[2].y - endPos[2].y, self.startPos[2].z - endPos[2].z)
        self.d3 = XYZ(self.startPos[3].x - endPos[3].x, self.startPos[3].y - endPos[3].y, self.startPos[3].z - endPos[3].z)
        l= (self.d0.getLen(),self.d1.getLen(),self.d2.getLen(),self.d3.getLen())
        m = max(l)
        self.timeLeft = m/self.state.Speed
        self.timeMax = self.timeLeft
        
    def _update(self):
        timeDelta = time.time() - self.lastTime
        self.timeLeft -= timeDelta
        if self.timeLeft <= 0:
            self.next.start(timeOffSet = self.timeLeft)
        else:
            self.next.start(self.timeOffset)
            self.state.currentAction=self.next


class Walking(Action):
    def __init__(self, s):
        self.i = -1
        self.timeLeft = 1
        self.startTime = 1
        self.minATime = 4
        self.stepL = 100 #mm
        self.stepH = 50 #mm
        self.d:[float,float] =[1,0]
        self.done = True
        super().__init__(s)
    def _update(self):
        if self.done:
            self.updateA()
        if self.running:
            self.updateB()

    # updates next animation segments
    def updateA(self):#update to next part of animation
        self.done=False
        self.i += 1
        print("i:",self.i,"_____________________________________________")
        if self.i>=4:
            self.i = 0
        cs = self.state.controllerState
        jx = cs["RightJoystickX"]
        jy = cs["RightJoystickY"]
        t = math.sqrt(jx*jx + jy*jy)
        if self.i == 0 or self.i == 2:
            if t<0.3:
                self.running=False
                self.state.currentAction = inBetween(self.state,Standing(self.state))
                self.state.currentAction.start(self.timeOffset)
            else:
                pass
                #self.d=jx/t, jy/t
                #print("d:",self.d)
                #self.startTime = (2-t) * self.minATime
        self.timeLeft = self.startTime

    #updates motors to new position between animation segments
    def updateB(self):
        self.timeLeft -= self.DeltaTime
        if self.timeLeft <= 0:
            self.timeOffset = self.timeLeft
            self.timeLeft = 0
            self.done = True
        o1: [float, float] = [0, 0]
        o2: [float, float] = [0, 0]
        if self.i == 0:
            o1 = self.eq1()
            o2 = self.eq3()
        elif self.i == 1:
            o1 = self.eq2()
            o2 = self.eq4()
        elif self.i == 2:
            o1 = self.eq3()
            o2 = self.eq1()
        elif self.i == 3:
            o1 = self.eq4()
            o2 = self.eq2()
        FL = Leg_XYZ(1, o1[0] * self.d[0], self.state.bodyHeight - o1[1], o1[0] * self.d[1])
        FR = Leg_XYZ(2, o2[0] * self.d[0], self.state.bodyHeight - o2[1], o2[0] * self.d[1])
        BL = Leg_XYZ(3, o1[0] * self.d[0], self.state.bodyHeight - o1[1], o1[0] * self.d[1])
        BR = Leg_XYZ(4, o2[0] * self.d[0], self.state.bodyHeight - o2[1], o2[0] * self.d[1])
        print("FL:",FL.toMotorANG())
        self.state.body.setLegPos(FL,FR,BR,BL)
    def getDefaultPose(self) -> [Leg_XYZ,Leg_XYZ,Leg_XYZ,Leg_XYZ]:
        o1 = self.eq1()
        o2 = self.eq3()
        FL = Leg_XYZ(1, o1[0] * self.d[0], self.state.bodyHeight - o1[1], o1[0] * self.d[1])
        FR = Leg_XYZ(2, o2[0] * self.d[0], self.state.bodyHeight - o2[1], o2[0] * self.d[1])
        BL = Leg_XYZ(3, o1[0] * self.d[0], self.state.bodyHeight - o1[1], o1[0] * self.d[1])
        BR = Leg_XYZ(4, o2[0] * self.d[0], self.state.bodyHeight - o2[1], o2[0] * self.d[1])
        return FL,FR,BL,BR
    def st(self)->float:
        return self.timeLeft/self.startTime
    def eq1(self)->[float,float]:
        x = -(self.st()-1) * self.stepL/2
        y = -math.pow(self.st()-1,2)*self.stepH+self.stepH
        return x,y
    def eq2(self)->[float,float]:
        x = self.st() * self.stepL/2
        y = 0
        return x,y
    def eq3(self)->[float,float]:
        x = (self.st()-1) *self.stepL/2
        y = 0
        return x,y
    def eq4(self)->[float,float]:
        y = -math.pow(self.st(),2)*self.stepH+self.stepH
        x = -self.st() * self.stepL/2
        return x,y
class Standing(Action):
    def __init__(self, s):
        super().__init__(s)
    def getDefaultPose(self) -> [Leg_XYZ, Leg_XYZ, Leg_XYZ, Leg_XYZ]:
        FL = Leg_XYZ(1,0, self.state.bodyHeight,0)
        FR = Leg_XYZ(2,0, self.state.bodyHeight,0)
        BL = Leg_XYZ(3,0, self.state.bodyHeight,0)
        BR = Leg_XYZ(4,0, self.state.bodyHeight,0)
        return FL, FR, BL, BR
    def _update(self):
        cs = self.state.controllerState
        jx = cs["RightJoystickX"]
        jy = cs["RightJoystickY"]
        t = math.sqrt(jx * jx + jy * jy)
        if t > 0.35:
            print("swiching to walking")
            self.running = False
            self.state.currentAction = inBetween(self.state,Walking(self.state))
            self.state.currentAction.start(0)
            FL,FR,BR,BL = self.getDefaultPose()
            self.state.body.setLegPos(FL,FR,BR,BL)

class Body:
    def __init__(self):
        #FL FR BL BR
        self._Legs=[Leg(1),Leg(2),Leg(3),Leg(4)]
    def getLegPos(self):
        return self._Legs[0].pos,self._Legs[1].pos,self._Legs[2].pos,self._Legs[3].pos
    def getLeg(self,ID:int|str)->Leg:
        if isinstance(ID,int):
            return self._Legs[ID-1]
        else:
            ID = ID.capitalize()
            if ID == "FL":
                return self._Legs[0]
            elif ID == "FR":
                return self._Legs[1]
            elif ID == "BL":
                return self._Legs[2]
            elif ID == "BR":
                return self._Legs[3]
            raise Exception("ID not recognised ID = " + ID)
    def setLegPos(self,FL:Leg_XYZ,FR:Leg_XYZ,BL:Leg_XYZ,BR:Leg_XYZ):
        self.getLeg(1).GoToXYZ(FL)
        self.getLeg(2).GoToXYZ(FR)
        self.getLeg(3).GoToXYZ(BL)
        self.getLeg(4).GoToXYZ(BR)
#FL =1 FR =2 BL = 3 BR =4
class Leg:
    def __init__(self, ID):
        self.ID = ID
        self.pos:Leg_XYZ = Leg_XYZ(ID,0,100,0)
        self.acc:XYZ = XYZ(0,0,0)
        self.vel:XYZ = XYZ(0,0,0)
        self.MaxSpeed:XYZ = XYZ(0,0,0)
    def GoToXYZ(self,xyz:Leg_XYZ):
        self.pos = xyz.toLegXYZ(self.ID)
        self.GoToANG(xyz.toLEG_ANG(self.ID))
    def GoToANG(self,ang:LEG_ANG):
        #self.pos = ang.toLEG_XYZ()
        ang2 = ang.toMotorANG()
        ang2.checkValid()
        cmd = "LEG,"+str(self.ID)+","+str(ang2.body/math.pi*180)+","+str(ang2.shoulder/math.pi*180)+","+str(ang2.knee/math.pi*180)
        print(cmd)
        mc.sendCMD(cmd)

class MC:
    def __init__(self):
        self.PORT = 4000
        self.IP = "127.0.0.1"
        self.addr=(self.IP,self.PORT)
        self.s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    def sendCMD(self,message):
        pass
        self.s.sendto(message.encode(), self.addr)

mc = MC()