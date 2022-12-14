import config.base

import os
import re

class Configure(config.base.Configure):
  def __init__(self, framework):
    config.base.Configure.__init__(self, framework)
    self.headerPrefix = 'PETSC'
    self.substPrefix  = 'PETSC'
    self.defineAutoconfMacros()
    return

  def __str2__(self):
    desc = []
    desc.append('xxx=========================================================================xxx')
    desc.append('   Configure stage complete. Now build PETSc libraries with:')
    desc.append('   make PETSC_DIR='+self.petscdir.dir+' PETSC_ARCH='+self.arch.arch+' all')
    desc.append('xxx=========================================================================xxx')
    return '\n'.join(desc)+'\n'

  def setupHelp(self, help):
    import nargs
    help.addArgument('PETSc',  '-prefix=<path>',                  nargs.Arg(None, '', 'Specifiy location to install PETSc (eg. /usr/local)'))
    help.addArgument('Windows','-with-windows-graphics=<bool>',   nargs.ArgBool(None, 1,'Enable check for Windows Graphics'))
    help.addArgument('PETSc', '-with-default-arch=<bool>',        nargs.ArgBool(None, 1, 'Allow using the last configured arch without setting PETSC_ARCH'))
    help.addArgument('PETSc','-with-single-library=<bool>',       nargs.ArgBool(None, 1,'Put all PETSc code into the single -lpetsc library'))

    return

  def setupDependencies(self, framework):
    config.base.Configure.setupDependencies(self, framework)
    self.setCompilers  = framework.require('config.setCompilers',      self)
    self.arch          = framework.require('PETSc.utilities.arch',     self.setCompilers)
    self.petscdir      = framework.require('PETSc.utilities.petscdir', self.setCompilers)
    self.languages     = framework.require('PETSc.utilities.languages',self.setCompilers)
    self.debugging     = framework.require('PETSc.utilities.debugging',self.setCompilers)
    self.make          = framework.require('PETSc.utilities.Make',     self)
    self.CHUD          = framework.require('PETSc.utilities.CHUD',     self)        
    self.compilers     = framework.require('config.compilers',         self)
    self.types         = framework.require('config.types',             self)
    self.headers       = framework.require('config.headers',           self)
    self.functions     = framework.require('config.functions',         self)
    self.libraries     = framework.require('config.libraries',         self)
    if os.path.isdir(os.path.join('config', 'PETSc')):
      for d in ['utilities', 'packages']:
        for utility in os.listdir(os.path.join('config', 'PETSc', d)):
          (utilityName, ext) = os.path.splitext(utility)
          if not utilityName.startswith('.') and not utilityName.startswith('#') and ext == '.py' and not utilityName == '__init__':
            utilityObj                    = self.framework.require('PETSc.'+d+'.'+utilityName, self)
            utilityObj.headerPrefix       = self.headerPrefix
            utilityObj.archProvider       = self.arch
            utilityObj.languageProvider   = self.languages
            utilityObj.installDirProvider = self.petscdir
            setattr(self, utilityName.lower(), utilityObj)
    self.qd    = framework.require('PETSc.packages.qd', self)
    self.qd.archProvider      = self.arch
    self.qd.precisionProvider = self.scalartypes
    self.qd.installDirProvider= self.petscdir
    
    #force blaslapack to depend on scalarType so precision is set before BlasLapack is built
    self.blaslapack    = framework.require('config.packages.BlasLapack', self)
    framework.require('PETSc.utilities.scalarTypes', self.blaslapack)
    self.blaslapack.archProvider      = self.arch
    self.blaslapack.precisionProvider = self.scalartypes
    self.blaslapack.installDirProvider= self.petscdir

    self.mpi           = framework.require('config.packages.MPI',        self)
    self.mpi.archProvider             = self.arch
    self.mpi.languageProvider         = self.languages
    self.mpi.installDirProvider       = self.petscdir
    self.umfpack       = framework.require('config.packages.UMFPACK',    self)
    self.umfpack.archProvider         = self.arch
    self.umfpack.languageProvider     = self.languages
    self.umfpack.installDirProvider   = self.petscdir
    self.Boost         = framework.require('config.packages.Boost',      self)
    self.Boost.archProvider           = self.arch
    self.Boost.languageProvider       = self.languages
    self.Boost.installDirProvider     = self.petscdir
    self.Fiat          = framework.require('config.packages.Fiat',       self)
    self.Fiat.archProvider            = self.arch
    self.Fiat.languageProvider        = self.languages
    self.Fiat.installDirProvider      = self.petscdir
    self.ExodusII      = framework.require('config.packages.ExodusII',   self)
    self.ExodusII.archProvider        = self.arch
    self.ExodusII.languageProvider    = self.languages
    self.ExodusII.installDirProvider  = self.petscdir

    self.compilers.headerPrefix = self.headerPrefix
    self.types.headerPrefix     = self.headerPrefix
    self.headers.headerPrefix   = self.headerPrefix
    self.functions.headerPrefix = self.headerPrefix
    self.libraries.headerPrefix = self.headerPrefix
    self.blaslapack.headerPrefix = self.headerPrefix
    self.mpi.headerPrefix        = self.headerPrefix
    headersC = map(lambda name: name+'.h', ['dos', 'endian', 'fcntl', 'float', 'io', 'limits', 'malloc', 'pwd', 'search', 'strings',
                                            'unistd', 'machine/endian', 'sys/param', 'sys/procfs', 'sys/resource',
                                            'sys/systeminfo', 'sys/times', 'sys/utsname','string', 'stdlib','memory',
                                            'sys/socket','sys/wait','netinet/in','netdb','Direct','time','Ws2tcpip','sys/types',
                                            'WindowsX', 'cxxabi','float','ieeefp','stdint'])
    functions = ['access', '_access', 'clock', 'drand48', 'getcwd', '_getcwd', 'getdomainname', 'gethostname', 'getpwuid',
                 'gettimeofday', 'getwd', 'memalign', 'memmove', 'mkstemp', 'popen', 'PXFGETARG', 'rand', 'getpagesize',
                 'readlink', 'realpath',  'sigaction', 'signal', 'sigset', 'nanosleep', 'usleep', 'sleep', '_sleep', 'socket', 
                 'times', 'gethostbyname', 'uname','snprintf','_snprintf','_fullpath','lseek','_lseek','time','fork','stricmp',
                 'strcasecmp', 'bzero', 'dlopen', 'dlsym', 'dlclose', 'dlerror',
                 '_intel_fast_memcpy','_intel_fast_memset']
    libraries1 = [(['socket', 'nsl'], 'socket'), (['fpe'], 'handle_sigfpes')]
    self.headers.headers.extend(headersC)
    self.functions.functions.extend(functions)
    self.libraries.libraries.extend(libraries1)
    return

  def defineAutoconfMacros(self):
    self.hostMacro = 'dnl Version: 2.13\ndnl Variable: host_cpu\ndnl Variable: host_vendor\ndnl Variable: host_os\nAC_CANONICAL_HOST'
    return
    
  def Dump(self):
    ''' Actually put the values into the configuration files '''
    # eventually everything between -- should be gone
