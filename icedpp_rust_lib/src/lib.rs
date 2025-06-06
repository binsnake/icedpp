use iced_x86::{Decoder, DecoderOptions, Formatter, Instruction, MemorySize, Mnemonic, NasmFormatter, OpKind, Register, SpecializedFormatter, SpecializedFormatterTraitOptions};
use std::{ptr, slice};
use std::os::raw::c_char;
use memoffset::offset_of;

#[repr(u8)]
#[derive(Clone, Copy)]
pub enum MergenPrefix {
  None = 0,
  Rep = 1 << 0,  // 0b0001
  Repne = 1 << 1, // 0b0010
  Lock = 1 << 2,  // 0b0100
}

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

// Compile-time offset assertions
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

#[inline(always)]
fn convert_type_to_mergen(instr: &Instruction, index: u32) -> OperandType {
  match instr.op_kind(index) {
    OpKind::Register => {
      match instr.op_register(index).size() {
        1 => OperandType::Register8,
        2 => OperandType::Register16,
        4 => OperandType::Register32,
        8 => OperandType::Register64,
        16 => OperandType::Register128,
        32 => OperandType::Register256,
        64 => OperandType::Register512,
        _ => OperandType::Register64,
      }
    },

    OpKind::Memory => {
      match instr.memory_size().size() {
        1 => OperandType::Memory8,
        2 => OperandType::Memory16,
        4 => OperandType::Memory32,
        8 => OperandType::Memory64,
        16 => OperandType::Memory128,
        32 => OperandType::Memory256,
        64 => OperandType::Memory512,
        _ => OperandType::Memory64,
      }
    },
    OpKind::MemorySegDI | OpKind::MemorySegSI => OperandType::Memory8,
	OpKind::MemoryESDI => OperandType::Memory16,
    OpKind::MemoryESEDI | OpKind::MemorySegESI | OpKind::MemorySegEDI  => OperandType::Memory32,
    OpKind::MemoryESRDI | OpKind::MemorySegRSI | OpKind::MemorySegRDI => OperandType::Memory64,

    OpKind::Immediate8 | OpKind::Immediate8to16 | OpKind::Immediate8to32 | OpKind::Immediate8to64 =>
      OperandType::Immediate8,
    OpKind::Immediate8_2nd => OperandType::Immediate8_2nd,
    OpKind::Immediate16 => OperandType::Immediate16,
    OpKind::Immediate32 | OpKind::Immediate32to64 => OperandType::Immediate32,
    OpKind::Immediate64 => OperandType::Immediate64,
    OpKind::NearBranch16 | OpKind::NearBranch32 | OpKind::NearBranch64 => OperandType::NearBranch,
    OpKind::FarBranch16 | OpKind::FarBranch32 => OperandType::FarBranch,
    _ => OperandType::Invalid,
  }
}

#[inline(always)]
fn set_attributes(instr: &Instruction) -> u8 {
  let mut flags = MergenPrefix::None as u8;
  if instr.has_rep_prefix() {
    flags |= MergenPrefix::Rep as u8;
  }
  if instr.has_repne_prefix() {
    flags |= MergenPrefix::Repne as u8;
  }
  if instr.has_lock_prefix() {
    flags |= MergenPrefix::Lock as u8;
  }
  flags
}

const HAS_64BIT_IMM: u8 = 0b01;
const IS_RELATIVE: u8 = 0b10;

#[inline(always)]
fn analyze_instruction_bitfield(instr: &Instruction) -> u8 {
  let mut flags = 0u8;

  // Check RIP-relative first (common case)
  if instr.memory_base() == Register::RIP {
    flags |= IS_RELATIVE;
  }

  // Use iterator with early termination
  let op_count = instr.op_count();
  for i in 0..op_count {
    match instr.op_kind(i) {
      OpKind::Immediate64 => {
        flags |= HAS_64BIT_IMM;
        if flags == (HAS_64BIT_IMM | IS_RELATIVE) { return flags; }
      },
      OpKind::NearBranch16 | OpKind::NearBranch32 | OpKind::NearBranch64 => {
        flags |= IS_RELATIVE;
        if flags == (HAS_64BIT_IMM | IS_RELATIVE) { return flags; }
      },
      _ => {}
    }
  }

  flags
}

