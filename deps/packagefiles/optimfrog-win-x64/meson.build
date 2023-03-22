project('OptimFROG_Win_x64_5100', 'c', license: 'http://losslessaudio.org/License.php')

if target_machine.system() == 'windows' and target_machine.cpu_family() == 'x86_64' and compiler.get_argument_syntax() == 'msvc'
	compiler = meson.get_compiler('c')

	optimfrog_dep = declare_dependency(
		dependencies : compiler.find_library('OptimFROG', dirs : meson.current_source_dir() / 'SDK' / 'Library'),
		include_directories : include_directories('SDK')
	)

	install_data(
		'SDK/Library/libOptimFROG.dll',
		install_dir: get_option('bindir')
	)

	meson.override_dependency('optimfrog', optimfrog_dep)
endif
