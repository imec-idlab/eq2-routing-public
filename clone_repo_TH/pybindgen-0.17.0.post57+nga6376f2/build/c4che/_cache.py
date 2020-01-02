AR = '/usr/bin/ar'
ARFLAGS = 'rcs'
BINDIR = '/usr/local/bin'
CC = ['/usr/bin/gcc']
CCDEFINES = []
CCFLAGS = ['-O2', '-g', '-Wall']
CCFLAGS_PYEXT = ['-fvisibility=hidden']
CCLNK_SRC_F = []
CCLNK_TGT_F = ['-o']
CC_NAME = 'gcc'
CC_SRC_F = []
CC_TGT_F = ['-c', '-o']
CC_VERSION = ('8', '3', '0')
CFLAGS_MACBUNDLE = ['-fPIC']
CFLAGS_PYEMBED = ['-fno-strict-aliasing', '-fwrapv', '-fdebug-prefix-map=/build/python2.7-PPrPZj/python2.7-2.7.15=.', '-fstack-protector-strong', '-fno-strict-aliasing']
CFLAGS_PYEXT = ['-pthread', '-fno-strict-aliasing', '-fwrapv', '-fdebug-prefix-map=/build/python2.7-PPrPZj/python2.7-2.7.15=.', '-fstack-protector-strong', '-fno-strict-aliasing', '-fwrapv', '-fdebug-prefix-map=/build/python2.7-PPrPZj/python2.7-2.7.15=.', '-fstack-protector-strong', '-fno-strict-aliasing']
CFLAGS_cshlib = ['-fPIC']
COMPILER_CC = 'gcc'
COMPILER_CXX = 'g++'
CPPPATH_ST = '-I%s'
CXX = ['/usr/bin/g++']
CXXDEFINES = []
CXXFLAGS = ['-O2', '-g', '-Wall']
CXXFLAGS_MACBUNDLE = ['-fPIC']
CXXFLAGS_PYEMBED = ['-fno-strict-aliasing', '-fwrapv', '-fdebug-prefix-map=/build/python2.7-PPrPZj/python2.7-2.7.15=.', '-fstack-protector-strong', '-fno-strict-aliasing']
CXXFLAGS_PYEXT = ['-pthread', '-fno-strict-aliasing', '-fwrapv', '-fdebug-prefix-map=/build/python2.7-PPrPZj/python2.7-2.7.15=.', '-fstack-protector-strong', '-fno-strict-aliasing', '-fwrapv', '-fdebug-prefix-map=/build/python2.7-PPrPZj/python2.7-2.7.15=.', '-fstack-protector-strong', '-fno-strict-aliasing', '-fvisibility=hidden']
CXXFLAGS_cxxshlib = ['-fPIC']
CXXLNK_SRC_F = []
CXXLNK_TGT_F = ['-o']
CXX_NAME = 'gcc'
CXX_SRC_F = []
CXX_TGT_F = ['-c', '-o']
DEFINES = ['PYTHONDIR="/usr/local/lib/python2.7/dist-packages"', 'PYTHONARCHDIR="/usr/local/lib/python2.7/dist-packages"', 'HAVE_PYTHON_H=1', 'HAVE_STDINT_H=1', 'HAVE_BOOST_SHARED_PTR_HPP=1']
DEFINES_PYEMBED = ['NDEBUG', '_FORTIFY_SOURCE=2']
DEFINES_PYEXT = ['NDEBUG', '_FORTIFY_SOURCE=2', 'NDEBUG', '_FORTIFY_SOURCE=2']
DEFINES_ST = '-D%s'
DEST_BINFMT = 'elf'
DEST_CPU = 'x86_64'
DEST_OS = 'linux'
ENABLE_BOOST_SHARED_PTR = True
ENABLE_CXX11 = True
ENABLE_PYGCCXML = False
INCLUDES_PYEMBED = ['/usr/include/python2.7', '/usr/include/x86_64-linux-gnu/python2.7']
INCLUDES_PYEXT = ['/usr/include/python2.7', '/usr/include/x86_64-linux-gnu/python2.7']
LIBDIR = '/usr/local/lib'
LIBPATH_PYEMBED = ['/usr/lib']
LIBPATH_PYEXT = ['/usr/lib']
LIBPATH_PYTHON2.7 = ['/usr/lib']
LIBPATH_ST = '-L%s'
LIB_PYEMBED = ['python2.7']
LIB_PYEXT = ['python2.7']
LIB_PYTHON2.7 = ['python2.7']
LIB_ST = '-l%s'
LINKFLAGS_MACBUNDLE = ['-bundle', '-undefined', 'dynamic_lookup']
LINKFLAGS_PYEMBED = ['-Wl,-Bsymbolic-functions', '-Wl,-z,relro']
LINKFLAGS_PYEXT = ['-Wl,-Bsymbolic-functions', '-Wl,-z,relro', '-pthread', '-Wl,-O1', '-Wl,-Bsymbolic-functions', '-Wl,-Bsymbolic-functions', '-Wl,-z,relro']
LINKFLAGS_cshlib = ['-shared']
LINKFLAGS_cstlib = ['-Wl,-Bstatic']
LINKFLAGS_cxxshlib = ['-shared']
LINKFLAGS_cxxstlib = ['-Wl,-Bstatic']
LINK_CC = ['/usr/bin/gcc']
LINK_CXX = ['/usr/bin/g++']
PREFIX = '/usr/local'
PYC = 1
PYCMD = '"import sys, py_compile;py_compile.compile(sys.argv[1], sys.argv[2])"'
PYFLAGS = ''
PYFLAGS_OPT = '-O'
PYO = 1
PYTHON = ['/usr/bin/python']
PYTHONARCHDIR = '/usr/local/lib/python2.7/dist-packages'
PYTHONDIR = '/usr/local/lib/python2.7/dist-packages'
PYTHON_CONFIG = '/usr/bin/python-config'
PYTHON_VERSION = '2.7'
RPATH_ST = '-Wl,-rpath,%s'
SHLIB_MARKER = '-Wl,-Bdynamic'
SONAME_ST = '-Wl,-h,%s'
STLIBPATH_ST = '-L%s'
STLIB_MARKER = '-Wl,-Bstatic'
STLIB_ST = '-l%s'
cprogram_PATTERN = '%s'
cshlib_PATTERN = 'lib%s.so'
cstlib_PATTERN = 'lib%s.a'
cxxprogram_PATTERN = '%s'
cxxshlib_PATTERN = 'lib%s.so'
cxxstlib_PATTERN = 'lib%s.a'
define_key = ['PYTHONDIR', 'PYTHONARCHDIR', 'HAVE_PYTHON_H', 'HAVE_STDINT_H', 'HAVE_BOOST_SHARED_PTR_HPP']
macbundle_PATTERN = '%s.bundle'
pyext_PATTERN = '%s.so'
