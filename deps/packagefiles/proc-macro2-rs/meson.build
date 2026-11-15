# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2018-2026 Rachel Mant <git@dragonmux.network>
project(
	'proc-macro2',
	'rust',
	default_options: {
		'warning_level': '0'
	},
	version: '1.0.107',
	meson_version: '>= 1.11.0',
	license: 'MIT OR Apache-2.0'
)

rust = import('rust')

proc_macro2_workspace = rust.workspace()
proc_macro2_package = proc_macro2_workspace.package('proc-macro2', native: false)

# Turn on all the features and config needed to correctly compile proc-macro2
extra_args = [
	'--cfg', 'feature="proc-macro"',
	'--cfg', 'feature="default"',
	'--cfg', 'use_proc_macro',
	'--cfg', 'wrap_proc_macro',
]

lib = proc_macro2_package.library(
	'proc_macro2+1',
	rust_args: extra_args,
	rust_abi: 'rust'
)

dep = declare_dependency(
	link_with: lib,
	version: proc_macro2_package.version()
)

proc_macro2_package.override_dependency(dep, rust_abi: 'rust')
