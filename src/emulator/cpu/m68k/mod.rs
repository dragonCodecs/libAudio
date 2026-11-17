// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

mod instruction;
mod irq;

use bitmask_enum::bitmask;
use instruction::{DecodedOperation, Instruction};
pub use irq::IrqRequester;

use crate::emulator::MemoryMap;

pub struct Motorola68k<'a>
{
	peripherals: MemoryMap<'a, u32>,
	interrupt_requesters: [Option<Box<dyn IrqRequester + 'a>>; 7],
	#[allow(unused)]
	clock_frequency: u32,

	// There are 8 data registers, 7 address registers, 3 stack pointers,
	// a program counter and a status register in a m68k
	d: [u32; 8],
	a: [u32; 7],
	system_stack_pointer: u32,
	#[allow(unused)]
	interrupt_stack_pointer: u32,
	user_stack_pointer: u32,
	program_counter: u32,

	status: M68kStatusBits,
	// What IRQs are pending execution (7 levels, masked by the status register IRQ bits)
	pending_irqs: u8,

	trap_state: bool,
}

#[bitmask(u16)]
enum M68kStatusBits
{
	// User byte bit meanings
	Carry = 1 << 0,
	Overflow = 1 << 1,
	Zero = 1 << 2,
	Negative = 1 << 3,
	Extend = 1 << 4,

