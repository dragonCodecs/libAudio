// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

use std::cell::RefCell;
use std::ffi::c_void;
use std::ptr::null_mut;
use std::rc::Rc;

use super::MC68901;
use crate::emulator::cpu::m68k::IrqRequester;
use crate::emulator::cpu::m68k::tests::Motorola68k;
use crate::emulator::{ClockedPeripheral, MemoryMap, Peripheral};

fn write_register_u8(periph: &mut impl Peripheral<u32>, reg: u32, value: u8)
{
	periph.write_address(reg, &[value]);
}

fn read_register_u8(periph: &impl Peripheral<u32>, reg: u32) -> u8
{
	let mut value = [0; 1];
	periph.read_address(reg, &mut value);
	value[0]
}

fn write_register_u16(periph: &mut impl Peripheral<u32>, reg: u32, value: u16)
{
	periph.write_address(reg, &value.to_be_bytes());
}

fn read_register_u16(periph: &impl Peripheral<u32>, reg: u32) -> u16
{
	let mut value = [0; 2];
	periph.read_address(reg, &mut value);
	u16::from_be_bytes(value)
}

fn write_register_u32(periph: &mut impl Peripheral<u32>, reg: u32, value: u32)
{
	periph.write_address(reg, &value.to_be_bytes());
}

fn read_register_u32(periph: &impl Peripheral<u32>, reg: u32) -> u32
{
	let mut value = [0; 4];
	periph.read_address(reg, &mut value);
	u32::from_be_bytes(value)
}

#[test]
fn register_io()
{
	let mut mfp = MC68901::new(2_457_600, null_mut(), 6);

	// Verify the function of the GPIO registers
	write_register_u16(&mut mfp, 0x00, 0xa55a);
	assert_eq!(read_register_u16(&mfp, 0x00), 0x005a);
	assert_eq!(read_register_u8(&mfp, 0x01), 0x5a);
	write_register_u8(&mut mfp, 0x01, 0xff);
	assert_eq!(read_register_u8(&mfp, 0x01), 0xff);
	write_register_u16(&mut mfp, 0x02, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x02), 0x00ff);
	write_register_u16(&mut mfp, 0x04, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x04), 0x00ff);
	// Verify the function of the interrupt control registers
	write_register_u16(&mut mfp, 0x06, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x06), 0x00ff);
	write_register_u16(&mut mfp, 0x08, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x08), 0x00ff);
	write_register_u16(&mut mfp, 0x0a, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x0a), 0x00ff);
	write_register_u16(&mut mfp, 0x0c, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x0c), 0x00ff);
	write_register_u16(&mut mfp, 0x0e, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x0e), 0x00ff);
	write_register_u16(&mut mfp, 0x10, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x10), 0x00ff);
	write_register_u16(&mut mfp, 0x12, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x12), 0x00ff);
	write_register_u16(&mut mfp, 0x14, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x14), 0x00ff);
	// Verify the function of the vector register
	write_register_u16(&mut mfp, 0x16, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x16), 0x00f8);
	// Verify the function of the timer registers
	write_register_u16(&mut mfp, 0x18, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x18), 0x001f);
	write_register_u16(&mut mfp, 0x1e, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x1e), 0x0000);
	write_register_u8(&mut mfp, 0x19, 0x00);
	write_register_u16(&mut mfp, 0x1a, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x1a), 0x001f);
	write_register_u16(&mut mfp, 0x20, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x20), 0x0000);
	write_register_u8(&mut mfp, 0x1b, 0x00);
	write_register_u16(&mut mfp, 0x1c, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x1c), 0x0077);
	write_register_u16(&mut mfp, 0x22, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x22), 0x0000);
	write_register_u16(&mut mfp, 0x24, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x24), 0x0000);
	write_register_u8(&mut mfp, 0x1d, 0x00);
	write_register_u16(&mut mfp, 0x1e, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x1e), 0x00ff);
	write_register_u16(&mut mfp, 0x20, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x20), 0x00ff);
	write_register_u16(&mut mfp, 0x22, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x22), 0x00ff);
	write_register_u16(&mut mfp, 0x24, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x24), 0x00ff);
	// Verify the function of the UART registers
	write_register_u16(&mut mfp, 0x26, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x26), 0x00ff);
	write_register_u16(&mut mfp, 0x28, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x28), 0x00ff);
	write_register_u16(&mut mfp, 0x2a, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x2a), 0x00ff);
	write_register_u16(&mut mfp, 0x2c, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x2c), 0x00ff);
	write_register_u16(&mut mfp, 0x2e, 0xffff);
	assert_eq!(read_register_u16(&mfp, 0x2e), 0x00ff);
}

