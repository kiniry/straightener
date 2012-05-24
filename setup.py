from distutils.core import *
import numpy as np

setup(name='lineDetect',
      version='1.0',
      ext_modules=[Extension('lineDetect', ['lineDetectmodule.c'])],
      include_dirs = [np.get_include()]
     )
