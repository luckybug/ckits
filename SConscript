Import('*')

_env = env.Clone()
_env.Append(CPPFLAGS = '-g -DTHREAD_SAFE -DTEST_LIST_SORT -DCONCRET_LIMIT=1024 ')
_env.Append(CPPPATH = 'include')
_env.Append(LIBPATH = '.')
_env['LIBS'] = ['c', 'm', 'pthread']

objs = []
totalSources = """
    myAlloc.c \
    shmAlloc.c \
    timeList.c \
    ioStream.c \
    hashTable.c \
    epCore.c \
    sh_print.c \
    cJSON.c \
    metaBuffer.c
"""
target = _env.SharedLibrary('libckits.so', totalSources.split(), _LIBFLAGS=' -Wl,-Bsymbolic')
objs += target

target = _env.Program('test_mb', Glob('test/*.c') + totalSources.split())
objs += target

Return('objs')
