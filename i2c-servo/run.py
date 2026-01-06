import math

def cos(deg):
    radians = (deg / 180.0) * math.pi
    return math.cos(radians)

def sin(deg):
    radians = (deg / 180.0) * math.pi
    return math.sin(radians)

def asin(ratio):
    radians = math.asin(ratio)
    return radians * 180.0 / math.pi

def acos(ratio):
    radians = math.asin(ratio)
    return radians * 180.0 / math.pi


# Each leg segment is 150mm
SEGMENT = 150
UPPER = SEGMENT
LOWER = SEGMENT
EXTENTION = UPPER + LOWER


# Assuming the leg rotates 30 deg forward and back 
# figure out the height and delta forward at full 
# leg extention and rotation.
SWING = 20
HEIGHT = cos(SWING) * EXTENTION
DELTAX = sin(SWING) * EXTENTION


# Now given a height and deltaX, compute rotations
def GetRotations(x, y):
    global EXTENTION
    print("Compute for x=%f, y=%f" % (x,y))
    l_squared = (x*x + y*y)
    l = math.sqrt(l_squared)
    
    # Make sure height is positive and length is
    # less than full extention.
    assert(y >= 0.0)
    assert(l < EXTENTION)

    # Compute knee bend to achieve that length.
    lower = asin(l / EXTENTION)

    # Rotation to reach X + offset to knee
    upper = asin(x / l) + (lower / 2.0)

    return upper, lower

print("Full extention:  %f" % EXTENTION)
print("Standing Height: %f" % HEIGHT)
print("Delta length:    %f" % DELTAX)
print("Tread length:    %f" % (DELTAX * 2))

FRAMES = 3
DELTAX_FRAC = DELTAX / (FRAMES-1)
uppers = []
lowers = []
for i in range(FRAMES):
    upper, lower = GetRotations(DELTAX_FRAC * i, HEIGHT - 5)
    uppers.append(upper)
    lowers.append(lowers)
    print("%f %f" % (upper, lower))
