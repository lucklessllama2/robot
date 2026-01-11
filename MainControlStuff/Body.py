from __future__ import annotations
import socket
from settings import bodyData
from XYZandANGs import *

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
    def TurnON(self):
        _mc.sendCMD("PWR,0,1")#turn all on
    def TurnOFF(self):
        _mc.sendCMD("PWR,0,0")#turn all off
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
        _mc.sendCMD(cmd)

class MC:
    def __init__(self):
        self.PORT = 4000
        self.IP = "127.0.0.1"
        self.addr=(self.IP,self.PORT)
        self.s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    def sendCMD(self,message):
        pass
        self.s.sendto(message.encode(), self.addr)
_mc = MC()