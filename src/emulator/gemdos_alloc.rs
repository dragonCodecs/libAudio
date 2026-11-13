// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cmp::Ordering;

pub struct GemdosAllocator
{
	heap_begin: u32,
	heap_end: u32,
	heap_current: u32,

	free_list: Vec<AllocChunk>,
	alloc_list: Vec<AllocChunk>,
}

#[derive(Default, Clone, Copy)]
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

// NB: alignment must be a power of two coming into this function(!)
fn align(size: usize, alignment: usize) -> usize
{
	(size + alignment - 1) & !(alignment - 1)
}

impl GemdosAllocator
{
	pub fn new(heap_base: u32, heap_size: u32) -> Self
	{
		Self {
			heap_begin: heap_base,
			heap_end: heap_base + heap_size,
			heap_current: heap_base,
			free_list: Vec::new(),
			alloc_list: Vec::new(),
		}
	}

	fn change_heap_size(&mut self, amount: i32) -> u32
	{
		// Check if this allocation would exhaust the heap
		if self.heap_current.strict_add_signed(amount) > self.heap_end {
			return u32::MAX;
		}

		// Everything okay? okay.. make a copy of the heap pointer to return and
		// then add the allocation to the heap pointer
		let result = self.heap_current;
		self.heap_current = self.heap_current.strict_add_signed(amount);
		result
	}

	pub fn alloc(&mut self, size: usize) -> Option<u32>
	{
		// Allocations of 0 bytes are not allowed
		if size == 0 {
			return None;
		}

		// Figure out allocation padding and size
		let alloc_size = {
			let aligned_size = align(size, CHUNK_ALIGNMENT) + MALLOC_PADDING;
			usize::max(aligned_size, MALLOC_MIN_CHUNK_SIZE)
		};

		// Check if the allocation is just too big, or if the computed size is less than requested)
		if alloc_size as u32 > self.heap_end - self.heap_begin || alloc_size < size {
			return None;
		}

		// Set up to store where we can allocate from
		let mut chunk = AllocChunk::default();

		// Now step through the free list by iterator
		for (index, entry) in self.free_list.iter_mut().enumerate() {
			// Compute how much memory would be remaining if this free chunk was used
			let remainder = entry.size.wrapping_sub(alloc_size as u32) as i32;
			// If it would fit, then
			if remainder >= 0 {
				// Check if the remainder is enough for another chunk
				if remainder as usize >= MALLOC_MIN_CHUNK_SIZE {
					// Split the node, and extract the necessary memory for the allocation
					chunk.base = entry.base;
					chunk.size = alloc_size as u32;
					entry.base += alloc_size as u32;
					entry.size -= alloc_size as u32;
				}
				// If we found chunk that's exactly or barely bigger than the right size,
				// extract it from the free list entirely
				else {
					chunk = self.free_list.remove(index);
				}
				// Happy case, we're done now!
				break;
			}
		}

		// Check to see if we managed to liberate a chunk from the free list, if not
		// then ask for some more memory from the heap
		if chunk.size == 0 {
			// Get some memory
			chunk.base = self.change_heap_size(alloc_size as i32);
			// If that failed
			if chunk.base == u32::MAX {
				// If the free list is empty, we're done
				if self.free_list.is_empty() {
					return None;
				}
				// Check to see if the last item in the free list could be grown
				// if it's at the end of the heap
				let &entry = self.free_list.last()?;
				if entry.base + entry.size == self.heap_current {
					// Ask for the difference and extract the node if that succeeds
					if self.change_heap_size((alloc_size as u32 - entry.size) as i32) == u32::MAX {
						return None;
					}
					chunk.base = entry.base;
					chunk.size = alloc_size as u32;
					self.free_list.pop();
				}
				// Otherwise, we couldn't find any more free memory
				else {
					return None;
				}
			}
			// Otherwise, hppy case - set the allocation size
			else {
				chunk.size = alloc_size as u32;
			}
		}
		// Happy days, we got some memory - return it!
		if chunk.size != 0 {
			self.alloc_list.push(chunk);
			Some(chunk.base)
		} else {
			None
		}
	}

	pub fn free(&mut self, ptr: u32) -> bool
	{
		// Find the chunk for this pointer from the alloc list
		let node = self.alloc_list.iter().position(|chunk| chunk.base == ptr);
		match node {
			// If it's not in this list, something is wrong but we're also done here
			None => false,
			Some(index) => {
				// Otherwise, extract it from the alloc list and find where to insert it in the free list
				// (strict requirement: free block list must remain sorted!)
				let chunk = self.alloc_list.remove(index);
				let index = self
					.free_list
					.binary_search_by(|chunk| match chunk.base.cmp(&ptr) {
						Ordering::Equal => Ordering::Greater,
						ord => ord,
					})
					.expect_err("Insertion position search never fails");
				self.free_list.insert(index, chunk);
				true
			},
		}
	}

	pub fn largest_free_block(&self) -> u32
	{
		// Start with the size of the heap remaining to be allocated
		let heap_free = self.heap_end - self.heap_current;
		// And see if anything in the free list is larger, returning whatever we can find
		self.free_list.iter().fold(heap_free, |a, b| a.max(b.size))
	}
}
