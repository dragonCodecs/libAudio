project('OptimFROG_OSX_x64_5100', 'c', license: 'http://losslessaudio.org/License.php')

if target_machine.system() == 'darwin' and target_machine.cpu_family() == 'x86_64'
	compiler = meson.get_compiler('c')

	optimfrog_dep = declare_dependency(
		dependencies : compiler.find_library('OptimFROG', dirs : meson.current_source_dir() / 'SDK' / 'Library'),
		include_directories : include_directories('SDK')
	)

	install_data(
		'SDK/Library/libOptimFROG.0.dylib',
		install_dir: get_option('libdir')
	)

	install_symlink(
		'libOptimFROG.dylib',
		install_dir: get_option('libdir'),
		pointing_to: 'libOptimFROG.0.dylib'
	)

	meson.override_dependency('optimfrog', optimfrog_dep)
endif
