project('OptimFROG_Win_x64_5100', 'c', license: 'http://losslessaudio.org/License.php')

compiler = meson.get_compiler('c')

is_msvc = compiler.get_id() == 'msvc' or compiler.get_id() == 'clang-cl'

if target_machine.system() == 'windows' and target_machine.cpu_family() == 'x86_64' and is_msvc
	optimfrog_dep = declare_dependency(
		dependencies : compiler.find_library('OptimFROG', dirs : meson.current_source_dir() / 'SDK' / 'Library'),
		include_directories : include_directories('SDK')
	)

	install_data(
		'SDK/Library/OptimFROG.dll',
		install_dir: get_option('bindir')
	)

	meson.override_dependency('optimfrog', optimfrog_dep)
endif
