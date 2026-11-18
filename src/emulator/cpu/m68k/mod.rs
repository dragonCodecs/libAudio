// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

mod instruction;
mod irq;

use std::cell::RefCell;
use std::rc::Rc;

use bitmask_enum::bitmask;
use instruction::{DecodedOperation, Instruction};
pub use irq::IrqRequester;

use crate::emulator::{MemoryMap, PeripheralValue};

pub struct Motorola68k<'a>
{
	peripherals: MemoryMap<'a, u32>,
	interrupt_requesters: [Option<Rc<RefCell<dyn IrqRequester + 'a>>>; 7],
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
	Trap,
	Invalid,
}

trait Cast<T>: Sized
{
	fn cast_into(self) -> T;
	fn cast_from(value: T) -> Self;
}

trait UnderlyingStorageFor: Sized
{
	type Width8: Default + PeripheralValue + Into<Self>;
	type Width16: Default + PeripheralValue + Into<Self>;
	type Width32: Default + PeripheralValue + Into<Self>;
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

	#[cfg(test)]
	pub(self) fn execute_from_exception(&mut self, entry_address: u32, stack_top: u32, vector_number: u8)
	{
		// Copy in the new program counter value
		self.program_counter = entry_address;
		// Force supervisor mode
		self.status |= M68kStatusBits::Supervisor;
		// Start stacking up a throwaway exception frame at the new stack top
		let mut stack_pointer = stack_top - 2;
		// Stack the start of a throwaway frame for the selected vector number
		self.peripherals
			.write_address(stack_pointer, 0x1000 | ((vector_number as u16) << 2));
		// Set up a sentinel value on the stack for RTE to hit to tell us we're done
		stack_pointer -= 4;
		self.peripherals.write_address(stack_pointer, u32::MAX);
		// Set up a dummy status register to complete the exception frame
		stack_pointer -= 2;
		self.peripherals.write_address::<u16>(stack_pointer, 0x0000);
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

	pub fn register_interrupt_requester(&mut self, requester: Rc<RefCell<dyn IrqRequester + 'a>>)
	{
		// Extract what IRQ level this is for
		let level = requester.borrow().irq_level();
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
				let cause = requester.borrow_mut().irq_cause();
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
		let insn: DecodedOperation = self.peripherals.read_address::<u16>(self.program_counter).into();
		self.program_counter += 2;
		// Now we have an instruction to run, try to dispatch it
		match insn.operation {
			Instruction::Bra => return self.dispatch_bra(&insn),
			Instruction::Bsr => return self.dispatch_bsr(&insn),
			Instruction::Jmp => return self.dispatch_jmp(&insn),
			Instruction::Nop => return StepResult::Valid,
			Instruction::Rte => return self.dispatch_rte(),
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

	pub fn display_regs(&self)
	{
		// Start by displaying the d-regs
		for idx in 0..8 {
			if (idx & 3) == 0 {
				print!(" [DBG] ");
			}
			print!("  d{idx}: {:08x}", self.d[idx]);

			if (idx & 3) == 3 {
				println!()
			} else {
				print!(" ");
			}
		}

		// Now the a-regs, including the active the stack pointer
		for idx in 0..8 {
			if (idx & 3) == 0 {
				print!(" [DBG] ");
			}
			if idx != 7 {
				print!("  a{idx}: {:08x}", self.d[idx]);
			} else {
				print!(
					"  a7: {:08x}",
					if self.status.contains(M68kStatusBits::Supervisor) {
						self.system_stack_pointer
					} else {
						self.user_stack_pointer
					}
				);
			}

			if (idx & 3) == 3 {
				println!()
			} else {
				print!(" ");
			}
		}

		// Now display the stack pointers, program counter, and finally the status register
		println!(
			" [DBG]  ssp: {:08x}  usp: {:08x}   pc: {:08x}   sr: {:04x}",
			self.system_stack_pointer, self.user_stack_pointer, self.program_counter, self.status.bits
		);

		// Display the current instruction pointed to by the program counter
		let raw_instruction = self.peripherals.read_address::<u16>(self.program_counter);
		let instruction: DecodedOperation = raw_instruction.into();
		println!(" [DBG] insn: {raw_instruction:04x}");
	}

	fn read_index(&mut self, extension: u16) -> i32
	{
		// Get the index value that will be used
		let index = {
			let needs_extending = (extension & 0x0800) == 0;
			// Extract out the register value that will be used to construct this index
			let value = {
				// Determine data or address, then extract that register's value
				if (extension & 0x8000) != 0 {
					self.a[((extension & 0x7000) >> 12) as usize]
				} else {
					self.d[((extension & 0x7000) >> 12) as usize]
				}
			};
			// Fix up anything that we need to fix up
			if needs_extending {
				value as i16 as i32
			} else {
				value as i32
			}
		};
		// Extract what scaling it needs, and dispatch
		match extension & 0x0600 {
			// * 1
			0x0000 => index,
			// * 2
			0x0200 => index * 2,
			// * 4
			0x0400 => index * 4,
			// * 8
			0x0600 => index * 8,
			_ => unreachable!(),
		}
	}

	fn read_extra_displacement(&mut self, displacement_size: u16) -> i32
	{
		match displacement_size {
			// s16 displacement follows
			2 => {
				let result = self.peripherals.read_address::<i16>(self.program_counter);
				self.program_counter += 2;
				result as i32
			},
			// s32 displacement follows
			3 => {
				let result = self.peripherals.read_address::<i32>(self.program_counter);
				self.program_counter += 4;
				result
			},
			// No displacement follows (or we got the reserved size)
			_ => 0,
		}
	}

	fn compute_indrect(&mut self, base_address: u32) -> u32
	{
		let extra = self.peripherals.read_address::<u16>(self.program_counter);
		self.program_counter += 2;
		// Determine which extension mode we're in
		if (extra & 0x0100) != 0 {
			// Figure out what do for the base displacement
			let base_displacement = {
				// If it's suppressed, make it 0
				if (extra & 0x0080) != 0 {
					0
				} else {
					// Otherwise, determine how many bytes follow and read them
					self.read_extra_displacement((extra & 0x0030) >> 4)
				}
			};
			// Now the index, which can also be suppressed
			let index = if (extra & 0x0040) == 0 {
				self.read_index(extra)
			} else {
				0
			};
			// Extract the indirection action selection
			let action = extra & 0x0007;
			let outer_displacement = {
				// If it is 4, for any reason, that is reserved and should be ignored
				if action == 4 {
					0
				}
				// If the index is being suppressed, it changes what happens next
				else if (extra & 0x0040) != 0 {
					// If the action is above 4, it is reserved in this mode
					if action > 4 {
						0
					} else {
						self.read_extra_displacement(action)
					}
				}
				// All combinations are allowed, get the displacement back
				else {
					self.read_extra_displacement(action & 0x0003)
				}
			};

			// If indexing is being suppressed
			if (extra & 0x0040) != 0 {
				// Determine what outer displacement needs applying (treat reserved modes as no memory indirection)
				// Memory Indirect with Outer Displacement
				if let 1..=3 = action {
					// Start by pulling back the indirect memory address to use
					let indirect_address = self
						.peripherals
						.read_address::<u32>(base_address.wrapping_add_signed(base_displacement));
					// Now compute the final address
					return indirect_address.wrapping_add_signed(outer_displacement);
				}
			} else {
				// Determine what outer displacement needs applying (treat reserved modes as no memory indirection)
				match action {
					// Indirect Pre-Indexed with Outer Displacement
					1..=3 => {
						// Start by pulling back the indexed indirect memory address to use
						let indirect_address = self.peripherals.read_address::<u32>(
							base_address
								.wrapping_add_signed(base_displacement)
								.wrapping_add_signed(index),
						);
						// Now build the final address
						return indirect_address.wrapping_add_signed(outer_displacement);
					},
					// Indirect Post-Indexed with Outer Displacement
					5..=7 => {
						// Start by pulling back the indirect memory address to use
						let indirect_address = self
							.peripherals
							.read_address::<u32>(base_address.wrapping_add_signed(base_displacement));
						// Now build the indexed final address
						return indirect_address
							.wrapping_add_signed(index)
							.wrapping_add_signed(outer_displacement);
					},
					_ => {},
				}
			}

			base_address
				.wrapping_add_signed(base_displacement)
				.wrapping_add_signed(index)
		} else {
			let index = self.read_index(extra);
			let displacement = extra as i8;
			base_address
				.wrapping_add_signed(displacement as i32)
				.wrapping_add_signed(index)
		}
	}

	fn compute_effective_address(&mut self, mode: u8, reg: u8, operand_size: usize) -> u32
	{
		match mode {
			// (An)
			2 => *self.addr_register(reg as usize),
			// (An)+
			3 => {
				let ptr = *self.addr_register(reg as usize);
				// Special-case for the stack pointer to maintain alignment
				if reg == 7 && operand_size == 1 {
					*self.addr_register_mut(reg as usize) = ptr + 2;
				} else {
					*self.addr_register_mut(reg as usize) = ptr + operand_size as u32;
				}
				ptr
			},
			// -(An)
			4 => {
				let ptr = *self.addr_register(reg as usize) - {
					// Special-case for the stack pointer to maintain alignment
					if reg == 7 && operand_size == 1 {
						2
					} else {
						operand_size as u32
					}
				};
				*self.addr_register_mut(reg as usize) = ptr;
				ptr
			},
			// (d16,An)
			5 => {
				let displacement = self.peripherals.read_address::<i16>(self.program_counter);
				self.program_counter += 2;
				self.addr_register(reg as usize)
					.wrapping_add_signed(displacement as i32)
			},
			// (d8,An,Xn.SIZE*SCALE), (bd,An,Xn.SIZE*SCALE),
			// ([bd,An],Xn.SIZE*SCALE,od), and ([bd,An,Xn.SIZE*SCALE].od)
			6 => self.compute_indrect(*self.addr_register(reg as usize)),
			// PC-rel and absolute modes
			7 => match reg {
				// (xxx).W
				0 => {
					let ptr = self.peripherals.read_address::<i16>(self.program_counter);
					self.program_counter += 2;
					ptr as i32 as u32
				},
				// (xxx).L
				1 => {
					let ptr = self.peripherals.read_address::<i32>(self.program_counter);
					self.program_counter += 4;
					ptr as u32
				},
				// (d16,PC)
				2 => {
					let ptr = self
						.program_counter
						.wrapping_add_signed(self.peripherals.read_address::<i16>(self.program_counter) as i32);
					self.program_counter += 2;
					ptr
				},
				// (d8,PC,Xn.SIZE*SCALE), (bd,PC,Xn.SIZE*SCALE),
				// ([bd,PC],Xn.SIZE*SCALE,od), and ([bd,PC,Xn.SIZE*SCALE],od)
				3 => self.compute_indrect(self.program_counter),
				// #<xxx>
				4 => {
					// u8 and u16 operation is trivial enough
					if operand_size == 1 || operand_size == 2 {
						// Read the u16 that follows the instruction
						let value = self.peripherals.read_address::<u16>(self.program_counter);
						self.program_counter += 2;
						// Truncate if only a byte is needed
						if operand_size == 1 {
							value as u8 as u32
						} else {
							value as u32
						}
					}
					// u32 is even easier, just read the u32 and return it
					else if operand_size == 4 {
						let value = self.peripherals.read_address::<u32>(self.program_counter);
						self.program_counter += 4;
						value
					} else {
						// TODO: Deal with float modes
						todo!()
					}
				},
				_ => unreachable!(),
			},
			_ => unreachable!(),
		}
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

	fn dispatch_bsr(&mut self, insn: &DecodedOperation) -> StepResult
	{
		// Extract the displacement for this instruction
		let displacement = self.read_immediate_displacement(insn);
		// Now we have the displacement, calculate the post-instruction program counter, and push it to stack
		let stack_pointer = *self.active_stack_pointer() - 4;
		self.peripherals
			.write_address(stack_pointer, self.program_counter + insn.trailing_bytes as u32);
		*self.active_stack_pointer_mut() = stack_pointer;
		// Now update the program counter to the new execution address and get done
		self.program_counter = self.program_counter.wrapping_add_signed(displacement);
		StepResult::Valid
	}

	fn dispatch_jmp(&mut self, insn: &DecodedOperation) -> StepResult
	{
		// Extract the address to jump to from the effective address for the instruction
		// and then load that into the program counter, competing the jump
		self.program_counter = self.compute_effective_address(insn.mode, insn.ry, 4);
		// Say that this instruction succeeded and was valid
		StepResult::Valid
	}

	fn dispatch_rte(&mut self) -> StepResult
	{
		// Grab the current stack pointer
		let mut stack_pointer = *self.active_stack_pointer();
		// Unstack the status register and adjust the stack pointer
		self.status.bits = self.peripherals.read_address(stack_pointer);
		stack_pointer += 2;
		// Unstack the program counter and adjust the stack pointer
		self.program_counter = self.peripherals.read_address(stack_pointer);
		stack_pointer += 4;
		// Unstack the frame format and offset
		let format = self.peripherals.read_address::<u16>(stack_pointer);
		stack_pointer += 2;
		*self.active_stack_pointer_mut() = stack_pointer;
		let frame_type = format >> 12;
		// For now, explode if this is not a format 0 or 1 frame
		if frame_type != 0 && frame_type != 1 {
			StepResult::Invalid
		} else {
			StepResult::Valid
		}
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

impl Cast<i8> for u32
{
	fn cast_into(self) -> i8
	{
		self as i8
	}

	fn cast_from(value: i8) -> Self
	{
		value as u32
	}
}

impl Cast<i16> for u32
{
	fn cast_into(self) -> i16
	{
		self as i16
	}

	fn cast_from(value: i16) -> Self
	{
		value as u32
	}
}

impl Cast<i32> for u32
{
	fn cast_into(self) -> i32
	{
		self as i32
	}

	fn cast_from(value: i32) -> Self
	{
		value as u32
	}
}

impl Cast<u8> for u32
{
	fn cast_into(self) -> u8
	{
		self as u8
	}

	fn cast_from(value: u8) -> Self
	{
		value as u32
	}
}

impl Cast<u16> for u32
{
	fn cast_into(self) -> u16
	{
		self as u16
	}

	fn cast_from(value: u16) -> Self
	{
		value as u32
	}
}

impl Cast<u32> for u32
{
	fn cast_into(self) -> u32
	{
		self
	}

	fn cast_from(value: u32) -> Self
	{
		value
	}
}

impl UnderlyingStorageFor for i32
{
	type Width16 = i16;
	type Width32 = i32;
	type Width8 = i8;
}

impl UnderlyingStorageFor for u32
{
	type Width16 = u16;
	type Width32 = u32;
	type Width8 = u8;
}

#[cfg(test)]
pub(crate) mod tests
{
	include!("tests/cpu.rs");
}
