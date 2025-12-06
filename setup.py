from setuptools import setup
from setuptools.command.install import install

class CustomInstall(install):
    def run(self):
        install.run(self)

setup(
    name='CombinedAPISample',
    version='1.0',
    py_modules=['ndiTrack'],
    cmdclass={'install': CustomInstall},
    # entry_points={
    #     'console_scripts': ['ardemo=ardemo:Ardemo']
    # },
)