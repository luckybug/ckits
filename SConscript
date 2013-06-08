Import('*')

_env = env.Clone()
_env.Append(CPPFLAGS = '-DTEST_LIST_SORT ')
_env.Append(CPPPATH = 'include')
_env.Append(LIBPATH = 'lib')
_env['LIBS'] = ['c']

target = _env.SharedLibrary('libruntime.so-dev', Glob('src/*.c'), _LIBFLAGS=' -Wl,-Bsymbolic')
objs = target

target = _env.Command('libruntime.so', 'libruntime.so-dev', "$STRIP $SOURCE -o $TARGET")
objs += target

_env.copyinfo = [
      ('/lib', "libruntime.so-dev", 0755),
      ('/include', 'include/*.h', 0644)
    ]
target = _env.Tarball('runtime-dev.tar.gz', objs)

all = objs + target
Return('all')