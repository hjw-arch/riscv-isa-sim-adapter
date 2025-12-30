// Copyright (c) 2024-2025 DiveFuzz Project
// SPDX-License-Identifier: Mulan PSL v2

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "spike_engine.h"

namespace py = pybind11;
using namespace spike_engine;

PYBIND11_MODULE(spike_engine, m) {
    m.doc() = "Efficient Spike execution engine with checkpointing for DiveFuzz";

    // Floating-point register index offset
    // Register index convention:
    // - 0-31: Integer registers (x0-x31)
    // - 32-63: Floating-point registers (f0-f31, use FPR_OFFSET + reg_num)
    m.attr("FPR_OFFSET") = 32;

    // Checkpoint class
    py::class_<Checkpoint>(m, "Checkpoint")
        .def(py::init<>())
        .def_readwrite("xpr", &Checkpoint::xpr, "General-purpose registers (x0-x31)")
        .def_readwrite("fpr", &Checkpoint::fpr, "Floating-point registers (f0-f31)")
        .def_readwrite("pc", &Checkpoint::pc, "Program counter")
        .def_readwrite("instr_index", &Checkpoint::instr_index, "Current instruction index");

    // SpikeEngine class
    py::class_<SpikeEngine>(m, "SpikeEngine")
        .def(py::init<const std::string&, const std::string&, size_t, bool>(),
             py::arg("elf_path"),
             py::arg("isa") = "rv64gc",
             py::arg("num_instrs") = 200,
             py::arg("verbose") = false,
             R"pbdoc(
             Create a SpikeEngine instance

             Args:
                 elf_path: Path to pre-compiled ELF file with nops
                 isa: ISA string (default: "rv64gc")
                 num_instrs: Number of instructions to generate (default: 200)
                 verbose: Enable verbose output (default: false)
             )pbdoc")

        .def_static("get_instruction_size", &SpikeEngine::get_instruction_size,
             py::arg("machine_code"),
             R"pbdoc(
             Detect instruction size from machine code

             Args:
                 machine_code: 32-bit machine code

             Returns:
                 Instruction size in bytes (2 for compressed, 4 for standard)
             )pbdoc")

        .def("initialize", &SpikeEngine::initialize,
             R"pbdoc(
             Initialize Spike and execute template initialization code
             Returns True on success, False on error (check get_last_error())
             )pbdoc")

        .def("set_checkpoint", &SpikeEngine::set_checkpoint,
             "Save current processor state as checkpoint")

        .def("restore_checkpoint", &SpikeEngine::restore_checkpoint,
             "Restore processor state from last checkpoint")

        .def("execute_sequence", &SpikeEngine::execute_sequence,
             py::arg("machine_codes"),
             py::arg("sizes"),
             py::arg("max_steps") = 10000,
             R"pbdoc(
             Execute a sequence of instructions

             Unified execution method that handles all cases:
             - Single instruction: execute_sequence([code], [size])
             - Forward jump: execute_sequence([jump, middle...], [sizes...])
             - Backward loop: execute_sequence([init, body..., decr, branch], [sizes...])

             Execution logic:
             1. Write all instructions to memory
             2. Calculate end_addr = current_addr + sum(sizes)
             3. Execute until PC >= end_addr
             4. Each step handles traps automatically

             For loops (backward branches):
             - When branch jumps back, PC < end_addr, so execution continues
             - When branch falls through, PC >= end_addr, loop exits

             Args:
                 machine_codes: List of machine codes to execute
                 sizes: List of instruction sizes (2 or 4 bytes each)
                 max_steps: Maximum execution steps (safety limit, default: 10000)

             Returns:
                 Number of steps executed
             )pbdoc")

        .def("get_xpr", &SpikeEngine::get_xpr,
             py::arg("reg_index"),
             "Get general-purpose register value (x0-x31)")

        .def("get_fpr", &SpikeEngine::get_fpr,
             py::arg("reg_index"),
             "Get floating-point register value (f0-f31)")

        .def("get_pc", &SpikeEngine::get_pc,
             "Get program counter value")

        .def("get_all_xpr", &SpikeEngine::get_all_xpr,
             "Get all general-purpose register values (x0-x31)")

        .def("get_all_fpr", &SpikeEngine::get_all_fpr,
             "Get all floating-point register values (f0-f31)")

        .def("get_csr", &SpikeEngine::get_csr,
             py::arg("csr_addr"),
             "Get CSR value by address (e.g., 0x300 for mstatus)")

        .def("get_all_csrs", &SpikeEngine::get_all_csrs,
             "Get all accessible CSR values as dict {addr: value}")

        .def("get_mem_region_start", &SpikeEngine::get_mem_region_start,
             "Get mem_region start address (for testing memory operations)")

        .def("get_mem_region_size", &SpikeEngine::get_mem_region_size,
             "Get mem_region size in bytes")

        .def("read_mem", &SpikeEngine::read_mem,
             py::arg("addr"),
             py::arg("size"),
             "Read memory at specified address, returns list of bytes")

        .def("get_current_index", &SpikeEngine::get_current_index,
             "Get current instruction index")

        .def("get_num_instrs", &SpikeEngine::get_num_instrs,
             "Get total number of instructions")

        .def("get_last_error", &SpikeEngine::get_last_error,
             "Get last error message")

        .def("was_last_execution_trapped", &SpikeEngine::was_last_execution_trapped,
             R"pbdoc(
             Check if the last executed instruction triggered a trap/exception.

             This is useful for logging - instructions that cause traps are handled
             by the exception handler (which skips them), but they are still "accepted"
             from the fuzzer's perspective.

             Returns:
                 True if the last instruction triggered a trap, False otherwise
             )pbdoc")

        .def("get_last_trap_handler_steps", &SpikeEngine::get_last_trap_handler_steps,
             R"pbdoc(
             Get the number of trap handler steps executed in the last execution.

             Returns 0 if no trap occurred.

             Returns:
                 Number of steps executed in trap handler
             )pbdoc");

    // Version info
    m.attr("__version__") = "3.0.0";
}
