project('4va', 'c',
  version : '1.23',
  meson_version : '>=1.1',
  default_options : ['c_std=gnu17', 'warning_level=0', 'buildtype=release'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
x11_dep = dependency('x11')
xrandr_dep = dependency('xrandr', required : get_option('xrandr'))
xrender_dep = dependency('xrender', required : get_option('xrender'))
xpresent_dep = dependency('xpresent', required : get_option('xpresent'))

x_args = []
if xrandr_dep.found()
  x_args += '-DHAVE_XRANDR'
endif
if xrender_dep.found()
  x_args += '-DHAVE_XRENDER'
endif
if xpresent_dep.found()
  x_args += '-DHAVE_XPRESENT'
endif

executable('4va',
  'src/4vamain.c', 'src/4vaproj.c', 'src/4vartn.c', 'src/4vad_x.c',
  dependencies : [m_dep, x11_dep, xrandr_dep, xrender_dep, xpresent_dep],
  c_args : x_args,
  install : true)

foreach prog : ['ctorus', 'cutctorus', '4vdmake']
  executable(prog, 'src' / prog + '.c',
    dependencies : m_dep,
    install : true)
endforeach

install_man('doc/4va.1')
install_subdir('data', install_dir : get_option('datadir') / '4va', strip_directory : true)
