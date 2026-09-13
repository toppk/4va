project('4va', 'c',
  version : '1.21',
  default_options : ['c_std=gnu17', 'warning_level=0', 'buildtype=release'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
x11_dep = dependency('x11')

executable('4va',
  '4vamain.c', '4vaproj.c', '4vartn.c', '4vad_x.c',
  dependencies : [m_dep, x11_dep],
  install : true)

foreach prog : ['ctorus', 'cutctorus', '4vdmake']
  executable(prog, prog + '.c',
    dependencies : m_dep,
    install : true)
endforeach
