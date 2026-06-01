# Copyright © 2015 CCP ehf.

import blue
from destiny._util.decometaclass import WrapBlueClass

# This is a hack to allow PyCharm to parse stub files for _destiny. The _destiny_stub stub is located
# in packages/stubgen/stubs and will always generate an ImportError.
try:
    from _destiny_stub import *
except ImportError:
    pass

_destiny = blue.LoadExtension("_destiny")
_destiny.Ball = WrapBlueClass("destiny.Ball")
_destiny.Ballpark = WrapBlueClass("destiny.Ballpark")
_destiny.ClientBall = WrapBlueClass("destiny.ClientBall")

for memberName in dir(_destiny):
    if memberName in ["__name__", "__file__"]:
        continue  # Let's not lie about who we are.
    globals()[memberName] = getattr(_destiny, memberName)
