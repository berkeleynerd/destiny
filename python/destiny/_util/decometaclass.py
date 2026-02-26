# Copyright (c) CCP 2012

"""
See documentation for metaclasses to understand.
It creates a class object with a custom class creation function that, instead
of creating instances of these objects, creates the underlying blue object with
a decorator hanging off of it.
"""

import blue
import types


types.BlueType = type(blue.os)

class DecoMetaclass(type):
    """
    Metaclass that gets a blue class instance and returns it as a blue.BlueWrapper
    object.
    """
    def __new__(mcs, name, bases, dict):
        cls = type.__new__(mcs, name, bases, dict)
        cls.__persistvars__    = cls.CombineLists("__persistvars__")
        cls.__nonpersistvars__ = cls.CombineLists("__nonpersistvars__")
        return cls

    def __call__(cls, inst=None, initDict=None, *args, **kwargs):
        """
        Override Instance creation. We don't actually create an instance of cls,
        But rather attach a deco to a BlueWrapper of a blue object.
        inst and initDict arguments are used when unpickling
        """
        #TODO: Change this later to provide a custom function for unpicling,
        #so that the __call__ can directly map to the __init__ function

        if not inst:
            inst = blue.classes.CreateInstance(cls.__cid__)

        #Attach the class (the deco) to the Blue wrapper object:
        #see BluePyWrap.cpp, line 700
        inst.__klass__ = cls

        #init with a provided dict
        #note, we are not initializing an instance of type "cls", this
        #is a special deco dict of the bluewrapper
        if initDict:
            #apply the dict to the inst, don't just update the dict.
            for k,v in initDict.items():
                setattr(inst, k, v)

        #call init function if defined in the deco (or py method)
        if hasattr(inst, "__init__"):
            inst.__init__()
        return inst

    def CombineLists(cls, name):
        """
        Combine attribute lists by walking over all parent classes
        """
        result = []
        for b in cls.__mro__:
            if hasattr(b, name):
                result += list(getattr(b, name))
        return result

    subclasses = {}


def GetDecoMetaclassInst(cid):
    #Create a metaclass subclass that knows about __cid__
    #when looking for an attribute on a type object (cld)
    #its parents are searhced too.  So, putting __cid__ in
    #a parent class is ok.
    class parentclass(object, metaclass=DecoMetaclass):
        __cid__ = cid

    return parentclass

#Compatibility layer
BlueWrappedMetaclass = DecoMetaclass
WrapBlueClass = GetDecoMetaclassInst