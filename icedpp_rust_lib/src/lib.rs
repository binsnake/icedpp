use iced_x86::{Decoder, DecoderOptions, Formatter, Instruction, MemorySize, Mnemonic, NasmFormatter, OpKind, Register, SpecializedFormatter, SpecializedFormatterTraitOptions};
use std::{ptr, slice};
use std::os::raw::c_char;
use memoffset::offset_of;

pub const PREFIX_NONE: u8 = 0;
pub const PREFIX_REP: u8 = 1;
pub const PREFIX_REPNE: u8 = 2;
pub const PREFIX_LOCK: u8 = 3;

#[derive(Debug, Clone, Copy)]
enum OperandType {
  Invalid,
  Register8,
  Register16,
  Register32,
  Register64,
  Register128,
  Register256,
  Register512,
  Memory8,
  Memory16,
  Memory32,
  Memory64,
  Memory128,
  Memory256,
  Memory512,
  Immediate8,
  Immediate8_2nd,
  Immediate16,
  Immediate32,
  Immediate64,
  NearBranch,
  FarBranch,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct MergenDisassembledInstructionBase {
  pub mnemonic: u16,
  pub mem_base: u8,
  pub mem_index: u8,
  pub mem_scale: u8,
  pub stack_growth: u8,
  pub regs: [u8; 4],
  pub types: [u8; 4],
  pub attributes: u8,
  pub length: u8,
  pub operand_count_visible: u8,
  pub immediate: u64,
  pub mem_disp: u64,
  pub text: [u8; 64],
}

const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, mnemonic) == 0, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, mem_base) == 2, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, mem_index) == 3, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, mem_scale) == 4, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, stack_growth) == 5, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, regs) == 6, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, types) == 10, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, attributes) == 14, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, length) == 15, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, operand_count_visible) == 16, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, immediate) == 24, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, mem_disp) == 32, "invalid offset");
const _: () = assert!(offset_of!(MergenDisassembledInstructionBase, text) == 40, "invalid offset");

#[inline]
fn has_64bit_immediate(instr: &Instruction) -> bool {
  (0..instr.op_count()).any(|i| instr.op_kind(i) == OpKind::Immediate64)
}

#[inline]
fn is_relative_jump(instr: &Instruction) -> bool {
  instr.memory_base() == Register::RIP ||
      (0..instr.op_count()).any(|i| matches!(instr.op_kind(i), OpKind::NearBranch16 | OpKind::NearBranch32 | OpKind::NearBranch64))
}

#[inline]
fn convert_type_to_mergen(instr: &Instruction, index: u32) -> OperandType {
  match instr.op_kind(index) {
    OpKind::Register => match instr.op_register(index).size() {
      1 => OperandType::Register8,
      2 => OperandType::Register16,
      4 => OperandType::Register32,
      8 => OperandType::Register64,
      16 => OperandType::Register128,
      32 => OperandType::Register256,
      64 => OperandType::Register512,
      _ => OperandType::Invalid,
    },
    OpKind::Immediate8 | OpKind::Immediate8to16 | OpKind::Immediate8to32 | OpKind::Immediate8to64 => OperandType::Immediate8,
    OpKind::Immediate8_2nd => OperandType::Immediate8_2nd,
    OpKind::Immediate16 => OperandType::Immediate16,
    OpKind::Immediate32 | OpKind::Immediate32to64 => OperandType::Immediate32,
    OpKind::Immediate64 => OperandType::Immediate64,
    OpKind::NearBranch16 | OpKind::NearBranch32 | OpKind::NearBranch64 => OperandType::NearBranch,
    OpKind::FarBranch16 | OpKind::FarBranch32 => OperandType::FarBranch,
    OpKind::Memory => match instr.memory_size().size() {
      1 => OperandType::Memory8,
      2 => OperandType::Memory16,
      4 => OperandType::Memory32,
      8 => OperandType::Memory64,
      16 => OperandType::Memory128,
      32 => OperandType::Memory256,
      64 => OperandType::Memory512,
      _ => OperandType::Invalid,
    },
    _ => OperandType::Invalid,
  }
}

#[inline]
fn decode_instruction(code_ptr: *const u8, len: usize) -> Option<(Instruction, &'static [u8])> {
  if code_ptr.is_null() || len == 0 {
    return None;
  }
  let code = unsafe { slice::from_raw_parts(code_ptr, len) };
  let mut decoder = Decoder::new(64, code, DecoderOptions::NO_INVALID_CHECK);
  Some((decoder.decode(), code))
}

#[inline]
fn set_attributes(instr: &Instruction) -> u8 {
  let mut attrs = PREFIX_NONE;
  if instr.has_rep_prefix() { attrs |= PREFIX_REP; }
  if instr.has_repne_prefix() { attrs |= PREFIX_REPNE; }
  if instr.has_lock_prefix() { attrs |= PREFIX_LOCK; }
  attrs
}

