"""
Setup script for RedFlag
"""
from setuptools import setup, find_packages
import os

# read version from redflag/__init__.py
def get_version():
    version_file = os.path.join(os.path.dirname(__file__), 'redflag', '__init__.py')
    with open(version_file, 'r', encoding='utf-8') as f:
        for line in f:
            if line.startswith('__version__'):
                return line.split('=')[1].strip().strip('"').strip("'")
    return '0.0.0'

# read readme if it exists
readme_path = os.path.join(os.path.dirname(__file__), 'README.md')
long_description = ""
if os.path.exists(readme_path):
    with open(readme_path, 'r', encoding='utf-8') as f:
        long_description = f.read()

setup(
    name='redflag',
    version=get_version(),
    description='RedFlag - Malware Analysis Tool for C++ Projects',
    long_description=long_description,
    long_description_content_type='text/markdown',
    author='RoRvzzz',
    url='https://github.com/RoRvzzz/RedFlag',
    packages=find_packages(),
    include_package_data=True,
    package_data={
        'redflag': ['assets/*'],
    },
    python_requires='>=3.7',
    install_requires=[
        'rich>=10.0.0',
        'urlextract>=1.8.0',
        'filetype>=1.2.0',
    ],
    extras_require={
        'yara': ['yara-python>=4.3.0'],
    },
    entry_points={
        'console_scripts': [
            'redflag=redflag.core.engine:main',
        ],
    },
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Topic :: Security',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
    ],
)
