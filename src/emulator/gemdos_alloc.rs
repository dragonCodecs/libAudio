// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

#[allow(unused)]
pub struct GemdosAllocator
{
	heap_begin: u32,
	heap_end: u32,
	heap_current: u32,

	free_list: Vec<AllocChunk>,
	alloc_list: Vec<AllocChunk>,
}

#[allow(unused)]
struct AllocChunk
{
	base: u32,
	size: u32,
}

// Define the alignment requirements on allocations
const MALLOC_ALIGNMENT: usize = 8;
const CHUNK_ALIGNMENT: usize = 4;
// Figure out how much padding is needed for an allocation
// NB: we'd use usize::max() here between MALLOC_ALIKGNMENT and CHUNK_ALIGNMENT, but
// that isn't yet stable, so we can't
const MALLOC_PADDING: usize = MALLOC_ALIGNMENT - CHUNK_ALIGNMENT;
// Allow no smaller than a pointer wide allocation on the target platform
const MALLOC_MIN_SIZE: usize = 4;
// The minimal chunk size is the padding + the minimum allocation size
const MALLOC_MIN_CHUNK_SIZE: usize = MALLOC_PADDING + MALLOC_MIN_SIZE;

impl GemdosAllocator
{
	pub fn new(base: u32, size: u32) -> Self
	{
		Self {
			heap_begin: base,
			heap_end: base + size,
			heap_current: base,
			free_list: Vec::new(),
			alloc_list: Vec::new(),
		}
	}
}
