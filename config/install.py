#!/usr/bin/env python
import os, re, shutil, sys

if os.environ.has_key('PETSC_DIR'):
  PETSC_DIR = os.environ['PETSC_DIR']
else:
  fd = file(os.path.join('lib','petsc','conf','petscvariables'))
  a = fd.readline()
  a = fd.readline()
  PETSC_DIR = a.split('=')[1][0:-1]
  fd.close()

if os.environ.has_key('PETSC_ARCH'):
  PETSC_ARCH = os.environ['PETSC_ARCH']
else:
  fd = file(os.path.join('lib','petsc','conf','petscvariables'))
  a = fd.readline()
  PETSC_ARCH = a.split('=')[1][0:-1]
  fd.close()

print '*** Using PETSC_DIR='+PETSC_DIR+' PETSC_ARCH='+PETSC_ARCH+' ***'
sys.path.insert(0, os.path.join(PETSC_DIR, 'config'))
sys.path.insert(0, os.path.join(PETSC_DIR, 'config', 'BuildSystem'))

import script

try:
  WindowsError
except NameError:
  WindowsError = None

class Installer(script.Script):
  def __init__(self, clArgs = None):
    import RDict
    argDB = RDict.RDict(None, None, 0, 0, readonly = True)
    argDB.saveFilename = os.path.join(PETSC_DIR, PETSC_ARCH, 'lib','petsc','conf', 'RDict.db')
    argDB.load()
    script.Script.__init__(self, argDB = argDB)
    if not clArgs is None: self.clArgs = clArgs
    self.copies = []
    return

  def setupHelp(self, help):
    import nargs
    script.Script.setupHelp(self, help)
    help.addArgument('Installer', '-destDir=<path>', nargs.Arg(None, None, 'Destination Directory for install'))
    return


  def setupModules(self):
    self.setCompilers  = self.framework.require('config.setCompilers',         None)
    self.arch          = self.framework.require('PETSc.options.arch',        None)
    self.petscdir      = self.framework.require('PETSc.options.petscdir',    None)
    self.compilers     = self.framework.require('config.compilers',            None)
    return

  def setup(self):
    script.Script.setup(self)
    self.framework = self.loadConfigure()
    self.setupModules()
    return

  def setupDirectories(self):
    self.rootDir    = self.petscdir.dir
    self.destDir    = os.path.abspath(self.argDB['destDir'])
    self.installDir = os.path.abspath(os.path.expanduser(self.framework.argDB['prefix']))
    self.arch       = self.arch.arch
    self.archDir           = os.path.join(self.rootDir, self.arch)
    self.rootIncludeDir    = os.path.join(self.rootDir, 'include')
    self.archIncludeDir    = os.path.join(self.rootDir, self.arch, 'include')
    self.rootConfDir       = os.path.join(self.rootDir, 'lib','petsc','conf')
    self.archConfDir       = os.path.join(self.rootDir, self.arch, 'lib','petsc','conf')
    self.rootBinDir        = os.path.join(self.rootDir, 'bin')
    self.archBinDir        = os.path.join(self.rootDir, self.arch, 'bin')
    self.archLibDir        = os.path.join(self.rootDir, self.arch, 'lib')
    self.destIncludeDir    = os.path.join(self.destDir, 'include')
    self.destConfDir       = os.path.join(self.destDir, 'lib','petsc','conf')
    self.destLibDir        = os.path.join(self.destDir, 'lib')
    self.destBinDir        = os.path.join(self.destDir, 'bin')
    self.installIncludeDir = os.path.join(self.installDir, 'include')
    self.installBinDir     = os.path.join(self.installDir, 'bin')
    self.rootShareDir      = os.path.join(self.rootDir, 'share')
    self.destShareDir      = os.path.join(self.destDir, 'share')

    self.ranlib      = self.compilers.RANLIB
    self.arLibSuffix = self.compilers.AR_LIB_SUFFIX
    return

  def checkPrefix(self):
    if not self.installDir:
      print '********************************************************************'
      print 'PETSc is built without prefix option. So "make install" is not appropriate.'
      print 'If you need a prefix install of PETSc - rerun configure with --prefix option.'
      print '********************************************************************'
      sys.exit(1)
    return

  def checkDestdir(self):
    if os.path.exists(self.destDir):
      if os.path.samefile(self.destDir, self.rootDir):
        print '********************************************************************'
        print 'Incorrect prefix usage. Specified destDir same as current PETSC_DIR'
        print '********************************************************************'
        sys.exit(1)
      if os.path.samefile(self.destDir, os.path.join(self.rootDir,self.arch)):
        print '********************************************************************'
        print 'Incorrect prefix usage. Specified destDir same as current PETSC_DIR/PETSC_ARCH'
        print '********************************************************************'
        sys.exit(1)
      if not os.path.isdir(os.path.realpath(self.destDir)):
        print '********************************************************************'
        print 'Specified destDir', self.destDir, 'is not a directory. Cannot proceed!'
        print '********************************************************************'
        sys.exit(1)
      if not os.access(self.destDir, os.W_OK):
        print '********************************************************************'
        print 'Unable to write to ', self.destDir, 'Perhaps you need to do "sudo make install"'
        print '********************************************************************'
        sys.exit(1)
    return

  def copyfile(self, src, dst, symlinks = False, copyFunc = shutil.copy2):
    """Copies a single file    """
    copies = []
    errors = []
    if not os.path.exists(dst):
      os.makedirs(dst)
    elif not os.path.isdir(dst):
      raise shutil.Error, 'Destination is not a directory'
    srcname = src
    dstname = os.path.join(dst, os.path.basename(src))
    try:
      if symlinks and os.path.islink(srcname):
        linkto = os.readlink(srcname)
        os.symlink(linkto, dstname)
      else:
        copyFunc(srcname, dstname)
        copies.append((srcname, dstname))
    except (IOError, os.error), why:
      errors.append((srcname, dstname, str(why)))
    except shutil.Error, err:
      errors.extend((srcname,dstname,str(err.args[0])))
    if errors:
      raise shutil.Error, errors
    return copies

  def copyexamplefiles(self, src, dst, copyFunc = shutil.copy2):
    """Copies all files, but not directories in a single file    """
    names  = os.listdir(src)
    for name in names:
      if not name.endswith('.html'):
        srcname = os.path.join(src, name)
        if os.path.isfile(srcname):
           self.copyfile(srcname,dst)

  def fixExamplesMakefile(self, src):
    '''Change ././${PETSC_ARCH} in makefile in root petsc directory with ${PETSC_DIR}'''
    lines   = []
    oldFile = open(src, 'r')
    alllines=oldFile.read()
    oldFile.close()
    newlines=alllines.split('\n')[0]+'\n'  # Firstline
    # Hardcode PETSC_DIR and PETSC_ARCH to avoid users doing the worng thing
    newlines+='PETSC_DIR='+self.installDir+'\n'
    newlines+='PETSC_ARCH=\n'
    for line in alllines.split('\n')[1:]:
      if line.startswith('#'):
        newlines+=line+'\n'
      elif line.startswith('TESTLOGFILE'):
        newlines+='TESTLOGFILE = $(TESTDIR)/examples-install.log\n'
      elif line.startswith('CONFIGDIR'):
        newlines+='CONFIGDIR:=$(PETSC_DIR)/$(PETSC_ARCH)/share/petsc/examples/config\n'
        newlines+='EXAMPLESDIR:=$(PETSC_DIR)/$(PETSC_ARCH)/share/petsc/examples\n'
      elif line.startswith('$(generatedtest)') and 'petscvariables' in line:
        newlines+='all: test\n\n'+line+'\n'
      elif line.startswith('$(TESTDIR)/'):
        newlines+=re.sub(' %.',' $(EXAMPLESDIR)/%.',line)+'\n'
      elif line.startswith('include ./lib/petsc/conf/variables'):
        newlines+=re.sub('include ./lib/petsc/conf/variables',
                         'include $(PETSC_DIR)/$(PETSC_ARCH)/lib/petsc/conf/variables',
                         line)+'\n'
      else:
        newlines+=re.sub('PETSC_ARCH','PETSC_DIR)/$(PETSC_ARCH',line)+'\n'
    newFile = open(src, 'w')
    newFile.write(newlines)
    newFile.close()
    return

  def copyConfig(self, src, dst):
    """Recursively copy the examples directories
    """
    if not os.path.isdir(dst):
      raise shutil.Error, 'Destination is not a directory'

    self.copyfile('gmakefile.test',dst)
    newConfigDir=os.path.join(dst,'config')  # Am not renaming at present
    if not os.path.isdir(newConfigDir): os.mkdir(newConfigDir)
    testConfFiles="gmakegentest.py gmakegen.py testparse.py example_template.py".split()
    testConfFiles+="petsc_harness.sh report_tests.py watchtime.sh".split()
    testConfFiles+=["cmakegen.py"]
    for tf in testConfFiles:
      self.copyfile(os.path.join('config',tf),newConfigDir)
    return

  def copyExamples(self, src, dst):
    """Recursively copy the examples directories
    """
    if not os.path.isdir(dst):
      raise shutil.Error, 'Destination is not a directory'

    names  = os.listdir(src)
    nret2 = 0
    for name in names:
      srcname = os.path.join(src, name)
      dstname = os.path.join(dst, name)
      if not name.startswith('arch') and os.path.isdir(srcname) and os.path.isfile(os.path.join(srcname,'makefile')):
        os.mkdir(dstname)
        nret = self.copyExamples(srcname,dstname)
        if name == 'tests' or name == 'tutorials':
          self.copyexamplefiles(srcname,dstname)
          if os.path.isdir(os.path.join(srcname,'output')):
            os.mkdir(os.path.join(dstname,'output'))
            self.copyexamplefiles(os.path.join(srcname,'output'),os.path.join(dstname,'output'))
          nret = 1
        if not nret:
          # prune directory branches that don't have examples under them
          os.rmdir(dstname)
        nret2 = nret + nret2
    return nret2

  def copytree(self, src, dst, symlinks = False, copyFunc = shutil.copy2, exclude = []):
    """Recursively copy a directory tree using copyFunc, which defaults to shutil.copy2().

       The copyFunc() you provide is only used on the top level, lower levels always use shutil.copy2

    The destination directory must not already exist.
    If exception(s) occur, an shutil.Error is raised with a list of reasons.

    If the optional symlinks flag is true, symbolic links in the
    source tree result in symbolic links in the destination tree; if
    it is false, the contents of the files pointed to by symbolic
    links are copied.
    """
    copies = []
    names  = os.listdir(src)
    if not os.path.exists(dst):
      os.makedirs(dst)
    elif not os.path.isdir(dst):
      raise shutil.Error, 'Destination is not a directory'
    errors = []
    for name in names:
      srcname = os.path.join(src, name)
      dstname = os.path.join(dst, name)
      try:
        if symlinks and os.path.islink(srcname):
          linkto = os.readlink(srcname)
          os.symlink(linkto, dstname)
        elif os.path.isdir(srcname):
          copies.extend(self.copytree(srcname, dstname, symlinks,exclude = exclude))
        elif not os.path.basename(srcname) in exclude:
          copyFunc(srcname, dstname)
          copies.append((srcname, dstname))
        # XXX What about devices, sockets etc.?
      except (IOError, os.error), why:
        errors.append((srcname, dstname, str(why)))
      # catch the Error from the recursive copytree so that we can
      # continue with other files
      except shutil.Error, err:
        errors.extend((srcname,dstname,str(err.args[0])))
    try:
      shutil.copystat(src, dst)
    except OSError, e:
      if WindowsError is not None and isinstance(e, WindowsError):
        # Copying file access times may fail on Windows
        pass
      else:
        errors.extend((src, dst, str(e)))
    if errors:
      raise shutil.Error, errors
    return copies


  def fixConfFile(self, src):
    lines   = []
    oldFile = open(src, 'r')
    for line in oldFile.readlines():
      # paths generated by configure could be different link-path than whats used by user, so fix both
      line = line.replace(os.path.join(self.rootDir, self.arch), self.installDir)
      line = line.replace(os.path.realpath(os.path.join(self.rootDir, self.arch)), self.installDir)
      line = line.replace(os.path.join(self.rootDir, 'bin'), self.installBinDir)
      line = line.replace(os.path.realpath(os.path.join(self.rootDir, 'bin')), self.installBinDir)
      line = line.replace(os.path.join(self.rootDir, 'include'), self.installIncludeDir)
      line = line.replace(os.path.realpath(os.path.join(self.rootDir, 'include')), self.installIncludeDir)
      # remove PETSC_DIR/PETSC_ARCH variables from conf-makefiles. They are no longer necessary
      line = line.replace('${PETSC_DIR}/${PETSC_ARCH}', self.installDir)
      line = line.replace('PETSC_ARCH=${PETSC_ARCH}', '')
      line = line.replace('${PETSC_DIR}', self.installDir)
      lines.append(line)
    oldFile.close()
    newFile = open(src, 'w')
    newFile.write(''.join(lines))
    newFile.close()
    return

  def fixConf(self):
    import shutil
    for file in ['rules', 'variables','petscrules', 'petscvariables']:
      self.fixConfFile(os.path.join(self.destConfDir,file))
    self.fixConfFile(os.path.join(self.destLibDir,'pkgconfig','PETSc.pc'))
    return

  def createUninstaller(self):
    uninstallscript = os.path.join(self.destConfDir, 'uninstall.py')
    f = open(uninstallscript, 'w')
    # Could use the Python AST to do this
    f.write('#!'+sys.executable+'\n')
    f.write('import os\n')

    f.write('copies = '+repr(self.copies).replace(self.destDir,self.installDir))
    f.write('''
for src, dst in copies:
  try:
    os.remove(dst)
  except:
    pass
''')
    #TODO: need to delete libXXX.YYY.dylib.dSYM directory on Mac
    dirs = [os.path.join('include','petsc','finclude'),os.path.join('include','petsc','mpiuni'),os.path.join('include','petsc','private'),os.path.join('bin'),os.path.join('lib','petsc','conf')]
    newdirs = []
    for dir in dirs: newdirs.append(os.path.join(self.installDir,dir))
    f.write('dirs = '+str(newdirs))
    f.write('''
for dir in dirs:
  import shutil
  try:
    shutil.rmtree(dir)
  except:
    pass
''')
    f.close()
    os.chmod(uninstallscript,0744)
    return

  def installIncludes(self):
    # TODO: should exclude petsc-mpi.uni except for uni builds
    # TODO: should exclude petsc/finclude except for fortran builds
    self.copies.extend(self.copytree(self.rootIncludeDir, self.destIncludeDir,exclude = ['makefile']))
    self.copies.extend(self.copytree(self.archIncludeDir, self.destIncludeDir))
    return

  def installConf(self):
    self.copies.extend(self.copytree(self.rootConfDir, self.destConfDir))
    self.copies.extend(self.copytree(self.archConfDir, self.destConfDir, exclude = ['sowing', 'configure.log.bkp']))
    return

  def installBin(self):
    self.copies.extend(self.copytree(os.path.join(self.rootBinDir), os.path.join(self.destBinDir)))
    # TODO: should copy over petsc-mpiexec.uni only for uni builds
    self.copies.extend(self.copyfile(os.path.join(self.rootBinDir,'petsc-mpiexec.uni'), self.destBinDir))
    self.copies.extend(self.copytree(self.archBinDir, self.destBinDir, exclude = ['bfort','bib2html','doc2lt','doctext','mapnames', 'pstogif','pstoxbm','tohtml']))
    return

  def installShare(self):
    self.copies.extend(self.copytree(self.rootShareDir, self.destShareDir))
    examplesdir=os.path.join(self.destShareDir,'petsc','examples')
    if os.path.exists(examplesdir):
      shutil.rmtree(examplesdir)
    os.mkdir(examplesdir)
    self.copyExamples(self.rootDir,examplesdir)
    self.copyConfig(self.rootDir,examplesdir)
    self.fixExamplesMakefile(os.path.join(examplesdir,'gmakefile.test'))
    return

  def copyLib(self, src, dst):
    '''Run ranlib on the destination library if it is an archive. Also run install_name_tool on dylib on Mac'''
    # Symlinks (assumed local) are recreated at dst
    if os.path.islink(src):
      linkto = os.readlink(src)
      try:
        os.remove(dst)            # In case it already exists
      except OSError:
        pass
      os.symlink(linkto, dst)
      return
    # Do not install object files
    if not os.path.splitext(src)[1] == '.o':
      shutil.copy2(src, dst)
    if os.path.splitext(dst)[1] == '.'+self.arLibSuffix:
      self.executeShellCommand(self.ranlib+' '+dst)
    if os.path.splitext(dst)[1] == '.dylib' and os.path.isfile('/usr/bin/install_name_tool'):
      [output,err,flg] = self.executeShellCommand("otool -D "+src)
      oldname = output[output.find("\n")+1:]
      installName = oldname.replace(self.archDir, self.installDir)
      self.executeShellCommand('/usr/bin/install_name_tool -id ' + installName + ' ' + dst)
    # preserve the original timestamps - so that the .a vs .so time order is preserved
    shutil.copystat(src,dst)
    return

  def installLib(self):
    self.copies.extend(self.copytree(self.archLibDir, self.destLibDir, copyFunc = self.copyLib, exclude = ['.DIR', 'sowing']))
    return


  def outputInstallDone(self):
    print '''\
====================================
Install complete.
Now to check if the libraries are working do (in current directory):
make PETSC_DIR=%s PETSC_ARCH="" test
====================================\
''' % (self.installDir)
    return

  def outputDestDirDone(self):
    print '''\
====================================
Copy to DESTDIR %s is now complete.
Before use - please copy/install over to specified prefix: %s
====================================\
''' % (self.destDir,self.installDir)
    return

  def runsetup(self):
    self.setup()
    self.setupDirectories()
    self.checkPrefix()
    self.checkDestdir()
    return

  def runcopy(self):
    if self.destDir == self.installDir:
      print '*** Installing PETSc at prefix location:',self.destDir, ' ***'
    else:
      print '*** Copying PETSc to DESTDIR location:',self.destDir, ' ***'
    if not os.path.exists(self.destDir):
      try:
        os.makedirs(self.destDir)
      except:
        print '********************************************************************'
        print 'Unable to create', self.destDir, 'Perhaps you need to do "sudo make install"'
        print '********************************************************************'
        sys.exit(1)
    self.installIncludes()
    self.installConf()
    self.installBin()
    self.installLib()
    self.installShare()
    return

  def runfix(self):
    self.fixConf()
    return

  def rundone(self):
    self.createUninstaller()
    if self.destDir == self.installDir:
      self.outputInstallDone()
    else:
      self.outputDestDirDone()
    return

  def run(self):
    self.runsetup()
    self.runcopy()
    self.runfix()
    self.rundone()
    return

if __name__ == '__main__':
  Installer(sys.argv[1:]).run()
  # temporary hack - delete log files created by BuildSystem - when 'sudo make install' is invoked
  delfiles=['RDict.db','RDict.log','buildsystem.log','default.log','buildsystem.log.bkp','default.log.bkp']
  for delfile in delfiles:
    if os.path.exists(delfile) and (os.stat(delfile).st_uid==0):
      os.remove(delfile)
