#*
#* Created by v1tr10l7 on 02.09.2025.
#* Copyright (c) 2025-2025, Szymon Zemke <v1tr10l7@proton.me>
#*
#* SPDX-License-Identifier: GPL-3
#*/
project('AwesomeShell', ['c', 'cpp'],
  version: 'v0.2.0', license: 'GPL-3.0',
  default_options: [
    'optimization=3',
    'warning_level=2',
    'werror=true',
    'c_std=c17',
    'cpp_std=c++23',
  ],
)

if not meson.get_compiler('cpp').has_argument('-std=gnu++23')
  error('C++23 support is needed. Please install clang 17 or higher.')
endif

c = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
ld = cxx.get_linker_id()
arch = host_machine.cpu_family()

cxx_args = [
  '-fno-rtti',
  '-unwindlib=none',
  '-fno-exceptions',
  '-DFMT_HEADER_ONLY',
  '-DFMT_STATIC_THOUSANDS_SEPARATOR=1',
  '-DFMT_USE_LONG_DOUBLE=0',
  '-DFMT_USE_DOUBLE=0',
  '-DFMT_USE_FLOAT=0',
  '-DFMT_USE_FLOAT128=0',
  '-DFMT_USE_EXCEPTIONS=0',
  '-DPRISM_USE_NAMESPACE=1',
  '-DPRISM_LOG_ENABLE=1',
  '-DPRISM_BUILD_DEBUG=1',
]
add_global_arguments(cxx_args, language: 'cpp')

srcs = files(
  'Source/Builtins.cpp',
  'Source/Environment.cpp',
  'Source/Executor.cpp',
  'Source/Expander.cpp',
  'Source/Lexer.cpp',
  'Source/Lowerer.cpp',
  'Source/Parser.cpp',
  'Source/Shell.cpp',
)
incs = include_directories(
  'Source',
)

extraincs = get_option('extra_incs')
build_tests = get_option('build_tests')

if extraincs != ''
  cxx_args += '-I' + extraincs
  message('\n\n\n\nextra incs: ', extraincs)
else 
  message('no extra incs\n\n\n\n')
endif

deps = [
  dependency('prism'),
  dependency('neon'),
]

git_tag = run_command('git', 'rev-parse', 'HEAD').stdout().strip()

version = meson.project_version().substring(1).split('.')
versionConf = configuration_data()
versionConf.set('AS_VERSION_MAJOR', version[0])
versionConf.set('AS_VERSION_MINOR', version[1])
versionConf.set('AS_VERSION_PATCH', version[2])
versionConf.set('git_tag', git_tag)
versionConf.set('compiler', cxx.get_id())
versionConf.set('compiler_version', cxx.version())

kernel_version_string_template = '@0@.@1@.@2@-@3@'

configure_file(
  input: 'Source/Version.hpp.in', 
  output: 'Version.hpp',
  configuration: versionConf)

executable(
  'AwesomeShell', [srcs, files('Source/main.cpp')],
  include_directories: incs, 
  link_args: ['-lsupc++', '-lgcc_s'], 
  dependencies: deps, install: true
)

subdir('Tests')
