# custom-isa-8bit-vm
A 8-bit simulated CPU in a VM coded from scratch, with custom instruction set and assembler. 

## Supported features
### Register Architecture and Memory
* **General-Purpose Registers (8-bit):** Four hardware registers (`A`, `B`, `C`, `D`) for arithmetic computations, logic operations, and fast parameter passing.
* **Program Counter (16-bit):** Dedicated register tracking the address of the current instruction under execution.
* **Stack Pointer (16-bit):** Dedicated register managing the top of the Stack via a pre-decrement/post-increment geometry.
* **RAM Sandboxing:** Complete process isolation using relative addressing enforced via a segment base boundary (`first_sector`).

---

### Addressing Modes
* **Immediate Values:** Literal numeric constants embedded directly within the bytecode stream.
* **Direct Register Addressing:** Direct manipulation of values stored inside `A`, `B`, `C`, or `D`.
* **Indirect Register Addressing (`[REG]`):** Single-level RAM dereferencing where the register holds the effective memory address. Features native support for cascaded, multi-level pointer chasing.

---

### Stack Management
* **`SPUSH` and `SPOP` Instructions:** Native support for pushing and popping 8-bit and 16-bit data to and from memory.
* **Context Preservation:** Standard LIFO (Last-In, First-Out) topology utilized for temporary data storage and execution flow control.

---

### Heap Management
* **First-Fit Allocation (`HALLOC`):** Automatic linear scanning of heap memory blocks to locate the first available free slot of sufficient size.
* **3-Byte Intrinsic Header:** Embedded block metadata layout (Payload Size and State Flag `FREE`/`OWNED`) written in Big Endian.
* **Block Splitting:** Automatic fragmentation mitigation that subdivides oversized free memory chunks during allocation.
* **Global Coalescence (`HFREE`):** Linear $O(N)$ defragmentation algorithm that automatically merges adjacent free memory blocks upon release.

---

### Labels and Flow Control (Jumps and Functions)
* **Positioning Labels:** Symbolic markers used for resolving target jump addresses at compile time.
* **Unconditional Jumps (`JMP`):** Immediate, deterministic deviation of the `Program Counter` execution path.
* **Conditional Jumps (`JE`, etc.):** Execution path routing based on the runtime evaluation of CPU flags set by comparison instructions (`CMP`).
* **Nested Functions (`CALL` / `RET`):** Deep hardware nesting support driven by automatic pushing and popping of the 16-bit Return Address on the Stack.
* **FastCall Convention:** Architectural standard restricting parameter passing to registers and the Heap, isolating the Stack to guarantee execution flow integrity.

---

### Safety and Runtime Protection
* **Stack Overflow / Underflow Detection:** Continuous hardware-like boundary monitoring of the `Stack Pointer` limits.
* **Heap Overflow Detection:** Safety abort mechanism triggered whenever a `HALLOC` request exceeds the maximum allocated heap capacity.
* **Double Free & Invalid Free Protection:** Consistency validation on heap headers to catch duplicate releases or misaligned/corrupted pointers.
* **Opcode Alignment Checker:** Fetch-decode-execute cycle validation that triggers immediate exceptions when encountering unknown or null (`0`) instruction opcodes.
