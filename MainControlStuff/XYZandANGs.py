from __future__ import annotations
from settings import bodyData
import math

class ANG:
    def __init__(self,ID,b,s,k):
        self.ID = ID
        self.body = b
        self.shoulder = s
        self.knee = k
    def toLEG_ANG(self)->LEG_ANG:
        pass
    def toMotorANG(self)->MOTOR_ANG:
        pass
    def toLEG_XYZ(self)->Leg_XYZ:
        pass
    def toBody_XYZ(self)->Body_XYZ:
        pass
    def __str__(self):
        return "Body = "+str(self.body/math.pi*180)+", Shoulder = "+str(self.shoulder/math.pi*180) +", Knee = "+str(self.knee/math.pi*180)+", ID ="+str(self.ID)
    def checkValid(self):
        pass
class LEG_ANG(ANG):#body shoulder knee angle not adjusted for motor mounting angles/offsets
    def __init__(self, ID, b, s, k):
        super().__init__(ID, b, s, k)
    def toLEG_ANG(self) -> LEG_ANG:
        return self
    def toMotorANG(self) -> MOTOR_ANG:
        print(self)
        b = self.body
        s = self.shoulder
        k = math.pi-(self.knee)-math.pi/4
        if self.ID == 1: #FL
            pass
        elif self.ID==2: #FR
            k = -k + math.pi
            s = - s
            b = - b
        elif self.ID == 3: #BL
            pass
        elif self.ID == 4: #BR
            b=-b
            k = -k+math.pi
            s=-s
        else:
            raise Exception("ID is invalid ID = "+str(self.ID))
        b += math.pi/2
        s += math.pi/2
        return MOTOR_ANG(self.ID,b,s,k)
    def toLEG_XYZ(self) -> Leg_XYZ:
        b = bodyData["legs"]["thighLength"]
        c = bodyData["legs"]["calfLength"]
        BTS_y = bodyData["legs"]["BTS"]["y"]
        # a = sqrt(b^2 +c^2 - 2bc*cos(A) {A is the angle of the knee}
        StoF = math.sqrt(b*b+c*c-2*b*c*math.cos(self.knee))#shoulder to foot length
        x = math.sin(self.shoulder)*StoF
        BtoF = math.sqrt(StoF*StoF - x*x + BTS_y*BTS_y)#leg distance from the origin to foot no x
        bAng2 = self.body-math.acos(BTS_y/BtoF)
        y = math.sin(bAng2)*BtoF
        z = math.cos(bAng2)*BtoF
        return Leg_XYZ(self.ID,x,y,z)
    def toBody_XYZ(self) -> Body_XYZ:
        return self.toLEG_XYZ().toBody_XYZ()
class MOTOR_ANG(ANG):
    def __init__(self, ID, b, s, k):
        super().__init__(ID, b, s, k)
    def toLEG_ANG(self)->LEG_ANG:
        pass
    def toMotorANG(self)->MOTOR_ANG:
        return self
    def toLEG_XYZ(self)->Leg_XYZ:
        return self.toLEG_ANG().toLEG_XYZ()
    def toBody_XYZ(self)->Body_XYZ:
        return self.toLEG_ANG().toBody_XYZ()
    def checkValid(self):
        if self.ID == 1:
            if self.shoulder < 0 or self.shoulder > math.pi:
                print('\033[93m'"shoulder angle out of range 0 to 180 degrees, angle = " + str(self.shoulder/math.pi*180,'\033[96m'))
            if self.knee < 0 or self.knee > math.pi:
                print('\033[93m'"knee angle out of range 0 to 180 degrees, angle = " + str(self.knee/math.pi*180),'\033[96m')
            if self.body < 0 or self.body > math.pi:
                print('\033[93m'"body angle out of range 0 to 180 degrees, angle = " + str(self.body/math.pi*180),'\033[96m')

class XYZ:
    def __init__(self, x,y,z):
        self.x=x
        self.y=y
        self.z=z
    def toBody_XYZ(self)->Body_XYZ:
        pass
    def toLegXYZ(self,ID = -1)->Leg_XYZ:
        pass
    def toLEG_ANG(self,ID = -1)->LEG_ANG:
        pass
    def toMotorANG(self,ID = -1)-> MOTOR_ANG:
        pass
    def __str__(self):
        return "x = "+str(self.x)+", y = "+str(self.y)+", z = "+str(self.z)
    def getLen(self):
        return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z)
