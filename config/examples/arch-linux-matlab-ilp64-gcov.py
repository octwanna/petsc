#!/usr/bin/python

# This test is done on grind.mcs.anl.gov. It uses IPL64 MKL/BLAS packaged
# with MATLAB.

# Note: regular BLAS [with 32bit integers] conflict wtih
# MATLAB BLAS - hence requring -known-64-bit-blas-indices=1

# Note: MATLAB build requires petsc shared libraries

# Some versions of Matlab [R2013a] conflicted with -lgfortan - so the following workarround worked.
# export LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/4.6/libgfortran.so

if __name__ == '__main__':
  import sys
  import os
  sys.path.insert(0, os.path.abspath('config'))
  import configure
  configure_options = [
    '--download-mpich=1', # /usr/bin/mpicc does not resolve '__gcov_merge_add'? and gcc-4.4 gives gcov errors
    '--with-display=140.221.10.20:0.0', # for matlab example with graphics
    '--with-blaslapack-dir=/soft/com/packages/MATLAB/R2016a',
    '--with-matlab=1',
    '--with-matlab-engine=1',
    '--with-matlabengine-lib=-Wl,-rpath,/soft/com/packages/MATLAB/R2016a/sys/os/glnxa64:/soft/com/packages/MATLAB/R2016a/bin/glnxa64:/soft/com/packages/MATLAB/R2016a/extern/lib/glnxa64 -L/soft/com/packages/MATLAB/R2016a/bin/glnxa64 -L/soft/com/packages/MATLAB/R2016a/extern/lib/glnxa64 -leng -lmex -lmx -lmat -lut -lmwm_dispatcher -lmwopcmodel -lmwservices -lmwservices -lmwopcmodel -lmwopcmodel -lmwm_dispatcher -lmwmpath -lmwopcmodel -lmwservices -lmwopcmodel -lmwservices -lxerces-c',
    '--with-shared-libraries=1',
    '-known-64-bit-blas-indices=1',
    '--with-ssl=0',
    '--with-gcov=1',
  ]
  configure.petsc_configure(configure_options)