#-----------------------------------------------------------------------------------------------------    

    # Sometimes we need C compiler, even if built with C++
    self.setCompilers.pushLanguage('C')
    self.addMakeMacro('CC_FLAGS',self.setCompilers.getCompilerFlags())    
    self.setCompilers.popLanguage()

    # C preprocessor values
    self.addMakeMacro('CPP_FLAGS',self.setCompilers.CPPFLAGS+self.CHUD.CPPFLAGS)
    
    # compiler values
    self.setCompilers.pushLanguage(self.languages.clanguage)
    self.addMakeMacro('PCC',self.setCompilers.getCompiler())
    self.addMakeMacro('PCC_FLAGS',self.setCompilers.getCompilerFlags())
    self.setCompilers.popLanguage()
    # .o or .obj 
    self.addMakeMacro('CC_SUFFIX','o')

    # executable linker values
    self.setCompilers.pushLanguage(self.languages.clanguage)
    pcc_linker = self.setCompilers.getLinker()
    self.addMakeMacro('PCC_LINKER',pcc_linker)
    self.addMakeMacro('PCC_LINKER_FLAGS',self.setCompilers.getLinkerFlags())
    self.setCompilers.popLanguage()
    # '' for Unix, .exe for Windows
    self.addMakeMacro('CC_LINKER_SUFFIX','')
    self.addMakeMacro('PCC_LINKER_LIBS',self.libraries.toStringNoDupes(self.compilers.flibs+self.compilers.cxxlibs+self.compilers.LIBS.split(' '))+self.CHUD.LIBS)

    if hasattr(self.compilers, 'FC'):
      self.setCompilers.pushLanguage('FC')
      # need FPPFLAGS in config/setCompilers
      self.addDefine('HAVE_FORTRAN','1')
      self.addMakeMacro('FPP_FLAGS',self.setCompilers.CPPFLAGS)
    
      # compiler values
      self.addMakeMacro('FC_FLAGS',self.setCompilers.getCompilerFlags())
      self.setCompilers.popLanguage()
      # .o or .obj 
      self.addMakeMacro('FC_SUFFIX','o')

      # executable linker values
      self.setCompilers.pushLanguage('FC')
      # Cannot have NAG f90 as the linker - so use pcc_linker as fc_linker
      fc_linker = self.setCompilers.getLinker()
      if config.setCompilers.Configure.isNAG(fc_linker):
        self.addMakeMacro('FC_LINKER',pcc_linker)
      else:
        self.addMakeMacro('FC_LINKER',fc_linker)
      self.addMakeMacro('FC_LINKER_FLAGS',self.setCompilers.getLinkerFlags())
      # apple requires this shared library linker flag on SOME versions of the os
      if self.setCompilers.getLinkerFlags().find('-Wl,-commons,use_dylibs') > -1:
        self.addMakeMacro('DARWIN_COMMONS_USE_DYLIBS',' -Wl,-commons,use_dylibs ')
      self.setCompilers.popLanguage()

      # F90 Modules
      if self.setCompilers.fortranModuleIncludeFlag:
        self.addMakeMacro('FC_MODULE_FLAG', self.setCompilers.fortranModuleIncludeFlag)
      else: # for non-f90 compilers like g77
        self.addMakeMacro('FC_MODULE_FLAG', '-I')
      if self.setCompilers.fortranModuleIncludeFlag:
        self.addMakeMacro('FC_MODULE_OUTPUT_FLAG', self.setCompilers.fortranModuleOutputFlag)
    else:
      self.addMakeMacro('FC','')

    # shared library linker values
    self.setCompilers.pushLanguage(self.languages.clanguage)
    # need to fix BuildSystem to collect these separately
    self.addMakeMacro('SL_LINKER',self.setCompilers.getLinker())
    self.addMakeMacro('SL_LINKER_FLAGS','${PCC_LINKER_FLAGS}')
    self.setCompilers.popLanguage()
    # One of 'a', 'so', 'lib', 'dll', 'dylib' (perhaps others also?) depending on the library generator and architecture
    # Note: . is not included in this macro, consistent with AR_LIB_SUFFIX
    if self.setCompilers.sharedLibraryExt == self.setCompilers.AR_LIB_SUFFIX:
      self.addMakeMacro('SL_LINKER_SUFFIX', '')
      self.addDefine('SLSUFFIX','""')
    else:
      self.addMakeMacro('SL_LINKER_SUFFIX', self.setCompilers.sharedLibraryExt)
      self.addDefine('SLSUFFIX','"'+self.setCompilers.sharedLibraryExt+'"')
      
    #SL_LINKER_LIBS is currently same as PCC_LINKER_LIBS - so simplify
    self.addMakeMacro('SL_LINKER_LIBS','${PCC_LINKER_LIBS}')
    #self.addMakeMacro('SL_LINKER_LIBS',self.libraries.toStringNoDupes(self.compilers.flibs+self.compilers.cxxlibs+self.compilers.LIBS.split(' ')))

