* SPDX-License-Identifier: BSD-3-Clause
* SPDX-FileCopyrightText: 2025 Rachel Mant <git@dragonmux.network>
	.arch 68000

	.text
	.global _start
	.func _start

_start:
	* Define the 3 entrypoints to this SNDH
	bra.w init
	bra.w deinit
	bra.w play
	* SNDH headers - start by marking it as a valid file
	.ascii "SNDH"
	* "Composer"
	.ascii "COMM"
	.asciz "dragonmux"
	* "Title"
	.ascii "TITL"
	.asciz "Atari STe test"
	* "Ripper name"
	.ascii "RIPP"
	.asciz "Original work"
	* "Converter"
	.ascii "CONV"
	.asciz "dragonmux/libAudio 2025"
	.ascii "YEAR"
	.asciz "2025"
	* Mark there as being just the one tune
	.asciz "##01"
	* Which is a couple of seconds long
	.ascii "TIME"
	dc.w 2
	* Use timer C @ 50Hz
	.asciz "TC50"
	* End the SNDH header
	.ascii "HDNS"
	.even

psgRegs:
	* Define an area that will serve as a backup of the YM2149 registers before we start
	ds.b 16

dmaRegs:
	* Define an area that will serve as a backup of the MC68901 registers before we start
	ds.b 11
	.even

init:
	* Initialisation for this file is pretty darn simple - make sure the YM2149 (PSG)
	* is in a good state, make sure the MFP isn't going to do anything we don't want,
	* and make sure the DAC won't either..
	* To start, save all the registers we're about to clobber to be nice
	movem.l d0-d1/a0-a1, -(sp)
	* Now, load the address of the PSG into a0
	movea.w #0x8800, a0
	* Load the address of our save area into a1
	lea psgRegs(pc), a1
	* Start the process at reg 0, and process 16 regs
	moveq #0, d0
	move.w #15, d1
.savePSGRegs:
	* Select the next reg to save, then read its value out to our save area
	move.b d0, (a0)
	move.b (a0), (a1)+
	* Step to the next register, and decrement our counter
	addq #1, d0
	dbf d1, .savePSGRegs

.loadPSGDefaults:
	* Load the PSG regs up w/ a suite of sane defaults - nothing playing
	move.w #0x0000, d0
	movep.w d0, 0(a0)
	move.w #0x0100, d0
	movep.w d0, 0(a0)
	move.w #0x0200, d0
	movep.w d0, 0(a0)
	move.w #0x0300, d0
	movep.w d0, 0(a0)
	move.w #0x0400, d0
	movep.w d0, 0(a0)
	move.w #0x0500, d0
	movep.w d0, 0(a0)
	move.w #0x0600, d0
	movep.w d0, 0(a0)
	move.w #0x073f, d0
	movep.w d0, 0(a0)
	move.w #0x0800, d0
	movep.w d0, 0(a0)
	move.w #0x0900, d0
	movep.w d0, 0(a0)
	move.w #0x0a00, d0
	movep.w d0, 0(a0)
	move.w #0x0b00, d0
	movep.w d0, 0(a0)
	move.w #0x0c00, d0
	movep.w d0, 0(a0)