#[test]
fn bad_begister_io()
{
	let mut mfp = MC68901::new(2_457_600, null_mut(), 6);

	// Should be discarded as writing to an even address for an odd byte register
	write_register_u8(&mut mfp, 0x00, 0xff);
	assert_eq!(read_register_u8(&mfp, 0x00), 0x00);
	assert_eq!(read_register_u16(&mfp, 0x00), 0x0000);
	// Should be discarded as writing 32 bits in a go is inadmissable
	write_register_u32(&mut mfp, 0x00, u32::MAX);
	assert_eq!(read_register_u16(&mfp, 0x00), 0x0000);
	assert_eq!(read_register_u16(&mfp, 0x02), 0x0000);
	// Should be a no-op as reading 32 bits in a go is inadmissable
	assert_eq!(read_register_u32(&mfp, 0x02), 0x00000000);
}

#[test]
fn configure_timer()
{
	let mut mfp = MC68901::new(2_457_600, null_mut(), 6);

	// Check that we can configure timer A for ~50Hz operation
	mfp.configure_timer(0, 245, 7);
	assert_eq!(read_register_u8(&mfp, 0x19), 0x07);
	assert_eq!(read_register_u8(&mfp, 0x1f), 245);

	// Check that we can configure timer C for 200Hz operation
	mfp.configure_timer(2, 192, 5);
	assert_eq!(read_register_u8(&mfp, 0x1d), 0x50);
	assert_eq!(read_register_u8(&mfp, 0x23), 192);

	// This call will be a no-op
	mfp.configure_timer(4, 192, 5);
}

#[test]
fn irq_generation()
{
	let memory_map = Rc::new(RefCell::new(MemoryMap::new(0x00ffffff)));
	let mut cpu = Box::new(Motorola68k::new(memory_map, 8_000_000));
	let mut mfp = MC68901::new(2_457_600, Box::as_mut_ptr(&mut cpu) as *mut c_void, 6);

	// Reconfigure the interrupt registers so just timers A and C can generate events
	// Enable just timer A and C IRQs
	write_register_u8(&mut mfp, 0x07, 0x20);
	write_register_u8(&mut mfp, 0x09, 0x20);
	// Reset the pending and in-service status for both
	write_register_u8(&mut mfp, 0x0b, 0x00);
	write_register_u8(&mut mfp, 0x0d, 0x00);
	write_register_u8(&mut mfp, 0x0f, 0x00);
	write_register_u8(&mut mfp, 0x11, 0x00);
	// Make sure IRQs will be delivered at the correct vector base address
	write_register_u8(&mut mfp, 0x17, 0x40);
	// Make sure the mask registers allow IRQ generation so we can
	// mark pending interrupts for the timers
	write_register_u8(&mut mfp, 0x13, 0x20);
	write_register_u8(&mut mfp, 0x15, 0x20);
	assert!(!cpu.has_pending_interrupts());

	// Configure and enable timer A for ~50Hz operation
	mfp.configure_timer(0, 245, 7);
	// Configure and enable timer C for 200Hz operation
	mfp.configure_timer(2, 192, 5);
	// Configure and enable timer B but not its IRQs generation
	mfp.configure_timer(1, 2, 1);

	// Cycle the clock until timer C fires and we generate an interrupt
	for _ in 0..12288 {
		// Make sure no interrupts are set yet
		assert_eq!(read_register_u8(&mfp, 0x0b), 0x00);
		assert_eq!(read_register_u8(&mfp, 0x0d), 0x00);
		// Now wiggle the clock
		assert!(mfp.clock_cycle());
	}
	// Now verify that it's TC's IRQ that's actually pending
	assert_eq!(read_register_u8(&mfp, 0x0b), 0x00);
	assert_eq!(read_register_u8(&mfp, 0x0d), 0x20);
	assert!(cpu.has_pending_interrupts());
	// Assert that the TC interrupt creates the proper cause number
	assert_eq!(mfp.irq_cause(), 0x45);
	// And that that reset the pending bit
	assert_eq!(read_register_u8(&mfp, 0x0d), 0x00);

	// Reconfigure timer A to be sensitive to external events
	write_register_u8(&mut mfp, 0x19, 0x00);
	mfp.configure_timer(0, 1, 8);

	// Generate some more clock cycles so we're 50% of the way to timer C firing again
	for _ in 0..6144 {
		assert_eq!(read_register_u8(&mfp, 0x0b), 0x00);
		assert_eq!(read_register_u8(&mfp, 0x0d), 0x00);
		assert!(mfp.clock_cycle());
	}
	// Mark timer A with an external event
	mfp.fire_dma_event();
	assert!(mfp.clock_cycle());
	// Generate some more clock cycles so we're so timer C fires again
	for _ in 0..6143 {
		assert_eq!(read_register_u8(&mfp, 0x0b), 0x20);
		assert_eq!(read_register_u8(&mfp, 0x0d), 0x00);
		assert!(mfp.clock_cycle());
	}

	// Check we now have two pending interrupts
	assert_eq!(read_register_u8(&mfp, 0x0b), 0x20);
	assert_eq!(read_register_u8(&mfp, 0x0d), 0x20);
	// Verify that A's pops before C's
	assert_eq!(mfp.irq_cause(), 0x4d);
	assert_eq!(mfp.irq_cause(), 0x45);
}

