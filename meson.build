project('julia-set-explorer', 'c')

gtkdep = dependency('gtk+-3.0')
# http://mesonbuild.com/howtox.html#add-math-library-lm-portably
cc = meson.get_compiler('c')
mdep = cc.find_library('m', required : false)

executable('jse', 'main.c', dependencies : [gtkdep, mdep])
