#!/usr/bin/env python

import os
import glob
from subprocess import call
from distutils.core import setup
from distutils.command.build_py import build_py

class cmake_lib(build_py):

    def run(self):

        lib_extensions = [ ".so", ".dylib", ".dll" ]

        module_name = "pycmc"
        source_dir  = os.path.abspath(".")
        build_dir   = os.path.abspath("build_pycmc")
        lib_dir     = os.path.join(build_dir, "python")
        lib_base    = os.path.join(lib_dir, module_name)

        if not self.dry_run:

            # create our shared object file
            call(["./compile_wrapper.sh", source_dir, build_dir, 'rename_pycmc_lib'])

            target_dir = self.build_lib
            print("target_dir: " + target_dir)
            module_dir = os.path.join(target_dir, module_name)

            # make sure the module dir exists
            self.mkpath(module_dir)

            # copy our library to the module dir
            print(lib_base)
            lib_file = [ lib_base + e for e in lib_extensions if os.path.isfile(lib_base + e) ][0]
            self.copy_file(lib_file, module_dir)

        # run parent implementation
        build_py.run(self)

setup(
    name='pycmc',
    version='0.0.1',
    author='Jan Funke',
    author_email='jfunke@iri.upc.edu',
    description='Python wrapper for the Candidate MC.',
    packages=['pycmc'],
    cmdclass={'build_py' : cmake_lib}
)
