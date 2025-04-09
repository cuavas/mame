#ifndef MAME_CPU_DRCTESTER_DRCTESTER_H
#define MAME_CPU_DRCTESTER_DRCTESTER_H

#pragma once

#include "cpu/drcuml.h"
#include "cpu/drcumlsh.h"

#include <memory>
#include <optional>


class drctester_cpu_device : public cpu_device
{
public:
	drctester_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

	void func_display_result();

protected:
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_stop() override ATTR_COLD;

	virtual void execute_run() override;

	virtual space_config_vector memory_space_config() const override;

protected:
	virtual std::unique_ptr<util::disasm_interface> create_disassembler() override;

private:
	static constexpr uint64_t FLAGS_UNCHANGED = 0x80;
	static constexpr uint64_t FLAG_UNDEFINED_C = uint64_t(uml::FLAG_C << 8);
	static constexpr uint64_t FLAG_UNDEFINED_V = uint64_t(uml::FLAG_V << 8);
	static constexpr uint64_t FLAG_UNDEFINED_Z = uint64_t(uml::FLAG_Z << 8);
	static constexpr uint64_t FLAG_UNDEFINED_S = uint64_t(uml::FLAG_S << 8);
	static constexpr uint64_t FLAG_UNDEFINED_U = uint64_t(uml::FLAG_U << 8);
	static constexpr uint64_t FLAGS_UNDEFINED_VS = FLAG_UNDEFINED_V | FLAG_UNDEFINED_S;

	enum {
		METHOD_MEM = 1,
		METHOD_REG,
		METHOD_IMM,
		METHOD_MAPVAR,
	};

	static constexpr uint64_t param_methods[] = { METHOD_MEM, METHOD_REG, METHOD_IMM };
	static constexpr uint64_t result_methods[] = { METHOD_MEM, METHOD_REG };

	static constexpr uml::condition_t conditions[] = {
		uml::COND_ALWAYS,
		uml::COND_Z,
		uml::COND_NZ,
		uml::COND_S,
		uml::COND_NS,
		uml::COND_C,
		uml::COND_NC,
		uml::COND_V,
		uml::COND_NV,
		uml::COND_U,
		uml::COND_NU,
		uml::COND_A,
		uml::COND_BE,
		uml::COND_G,
		uml::COND_LE,
		uml::COND_L,
		uml::COND_GE,
	};

	char const *const condition_strings[16] = {
		"COND_Z",
		"COND_NZ",
		"COND_S",
		"COND_NS",
		"COND_C",
		"COND_NC",
		"COND_V",
		"COND_NV",
		"COND_U",
		"COND_NU",
		"COND_A",
		"COND_BE",
		"COND_G",
		"COND_LE",
		"COND_L",
		"COND_GE"
	};

	static constexpr uint64_t DEFAULT_PATTERN = 0x5731573113751375;

	static constexpr uint32_t FLAG_COMBOS[] = {
		0,
		uml::FLAG_C,
		uml::FLAG_S,
		uml::FLAG_U,
		uml::FLAG_V,
		uml::FLAG_Z,
		uml::FLAG_C | uml::FLAG_S,
		uml::FLAG_C | uml::FLAG_U,
		uml::FLAG_C | uml::FLAG_V,
		uml::FLAG_C | uml::FLAG_Z,
		uml::FLAG_S | uml::FLAG_U,
		uml::FLAG_V | uml::FLAG_S,
		uml::FLAG_V | uml::FLAG_U,
		uml::FLAG_V | uml::FLAG_Z,
		uml::FLAG_Z | uml::FLAG_S,
		uml::FLAG_Z | uml::FLAG_U,
		uml::FLAG_C | uml::FLAG_V | uml::FLAG_U,
		uml::FLAG_C | uml::FLAG_V | uml::FLAG_Z,
		uml::FLAG_C | uml::FLAG_S | uml::FLAG_U,
		uml::FLAG_C | uml::FLAG_V | uml::FLAG_S,
		uml::FLAG_C | uml::FLAG_Z | uml::FLAG_S,
		uml::FLAG_C | uml::FLAG_Z | uml::FLAG_U,
		uml::FLAG_V | uml::FLAG_S | uml::FLAG_U,
		uml::FLAG_V | uml::FLAG_Z | uml::FLAG_S,
		uml::FLAG_V | uml::FLAG_Z | uml::FLAG_U,
		uml::FLAG_Z | uml::FLAG_S | uml::FLAG_U,
		uml::FLAG_C | uml::FLAG_V | uml::FLAG_S | uml::FLAG_U,
		uml::FLAG_C | uml::FLAG_V | uml::FLAG_Z | uml::FLAG_S,
		uml::FLAG_C | uml::FLAG_V | uml::FLAG_Z | uml::FLAG_U,
		uml::FLAG_C | uml::FLAG_Z | uml::FLAG_S | uml::FLAG_U,
		uml::FLAG_V | uml::FLAG_Z | uml::FLAG_S | uml::FLAG_U,
		uml::FLAG_C | uml::FLAG_V | uml::FLAG_Z | uml::FLAG_S | uml::FLAG_U
	};

