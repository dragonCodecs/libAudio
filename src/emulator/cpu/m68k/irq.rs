// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2026 Rachel Mant <git@dragonmux.network>

pub trait IrqRequester
{
	fn request_interrupt(&mut self);
	fn irq_cause(&mut self) -> u8;
	fn irq_level(&self) -> u8;
}
