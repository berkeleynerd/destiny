import blue
import sys
import decometaclass

_destiny = blue.LoadExtension("_destiny")
_destiny.Ball = decometaclass.WrapBlueClass("destiny.Ball")
_destiny.Ballpark = decometaclass.WrapBlueClass("destiny.Ballpark")
_destiny.ClientBall = decometaclass.WrapBlueClass("destiny.ClientBall")
sys.modules[__name__] = _destiny
