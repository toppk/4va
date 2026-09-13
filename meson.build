project('4va', 'c',
  version : '1.21',
  meson_version : '>=1.1',
  default_options : ['c_std=gnu17', 'warning_level=0', 'buildtype=release'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
x11_dep = dependency('x11')
xrandr_dep = dependency('xrandr', required : get_option('xrandr'))

executable('4va',
  'src/4vamain.c', 'src/4vaproj.c', 'src/4vartn.c', 'src/4vad_x.c',
  dependencies : [m_dep, x11_dep, xrandr_dep],
  c_args : xrandr_dep.found() ? ['-DHAVE_XRANDR'] : [],
  install : true)

foreach prog : ['ctorus', 'cutctorus', '4vdmake']
  executable(prog, 'src' / prog + '.c',
    dependencies : m_dep,
    install : true)
endforeach

install_man('doc/4va.1')
install_subdir('data', install_dir : get_option('datadir') / '4va', strip_directory : true)
