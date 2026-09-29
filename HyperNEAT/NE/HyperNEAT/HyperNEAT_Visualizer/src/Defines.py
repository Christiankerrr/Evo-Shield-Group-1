from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *
from math import *

from Vector3 import *
from Vector2 import *

import sys,os

import GlobalData

pyHyperNEATPath = os.path.split(os.path.split(os.getcwd())[0])[0] + '/out'
#sys.path.append(pyHyperNEATPath)
print('Looking for PyHyperNEAT DLL at', pyHyperNEATPath)

#print('----------------------------------------------------------------------------------------')
#print(sys.path)
if pyHyperNEATPath not in sys.path:
    sys.path.append(pyHyperNEATPath)
sys.path.append('../../out/Release')
#print(sys.path)
#print('----------------------------------------------------------------------------------------')

from PyHyperNEAT import *  