// Shared implementation for both disas functions
#[inline(always)]
fn disassemble_instruction(instr: &Instruction) -> MergenDisassembledInstructionBase {
  let instr_len = instr.len();
  let instr_len_u8 = instr_len as u8;
  let instr_len_u64 = instr_len as u64;
  let disp64 = instr.memory_displacement64();
  let flags = analyze_instruction_bitfield(&instr);
  let is_rel = (flags & IS_RELATIVE) != 0;
  let adjusted_disp = disp64.wrapping_sub(instr_len_u64);

  let immediate = if is_rel {
    adjusted_disp
  } else if (flags & HAS_64BIT_IMM) != 0 {
    instr.immediate64()
  } else {
    instr.immediate32() as u64
  };

  // Optimize type array creation
  let types = unsafe {
    let mut t = [0u8; 4];
    for i in 0..4 {
      *t.get_unchecked_mut(i) = convert_type_to_mergen(&instr, i as u32) as u8;
    }
    t
  };

  MergenDisassembledInstructionBase {
    mnemonic: instr.mnemonic() as u16,
    mem_base: instr.memory_base() as u8,
    mem_index: instr.memory_index() as u8,
    mem_scale: instr.memory_index_scale() as u8,
    mem_disp: if is_rel { adjusted_disp } else { disp64 },
    stack_growth: instr.stack_pointer_increment().unsigned_abs() as u8,
    immediate,
    regs: [
      instr.op0_register() as u8,
      instr.op1_register() as u8,
      instr.op2_register() as u8,
      instr.op3_register() as u8,
    ],
    types,
    operand_count_visible: instr.op_count() as u8,
    attributes: set_attributes(&instr),
    length: instr_len_u8,
    text: [0u8; 64],
  }
}

#[cold]
#[inline(never)]
fn handle_error() -> i32 { -1 }

#[no_mangle]
pub extern "C" fn disas(
  out: *mut MergenDisassembledInstructionBase,
  code_ptr: *const u8,
  len: usize,
) -> i32 {
  if code_ptr.is_null() || len == 0 || out.is_null() {
    return handle_error();
  }

  let code = unsafe { slice::from_raw_parts(code_ptr, len) };
  let mut decoder = Decoder::new(64, code, DecoderOptions::NO_INVALID_CHECK);
  let mut instr = Instruction::default();
  decoder.decode_out(&mut instr);

  let result = disassemble_instruction(&instr);
  unsafe { *out = result; }

  0
}

// Thread-local reusable String to minimize allocations
thread_local! {
    static CACHED_STRING: std::cell::RefCell<String> = std::cell::RefCell::new(String::with_capacity(64));
}

#[no_mangle]
pub extern "C" fn disas2(
  out: *mut MergenDisassembledInstructionBase,
  code_ptr: *const u8,
  len: usize,
) -> i32 {
  if out.is_null() || code_ptr.is_null() || len == 0 {
    return handle_error();
  }

  let code = unsafe { slice::from_raw_parts(code_ptr, len) };
  let mut decoder = Decoder::new(64, code, DecoderOptions::NO_INVALID_CHECK);
  let mut instr = Instruction::default();
  decoder.decode_out(&mut instr);

  // Specialized formatter options
  struct MyTraitOptions;
  impl SpecializedFormatterTraitOptions for MyTraitOptions {
    const ENABLE_DB_DW_DD_DQ: bool = false;
    unsafe fn verify_output_has_enough_bytes_left() -> bool {
      false
    }
  }

  type MyFormatter = SpecializedFormatter<MyTraitOptions>;
  let mut formatter = MyFormatter::new();

  // Use thread-local cached string to avoid repeated allocations
  let text = CACHED_STRING.with(|s| {
    let mut s = s.borrow_mut();
    s.clear(); // Clear previous content
    formatter.format(&instr, &mut *s);

    // Copy to fixed-size array
    let bytes = s.as_bytes();
    let mut text_array = [0u8; 64];
    let copy_len = bytes.len().min(63);
    text_array[..copy_len].copy_from_slice(&bytes[..copy_len]);
    text_array[copy_len] = 0; // Null-terminate
    text_array
  });

  // Build the result
  let mut result = disassemble_instruction(&instr);
  result.text = text;

  unsafe { *out = result; }

  0
}