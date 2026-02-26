import functools
import blue


def TimedFunction(context):
    def decorator(function):
        @functools.wraps(function)
        def Wrapper(*args, **kwargs):
            newContext = blue.pyos.taskletTimer.EnterTasklet(context)
            try:
                return function(*args, **kwargs)
            finally:
                blue.pyos.taskletTimer.ReturnFromTasklet(newContext)
        return Wrapper
    return decorator


class Timer(object):

    __slots__ = ["ctxt"]

    def __init__(self, context):
        self.ctxt = context

    def __enter__(self):
        self.ctxt = blue.pyos.taskletTimer.EnterTasklet(self.ctxt)

    def __exit__(self, *_):
         blue.pyos.taskletTimer.ReturnFromTasklet(self.ctxt)
