project('julia-set-explorer', 'c')

thread_dep = dependency('threads')
gtk_dep = dependency('gtk+-3.0')

# http://mesonbuild.com/howtox.html#add-math-library-lm-portably
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

executable('jse', 'main.c', 'julia.c', dependencies : [gtk_dep, m_dep, thread_dep])
