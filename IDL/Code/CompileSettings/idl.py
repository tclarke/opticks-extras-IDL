import os
import os.path
import SCons.Warnings

class IdlNotFound(SCons.Warnings.Warning):
    pass
SCons.Warnings.enableWarningClass(IdlNotFound)

def generate(env):
    idlpath = os.environ.get('OPTICKSDEPENDENCIES',None)
    if not idlpath:
       SCons.Warnings.warn(IdlNotFound,"Could not detect IDL")
    else:
       env.AppendUnique(CXXFLAGS=["-I%s/Idl/include/6.4" % idlpath],
                        LIBPATH=['%s/bin/bin.solaris2.sparc64/6.4' % idlpath],
                        LIBS=["idl","curses","sunmath"])

def exists(env):
    return env.Detect('idl')