#[no_mangle]
pub extern "C" fn disas(
  out: *mut MergenDisassembledInstructionBase,
  code_ptr: *const u8,
  len: usize,
) -> i32 {
  let Some((instr, _)) = decode_instruction(code_ptr, len) else { return -1; };
  if out.is_null() { return -1; }

  let disp64 = instr.memory_displacement64();
  let instr_len = instr.len() as u64;
  let is_rel = is_relative_jump(&instr);
  let immediate = if is_rel {
    disp64.wrapping_sub(instr_len)
  } else if has_64bit_immediate(&instr) {
    instr.immediate64()
  } else {
    instr.immediate32() as u64
  };

  let out_value = MergenDisassembledInstructionBase {
    mnemonic: instr.mnemonic() as u16,
    mem_base: instr.memory_base() as u8,
    mem_index: instr.memory_index() as u8,
    mem_scale: instr.memory_index_scale() as u8,
    mem_disp: if is_rel { disp64.wrapping_sub(instr_len) } else { disp64 },
    stack_growth: instr.stack_pointer_increment().unsigned_abs() as u8,
    immediate,
    regs: [
      instr.op0_register() as u8,
      instr.op1_register() as u8,
      instr.op2_register() as u8,
      instr.op3_register() as u8,
    ],
    types: [
      convert_type_to_mergen(&instr, 0) as u8,
      convert_type_to_mergen(&instr, 1) as u8,
      convert_type_to_mergen(&instr, 2) as u8,
      convert_type_to_mergen(&instr, 3) as u8,
    ],
    operand_count_visible: instr.op_count() as u8,
    attributes: set_attributes(&instr),
    length: instr.len() as u8,
    text: [0u8; 64],
  };
  unsafe { ptr::write(out, out_value) };
  0
}

#[no_mangle]
pub extern "C" fn disas2(
  out: *mut MergenDisassembledInstructionBase,
  code_ptr: *const u8,
  len: usize,
) -> i32 {
  if out.is_null() || code_ptr.is_null() || len == 0 {
    return -1;
  }
  let code = unsafe { slice::from_raw_parts(code_ptr, len) };
  let mut decoder = Decoder::new(64, code, DecoderOptions::NONE);
  let instr = decoder.decode();
  struct MyTraitOptions;
  impl SpecializedFormatterTraitOptions for MyTraitOptions {
    const ENABLE_DB_DW_DD_DQ: bool = false;
    unsafe fn verify_output_has_enough_bytes_left() -> bool {
      false
    }
  }
  type MyFormatter = SpecializedFormatter<MyTraitOptions>;
  let mut formatter = MyFormatter::new();
  let mut formatted = String::new();
  formatter.format(&instr, &mut formatted);
  let bytes = formatted.as_bytes();
  let mut text = [0u8; 64];
  let copy_len = bytes.len().min(63); // Reserve 1 byte for null terminator
  text[..copy_len].copy_from_slice(&bytes[..copy_len]);
  text[copy_len] = 0; // Null-terminate
  let disp64 = instr.memory_displacement64();
  let instr_len = instr.len() as u64;
  let is_rel = is_relative_jump(&instr);
  let has_imm64 = has_64bit_immediate(&instr);
  let immediate = if is_rel {
    disp64.wrapping_sub(instr_len)
  } else if has_imm64 {
    instr.immediate64() as u64
  } else {
    instr.immediate32() as u64
  };
  let mut attrs = 0u8;
  if instr.has_rep_prefix() {
    attrs |= 0b001;
  }
  if instr.has_repne_prefix() {
    attrs |= 0b010;
  }
  if instr.has_lock_prefix() {
    attrs |= 0b100;
  }
  let out_value = MergenDisassembledInstructionBase {
    mnemonic: instr.mnemonic() as u16,
    mem_base: instr.memory_base() as u8,
    mem_index: instr.memory_index() as u8,
    mem_scale: instr.memory_index_scale() as u8,
    mem_disp: if is_rel { disp64.wrapping_sub(instr_len) } else { disp64 },
    stack_growth: instr.stack_pointer_increment().unsigned_abs() as u8,
    immediate,
    regs: [
      instr.op0_register() as u8,
      instr.op1_register() as u8,
      instr.op2_register() as u8,
      instr.op3_register() as u8,
    ],
    types: [
      convert_type_to_mergen(&instr, 0) as u8,
      convert_type_to_mergen(&instr, 1) as u8,
      convert_type_to_mergen(&instr, 2) as u8,
      convert_type_to_mergen(&instr, 3) as u8,
    ],
    operand_count_visible: instr.op_count() as u8,
    attributes: attrs,
    length: instr.len() as u8,
    text,
  };
  unsafe { ptr::write(out, out_value) };
  0
}