class Leg_XYZ(XYZ):
    def __init__(self, ID, x, y, z):
        self.leg_id = ID
        super().__init__(x, y, z)
    def toBody_XYZ(self) -> Body_XYZ:
        if self.leg_id ==1:  #FL leg
            return Body_XYZ(self.x + bodyData["body"]["length"] / 2, self.y, self.z - bodyData["body"]["width"] / 2)
        elif self.leg_id == 2:  #FR leg
            return Body_XYZ(self.x + bodyData["body"]["length"] / 2, self.y, self.z + bodyData["body"]["width"] / 2)
        elif self.leg_id == 3:  #BL leg
            return Body_XYZ(self.x - bodyData["body"]["length"] / 2, self.y, self.z - bodyData["body"]["width"] / 2)
        elif self.leg_id == 4:  # BR leg
            return Body_XYZ(self.x - bodyData["body"]["length"] / 2, self.y, self.z + bodyData["body"]["width"] / 2)
    def toLegXYZ(self, ID=-1) -> Leg_XYZ:
        if ID == -1:
            ID = self.leg_id
        if ID == self.leg_id:
            return self
        else:
            return self.toBody_XYZ().toLegXYZ(ID)
    def doInverts(self,x , y, z,):
        if self.leg_id == 1: #FL
            return x,y,z
        elif self.leg_id==2: #FR
            return x,y,-z
        elif self.leg_id == 3: #BL
            return x,y,z
        elif self.leg_id == 4: #BR
            return x,y,-z
    def toLEG_ANG(self, ID=-1) -> LEG_ANG:
        if ID == -1:
            ID = self.leg_id
        if ID != self.leg_id:
            return self.toBody_XYZ().toLEG_ANG(ID)
        else:
            t = bodyData["legs"]["thighLength"]
            c = bodyData["legs"]["calfLength"]
            # get x y and z in body formate
            x = self.x
            y = self.y
            z = self.z
            x,y,z = self.doInverts(x,y,z)
            # calculate total distance
            length = math.sqrt(x * x + y * y + z * z)
            a = math.asin(z / length)
            a2 = math.acos(bodyData["legs"]["BTS"]["y"] / length)
            bAng = a2 - a - math.pi/2# angle of body motor
            length2 = math.sqrt(length * length - bodyData["legs"]["BTS"]["y"] * bodyData["legs"]["BTS"]["y"])
            kAng = math.pi-math.acos((t * t + c * c - length2 * length2) / (2 * t * c))
            sAng = math.acos((t*t+length2*length2-c*c)/(2*t*length2)) - math.asin(x/length2)

            return LEG_ANG(ID, bAng,sAng,kAng)
    def toMotorANG(self, ID=-1) -> MOTOR_ANG:
        return self.toLEG_ANG(ID).toMotorANG()
class Body_XYZ(XYZ):
    def __init__(self,x, y, z):
        super().__init__(x, y, z)
    def toBody_XYZ(self)->XYZ:
        return Body_XYZ(self.x,self.y,self.z)
    def toLegXYZ(self,ID = -1) ->Leg_XYZ:
        if ID == -1:
            raise Exception("ID ==-1 when change Body XYZ to LEG_XYZ")
        if ID == 1:  # FL leg
            return Leg_XYZ(ID,self.x - bodyData["body"]["length"] / 2, self.y, self.z + bodyData["body"]["width"] / 2)
        elif ID == 2:  # FR leg
            return Leg_XYZ(ID,self.x - bodyData["body"]["length"] / 2, self.y, self.z - bodyData["body"]["width"] / 2)
        elif ID == 3:  # BL leg
            return Leg_XYZ(ID,self.x + bodyData["body"]["length"] / 2, self.y, self.z + bodyData["body"]["width"] / 2)
        elif ID == 4:  # BR leg
            return Leg_XYZ(ID,self.x + bodyData["body"]["length"] / 2, self.y, self.z - bodyData["body"]["width"] / 2)
        else:
            raise Exception("invaled value for ID: ID="+str(ID))
    def toLEG_ANG(self,ID = -1) ->LEG_ANG:
            return self.toLegXYZ(ID).toLEG_ANG()
    def toMotorANG(self,ID = -1) -> MOTOR_ANG:
            return self.toLegXYZ(ID).toMotorANG()


if __name__ == '__main__':
    a1 = Leg_XYZ(0,0,263.37,9.90)
    print(a1)
    a2 = a1.toLEG_ANG()
    print(a2)
    a3 = a2.toBody_XYZ()
    print(a3)