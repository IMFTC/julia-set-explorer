project('julia-set-explorer', 'c')

gnome = import('gnome')

thread_dep = dependency('threads')
gtk_dep = dependency('gtk+-3.0')
clutter_dep = dependency('clutter-gtk-1.0')
# http://mesonbuild.com/howtox.html#add-math-library-lm-portably
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

jse_resources = gnome.compile_resources('jse_resources',
                                        'jse.gresource.xml',
                                        source_dir: '.')

executable('jse', 'main.c', 'julia.c', 'jse-window.c', jse_resources,
           dependencies : [gtk_dep, clutter_dep, m_dep, thread_dep])
