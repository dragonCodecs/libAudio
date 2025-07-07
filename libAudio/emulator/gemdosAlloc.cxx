// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include "gemdosAlloc.hxx"

// Define the alignment requirements on allocations
constexpr static size_t mallocAlignment{8U};
constexpr static size_t chunkAlignment{4U};
// Figure out how much padding is needed for an allocation
constexpr static size_t mallocPadding{std::max(mallocAlignment, chunkAlignment) - chunkAlignment};
// Allow no smaller than a pointer wide allocation on the target platform
constexpr static size_t mallocMinSize{4U};
// The minimal chunk size is the padding + the minimum allocation size
constexpr static size_t mallocMinChunkSize{mallocPadding + mallocMinSize};

// NB: alignment must be a power of two coming into this function(!)
constexpr static inline size_t align(const size_t size, const size_t alignment) noexcept
	{ return (size + alignment - 1U) & ~(alignment - 1U); }

gemdosAllocator_t::gemdosAllocator_t(const uint32_t base, const uint32_t size) noexcept :
	heapBegin{base}, heapEnd{base + size}, heapCurrent{base} { }

uint32_t gemdosAllocator_t::changeHeapSize(const int32_t amount) noexcept
{
	// Check if this allocation would exhaust the heap
	if (heapCurrent + amount > heapEnd)
		return UINT32_MAX;

	// Everything okay? okay.. make a copy of the heap pointer to return and
	// then add the allocation to the heap pointer
	const auto result{heapCurrent};
	heapCurrent += amount;
	return result;
}

std::optional<uint32_t> gemdosAllocator_t::alloc(const size_t size) noexcept
{
	// Allocations of 0 bytes are not allowed
	if (size == 0U)
		return std::nullopt;

	const auto allocSize
	{
		[&]() -> size_t
		{
			// Figure out allocation padding and size
			const auto alignedSize{align(size, chunkAlignment) + mallocPadding};
			return std::max(alignedSize, mallocMinChunkSize);
		}()
	};

	// Check if the allocation is just too big, or if the computed size is less than requested)
	if (allocSize > heapEnd - heapBegin || allocSize < size)
		return std::nullopt;

	// Set up to store where we can allocate from
	allocChunk_t chunk{};

	// Now step through the free list by iterator
	for (auto entry{freeList.begin()}; entry != freeList.end(); ++entry)
	{
		// Compute how much memory would be remaining if this free chunk was used
		const auto remainder{static_cast<int32_t>(entry->size - allocSize)};
		// If it would fit, then
		if (remainder >= 0)
		{
			// Check if the remainder is enough for another chunk
			if (static_cast<size_t>(remainder) >= mallocMinChunkSize)
			{
				// Split the node, and extract the necessary memory for the allocation
				chunk.base = entry->base;
				chunk.size = static_cast<uint32_t>(allocSize);
				entry->base += static_cast<uint32_t>(allocSize);
				entry->size -= static_cast<uint32_t>(allocSize);
			}
			// If we found chunk that's exactly or barely bigger than the right size,
			// extract it from the free list entirely
			else
			{
				chunk = *entry;
				freeList.erase(entry);
			}
			// Happy case, we're done now!
			break;
		}
	}

	// Check to see if we managed to liberate a chunk from the free list, if not
	// then ask for some more memory from the heap
	if (chunk.size == 0U)
	{
		// Get some memory
		chunk.base = changeHeapSize(static_cast<int32_t>(allocSize));
		// If that failed
		if (chunk.base == UINT32_MAX)
		{
			// If the free list is empty, we're done
			if (freeList.size() == 0U)
				return std::nullopt;
			// Check to see if the last item in the free list could be grown
			// if it's at the end of the heap
			const auto entry{freeList.back()};
			if (entry.base + entry.size == heapCurrent)
			{
				// Ask for the difference and extract the node if that succeeds
				if (changeHeapSize(static_cast<int32_t>(allocSize - entry.size)) == UINT32_MAX)
					return std::nullopt;
				chunk.base = entry.base;
				chunk.size = static_cast<uint32_t>(allocSize);
				freeList.pop_back();
			}
			// Otherwise, we couldn't find any more free memory
			else
				return std::nullopt;
		}
		// Otherwise, hppy case - set the allocation size
		else
			chunk.size = static_cast<uint32_t>(allocSize);
	}
	// Happy days, we got some memory - return it!
	if (chunk.size != 0U)
	{
		allocList.push_back(chunk);
		return chunk.base;
	}
	return std::nullopt;
}

bool gemdosAllocator_t::free(const uint32_t ptr) noexcept
{
	// Find the chunk for this pointer from the alloc list
	const auto node{std::find(allocList.begin(), allocList.end(), ptr)};
	// If it's not in this list, something is wrong but we're also done here
	if (node == allocList.end())
		return false;
	// Otherwise, extract it from the alloc list and find where to insert it in the free list
	// (strict requirement: free block list must remain sorted!)
	const auto chunk{*node};
	allocList.erase(node);
	freeList.insert(std::lower_bound(freeList.begin(), freeList.end(), ptr), chunk);
	return true;
}

uint32_t gemdosAllocator_t::largestFreeBlock() const noexcept
{
	// Start with the size of the heap remaining to be allocated
	const uint32_t heapFree{heapEnd - heapCurrent};
	// And see if anything in the free list is larger, returning whatever we can find
	return std::reduce
	(
		freeList.begin(), freeList.end(), allocChunk_t{0U, heapFree},
		[](const allocChunk_t &a, const allocChunk_t &b) -> allocChunk_t
			{ return {0U, std::max(a.size, b.size)}; }
	).size;
}
