project('OptimFROG_OSX_x64_5100', 'c', license: 'http://losslessaudio.org/License.php')

if target_machine.system() == 'darwin' and target_machine.cpu_family() == 'x86_64'
	compiler = meson.get_compiler('c')

	repair_rpath = find_program('repair_rpath.py', required: true)

	optimfrog_lib = custom_target(
		'OptimFROG',
		input: meson.current_source_dir() / 'SDK' / 'Library' / 'libOptimFROG.0.dylib',
		output: 'libOptimFROG.5.100.dylib',
		install: true,
		install_dir: get_option('libdir'),
		command: [repair_rpath, '@INPUT@', '@OUTPUT@']
	)

	dummy_optimfrog = static_library(
		'dummy_optimfrog',
		sources: 'dummy.c',
		link_with: optimfrog_lib,
	)

	optimfrog_dep = declare_dependency(
		link_with: dummy_optimfrog,
		include_directories : include_directories('SDK')
	)

	install_symlink(
		'libOptimFROG.dylib',
		install_dir: get_option('libdir'),
		pointing_to: 'libOptimFROG.5.100.dylib'
	)

	meson.override_dependency('optimfrog', optimfrog_dep)
endif
