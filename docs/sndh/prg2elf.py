#!/usr/bin/env python
# SPDX-License-Identifier: BSD-3-Clause
from enum import IntFlag
from construct import Struct, Const, Int16ub, Int32ub, FlagsEnum, Bytes, GreedyBytes, this
from pathlib import Path
from sys import argv, exit
from makeelf.elf import ELF
from makeelf.elfstruct import EM as ELFMachine, ET as ELFType, ELFCLASS as ELFClass, ELFDATA as ELFData
from makeelf.elfstruct import SHT as SectionType, SHF as SectionFlags, Elf32_Shdr
from makeelf.elfstruct import PT as SegmentType, Elf32_Phdr, Elf32, Elf32_Ehdr
from typing import Union

class PrgFlags(IntFlag):
	fastLoad = 1
	ttramLoad = 2
	ttramMem = 4

prgFile = 'prg' / Struct(
	'magic' / Const(0x601a, Int16ub),
	'textSize' / Int32ub,
	'dataSize' / Int32ub,
	'bssSize' / Int32ub,
	'symTabSize' / Int32ub,
	'reserved' / Const(0, Int32ub),
	'flags' / FlagsEnum(Int32ub, PrgFlags),
	'absolute' / Int16ub,
	'textSeg' / Bytes(this.textSize),
	'dataSeg' / Bytes(this.dataSize),
	'symTabSeg' / Bytes(this.symTabSize),
	'fixupOffset' / Int32ub,
	'fixups' / GreedyBytes,
)

def fixupSection(*, section: Elf32_Shdr, type: SectionType, flags: SectionFlags, size: Union[int, None] = None,
	offset: Union[int, None] = None
) -> None:
	section.sh_type = type
	section.sh_flags = int(flags)
	if size is not None:
		section.sh_size = size
	if offset is not None:
		section.sh_offset = offset

def fixupSegment(*, segment: Elf32_Phdr, type: Union[SegmentType, None] = None, fileSize: Union[int, None] = None) -> None:
	segment.p_paddr = segment.p_vaddr
	if type is not None:
		segment.p_type = type
	if fileSize is not None:
		segment.p_filesz = fileSize

def fixupHeaders(elf: Elf32) -> None:
	ehdr: Elf32_Ehdr = elf.Ehdr
	offset = len(ehdr)

	phdrLength = 0
	for header in elf.Phdr_table:
		phdrLength += len(header)

	shdrLength = 0
	for header in elf.Shdr_table:
		shdrLength += len(header)

	ehdr.e_phoff = offset
	ehdr.e_phentsize = len(Elf32_Phdr())
	ehdr.e_phnum = len(elf.Phdr_table)
	offset += phdrLength

	ehdr.e_shoff = offset
	ehdr.e_shentsize = len(Elf32_Shdr())
	ehdr.e_shnum = len(elf.Shdr_table)
	offset += shdrLength

	for i, header in enumerate(elf.Shdr_table):
		sectionLength = len(elf.sections[i])
		header.sh_offset = offset
		header.sh_size = sectionLength
		offset += sectionLength
	elf.Shdr_table[0].sh_offset = 0 # The NULL section is not supposed to be offset.

def convert(fileName: Path) -> None:
	print(f'Parsing file {fileName.name}')
	with fileName.open('rb') as file:
		prgData = prgFile.parse_stream(file)

	print(f'Got GEMDOS executable with magic {prgData.magic:x}')
	print(f'\t-> .text section is {prgData.textSize} bytes long')
	print(f'\t-> .data section is {prgData.dataSize} bytes long')
	print(f'\t->  .bss section is {prgData.bssSize} bytes long')

	elfFileName = fileName.with_suffix('.elf')
	print(f'Building ELF {elfFileName.name}')

	elf = ELF(e_class = ELFClass.ELFCLASS32, e_data = ELFData.ELFDATA2MSB, e_type = ELFType.ET_DYN,
		e_machine = ELFMachine.EM_68K)

	sectionOffset = 0
	textSection = elf.append_section('.text', prgData.textSeg, sectionOffset)
	sectionOffset += prgData.textSize
	dataSection = elf.append_section('.data', prgData.dataSeg, sectionOffset)
	sectionOffset += prgData.dataSize
	bssSection = elf.append_section('.bss', b'', sectionOffset)

	print(f'-> Sections are .text = {textSection}, .data = {dataSection}, .bss = {bssSection}')

	fixupSection(
		section = elf.Elf.Shdr_table[textSection], type = SectionType.SHT_PROGBITS,
		flags = SectionFlags.SHF_ALLOC | SectionFlags.SHF_EXECINSTR
	)
	fixupSection(
		section = elf.Elf.Shdr_table[dataSection], type = SectionType.SHT_PROGBITS,
		flags = SectionFlags.SHF_ALLOC | SectionFlags.SHF_WRITE
	)
	fixupSection(
		section = elf.Elf.Shdr_table[bssSection], type = SectionType.SHT_NOBITS,
		flags = SectionFlags.SHF_ALLOC | SectionFlags.SHF_WRITE,
		size = int(prgData.bssSize), offset = sectionOffset
	)

	sectionOffset = 0
	textSegment = elf.append_segment(textSection, mem_size = prgData.textSize, flags = 'r-x')
	sectionOffset += prgData.textSize
	dataSegment = elf.append_segment(dataSection, mem_size = prgData.dataSize, flags = 'rw-')
	sectionOffset += prgData.dataSize
	bssSegment = elf.append_segment(bssSection, mem_size = prgData.bssSize, flags = 'rw-')

	print(f'-> Segments are .text = {textSegment}, .data = {dataSegment}, .bss = {bssSegment}')

	fixupSegment(segment = elf.Elf.Phdr_table[textSegment])
	fixupSegment(segment = elf.Elf.Phdr_table[dataSegment])
	fixupSegment(segment = elf.Elf.Phdr_table[bssSegment], fileSize = 0)

	fixupHeaders(elf.Elf)

	with elfFileName.open('wb') as file:
		file.write(bytes(elf.Elf))

	print('Done')
	print('----')
	print(f'PRG flags: {prgData.flags!r}')
	print(f'Absolute mode: {prgData.absolute}')
	print(f'Symbol Table size: {prgData.symTabSize}')
	print(f'First fixup is at offset {prgData.fixupOffset} and there are {len(prgData.fixups)} bytes of fixups')

if __name__ == '__main__':
	if len(argv) != 2:
		print('Usage:')
		print(f'\t{argv[0]} <file.prg>')
		exit(1)
	fileName = Path(argv[1])
	if not fileName.is_file():
		print(f'{fileName!s} is not a valid file, aborting')
		exit(1)
	convert(fileName)