	// System byte bit meanings
	// 8-10 are interrupt bits
	Supervisor = 1 << 13,
	Trace = 1 << 15,
}

#[derive(Debug, PartialEq, Eq)]
pub enum StepResult
{
	Valid,
	#[allow(unused)]
	Trap,
	Invalid,
}

#[allow(unused)]
impl<'a> Motorola68k<'a>
{
	pub fn new(peripherals: MemoryMap<'a, u32>, clock_frequency: u32) -> Self
	{
		Self {
			peripherals,
			interrupt_requesters: [const { None }; 7],
			clock_frequency,

			d: [0; 8],
			a: [0; 7],
			system_stack_pointer: 0,
			interrupt_stack_pointer: 0,
			user_stack_pointer: 0,
			program_counter: u32::MAX,

			// By default the system starts up in supervisor (system) mode
			status: M68kStatusBits::Supervisor,
			pending_irqs: 0,

			trap_state: false,
		}
	}

	pub fn execute_from(&mut self, entry_address: u32, stack_top: u32, as_user: bool)
	{
		// Copy in the new program counter value, and set up the stack pointer and CPU state
		self.program_counter = entry_address;
		if as_user {
			self.user_stack_pointer = stack_top;
			self.status &= !M68kStatusBits::Supervisor;
		} else {
			self.system_stack_pointer = stack_top;
			self.status |= M68kStatusBits::Supervisor;
		}
		// Set up a sentinel value on the stack for RTS to hit to tell us we're done
		let stack_pointer = *self.active_stack_pointer() - 4;
		self.peripherals.write_address::<u32>(stack_pointer, 0xffffffff);
		*self.active_stack_pointer_mut() = stack_pointer;
	}

	pub fn execute_to_return(&mut self, entry_address: u32, stack_top: u32, as_user: bool) -> bool
	{
		// Set up where we're going to execute from
		self.execute_from(entry_address, stack_top, as_user);
		// Now run till we RTS and hit the sentinel program counter state
		loop {
			// Try and run another instruction
			let result = self.step();
			// Check that something bad didn't happen
			if matches!(result, StepResult::Trap | StepResult::Invalid) {
				return false;
			}
			// Check for the sentinel program counter value
			if self.program_counter == 0xffffffff {
				break;
			}
		}
		true
	}

	pub fn request_interrupt(&mut self, level: u8)
	{
		// Make sure the caller isn't asking for an insane interrupt level, then mark it pending
		if level > 0 && level <= 7 {
			self.pending_irqs |= 1 << level;
		}
	}

	pub fn register_interrupt_requester(&mut self, requester: Box<dyn IrqRequester + 'a>)
	{
		// Extract what IRQ level this is for
		let level = requester.irq_level();
		// Stick it in the requesters array
		self.interrupt_requesters[level as usize] = Some(requester);
	}

	fn check_pending_irqs(&mut self) -> bool
	{
		// Extract the current exception mask from the status register
		let min_irq_priority = ((self.status.bits & 0x0700) >> 8) as u8;
		// Calculate a mask from this number
		let irq_mask = !((1 << (min_irq_priority + 1)) - 1);
		// Now see which (if any) pending IRQ bits are still set (level 7 is always allowed, it is NMI)
		let unmasked_irqs = self.pending_irqs & (irq_mask | 0x80);
		// If there are no bits set, we're done here..
		if unmasked_irqs == 0 {
			return false;
		}

		// Otherwise, figure out which priorty level of IRQ to stack
		for slot in 0..7 - min_irq_priority {
			let level = 7 - slot;
			// Check if this priorty is pending (level is a number from 0 to 7, but we have
			// to step backwards through the priorities)
			let irq_pending = unmasked_irqs & (1 << level);
			if irq_pending == 0 {
				continue;
			}
			// Ok, we've identified the current most high priority pending interrupt, clear it from pending
			self.pending_irqs &= !(1 << level);
			// Dispatch it using any available interrupt requester
			let requester = &mut self.interrupt_requesters[level as usize];
			if let Some(requester) = requester {
				// Identify the IRQ cause and stack the associated vector address
				let cause = requester.irq_cause();
				self.stage_irq_call((cause as u32) << 2);
			} else {
				// Turn this IRQ level into a request for the associated autovector slot and stack that
				self.stage_irq_call((level as u32 + 24) << 2);
			}
			// Mask any further IRQs at this level till the IRQ is handled
			self.mask_irqs(level);
		}

		// We entered an IRQ in this cycle, so step needs to stop here
		true
	}

	pub fn has_pending_interrupts(&self) -> bool
	{
		// Extract the current exception mask from the status register
		let min_irq_priority = ((self.status.bits & 0x0700) >> 8) as u8;
		// Calculate a mask from this number
		let irq_mask = !((1 << (min_irq_priority + 1)) - 1) as u8;
		// Now see which (if any) pending IRQ bits are still set (level 7 is always allowed, it is NMI)
		let unmasked_irqs = self.pending_irqs & (irq_mask | 0x80);
		unmasked_irqs != 0
	}

	fn mask_irqs(&mut self, level: u8)
	{
		// Extract the status register minus the current mask value
		let status_reg = self.status.bits & 0xf8ff;
		// Set the new one masking the new IRQ level
		self.status.bits = status_reg | (((level & 0x07) as u16) << 8);
	}

	fn stage_irq_call(&mut self, vector_address: u32)
	{
		// Grab the old status register value
		let status_reg = self.status.bits;
		// Force supervisor mode
		self.status |= M68kStatusBits::Supervisor;
		// Grab the active stack and start stacking up a throwaway exception frame
		let mut stack_pointer = *self.active_stack_pointer();
		// Stack the start of a throwaway frame for the selected vector number
		stack_pointer -= 2;
		self.peripherals
			.write_address(stack_pointer, (0x1000 | (vector_address & 0x0fff)) as u16);
		// Push the current program counter to the stack and set it to value at the requested vector address
		stack_pointer -= 4;
		self.peripherals.write_address(stack_pointer, self.program_counter);
		self.program_counter = self.peripherals.read_address(vector_address);
		// Stack up the previous status register to be restored on RTE
		stack_pointer -= 2;
		self.peripherals.write_address(stack_pointer, status_reg);
		// Write the stack pointer back now we're done modifying it
		*self.active_stack_pointer_mut() = stack_pointer;
	}

	fn data_register(&self, reg: usize) -> &u32
	{
		&self.d[reg]
	}

	fn data_register_mut(&mut self, reg: usize) -> &mut u32
	{
		&mut self.d[reg]
	}

	fn addr_register(&self, reg: usize) -> &u32
	{
		if reg == 7 {
			self.active_stack_pointer()
		} else {
			&self.a[reg]
		}
	}

	fn addr_register_mut(&mut self, reg: usize) -> &mut u32
	{
		if reg == 7 {
			self.active_stack_pointer_mut()
		} else {
			&mut self.a[reg]
		}
	}

	pub fn read_data_register(&self, reg: usize) -> u32
	{
		*self.data_register(reg)
	}

	pub fn write_data_register(&mut self, reg: usize, value: u32)
	{
		*self.data_register_mut(reg) = value;
	}

	pub fn read_addr_register(&self, reg: usize) -> u32
	{
		*self.addr_register(reg)
	}

	pub fn write_addr_register(&mut self, reg: usize, value: u32)
	{
		*self.addr_register_mut(reg) = value;
	}

	pub fn read_program_counter(&self) -> u32
	{
		self.program_counter
	}

	pub fn read_status(&self) -> u16
	{
		self.status.bits
	}

	pub fn step(&mut self) -> StepResult
	{
		// See if there are any pending interrupt requests
		if self.check_pending_irqs() {
			// There was, so return a step result indicating we "executed" an "instruction"
			return StepResult::Valid;
		}
		// Start by fetching a uint16_t for the instruction and trying to decode it
		let instruction: DecodedOperation = self.peripherals.read_address::<u16>(self.program_counter).into();
		self.program_counter += 2;
		// Now we have an instruction to run, try to dispatch it
		match instruction.operation {
			Instruction::Bra => return self.dispatch_bra(&instruction),
			Instruction::Nop => return StepResult::Valid,
			Instruction::Rts => return self.dispatch_rts(),
			_ => {},
		}

		StepResult::Invalid
	}

	pub fn advance_clock(&mut self) -> bool
	{
		// Let's run an instruction
		let result = self.step();
		// Unpack if the instruction trapped
		self.trap_state = result == StepResult::Trap;
		// If this was not a trap instruction and was not a valid instruction, reset the program counter back
		// an instruction so display works right
		if result == StepResult::Invalid {
			self.program_counter -= 2;
		}
		// Return if the instruction was valid
		result == StepResult::Valid
	}

	pub fn trapped(&self) -> bool
	{
		self.trap_state
	}

	fn active_stack_pointer(&self) -> &u32
	{
		if self.status.contains(M68kStatusBits::Supervisor) {
			&self.system_stack_pointer
		} else {
			&self.user_stack_pointer
		}
	}

	fn active_stack_pointer_mut(&mut self) -> &mut u32
	{
		if self.status.contains(M68kStatusBits::Supervisor) {
			&mut self.system_stack_pointer
		} else {
			&mut self.user_stack_pointer
		}
	}

	fn read_immediate_displacement(&mut self, insn: &DecodedOperation) -> i32
	{
		if insn.trailing_bytes == 0 {
			// If the branch includes the displacement in the instruction itself, extract and sign extend it
			(insn.rx as i8) as i32
		} else if insn.trailing_bytes == 2 {
			// If the branch is for a 16-bit displacement, extract, and sign-extend it
			self.peripherals.read_address::<i16>(self.program_counter) as i32
		} else {
			// Otherwise, extract a 32-bit displacement
			self.peripherals.read_address(self.program_counter)
		}
	}

	fn dispatch_bra(&mut self, insn: &DecodedOperation) -> StepResult
	{
		// Extract the displacement for this instruction
		let displacement = self.read_immediate_displacement(insn);
		// Now we have a displacement, update the program counter and get done
		self.program_counter = self.program_counter.wrapping_add_signed(displacement);
		StepResult::Valid
	}

	fn dispatch_rts(&mut self) -> StepResult
	{
		// Extract the new program counter value from the current stack pointer
		let stack_pointer = *self.active_stack_pointer();
		self.program_counter = self.peripherals.read_address(stack_pointer);
		// Now adjust the stack pointer by the size of an address
		*self.active_stack_pointer_mut() = stack_pointer + 4;
		// Say that this instruction succeeded and was valid
		StepResult::Valid
	}
}

#[cfg(test)]
pub(crate) mod tests
{
	include!("tests/cpu.rs");
}