	struct inout_desc
	{
		std::optional<uint64_t> value = 0;
		uint64_t method = 0;
		uml::parameter param;
	};

	// Data that needs to be stored close to the generated DRC code
	struct internal_drc_state
	{
		int m_icount;

		uint32_t pc;
		uint32_t test_step;

		uint32_t test_num;
		uint32_t test_opcode;
		uint32_t test_opcode_size;
		uint32_t test_input_count;
		uint32_t test_output_count;
		uint32_t test_flag_combo;

		uint32_t test_initial_status;
		uint32_t test_expected_status;
		uint32_t test_result_status;
		uint32_t test_counter;

		uint32_t condition;

		uint64_t test_inputs[8];
		uint64_t test_expected_outputs[8];
		uint64_t test_undefined_outputs[8];
		uint64_t test_inputs_preserved[8];
		uint64_t test_inputs_final[8];
		uint64_t test_result_outputs[8];
		uint64_t test_mem_value[8];
		uint64_t test_mem_result_value[8];
		uint64_t test_param_methods[8];
		uint64_t test_result_methods[8];
		uint64_t test_param_formats[8];
		uint64_t test_result_formats[8];

		uint64_t testval;

		uint64_t value, value2;
	};

	enum : int
	{
		EXECUTE_OUT_OF_CYCLES       = 0,
		EXECUTE_MISSING_CODE        = 1,
		EXECUTE_UNMAPPED_CODE       = 2,
		EXECUTE_RESET_CACHE         = 3
	};

	address_space_config m_program_config;
	address_space *m_program;

	drc_cache m_drccache;
	internal_drc_state *m_state;

	std::unique_ptr<drcuml_state> m_drcuml;

	uml::code_handle *m_entry;
	uml::code_handle *m_nocode;
	uml::code_handle *m_out_of_cycles;
	uml::code_handle *m_testhandle;

	uint32_t m_labelnum;
	bool m_cache_dirty;

	float m_large_float;

	int m_input_count;
	int m_output_count;
	uml::parameter m_input_params[uml::instruction::MAX_PARAMS];
	uml::parameter m_output_params[uml::instruction::MAX_PARAMS];

	void internal_map(address_map &map) ATTR_COLD;

	uint32_t fe_check_r(address_space &space, offs_t offset);
	void fe_check_w(offs_t offset, uint32_t data);
	static void cfunc_fe_check(drctester_cpu_device &that);

	void code_flush_cache();
	void code_compile_block(uint32_t pc);

	void static_generate_entry_point();
	void static_generate_nocode_handler();
	void static_generate_out_of_cycles();

	void generate_tests(drcuml_block &block, int step);
	uml::parameter generate_set_param(drcuml_block &block, uint64_t param, uint64_t method, uml::parameter mem, uml::parameter reg);
	uml::parameter generate_set_fparam(drcuml_block &block, uint64_t param, uint64_t method, uint64_t format, uml::parameter mem, uml::parameter freg, uml::parameter ireg);
	uml::parameter generate_set_result(drcuml_block &block, uint64_t method, uml::parameter mem, uml::parameter reg);
	template <unsigned N, unsigned M> void generate_test_start(drcuml_block &block, uint32_t opcode, uint32_t opcode_size, uint32_t carry, uint32_t flags, const inout_desc (&inputs)[N], const inout_desc (&outputs)[M], const uint64_t *input_formats, const uint64_t *result_formats, uint32_t flag_combo, uint32_t initial_flags = uml::FLAGS_ALL);
	template <unsigned N> void generate_test_start(drcuml_block &block, uint32_t opcode, uint32_t opcode_size, uint32_t carry, uint32_t flags, const inout_desc (&inputs)[N], std::nullptr_t, const uint64_t *input_formats, std::nullptr_t, uint32_t flag_combo, uint32_t initial_flags = uml::FLAGS_ALL);
	template <unsigned M> void generate_test_start(drcuml_block &block, uint32_t opcode, uint32_t opcode_size, uint32_t carry, uint32_t flags, std::nullptr_t, const inout_desc (&outputs)[M], std::nullptr_t, const uint64_t *result_formats, uint32_t flag_combo, uint32_t initial_flags = uml::FLAGS_ALL);
	void generate_test_start(drcuml_block &block, uint32_t opcode, uint32_t opcode_size, uint32_t input_count, uint32_t output_count, uint32_t carry, uint32_t flags, const inout_desc *inputs, const inout_desc *outputs, const uint64_t *input_formats, const uint64_t *result_formats, uint32_t flag_combo, uint32_t initial_flags);
	void generate_test_end(drcuml_block &block, uint32_t flag_combo);

	const char *method_str(uint32_t method);
	uint64_t normalize_nan_inf(uint64_t value, uint64_t size);