#-----------------------------------------------------------------------------------------------------

    # CONLY or CPP. We should change the PETSc makefiles to do this better
    if self.languages.clanguage == 'C': lang = 'CONLY'
    else: lang = 'CXXONLY'
    self.addMakeMacro('PETSC_LANGUAGE',lang)

    # real or complex
    self.addMakeMacro('PETSC_SCALAR',self.scalartypes.scalartype)
    # double or float
    self.addMakeMacro('PETSC_PRECISION',self.scalartypes.precision)

    if self.framework.argDB['with-batch']:
      self.addMakeMacro('PETSC_WITH_BATCH','1')

    # Test for compiler-specific macros that need to be defined.
    if self.setCompilers.isCray('CC'):
      self.addDefine('HAVE_CRAYC','1')

#-----------------------------------------------------------------------------------------------------
    if self.functions.haveFunction('gethostbyname') and self.functions.haveFunction('socket'):
      self.addDefine('USE_SOCKET_VIEWER','1')

#-----------------------------------------------------------------------------------------------------
    # print include and lib for external packages
    self.framework.packages.reverse()
    includes = []
    libs = []
    for i in self.framework.packages:
      self.addDefine('HAVE_'+i.PACKAGE, 1)
      if not isinstance(i.lib, list):
        i.lib = [i.lib]
      libs.extend(i.lib)
      self.addMakeMacro(i.PACKAGE+'_LIB', self.libraries.toStringNoDupes(i.lib))
      if hasattr(i,'include'):
        if not isinstance(i.include,list):
          i.include = [i.include]
        includes.extend(i.include)
        self.addMakeMacro(i.PACKAGE+'_INCLUDE',self.headers.toStringNoDupes(i.include))
    self.addMakeMacro('PACKAGES_LIBS',self.libraries.toStringNoDupes(libs+self.libraries.math))
    self.PACKAGES_LIBS = self.libraries.toStringNoDupes(libs+self.libraries.math)
    self.addMakeMacro('PACKAGES_INCLUDES',self.headers.toStringNoDupes(includes))
    self.PACKAGES_INCLUDES = self.headers.toStringNoDupes(includes)
    if hasattr(self.compilers, 'FC'):
      if self.compilers.fortranIsF90:
        self.addMakeMacro('PACKAGES_MODULES_INCLUDES',self.headers.toStringModulesNoDupes(includes))    
    
    self.addMakeMacro('INSTALL_DIR',self.installdir)
    self.addDefine('LIB_DIR','"'+os.path.join(self.installdir,'lib')+'"')

    if self.framework.argDB['with-single-library']:
      # overrides the values set in conf/variables
      self.addMakeMacro('LIBNAME','${INSTALL_LIB_DIR}/libpetsc.${AR_LIB_SUFFIX}')
      self.addMakeMacro('PETSC_SYS_LIB_BASIC','-lpetsc')
      self.addMakeMacro('PETSC_VEC_LIB_BASIC','-lpetsc')
      self.addMakeMacro('PETSC_MAT_LIB_BASIC','-lpetsc')
      self.addMakeMacro('PETSC_DM_LIB_BASIC','-lpetsc')
      self.addMakeMacro('PETSC_KSP_LIB_BASIC','-lpetsc')
      self.addMakeMacro('PETSC_SNES_LIB_BASIC','-lpetsc')
      self.addMakeMacro('PETSC_TS_LIB_BASIC','-lpetsc')
      self.addMakeMacro('PETSC_LIB_BASIC','-lpetsc')
      self.addMakeMacro('PETSC_CONTRIB_BASIC','-lpetsc')
      self.addMakeMacro('SHLIBS','libpetsc')
      self.addDefine('USE_SINGLE_LIBRARY', '1')
      
    if not os.path.exists(os.path.join(self.petscdir.dir,self.arch.arch,'lib')):
      os.makedirs(os.path.join(self.petscdir.dir,self.arch.arch,'lib'))

    # add a makefile entry for configure options
    self.addMakeMacro('CONFIGURE_OPTIONS', self.framework.getOptionsString(['configModules', 'optionsModule']).replace('\"','\\"'))
    return

  def dumpConfigInfo(self):
    import time
    fd = file(os.path.join(self.arch.arch,'include','petscconfiginfo.h'),'w')
    fd.write('static const char *petscconfigureruntime = "'+time.ctime(time.time())+'";\n')
    fd.write('static const char *petscconfigureoptions = "'+self.framework.getOptionsString(['configModules', 'optionsModule']).replace('\"','\\"')+'";\n')
    fd.close()
    return

  def dumpMachineInfo(self):
    import time
    import script
    fd = file(os.path.join(self.arch.arch,'include','petscmachineinfo.h'),'w')
    fd.write('static const char *petscmachineinfo = \"\\n\"\n')
    fd.write('\"-----------------------------------------\\n\"\n')
    if os.path.isfile(os.path.join('/usr', 'bin', 'cygcheck.exe')):
      fd.write('\"Libraries compiled on %s on %s \\n\"\n' % (time.ctime(time.time()), script.Script.executeShellCommand('hostname|/usr/bin/dos2unix')[0].strip()))
    else:
      fd.write('\"Libraries compiled on %s on %s \\n\"\n' % (time.ctime(time.time()), script.Script.executeShellCommand('hostname')[0].strip()))
    fd.write('\"Machine characteristics: %s\\n\"\n' % (script.Script.executeShellCommand('uname -a')[0].strip()))
    fd.write('\"Using PETSc directory: %s\\n\"' % (self.petscdir.dir))
    fd.write('\"Using PETSc arch: %s\\n\"' % (self.arch.arch))
    fd.write('\"-----------------------------------------\\n\"\n')
    fd.write('static const char *petsccompilerinfo = \"\\n\"\n')
    self.setCompilers.pushLanguage(self.languages.clanguage)
    fd.write('\"Using C compiler: %s %s ${COPTFLAGS} ${CFLAGS}\\n\"' % (self.setCompilers.getCompiler(), self.setCompilers.getCompilerFlags()))
    self.setCompilers.popLanguage()
    if hasattr(self.compilers, 'FC'):
      self.setCompilers.pushLanguage('FC')
      fd.write('\"Using Fortran compiler: %s %s ${FOPTFLAGS} ${FFLAGS} %s\\n\"' % (self.setCompilers.getCompiler(), self.setCompilers.getCompilerFlags(), self.setCompilers.CPPFLAGS))
      self.setCompilers.popLanguage()
    fd.write('\"-----------------------------------------\\n\"\n')
    fd.write('static const char *petsccompilerflagsinfo = \"\\n\"\n')
    fd.write('\"Using include paths: %s %s %s\\n\"' % ('-I'+os.path.join(self.petscdir.dir, self.arch.arch, 'include'), '-I'+os.path.join(self.petscdir.dir, 'include'), self.PACKAGES_INCLUDES))
    fd.write('\"-----------------------------------------\\n\"\n')
    fd.write('static const char *petsclinkerinfo = \"\\n\"\n')
    self.setCompilers.pushLanguage(self.languages.clanguage)
    fd.write('\"Using C linker: %s\\n\"' % (self.setCompilers.getLinker()))
    self.setCompilers.popLanguage()
    if hasattr(self.compilers, 'FC'):
      self.setCompilers.pushLanguage('FC')
      fd.write('\"Using Fortran linker: %s\\n\"' % (self.setCompilers.getLinker()))
      self.setCompilers.popLanguage()
    fd.write('\"Using libraries: %s%s -L%s %s %s %s\\n\"' % (self.setCompilers.CSharedLinkerFlag, os.path.join(self.petscdir.dir, self.arch.arch, 'lib'), os.path.join(self.petscdir.dir, self.arch.arch, 'lib'), '-lpetscts -lpetscsnes -lpetscksp -lpetscdm -lpetscmat -lpetscvec -lpetscsys', self.PACKAGES_LIBS, self.libraries.toStringNoDupes(self.compilers.flibs+self.compilers.cxxlibs+self.compilers.LIBS.split(' '))+self.CHUD.LIBS))
    fd.write('\"-----------------------------------------\\n\"\n')
    fd.close()
    return

  def configurePrefetch(self):
    '''Sees if there are any prefetch functions supported'''
    if config.setCompilers.Configure.isSolaris():
      self.addDefine('Prefetch(a,b,c)', ' ')
      return
    self.pushLanguage(self.languages.clanguage)      
    if self.checkLink('#include <xmmintrin.h>', 'void *v = 0;_mm_prefetch(v,(int)0);\n'):
      self.addDefine('HAVE_XMMINTRIN_H', 1)
      self.addDefine('Prefetch(a,b,c)', '_mm_prefetch((const void*)(a),(int)(c))')
    elif self.checkLink('#include <xmmintrin.h>', 'void *v = 0;_mm_prefetch((const char*)v,(int)0);\n'):
      self.addDefine('HAVE_XMMINTRIN_H', 1)
      self.addDefine('Prefetch(a,b,c)', '_mm_prefetch((const char*)(a),(int)(c))')
    elif self.checkLink('', 'void *v = 0;__builtin_prefetch(v,0,0);\n'):
      self.addDefine('Prefetch(a,b,c)', '__builtin_prefetch((a),(b),(c))')
    else:
      self.addDefine('Prefetch(a,b,c)', ' ')
    self.popLanguage()

  def configureExpect(self):
    '''Sees if the __builtin_expect directive is supported'''
    self.pushLanguage(self.languages.clanguage)
    if self.checkLink('', 'if (__builtin_expect(0,1)) return 1;'):
      self.addDefine('HAVE_BUILTIN_EXPECT', 1)
    self.popLanguage()

  def configureIntptrt(self):
    '''Determine what to use for uintptr_t'''
    def staticAssertSizeMatchesVoidStar(inc,typename):
      # The declaration is an error if either array size is negative.
      # It should be okay to use an int that is too large, but it would be very unlikely for this to be the case
      return self.checkCompile(inc, '#define SZ (sizeof(void*)-sizeof(%s))\nint type_is_too_large[SZ],type_is_too_small[-SZ];'%typename)
    self.pushLanguage(self.languages.clanguage)
    if self.checkCompile('#include <stdint.h>', 'int x; uintptr_t i = (uintptr_t)&x;'):
      self.addDefine('UINTPTR_T', 'uintptr_t')
    elif staticAssertSizeMatchesVoidStar('','unsigned long long'):
      self.addDefine('UINTPTR_T', 'unsigned long long')
    elif staticAssertSizeMatchesVoidStar('#include <stdlib.h>','size_t') or staticAssertSizeMatchesVoidStar('#include <string.h>', 'size_t'):
      self.addDefine('UINTPTR_T', 'size_t')
    elif staticAssertSizeMatchesVoidStar('','unsigned'):
      self.addDefine('UINTPTR_T', 'unsigned')
    self.popLanguage()
      
  def configureInline(self):
    '''Get a generic inline keyword, depending on the language'''
    if self.languages.clanguage == 'C':
      self.addDefine('STATIC_INLINE', self.compilers.cStaticInlineKeyword)
      self.addDefine('RESTRICT', self.compilers.cRestrict)
    elif self.languages.clanguage == 'Cxx':
      self.addDefine('STATIC_INLINE', self.compilers.cxxStaticInlineKeyword)
      self.addDefine('RESTRICT', self.compilers.cxxRestrict)
    return

  def configureSolaris(self):
    '''Solaris specific stuff'''
    if self.arch.hostOsBase.startswith('solaris'):
      if os.path.isdir(os.path.join('/usr','ucblib')):
        try:
          flag = getattr(self.setCompilers, self.language[-1]+'SharedLinkerFlag')
        except AttributeError:
          flag = None
        if flag is None:
          self.compilers.LIBS += ' -L/usr/ucblib'
        else:
          self.compilers.LIBS += ' '+flag+'/usr/ucblib'
    return

  def configureLinux(self):
    '''Linux specific stuff'''
    if self.arch.hostOsBase == 'linux':
      self.addDefine('HAVE_DOUBLE_ALIGN_MALLOC', 1)
    return

  def configureWin32(self):
    '''Win32 non-cygwin specific stuff'''
    kernel32=0
    if self.libraries.add('Kernel32.lib','GetComputerName',prototype='#include <Windows.h>', call='GetComputerName(NULL,NULL);'):
      self.addDefine('HAVE_WINDOWS_H',1)
      self.addDefine('HAVE_GETCOMPUTERNAME',1)
      kernel32=1
    elif self.libraries.add('kernel32','GetComputerName',prototype='#include <Windows.h>', call='GetComputerName(NULL,NULL);'):
      self.addDefine('HAVE_WINDOWS_H',1)
      self.addDefine('HAVE_GETCOMPUTERNAME',1)
      kernel32=1
    if kernel32:
      if self.framework.argDB['with-windows-graphics']:
        self.addDefine('USE_WINDOWS_GRAPHICS',1)
      if self.checkLink('#include <Windows.h>','LoadLibrary(0)'):
        self.addDefine('HAVE_LOADLIBRARY',1)
      if self.checkLink('#include <Windows.h>','GetProcAddress(0,0)'):
        self.addDefine('HAVE_GETPROCADDRESS',1)
      if self.checkLink('#include <Windows.h>','FreeLibrary(0)'):
        self.addDefine('HAVE_FREELIBRARY',1)
      if self.checkLink('#include <Windows.h>','GetLastError()'):
        self.addDefine('HAVE_GETLASTERROR',1)
      if self.checkLink('#include <Windows.h>','SetLastError(0)'):
        self.addDefine('HAVE_SETLASTERROR',1)
      if self.checkLink('#include <Windows.h>\n','QueryPerformanceCounter(0);\n'):
        self.addDefine('USE_NT_TIME',1)
    if self.libraries.add('Advapi32.lib','GetUserName',prototype='#include <Windows.h>', call='GetUserName(NULL,NULL);'):
      self.addDefine('HAVE_GET_USER_NAME',1)
    elif self.libraries.add('advapi32','GetUserName',prototype='#include <Windows.h>', call='GetUserName(NULL,NULL);'):
      self.addDefine('HAVE_GET_USER_NAME',1)
        
    if not self.libraries.add('User32.lib','GetDC',prototype='#include <Windows.h>',call='GetDC(0);'):
      self.libraries.add('user32','GetDC',prototype='#include <Windows.h>',call='GetDC(0);')
    if not self.libraries.add('Gdi32.lib','CreateCompatibleDC',prototype='#include <Windows.h>',call='CreateCompatibleDC(0);'):
      self.libraries.add('gdi32','CreateCompatibleDC',prototype='#include <Windows.h>',call='CreateCompatibleDC(0);')
      
    self.types.check('int32_t', 'int')
    if not self.checkCompile('#include <sys/types.h>\n','uid_t u;\n'):
      self.addTypedef('int', 'uid_t')
      self.addTypedef('int', 'gid_t')
    if not self.checkLink('#if defined(PETSC_HAVE_UNISTD_H)\n#include <unistd.h>\n#endif\n','int a=R_OK;\n'):
      self.framework.addDefine('R_OK', '04')
      self.framework.addDefine('W_OK', '02')
      self.framework.addDefine('X_OK', '01')
    if not self.checkLink('#include <sys/stat.h>\n','int a=0;\nif (S_ISDIR(a)){}\n'):
      self.framework.addDefine('S_ISREG(a)', '(((a)&_S_IFMT) == _S_IFREG)')
      self.framework.addDefine('S_ISDIR(a)', '(((a)&_S_IFMT) == _S_IFDIR)')
    if self.checkCompile('#include <Windows.h>\n','LARGE_INTEGER a;\nDWORD b=a.u.HighPart;\n'):
      self.addDefine('HAVE_LARGE_INTEGER_U',1)

    # Windows requires a Binary file creation flag when creating/opening binary files.  Is a better test in order?
    if self.checkCompile('#include <Windows.h>\n',''):
      self.addDefine('HAVE_O_BINARY',1)

    if self.compilers.CC.find('win32fe') >= 0:
      self.addDefine('PATH_SEPARATOR','\';\'')
      self.addDefine('DIR_SEPARATOR','\'\\\\\'')
      self.addDefine('REPLACE_DIR_SEPARATOR','\'/\'')
      self.addDefine('CANNOT_START_DEBUGGER',1)
    else:
      self.addDefine('PATH_SEPARATOR','\':\'')
      self.addDefine('REPLACE_DIR_SEPARATOR','\'\\\\\'')
      self.addDefine('DIR_SEPARATOR','\'/\'')
    return