.saveDACRegs:
	* Save the DMA DAC regs into our save area
	movea.w 0x8900, a0
	lea dmaRegs(pc), a1
	move.b 0x01(a0), (a1)+
	move.b 0x03(a0), (a1)+
	move.b 0x05(a0), (a1)+
	move.b 0x07(a0), (a1)+
	move.b 0x09(a0), (a1)+
	move.b 0x0b(a0), (a1)+
	move.b 0x0d(a0), (a1)+
	move.b 0x0f(a0), (a1)+
	move.b 0x11(a0), (a1)+
	move.b 0x13(a0), (a1)+
	move.b 0x21(a0), (a1)+

	* Make sure DMA is turned off (we don't much care about anything else..)
	move.b #0, 1(a0)

	* We could be nice here with the MFP, but we just don't care..
	* make sure only timer C is actually enabled and running
	movea.w 0xfa00, a0
	move.b #0, 0x19(a0)
	move.b #0, 0x1b(a0)
	and.b #0xf0, 0x1d(a0)

	* Make sure that we start at timestep 0
	lea timestep(pc), a0
	move.w #0, (a0)

	* Unstack the saved regs
	movem.l (sp)+, d0-d1/a0-a1
	* We're done, so return to the caller!
	rts

deinit:
	* Save all registers we're about to clobber
	movem.l d0-d1/a0-a1, -(sp)

.restoreDMARegs:
	* Restore the DMA DAC regs from our save area
	movea.w 0x8900, a0
	lea dmaRegs(pc), a1
	move.b (a1)+, 0x01(a0)
	move.b (a1)+, 0x03(a0)
	move.b (a1)+, 0x05(a0)
	move.b (a1)+, 0x07(a0)
	move.b (a1)+, 0x09(a0)
	move.b (a1)+, 0x0b(a0)
	move.b (a1)+, 0x0d(a0)
	move.b (a1)+, 0x0f(a0)
	move.b (a1)+, 0x11(a0)
	move.b (a1)+, 0x13(a0)
	move.b (a1)+, 0x21(a0)

	* Now, load the address of the PSG into a0
	movea.w #0x8800, a0
	* Load the address of our save area into a1
	lea psgRegs(pc), a1
	* Start the process at reg 0, and process 16 regs
	moveq #0, d0
	move.w #15, d1
.restorePSGRegs:
	* Read the next reg to restore's value back out from our save area
	move.b (a1)+, d0
	movep.w d0, 0(a0)
	* Step to the next register, and decrement our counter
	add #0x0100, d0
	dbf d1, .restorePSGRegs

	* Unstack the saved regs
	movem.l (sp)+, d0-d1/a0-a1
	* We're done, so return to the caller!
	rts

timestep:
	* Create some storage for which step of the tune we're in, so we can generate the right changes
	dc.w 0

play:
	* Save the registers we're about to clobber
	movem.l d0-d1/a0, -(sp)

	* Load the current timestep value to figure out what we need to do
	move.w timestep(pc), d0
	* Load the address of the PSG into a0 as well so the steps have easy access to it
	movea.w #0x8800, a0

	* Call into the play step dispatcher
	bsr playStep

	* Store that we completed this step
	lea timestep(pc), a0
	addi.w #1, (a0)
	* Unstack the saved regs
	movem.l (sp)+, d0-d1/a0
	* We're done, so return to the caller!
	rts

* Expects the timestep to be in d0, and PSG address in a0
playStep:
	* Check which action this step requires we take
	* Setup for first note (A#4)
	cmpi #0, d0
	beq .step0
	cmpi #3, d0
	beq .step1
	* Tail for note
	cmpi #6, d0
	beq .step2
	cmpi #9, d0
	beq .step3
	cmpi #12, d0
	beq .step4
	cmpi #15, d0
	beq .step5
	* Setup for second note (F3)
	cmpi #24, d0
	beq .step6
	cmpi #27, d0
	beq .step7
	* Tail for note
	cmpi #30, d0
	beq .step2
	cmpi #33, d0
	beq .step3
	cmpi #36, d0
	beq .step4
	cmpi #39, d0
	beq .step5
	* Setup for third note (C4)
	cmpi #48, d0
	beq .step8
	cmpi #51, d0
	beq .step9
	* Tail for note
	cmpi #54, d0
	beq .step2
	cmpi #57, d0
	beq .step3
	cmpi #60, d0
	beq .step4
	cmpi #63, d0
	beq .step5
	* Setup for fourth (and last) note (A#4)
	cmpi #72, d0
	beq .step0
	cmpi #75, d0
	beq .step1
	* Tail for note
	cmpi #78, d0
	beq .step2
	cmpi #81, d0
	beq .step3
	cmpi #84, d0
	beq .step4
	cmpi #87, d0
	beq .step5
	rts

.step0:
	* In the first step, we set up channel A of the PSG to play A#4 at full volume
	move.w #0x0102, d1
	movep.w d1, 0(a0)
	move.w #0x0018, d1
	movep.w d1, 0(a0)
	move.w #0x080f, d1
	movep.w d1, 0(a0)

	* Turn the channel on in the mixer
	move.w #0x073e, d1
	movep.w d1, 0(a0)
	rts

.step1:
	* In the second step, we set up channel B of the PSG to play A#4 at half volume
	move.w #0x0302, d1
	movep.w d1, 0(a0)
	move.w #0x0218, d1
	movep.w d1, 0(a0)
	move.w #0x0907, d1
	movep.w d1, 0(a0)

	* Turn the channel on in the mixer
	move.w #0x073c, d1
	movep.w d1, 0(a0)

	* And decrease the volume of A a step
	move.w #0x080e, d1
	movep.w d1, 0(a0)
	rts

.step2:
	* Third, fourth, fifth and sixth steps see the volumes of both active channels reduced
	move.w #0x080d, d1
	movep.w d1, 0(a0)
	move.w #0x0906, d1
	movep.w d1, 0(a0)
	rts

.step3:
	move.w #0x080c, d1
	movep.w d1, 0(a0)
	move.w #0x0905, d1
	movep.w d1, 0(a0)
	rts

.step4:
	move.w #0x0800, d1
	movep.w d1, 0(a0)
	move.w #0x0904, d1
	movep.w d1, 0(a0)
	rts

.step5:
	move.w #0x0900, d1
	movep.w d1, 0(a0)
	rts

.step6:
	* In this seventh step, channel A plays F3 at full volume
	move.w #0x00cc, d1
	movep.w d1, 0(a0)
	move.w #0x080f, d1
	movep.w d1, 0(a0)
	rts

.step7:
	* Eighth step sees channel B play F3 at half volume, and channel A reduce volume again
	move.w #0x02cc, d1
	movep.w d1, 0(a0)
	move.w #0x0907, d1
	movep.w d1, 0(a0)

	move.w #0x080e, d1
	movep.w d1, 0(a0)
	rts

.step8:
	* step2-5 are repeated before this, leading to us playing C4 on channel A at full volume
	move.w #0x0101, d1
	movep.w d1, 0(a0)
	move.w #0x00de, d1
	movep.w d1, 0(a0)
	move.w #0x080f, d1
	movep.w d1, 0(a0)
	rts

.step9:
	* Play C4 on channel B at half volume
	move.w #0x0301, d1
	movep.w d1, 0(a0)
	move.w #0x02de, d1
	movep.w d1, 0(a0)
	move.w #0x0907, d1
	movep.w d1, 0(a0)

	* And decrease the volume of A a step
	move.w #0x080e, d1
	movep.w d1, 0(a0)
	rts