	void TEST_ENTRY_1_NORET_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t carry, uint64_t flags, uint64_t param_method, uint32_t flag_combo);
	void TEST_ENTRY_2_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param_method, uint64_t result_method, uint32_t flag_combo);
	void TEST_ENTRY_2_NORET_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint32_t flag_combo);
	void TEST_ENTRY_3_SEXT_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uml::operand_size param2_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t result_method, uint32_t flag_combo);
	void TEST_ENTRY_3_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t result_method, uint32_t flag_combo);
	void TEST_ENTRY_4_SINGLE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, std::optional<uint64_t> result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t result_method, uint32_t flag_combo);
	void TEST_ENTRY_4_DOUBLE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, std::optional<uint64_t> result1_val, std::optional<uint64_t> result2_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t result1_method, uint64_t result2_method, uint32_t flag_combo);
	void TEST_ENTRY_4_TRIPLE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t param3_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t param3_method, uint64_t result_method, uint32_t flag_combo);
	void TEST_ENTRY_4_QUAD_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t param3_val, uint64_t result_in, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t param3_method, uint64_t result_method, uint32_t flag_combo);
	void TEST_ENTRY_FLOAT_2_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param_method, uint64_t result_method, uint64_t param_format, uint64_t result_format, uint32_t flag_combo);
	void TEST_ENTRY_FLOAT_2_NORET_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t param1_format, uint64_t param2_format, uint32_t flag_combo);
	void TEST_ENTRY_FLOAT_CMP_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t param1_format, uint64_t param2_format, uint32_t flag_combo);
	void TEST_ENTRY_FLOAT_3_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t result_method, uint64_t param1_format, uint64_t param2_format, uint64_t result_format, uint32_t flag_combo);
	void TEST_ENTRY_FLOAT_3_SIZE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uml::operand_size param2_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t result_method, uint64_t param1_format, uint64_t result_format, uint32_t flag_combo);
	void TEST_ENTRY_FLOAT_4_SIZE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uml::operand_size param2_val, uml::float_rounding_mode param3_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t result_method, uint64_t param1_format, uint32_t flag_combo);
	void TEST_ENTRY_1_NORET(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_2(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_2_CMP(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_2_NORET(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_3_SEXT(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uml::operand_size param2_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_3(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_4_SINGLE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, std::optional<uint64_t> result_val, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_4_DOUBLE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, std::optional<uint64_t> result1_val, std::optional<uint64_t> result2_val, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_4_TRIPLE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uint64_t param2_val, uint64_t param3_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_4_QUAD(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t result_in, uint64_t param1_val, uint64_t param2_val, uint64_t param3_val, uint64_t carry, uint64_t flags);
	void TEST_ENTRY_FLOAT_2(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param_val, uint64_t carry, uint64_t flags, uint64_t result_format, uint64_t param_format);
	void TEST_ENTRY_FLOAT_2_NORET(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t param1_format, uint64_t param2_format);
	void TEST_ENTRY_FLOAT_CMP(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t param1_format, uint64_t param2_format);
	void TEST_ENTRY_FLOAT_3(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t result_format, uint64_t param1_format, uint64_t param2_format);

	void TEST_ENTRY_MOV_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t param_method, uint64_t result_method, uint32_t flag_combo, uml::condition_t conditional_flags);
	void TEST_ENTRY_MOV(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val);

	void TEST_ENTRY_FMOV_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t param_method, uint64_t result_method, uint32_t flag_combo, uml::condition_t conditional_flags);
	void TEST_ENTRY_FMOV(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val);

	void TEST_ENTRY_FLOAT_3_SIZE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uml::operand_size param2_val, uint64_t carry, uint64_t flags, uint64_t result_format, uint64_t param1_format);
	void TEST_ENTRY_FLOAT_4_SIZE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uml::operand_size param2_val, uml::float_rounding_mode param3_val, uint64_t carry, uint64_t flags, uint64_t param1_format);

	void TEST_ENTRY_COND_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uml::condition_t cond, uint64_t result_val, uint64_t result_method, uint32_t flag_combo);
	void TEST_ENTRY_COND(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uml::condition_t cond, uint64_t result_val, uint64_t initial_flags);

	void TEST_MAPVAR_CONSTANT(drcuml_block &block, uml::parameter mapvar, uint32_t value);
	void TEST_MAPVAR_RECOVER(drcuml_block &block, uml::parameter mapvar, uint32_t value1, uint32_t value2, uint32_t value3);
};

DECLARE_DEVICE_TYPE(DRCTESTER, drctester_cpu_device)

class drctester_cpu_disassembler : public util::disasm_interface
{
public:
	drctester_cpu_disassembler() {}

	virtual ~drctester_cpu_disassembler() = default;

	virtual u32 opcode_alignment() const override
	{
		return 1;
	}

	virtual offs_t disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params) override
	{
		return pc;
	}
};

#endif // MAME_CPU_DRCTESTER_DRCTESTER_H