#-----------------------------------------------------------------------------------------------------
  def configureDefaultArch(self):
    conffile = os.path.join('conf', 'petscvariables')
    if self.framework.argDB['with-default-arch']:
      fd = file(conffile, 'w')
      fd.write('PETSC_ARCH='+self.arch.arch+'\n')
      fd.write('PETSC_DIR='+self.petscdir.dir+'\n')
      fd.write('include ${PETSC_DIR}/${PETSC_ARCH}/conf/petscvariables\n')
      fd.close()
      self.framework.actions.addArgument('PETSc', 'Build', 'Set default architecture to '+self.arch.arch+' in '+conffile)
    elif os.path.isfile(conffile):
      try:
        os.unlink(conffile)
      except:
        raise RuntimeError('Unable to remove file '+conffile+'. Did a different user create it?')
    return

#-----------------------------------------------------------------------------------------------------
  def configureScript(self):
    '''Output a script in the conf directory which will reproduce the configuration'''
    import nargs
    import sys
    scriptName = os.path.join(self.arch.arch,'conf', 'reconfigure-'+self.arch.arch+'.py')
    args = dict([(nargs.Arg.parseArgument(arg)[0], arg) for arg in self.framework.clArgs])
    if 'configModules' in args:
      if nargs.Arg.parseArgument(args['configModules'])[1] == 'PETSc.Configure':
        del args['configModules']
    if 'optionsModule' in args:
      if nargs.Arg.parseArgument(args['optionsModule'])[1] == 'PETSc.compilerOptions':
        del args['optionsModule']
    if not 'PETSC_ARCH' in args:
      args['PETSC_ARCH'] = 'PETSC_ARCH='+str(self.arch.arch)
    f = file(scriptName, 'w')
    f.write('#!'+sys.executable+'\n')
    f.write('if __name__ == \'__main__\':\n')
    f.write('  import sys\n')
    f.write('  import os\n')
    f.write('  sys.path.insert(0, os.path.abspath(\'config\'))\n')
    f.write('  import configure\n')
    # pretty print repr(args.values())
    f.write('  configure_options = [\n')
    for itm in args.values():
      f.write('    \''+str(itm)+'\',\n')
    f.write('  ]\n')
    f.write('  configure.petsc_configure(configure_options)\n')
    f.close()
    try:
      os.chmod(scriptName, 0775)
    except OSError, e:
      self.framework.logPrint('Unable to make reconfigure script executable:\n'+str(e))
    self.framework.actions.addArgument('PETSc', 'File creation', 'Created '+scriptName+' for automatic reconfiguration')
    return

  def configureInstall(self):
    '''Setup the directories for installation'''
    if self.framework.argDB['prefix']:
      self.installdir = self.framework.argDB['prefix']
      self.addMakeRule('shared_nomesg_noinstall','')
      self.addMakeRule('shared_install','',['-@echo "Now to install the libraries do:"',\
                                              '-@echo "make PETSC_DIR=${PETSC_DIR} PETSC_ARCH=${PETSC_ARCH} install"',\
                                              '-@echo "========================================="'])
    else:
      self.installdir = os.path.join(self.petscdir.dir,self.arch.arch)
      self.addMakeRule('shared_nomesg_noinstall','shared_nomesg')            
      self.addMakeRule('shared_install','',['-@echo "Now to check if the libraries are working do:"',\
                                              '-@echo "make PETSC_DIR=${PETSC_DIR} PETSC_ARCH=${PETSC_ARCH} test"',\
                                              '-@echo "========================================="'])
      return

  def configureGCOV(self):
    if self.framework.argDB['with-gcov']:
      self.addDefine('USE_GCOV','1')
    return

  def configureFortranFlush(self):
    if hasattr(self.compilers, 'FC'):
      for baseName in ['flush','flush_']:
        if self.libraries.check('', baseName, otherLibs = self.compilers.flibs, fortranMangle = 1):
          self.addDefine('HAVE_'+baseName.upper(), 1)
          return


  def configure(self):
    if not os.path.samefile(self.petscdir.dir, os.getcwd()):
      raise RuntimeError('Wrong PETSC_DIR option specified: '+str(self.petscdir.dir) + '\n  Configure invoked in: '+os.path.realpath(os.getcwd()))
    if self.framework.argDB['prefix'] and os.path.isdir(self.framework.argDB['prefix']) and os.path.samefile(self.framework.argDB['prefix'],self.petscdir.dir):
      raise RuntimeError('Incorrect option --prefix='+self.framework.argDB['prefix']+' specified. It cannot be same as PETSC_DIR!')
    self.framework.header          = self.arch.arch+'/include/petscconf.h'
    self.framework.cHeader         = self.arch.arch+'/include/petscfix.h'
    self.framework.makeMacroHeader = self.arch.arch+'/conf/petscvariables'
    self.framework.makeRuleHeader  = self.arch.arch+'/conf/petscrules'
    if self.libraries.math is None:
      raise RuntimeError('PETSc requires a functional math library. Please send configure.log to petsc-maint@mcs.anl.gov.')
    if self.languages.clanguage == 'Cxx' and not hasattr(self.compilers, 'CXX'):
      raise RuntimeError('Cannot set C language to C++ without a functional C++ compiler.')
    self.executeTest(self.configureInline)
    self.executeTest(self.configurePrefetch)
    self.executeTest(self.configureExpect);
    self.executeTest(self.configureIntptrt);
    self.executeTest(self.configureSolaris)
    self.executeTest(self.configureLinux)
    self.executeTest(self.configureWin32)
    self.executeTest(self.configureDefaultArch)
    self.executeTest(self.configureScript)
    self.executeTest(self.configureInstall)
    self.executeTest(self.configureGCOV)
    self.executeTest(self.configureFortranFlush)
    # dummy rules, always needed except for remote builds
    self.addMakeRule('remote','')
    self.addMakeRule('remoteclean','')
    
    self.Dump()
    self.dumpConfigInfo()
    self.dumpMachineInfo()
    self.framework.log.write('================================================================================\n')
    self.logClear()
    return
