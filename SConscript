Import('*')

_env = env.Clone()
_env.Append(CPPFLAGS = '-DTEST_LIST_SORT -DCONCRET_LIMIT=1024 ')
_env.Append(CPPPATH = 'include')
_env.Append(LIBPATH = 'lib')
_env['LIBS'] = ['c']

#objs = []
#target = _env.SharedLibrary('libckits.so-dev', Glob('*.c'), _LIBFLAGS=' -Wl,-Bsymbolic')
#objs = target
#
#target = _env.Command('libckits.so', 'libckit.so-dev', "$STRIP $SOURCE -o $TARGET")
#objs += target
#
#_env.copyinfo = [
#      ('/lib', "libruntime.so-dev", 0755),
#      ('/include', 'include/*.h', 0644)
#    ]
#target = _env.Tarball('runtime-dev.tar.gz', objs)
#
#all = objs + target
all = _env.SharedLibrary('libckits.so', Glob('*.c'), _LIBFLAGS=' -Wl,-Bsymbolic')

Return('all')
