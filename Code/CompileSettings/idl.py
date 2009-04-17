import os
import os.path
import SCons.Warnings

class IdlNotFound(SCons.Warnings.Warning):
    pass
SCons.Warnings.enableWarningClass(IdlNotFound)

def generate(env):
    idlpath = os.environ.get('IDL_DIR',None)
    if not idlpath:
       SCons.Warnings.warn(IdlNotFound,"Could not detect IDL")
    else:
       env.AppendUnique(CXXFLAGS=["-I%s/external/include" % idlpath],
                        LIBPATH=['%s/bin/bin.solaris2.sparc64' % idlpath],
                        LIBS=["idl","curses","sunmath"])

def exists(env):
    return env.Detect('idl')
