import blue
import sys
import decometaclass

# This is a hack to allow PyCharm to parse stub files for _destiny. The _destiny_stub stub is located
# in packages/stubgen/stubs and will always generate an ImportError.
try:
    from _destiny_stub import *
except ImportError:
    pass

_destiny = blue.LoadExtension("_destiny")
_destiny.Ball = decometaclass.WrapBlueClass("destiny.Ball")
_destiny.Ballpark = decometaclass.WrapBlueClass("destiny.Ballpark")
_destiny.ClientBall = decometaclass.WrapBlueClass("destiny.ClientBall")
sys.modules[__name__] = _destiny
