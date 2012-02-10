from distutils.core import *

setup(name='lineDetect',
      version='1.0',
      ext_modules=[Extension('lineDetect', ['lineDetectmodule.c'])]
     )