#[test]
fn gpio7_events()
{
	let memory_map = Rc::new(RefCell::new(MemoryMap::new(0x00ffffff)));
	let mut cpu = Box::new(Motorola68k::new(memory_map, 8_000_000));
	let mut mfp = MC68901::new(2_457_600, Box::as_mut_ptr(&mut cpu) as *mut c_void, 6);

	// Enable IRQs for just GPIO7
	write_register_u8(&mut mfp, 0x07, 0x80);
	write_register_u8(&mut mfp, 0x09, 0x00);
	// Reset the pending and in-service status for both
	write_register_u8(&mut mfp, 0x0b, 0x00);
	write_register_u8(&mut mfp, 0x0d, 0x00);
	write_register_u8(&mut mfp, 0x0f, 0x00);
	write_register_u8(&mut mfp, 0x11, 0x00);
	// Make sure IRQs will be delivered at the correct vector base address
	write_register_u8(&mut mfp, 0x17, 0x40);
	// Make sure the mask registers do not allow any IRQ generation just yet
	write_register_u8(&mut mfp, 0x13, 0x00);
	write_register_u8(&mut mfp, 0x15, 0x00);

	// Check that running the peripheral does nothing now
	assert!(mfp.clock_cycle());
	assert_eq!(read_register_u8(&mfp, 0x0b), 0x00);
	assert_eq!(read_register_u8(&mfp, 0x0d), 0x00);

	// Generate an interrupt on GPIO7
	mfp.fire_dma_event();
	assert!(mfp.clock_cycle());
	assert_eq!(read_register_u8(&mfp, 0x0b), 0x80);
	assert_eq!(read_register_u8(&mfp, 0x0d), 0x00);
	// The interrupt is presently masked, so.. irqCause should report a spurious error
	assert_eq!(mfp.irq_cause(), 0x18);
	// Unmask and do that again..
	write_register_u8(&mut mfp, 0x13, 0x80);
	assert_eq!(mfp.irq_cause(), 0x4f);
}

#[test]
fn timer_a_event_counting()
{
	let memory_map = Rc::new(RefCell::new(MemoryMap::new(0x00ffffff)));
	let mut cpu = Box::new(Motorola68k::new(memory_map, 8_000_000));
	let mut mfp = MC68901::new(2_457_600, Box::as_mut_ptr(&mut cpu) as *mut c_void, 6);

	// Enable timer A to set a pending interrupt
	write_register_u8(&mut mfp, 0x07, 0x20);
	mfp.configure_timer(0, 2, 8);

	// Run a sequence to generate two events, checking between each one what the counter is doing
	assert_eq!(read_register_u8(&mfp, 0x1f), 0x02);
	assert!(mfp.clock_cycle());
	assert_eq!(read_register_u8(&mfp, 0x1f), 0x02);
	mfp.fire_dma_event();
	assert_eq!(read_register_u8(&mfp, 0x1f), 0x02);
	assert!(mfp.clock_cycle());
	assert_eq!(read_register_u8(&mfp, 0x1f), 0x01);
	mfp.fire_dma_event();
	// Check there is no pending interrupt as the event's not yet processed
	assert_eq!(read_register_u8(&mfp, 0x0b), 0x00);
	assert!(mfp.clock_cycle());
	// Check the timer reloads and generates a pending IRQ now it is
	assert_eq!(read_register_u8(&mfp, 0x1f), 0x02);
	assert_eq!(read_register_u8(&mfp, 0x0b), 0x20);
}

#[test]
fn prescaling_mapping()
{
	assert_eq!(super::timer_prescaling_for(0), 1);
	assert_eq!(super::timer_prescaling_for(1), 4);
	assert_eq!(super::timer_prescaling_for(2), 10);
	assert_eq!(super::timer_prescaling_for(3), 16);
	assert_eq!(super::timer_prescaling_for(4), 50);
	assert_eq!(super::timer_prescaling_for(5), 64);
	assert_eq!(super::timer_prescaling_for(6), 100);
	assert_eq!(super::timer_prescaling_for(7), 200);
}
