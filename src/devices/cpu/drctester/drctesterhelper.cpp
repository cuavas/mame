#include "drctester.h"

#include "corefloat.h"

#include <algorithm>


// #define UML_BACKEND_TEST_SMALL

constexpr double FLOAT_ALLOWED_DELTA = 0.0025f;
constexpr double DOUBLE_ALLOWED_DELTA = 0.0025;

constexpr int DISPLAY_RESULTS = 1;

static void cfunc_display_result(void *param)
{
	((drctester_cpu_device *)param)->func_display_result();
}

uml::parameter drctester_cpu_device::generate_set_param(drcuml_block &block, uint64_t param, uint64_t method, uml::parameter mem, uml::parameter reg)
{
	if (method == METHOD_MEM)
	{
		UML_DMOV(block, mem, param);
		return mem;
	}
	else if (method == METHOD_REG)
	{
		UML_DMOV(block, reg, param);
		return reg;
	}
	else
	{
		return param;
	}
}

uml::parameter drctester_cpu_device::generate_set_fparam(drcuml_block &block, uint64_t param, uint64_t method, uint64_t format, uml::parameter mem, uml::parameter freg, uml::parameter ireg)
{
	if (method == METHOD_MEM)
	{
		UML_DMOV(block, mem, param);
		return mem;
	}
	else if (method == METHOD_REG)
	{
		UML_DMOV(block, ireg, param);
		if (format == uml::SIZE_DOUBLE)
		{
			UML_FDCOPYI(block, freg, ireg);
			return freg;
		}
		else if (format == uml::SIZE_SHORT)
		{
			UML_FSCOPYI(block, freg, ireg);
			return freg;
		}
		else
		{
			return ireg;
		}
	}
	else
	{
		return param;
	}
}

uml::parameter drctester_cpu_device::generate_set_result(drcuml_block &block, uint64_t method, uml::parameter mem, uml::parameter reg)
{
	if (method == METHOD_MEM)
		return mem;
	else
		return reg;
}

template <unsigned N, unsigned M>
inline void drctester_cpu_device::generate_test_start(drcuml_block &block, uint32_t opcode, uint32_t opcode_size, uint32_t carry, uint32_t flags, const inout_desc (&inputs)[N], const inout_desc (&outputs)[M], const uint64_t *input_formats, const uint64_t *result_formats, uint32_t flag_combo, uint32_t initial_flags)
{
	generate_test_start(block, opcode, opcode_size, N, M, carry, flags, inputs, outputs, input_formats, result_formats, flag_combo, initial_flags);
}

template <unsigned N>
inline void drctester_cpu_device::generate_test_start(drcuml_block &block, uint32_t opcode, uint32_t opcode_size, uint32_t carry, uint32_t flags, const inout_desc (&inputs)[N], std::nullptr_t, const uint64_t *input_formats, std::nullptr_t, uint32_t flag_combo, uint32_t initial_flags)
{
	generate_test_start(block, opcode, opcode_size, N, 0, carry, flags, inputs, nullptr, input_formats, nullptr, flag_combo, initial_flags);
}

template <unsigned M>
inline void drctester_cpu_device::generate_test_start(drcuml_block &block, uint32_t opcode, uint32_t opcode_size, uint32_t carry, uint32_t flags, std::nullptr_t, const inout_desc (&outputs)[M], std::nullptr_t, const uint64_t *result_formats, uint32_t flag_combo, uint32_t initial_flags)
{
	generate_test_start(block, opcode, opcode_size, 0, M, carry, flags, nullptr, outputs, nullptr, result_formats, flag_combo, initial_flags);
}

void drctester_cpu_device::generate_test_start(drcuml_block &block, uint32_t opcode, uint32_t opcode_size, uint32_t input_count, uint32_t output_count, uint32_t carry, uint32_t flags, const inout_desc *inputs, const inout_desc *outputs, const uint64_t *input_formats, const uint64_t *result_formats, uint32_t flag_combo, uint32_t initial_flags)
{
	// set carry first in case the flags are expected to be unchanged, so the flags
	UML_SETFLGS(block, ((carry == 0) || (carry == 1)) ? ((initial_flags & ~uml::FLAG_C) | (carry ? uml::FLAG_C : uml::FLAGS_NONE)) : initial_flags);

	UML_GETFLGS(block, I4, flag_combo);
	UML_MOV(block, uml::mem(&m_state->test_initial_status), I4);
	UML_MOV(block, uml::mem(&m_state->test_expected_status), flags);
	UML_MOV(block, uml::mem(&m_state->condition), 0);

	UML_MOV(block, uml::mem(&m_state->test_num), m_state->test_counter++);
	UML_MOV(block, uml::mem(&m_state->test_opcode), opcode);
	UML_MOV(block, uml::mem(&m_state->test_opcode_size), opcode_size);
	UML_MOV(block, uml::mem(&m_state->test_input_count), input_count);
	UML_MOV(block, uml::mem(&m_state->test_output_count), output_count);
	UML_MOV(block, uml::mem(&m_state->test_flag_combo), flag_combo);

	std::fill(std::begin(m_state->test_inputs), std::end(m_state->test_inputs), 0);
	std::fill(std::begin(m_state->test_result_outputs), std::end(m_state->test_result_outputs), 0);

	m_input_count = input_count;
	for (int i = 0; (i < input_count) && inputs; i++)
	{
		UML_DMOV(block, uml::mem(&m_state->test_inputs[i]), inputs[i].value.value_or(0));
		UML_DMOV(block, uml::mem(&m_state->test_param_methods[i]), inputs[i].method);

		m_state->test_param_methods[i] = inputs[i].method;
		m_input_params[i] = inputs[i].param;
	}

	m_output_count = output_count;
	for (int i = 0; (i < output_count) && outputs; i++)
	{
		UML_DMOV(block, uml::mem(&m_state->test_expected_outputs[i]), outputs[i].value.value_or(0));
		UML_DMOV(block, uml::mem(&m_state->test_undefined_outputs[i]), outputs[i].value ? 0 : 1);
		UML_DMOV(block, uml::mem(&m_state->test_result_methods[i]), outputs[i].method);

		m_state->test_result_methods[i] = outputs[i].method;
		m_output_params[i] = outputs[i].param;
	}

	for (int i = 0; i < std::size(m_state->test_inputs_final); i++)
		UML_DMOV(block, uml::mem(&m_state->test_inputs_final[i]), 0);

	for (int i = 0; i < std::size(m_state->test_result_outputs); i++)
		UML_DMOV(block, uml::mem(&m_state->test_result_outputs[i]), 0);

	for (int i = 0; i < std::size(m_state->test_param_formats); i++)
	{
		UML_DMOV(block, uml::mem(&m_state->test_param_formats[i]), (input_formats && (i < input_count)) ? input_formats[i] : 0);

		m_state->test_param_formats[i] = (input_formats && (i < input_count)) ? input_formats[i] : 0;
	}

	for (int i = 0; i < std::size(m_state->test_result_formats); i++)
	{
		UML_DMOV(block, uml::mem(&m_state->test_result_formats[i]), (result_formats && (i < output_count)) ? result_formats[i] : 0);

		m_state->test_result_formats[i] = (result_formats && (i < output_count)) ? result_formats[i] : 0;
	}
}

void drctester_cpu_device::generate_test_end(drcuml_block &block, uint32_t flag_combo)
{
	UML_GETFLGS(block, I9, flag_combo);
	UML_MOV(block, uml::mem(&m_state->test_result_status), I9);

	const auto output_params_end = m_output_params + m_output_count;
	for (int i = 0; i < m_input_count; i++)
	{
		const auto &method = m_state->test_param_methods[i];
		const auto &param = m_input_params[i];
		if ((method == METHOD_REG) && (m_state->test_param_formats[i] == uml::SIZE_DOUBLE))
			UML_ICOPYFD(block, mem(&m_state->test_inputs_final[i]), param);
		else if ((method == METHOD_REG) && (m_state->test_param_formats[i] == uml::SIZE_SHORT))
			UML_ICOPYFS(block, mem(&m_state->test_inputs_final[i]), param);
		else
			UML_DMOV(block, mem(&m_state->test_inputs_final[i]), param);

		const bool preserved = ((method == METHOD_REG) || (method == METHOD_MEM)) && (std::find(m_output_params, output_params_end, param) == output_params_end);
		UML_DMOV(block, mem(&m_state->test_inputs_preserved[i]), preserved ? 1 : 0);
	}

	for (int i = 0; i < m_output_count; i++)
	{
		const auto &method = m_state->test_result_methods[i];
		const auto &param = m_output_params[i];
		if ((method == METHOD_REG) && (m_state->test_result_formats[i] == uml::SIZE_DOUBLE))
			UML_ICOPYFD(block, mem(&m_state->test_result_outputs[i]), param);
		else if ((method == METHOD_REG) && (m_state->test_result_formats[i] == uml::SIZE_SHORT))
			UML_ICOPYFS(block, mem(&m_state->test_result_outputs[i]), param);
		else
			UML_DMOV(block, mem(&m_state->test_result_outputs[i]), param);
	}

	UML_LABEL(block, m_labelnum++);
	UML_CALLC(block, cfunc_display_result, this);
}

void drctester_cpu_device::func_display_result()
{
	bool valid = true;

	if (!DISPLAY_RESULTS)
		return;

	printf("Test #%d\n", m_state->test_num);
	printf("\tOpcode: %d %s condition[%s]\n", m_state->test_opcode_size, uml::instruction::get_name(m_state->test_opcode), m_state->condition ? condition_strings[m_state->condition - uml::COND_Z] : "ALWAYS");
	if (m_state->test_input_count > 0)
	{
		printf("\tInputs:\n");
		for (int i = 0; i < m_state->test_input_count; i++)
		{
			if (m_state->test_param_formats[i] == uml::SIZE_SHORT)
			{
				if (m_state->test_inputs_preserved[i])
				{
					m_state->test_inputs_final[i] &= 0xffffffff;

					const bool is_valid = (uint32_t)m_state->test_inputs[i] == (uint32_t)m_state->test_inputs_final[i];

					printf("\t\tmethod[%s] %08x %f final[%08x %f] valid[%d]\n",
							method_str(m_state->test_param_methods[i]),
							(uint32_t)m_state->test_inputs[i], u2f(m_state->test_inputs[i]),
							(uint32_t)m_state->test_inputs_final[i], u2f(m_state->test_inputs_final[i]),
							is_valid);
					valid = valid && is_valid;
				}
				else
				{
					printf("\t\tmethod[%s] %08x %f\n",
							method_str(m_state->test_param_methods[i]),
							(uint32_t)m_state->test_inputs[i], u2f(m_state->test_inputs[i]));
				}
			}
			else if (m_state->test_param_formats[i] == uml::SIZE_DOUBLE)
			{
				if (m_state->test_inputs_preserved[i])
				{
					const bool is_valid = m_state->test_inputs[i] == m_state->test_inputs_final[i];

					printf("\t\tmethod[%s] %016llx %lf final[%016llx %lf] valid[%d]\n",
							method_str(m_state->test_param_methods[i]),
							(unsigned long long)m_state->test_inputs[i], u2d(m_state->test_inputs[i]),
							(unsigned long long)m_state->test_inputs_final[i], u2d(m_state->test_inputs_final[i]),
							is_valid);
					valid = valid && is_valid;
				}
				else
				{
					printf("\t\tmethod[%s] %016llx %lf\n",
							method_str(m_state->test_param_methods[i]),
							(unsigned long long)m_state->test_inputs[i], u2d(m_state->test_inputs[i]));
				}
			}
			else
			{
				if (m_state->test_inputs_preserved[i])
				{
					if (m_state->test_opcode_size == 8)
					{
						const bool is_valid = m_state->test_inputs[i] == m_state->test_inputs_final[i];

						printf("\t\tmethod[%s] 0x%016llx final[0x%016llx] valid[%d]\n",
								method_str(m_state->test_param_methods[i]),
								(unsigned long long)m_state->test_inputs[i],
								(unsigned long long)m_state->test_inputs_final[i],
								is_valid);
						valid = valid && is_valid;
					}
					else
					{
						m_state->test_result_outputs[i] &= 0xffffffff;

						const bool is_valid = (uint32_t)m_state->test_inputs[i] == (uint32_t)m_state->test_inputs_final[i];

						printf("\t\tmethod[%s] 0x%08x final[0x%08x] valid[%d]\n",
								method_str(m_state->test_param_methods[i]),
								(uint32_t)m_state->test_inputs[i],
								(uint32_t)m_state->test_inputs_final[i],
								is_valid);
						valid = valid && is_valid;
					}
				}
				else
				{
					if (m_state->test_opcode_size == 8)
					{
						printf("\t\tmethod[%s] 0x%016llx\n",
								method_str(m_state->test_param_methods[i]),
								(unsigned long long)m_state->test_inputs[i]);
					}
					else
					{
						printf("\t\tmethod[%s] 0x%08x\n",
								method_str(m_state->test_param_methods[i]),
								(uint32_t)m_state->test_inputs[i]);
					}
				}
			}
		}
	}

	const auto undefined_outputs_end = m_state->test_undefined_outputs + m_state->test_output_count;
	const bool defined_outputs = std::find(m_state->test_undefined_outputs, undefined_outputs_end, 0) != undefined_outputs_end;
	bool is_default_pattern = defined_outputs;
	if (defined_outputs)
	{
		printf("\tOutputs:\n");
		for (int i = 0; i < m_state->test_output_count; i++)
		{
			if (m_state->test_undefined_outputs[i])
				continue;

			uint64_t default_pattern_mask = m_state->test_result_formats[i] == uml::SIZE_SHORT || m_state->test_opcode_size == 4 ? 0xffffffff : 0xffffffffffffffff;

			if (m_state->test_result_outputs[i] != DEFAULT_PATTERN && m_state->test_result_outputs[i] != (DEFAULT_PATTERN & default_pattern_mask))
				is_default_pattern = false;

			if (m_state->test_result_formats[i] == uml::SIZE_SHORT)
			{
				m_state->test_expected_outputs[i] &= 0xffffffff;
				m_state->test_result_outputs[i] &= 0xffffffff;

				const uint32_t normalized_expected = normalize_nan_inf(m_state->test_expected_outputs[i], uml::SIZE_SHORT);
				const uint32_t normalized_result = normalize_nan_inf(m_state->test_result_outputs[i], uml::SIZE_SHORT);

				const uint64_t normalized_exected_low = u2f(normalized_expected) < 0.0 ? f2u(u2f(normalized_expected) + FLOAT_ALLOWED_DELTA) : f2u(u2f(normalized_expected) - FLOAT_ALLOWED_DELTA);
				const uint64_t normalized_exected_high = u2f(normalized_expected) < 0.0 ? f2u(u2f(normalized_expected) - FLOAT_ALLOWED_DELTA) : f2u(u2f(normalized_expected) + FLOAT_ALLOWED_DELTA);

				const bool is_valid = (normalized_result >= normalized_exected_low && normalized_result <= normalized_exected_high) || (normalized_expected == normalized_result);

				printf("\t\tmethod[%s] expected[%08x %f] result[%08x %f] valid[%d]\n",
						method_str(m_state->test_result_methods[i]),
						(uint32_t)m_state->test_expected_outputs[i], u2f(m_state->test_expected_outputs[i]),
						(uint32_t)m_state->test_result_outputs[i], u2f(m_state->test_result_outputs[i]),
						is_valid);
				valid = valid && is_valid;
			}
			else if (m_state->test_result_formats[i] == uml::SIZE_DOUBLE)
			{
				const uint64_t normalized_expected = normalize_nan_inf(m_state->test_expected_outputs[i], uml::SIZE_DOUBLE);
				const uint64_t normalized_result = normalize_nan_inf(m_state->test_result_outputs[i], uml::SIZE_DOUBLE);

				const uint64_t normalized_exected_low = u2d(normalized_expected) < 0 ? d2u(u2d(normalized_expected) + DOUBLE_ALLOWED_DELTA) : d2u(u2d(normalized_expected) - DOUBLE_ALLOWED_DELTA);
				const uint64_t normalized_exected_high = u2d(normalized_expected) < 0 ? d2u(u2d(normalized_expected) - DOUBLE_ALLOWED_DELTA) : d2u(u2d(normalized_expected) + DOUBLE_ALLOWED_DELTA);

				const bool is_valid = (normalized_result >= normalized_exected_low && normalized_result <= normalized_exected_high) || (normalized_expected == normalized_result);

				printf("\t\tmethod[%s] expected[%016llx %lf] result[%016llx %lf] valid[%d]\n",
						method_str(m_state->test_result_methods[i]),
						(unsigned long long)m_state->test_expected_outputs[i], u2d(m_state->test_expected_outputs[i]),
						(unsigned long long)m_state->test_result_outputs[i], u2d(m_state->test_result_outputs[i]),
						is_valid);
				valid = valid && is_valid;
			}
			else
			{
				if (m_state->test_opcode_size == 8)
				{
					const bool is_valid = m_state->test_expected_outputs[i] == m_state->test_result_outputs[i];

					printf("\t\tmethod[%s] expected[0x%016llx] result[0x%016llx] valid[%d]\n",
							method_str(m_state->test_result_methods[i]),
							(unsigned long long)m_state->test_expected_outputs[i],
							(unsigned long long)m_state->test_result_outputs[i],
							is_valid);
					valid = valid && is_valid;
				}
				else
				{
					m_state->test_expected_outputs[i] &= 0xffffffff;
					m_state->test_result_outputs[i] &= 0xffffffff;

					const bool is_valid = (uint32_t)m_state->test_expected_outputs[i] == (uint32_t)m_state->test_result_outputs[i];

					printf("\t\tmethod[%s] expected[0x%08x] result[0x%08x] valid[%d]\n",
							method_str(m_state->test_result_methods[i]),
							(uint32_t)m_state->test_expected_outputs[i],
							(uint32_t)m_state->test_result_outputs[i],
							is_valid);
					valid = valid && is_valid;
				}
			}
		}
	}

	const auto outflags = uml::instruction::get_outflags(m_state->test_opcode);
	const auto modflags = uml::instruction::get_modflags(m_state->test_opcode);
	const auto is_conditional = uml::instruction::get_conditional(m_state->test_opcode);

	printf("\tStatus:\n");
	printf("\t\toutflags[%02x] modflags[%02x] tested[%02x] conditional[%d]\n", outflags, modflags, m_state->test_flag_combo, is_conditional);
	uint32_t expected_flags, modified_flags;

	if (m_state->test_expected_status == FLAGS_UNCHANGED)
	{
		expected_flags = m_state->test_initial_status;
		modified_flags = m_state->test_result_status;
	}
	else
	{
		expected_flags = ((modflags == uml::FLAGS_NONE) || is_conditional) ? m_state->test_initial_status : m_state->test_expected_status;
		modified_flags = ((modflags == uml::FLAGS_NONE) || is_conditional) ? m_state->test_result_status : (m_state->test_result_status & outflags);
	}

	expected_flags &= ~(m_state->test_expected_status >> 8) & uml::FLAGS_ALL;
	modified_flags &= ~(m_state->test_expected_status >> 8) & uml::FLAGS_ALL;
	if (!is_conditional)
		expected_flags &= m_state->test_flag_combo;

	bool modified_flags_valid = expected_flags == modified_flags;

	bool conditions_met = false;
	switch (m_state->condition)
	{
		case uml::COND_ALWAYS:
			conditions_met = true;
			break;

		case uml::COND_Z:
			conditions_met = (m_state->test_initial_status & uml::FLAG_Z) != 0;
			break;

		case uml::COND_NZ:
			conditions_met = (m_state->test_initial_status & uml::FLAG_Z) == 0;
			break;

		case uml::COND_S:
			conditions_met = (m_state->test_initial_status & uml::FLAG_S) != 0;
			break;

		case uml::COND_NS:
			conditions_met = (m_state->test_initial_status & uml::FLAG_S) == 0;
			break;

		case uml::COND_C:
			conditions_met = (m_state->test_initial_status & uml::FLAG_C) != 0;
			break;

		case uml::COND_NC:
			conditions_met = (m_state->test_initial_status & uml::FLAG_C) == 0;
			break;

		case uml::COND_V:
			conditions_met = (m_state->test_initial_status & uml::FLAG_V) != 0;
			break;

		case uml::COND_NV:
			conditions_met = (m_state->test_initial_status & uml::FLAG_V) == 0;
			break;

		case uml::COND_U:
			conditions_met = (m_state->test_initial_status & uml::FLAG_U) != 0;
			break;

		case uml::COND_NU:
			conditions_met = (m_state->test_initial_status & uml::FLAG_U) == 0;
			break;

		case uml::COND_A:
			conditions_met = (m_state->test_initial_status & (uml::FLAG_C | uml::FLAG_Z)) == 0;
			break;

		case uml::COND_BE:
			conditions_met = (m_state->test_initial_status & (uml::FLAG_C | uml::FLAG_Z)) != 0;
			break;

		case uml::COND_G:
			conditions_met = (!!(m_state->test_initial_status & uml::FLAG_S) == !!(m_state->test_initial_status & uml::FLAG_V)) && !(m_state->test_initial_status & uml::FLAG_Z);
			break;

		case uml::COND_LE:
			conditions_met = (!!(m_state->test_initial_status & uml::FLAG_S) != !!(m_state->test_initial_status & uml::FLAG_V)) || (m_state->test_initial_status & uml::FLAG_Z);
			break;

		case uml::COND_L:
			conditions_met = !!(m_state->test_initial_status & uml::FLAG_S) != !!(m_state->test_initial_status & uml::FLAG_V);
			break;

		case uml::COND_GE:
			conditions_met = !!(m_state->test_initial_status & uml::FLAG_S) == !!(m_state->test_initial_status & uml::FLAG_V);
			break;
	}

	if (conditions_met)
		valid = valid && !is_default_pattern;
	else
		valid = is_default_pattern;

	printf("\t\tinitial[%02x] expected[%02x (%02x)] result[%02x (%02x)] valid[%d]\n", m_state->test_initial_status, expected_flags, m_state->test_expected_status, modified_flags, m_state->test_result_status, modified_flags_valid);

	valid = valid && modified_flags_valid;
	printf("%s\n\n", valid ? "PASSED" : "FAILED");
}

const char *drctester_cpu_device::method_str(uint32_t method)
{
	if (method == METHOD_MEM)
		return "mem";
	else if (method == METHOD_REG)
		return "reg";
	else if (method == METHOD_IMM)
		return "imm";
	else if (method == METHOD_MAPVAR)
		return "mapvar";
	return "unk";
}

uint64_t drctester_cpu_device::normalize_nan_inf(uint64_t value, uint64_t size)
{
	if (size == uml::SIZE_DOUBLE)
	{
		// double
		if (value == d2u(std::numeric_limits<double>::quiet_NaN()))
			return d2u(-std::numeric_limits<double>::quiet_NaN());
		else if (value == d2u(std::numeric_limits<double>::signaling_NaN()))
			return d2u(-std::numeric_limits<double>::quiet_NaN());
		else if (value == d2u(-std::numeric_limits<double>::signaling_NaN()))
			return d2u(-std::numeric_limits<double>::quiet_NaN());
		else if (value == d2u(std::numeric_limits<double>::infinity()))
			return d2u(-std::numeric_limits<double>::infinity());
	}
	else
	{
		// float
		if (value == f2u(std::numeric_limits<float>::quiet_NaN()))
			return f2u(-std::numeric_limits<float>::quiet_NaN());
		else if (value == f2u(std::numeric_limits<float>::signaling_NaN()))
			return f2u(-std::numeric_limits<float>::quiet_NaN());
		else if (value == f2u(-std::numeric_limits<float>::signaling_NaN()))
			return f2u(-std::numeric_limits<float>::quiet_NaN());
		else if (value == f2u(std::numeric_limits<float>::infinity()))
			return f2u(-std::numeric_limits<float>::infinity());
	}

	return value;
}

void drctester_cpu_device::TEST_ENTRY_1_NORET_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t carry, uint64_t flags, uint64_t param_method, uint32_t flag_combo)
{
	uml::parameter param = generate_set_param(block, param_val, param_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
	const inout_desc inputs[] = { {param_val, param_method, param} };
	generate_test_start(block, opcode, size, carry, flags, inputs, nullptr, nullptr, nullptr, flag_combo);
	block.append().configure(opcode, size, param);
	generate_test_end(block, flag_combo);
}

void drctester_cpu_device::TEST_ENTRY_2_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param_method, uint64_t result_method, uint32_t flag_combo)
{
	{
		uml::parameter param = generate_set_param(block, param_val, param_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I1);
		const inout_desc inputs[] = { {param_val, param_method, param} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param);
		generate_test_end(block, flag_combo);
	}

	if ((param_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param = generate_set_param(block, param_val, param_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[0]), uml::I1);
		const inout_desc inputs[] = { {param_val, param_method, param} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param);
		generate_test_end(block, flag_combo);
	}

	if ((param_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param = generate_set_param(block, param_val, param_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I0);
		const inout_desc inputs[] = { {param_val, param_method, param} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param);
		generate_test_end(block, flag_combo);
	}
}

void drctester_cpu_device::TEST_ENTRY_2_NORET_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint32_t flag_combo)
{
	uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
	uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
	const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
	generate_test_start(block, opcode, size, carry, flags, inputs, nullptr, nullptr, nullptr, flag_combo);
	block.append().configure(opcode, size, param1, param2);
	generate_test_end(block, flag_combo);
}

void drctester_cpu_device::TEST_ENTRY_3_SEXT_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uml::operand_size param2_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t result_method, uint32_t flag_combo)
{
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I1);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, METHOD_IMM, param2_val} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, uml::parameter::make_size(param2_val));
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[0]), uml::I1);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, METHOD_IMM, param2_val} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, uml::parameter::make_size(param2_val));
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I0);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, METHOD_IMM, param2_val} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, uml::parameter::make_size(param2_val));
		generate_test_end(block, flag_combo);
	}
}

void drctester_cpu_device::TEST_ENTRY_3_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t result_method, uint32_t flag_combo)
{
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[0]), uml::I2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param2_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[1]), uml::I2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I0);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param2_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I1);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}
}

void drctester_cpu_device::TEST_ENTRY_4_SINGLE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, std::optional<uint64_t> result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t result_method, uint32_t flag_combo)
{
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[0]), uml::I2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param2_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[1]), uml::I2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I0);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param2_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I1);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, result, param1, param2);
		generate_test_end(block, flag_combo);
	}
}

void drctester_cpu_device::TEST_ENTRY_4_DOUBLE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, std::optional<uint64_t> result1_val, std::optional<uint64_t> result2_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t result1_method, uint64_t result2_method, uint32_t flag_combo)
{
	uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
	uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
	uml::parameter result1 = generate_set_result(block, result1_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I0);
	uml::parameter result2 = generate_set_result(block, result2_method, uml::mem(&m_state->test_mem_result_value[1]), uml::I1);
	const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
	const inout_desc outputs[] = { {result1_val, result1_method, result1}, {result2_val, result2_method, result2} };
	generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
	block.append().configure(opcode, size, result1, result2, param1, param2);
	generate_test_end(block, flag_combo);
}

void drctester_cpu_device::TEST_ENTRY_4_TRIPLE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t param3_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t param3_method, uint64_t result_method, uint32_t flag_combo)
{
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter param3 = generate_set_param(block, param3_val, param3_method, uml::mem(&m_state->test_mem_value[2]), uml::I2);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I3);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2}, {param3_val, param3_method, param3} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2, param3);
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter param3 = generate_set_param(block, param3_val, param3_method, uml::mem(&m_state->test_mem_value[2]), uml::I2);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[0]), uml::I3);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2}, {param3_val, param3_method, param3} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2, param3);
		generate_test_end(block, flag_combo);
	}

	if ((param2_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter param3 = generate_set_param(block, param3_val, param3_method, uml::mem(&m_state->test_mem_value[2]), uml::I2);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[1]), uml::I3);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2}, {param3_val, param3_method, param3} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2, param3);
		generate_test_end(block, flag_combo);
	}

	if ((param3_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter param3 = generate_set_param(block, param3_val, param3_method, uml::mem(&m_state->test_mem_value[2]), uml::I2);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[2]), uml::I3);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2}, {param3_val, param3_method, param3} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2, param3);
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter param3 = generate_set_param(block, param3_val, param3_method, uml::mem(&m_state->test_mem_value[2]), uml::I2);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I0);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2}, {param3_val, param3_method, param3} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2, param3);
		generate_test_end(block, flag_combo);
	}

	if ((param2_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter param3 = generate_set_param(block, param3_val, param3_method, uml::mem(&m_state->test_mem_value[2]), uml::I2);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I1);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2}, {param3_val, param3_method, param3} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2, param3);
		generate_test_end(block, flag_combo);
	}

	if ((param3_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
		uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
		uml::parameter param3 = generate_set_param(block, param3_val, param3_method, uml::mem(&m_state->test_mem_value[2]), uml::I2);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2}, {param3_val, param3_method, param3} };
		const inout_desc outputs[] = { {result_val, result_method, result} };
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
		block.append().configure(opcode, size, result, param1, param2, param3);
		generate_test_end(block, flag_combo);
	}
}

void drctester_cpu_device::TEST_ENTRY_4_QUAD_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t param3_val, uint64_t result_in, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t param3_method, uint64_t result_method, uint32_t flag_combo)
{
	uml::parameter param1 = generate_set_param(block, param1_val, param1_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
	uml::parameter param2 = generate_set_param(block, param2_val, param2_method, uml::mem(&m_state->test_mem_value[1]), uml::I1);
	uml::parameter param3 = generate_set_param(block, param3_val, param3_method, uml::mem(&m_state->test_mem_value[2]), uml::I2);
	uml::parameter result = generate_set_param(block, result_in, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I3);
	const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2}, {param3_val, param3_method, param3}, {result_in, result_method, result} };
	const inout_desc outputs[] = { {result_val, result_method, result} };
	generate_test_start(block, opcode, size, carry, flags, inputs, outputs, nullptr, nullptr, flag_combo);
	block.append().configure(opcode, size, result, param1, param2, param3);
	generate_test_end(block, flag_combo);
}

void drctester_cpu_device::TEST_ENTRY_FLOAT_2_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param_method, uint64_t result_method, uint64_t param_format, uint64_t result_format, uint32_t flag_combo)
{
	{
		uml::parameter param = generate_set_fparam(block, param_val, param_method, param_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::F1);
		const inout_desc inputs[] = { {param_val, param_method, param} };
		const uint64_t input_formats[] = {param_format};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param);
		generate_test_end(block, flag_combo);
	}

	if ((param_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param = generate_set_fparam(block, param_val, param_method, param_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[0]), uml::F1);
		const inout_desc inputs[] = { {param_val, param_method, param} };
		const uint64_t input_formats[] = {param_format};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param);
		generate_test_end(block, flag_combo);
	}

	if ((param_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param = generate_set_fparam(block, param_val, param_method, param_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::F0);
		const inout_desc inputs[] = { {param_val, param_method, param} };
		const uint64_t input_formats[] = {param_format};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param);
		generate_test_end(block, flag_combo);
	}
}

void drctester_cpu_device::TEST_ENTRY_FLOAT_CMP_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t param1_format, uint64_t param2_format, uint32_t flag_combo)
{
	uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
	uml::parameter param2 = generate_set_fparam(block, param2_val, param2_method, param2_format, uml::mem(&m_state->test_mem_value[1]), uml::F1, uml::I1);
	const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
	const uint64_t input_formats[] = {param1_format, param2_format};
	generate_test_start(block, opcode, size, carry, flags, inputs, nullptr, input_formats, nullptr, flag_combo);
	block.append().configure(opcode, size, param1, param2);
	generate_test_end(block, flag_combo);
}

void drctester_cpu_device::TEST_ENTRY_FLOAT_3_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t param2_method, uint64_t result_method, uint64_t param1_format, uint64_t param2_format, uint64_t result_format, uint32_t flag_combo)
{
	{
		uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter param2 = generate_set_fparam(block, param2_val, param2_method, param2_format, uml::mem(&m_state->test_mem_value[1]), uml::F1, uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::F2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const uint64_t input_formats[] = {param1_format, param2_format};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter param2 = generate_set_fparam(block, param2_val, param2_method, param2_format, uml::mem(&m_state->test_mem_value[1]), uml::F1, uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[0]), uml::F2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const uint64_t input_formats[] = {param1_format, param2_format};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param2_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter param2 = generate_set_fparam(block, param2_val, param2_method, param2_format, uml::mem(&m_state->test_mem_value[1]), uml::F1, uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[1]), uml::F2);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const uint64_t input_formats[] = {param1_format, param2_format};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter param2 = generate_set_fparam(block, param2_val, param2_method, param2_format, uml::mem(&m_state->test_mem_value[1]), uml::F1, uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::F0);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const uint64_t input_formats[] = {param1_format, param2_format};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}

	if ((param2_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter param2 = generate_set_fparam(block, param2_val, param2_method, param2_format, uml::mem(&m_state->test_mem_value[1]), uml::F1, uml::I1);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::F1);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, param2_method, param2} };
		const uint64_t input_formats[] = {param1_format, param2_format};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param1, param2);
		generate_test_end(block, flag_combo);
	}
}

void drctester_cpu_device::TEST_ENTRY_FLOAT_3_SIZE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uml::operand_size param2_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t result_method, uint64_t param1_format, uint64_t result_format, uint32_t flag_combo)
{
	{
		uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::F1);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, METHOD_IMM, param2_val} };
		const uint64_t input_formats[] = {param1_format, 0};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param1, uml::parameter::make_size(param2_val));
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_MEM) && (result_method == METHOD_MEM))
	{
		uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_value[0]), uml::F1);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, METHOD_IMM, param2_val} };
		const uint64_t input_formats[] = {param1_format, 0};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param1, uml::parameter::make_size(param2_val));
		generate_test_end(block, flag_combo);
	}

	if ((param1_method == METHOD_REG) && (result_method == METHOD_REG))
	{
		uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
		uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::F0);
		const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, METHOD_IMM, param2_val} };
		const uint64_t input_formats[] = {param1_format, 0};
		const inout_desc outputs[] = { {result_val, result_method, result} };
		const uint64_t output_formats[] = {result_format};
		generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, output_formats, flag_combo);
		block.append().configure(opcode, size, result, param1, uml::parameter::make_size(param2_val));
		generate_test_end(block, flag_combo);
	}
}

void drctester_cpu_device::TEST_ENTRY_FLOAT_4_SIZE_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uml::operand_size param2_val, uml::float_rounding_mode param3_val, uint64_t result_val, uint64_t carry, uint64_t flags, uint64_t param1_method, uint64_t result_method, uint64_t param1_format, uint32_t flag_combo)
{
	uml::parameter param1 = generate_set_fparam(block, param1_val, param1_method, param1_format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
	uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I1);
	const inout_desc inputs[] = { {param1_val, param1_method, param1}, {param2_val, METHOD_IMM, param2_val}, {param3_val, METHOD_IMM, param3_val} };
	const uint64_t input_formats[] = {param1_format, 0, 0};
	const inout_desc outputs[] = { {result_val, result_method, result} };
	generate_test_start(block, opcode, size, carry, flags, inputs, outputs, input_formats, nullptr, flag_combo);
	block.append().configure(opcode, size, result, param1, uml::parameter::make_size(param2_val), uml::parameter::make_rounding(param3_val));
	generate_test_end(block, flag_combo);
}

void drctester_cpu_device::TEST_ENTRY_1_NORET(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_1_NORET_GENERATOR(block, opcode, size, param_val, carry, flags, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param_method : param_methods)
			TEST_ENTRY_1_NORET_GENERATOR(block, opcode, size, param_val, carry, flags, param_method, FLAG_COMBOS[comboidx]);
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_2(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_2_GENERATOR(block, opcode, size, param_val, result_val, carry, flags, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param_method : param_methods)
		{
			for (auto result_method : result_methods)
				TEST_ENTRY_2_GENERATOR(block, opcode, size, param_val, result_val, carry, flags, param_method, result_method, FLAG_COMBOS[comboidx]);
		}
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_2_CMP(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 1; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_2_NORET_GENERATOR(block, opcode, size, param1_val, param2_val, carry, flags, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param1_method : param_methods)
		{
			for (auto param2_method : param_methods)
				TEST_ENTRY_2_NORET_GENERATOR(block, opcode, size, param1_val, param2_val, carry, flags, param1_method, param2_method, FLAG_COMBOS[comboidx]);
		}
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_2_NORET(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_2_NORET_GENERATOR(block, opcode, size, param1_val, param2_val, carry, flags, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param1_method : param_methods)
		{
			for (auto param2_method : param_methods)
				TEST_ENTRY_2_NORET_GENERATOR(block, opcode, size, param1_val, param2_val, carry, flags, param1_method, param2_method, FLAG_COMBOS[comboidx]);
		}
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_3_SEXT(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uml::operand_size param2_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_3_SEXT_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param1_method : param_methods)
		{
			for (auto result_method : result_methods)
				TEST_ENTRY_3_SEXT_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, param1_method, result_method, FLAG_COMBOS[comboidx]);
		}
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_3(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_REG, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param1_method : param_methods)
		{
			for (auto param2_method : param_methods)
			{
				for (auto result_method : result_methods)
					TEST_ENTRY_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, param1_method, param2_method, result_method, FLAG_COMBOS[comboidx]);
			}
		}
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_4_SINGLE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, std::optional<uint64_t> result_val, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_4_SINGLE_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_REG, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param1_method : param_methods)
		{
			for (auto param2_method : param_methods)
			{
				for (auto result_method : result_methods)
					TEST_ENTRY_4_SINGLE_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, param1_method, param2_method, result_method, FLAG_COMBOS[comboidx]);
			}
		}
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_4_DOUBLE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, std::optional<uint64_t> result1_val, std::optional<uint64_t> result2_val, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_4_DOUBLE_GENERATOR(block, opcode, size, param1_val, param2_val, result1_val, result2_val, carry, flags, METHOD_REG, METHOD_REG, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param1_method : param_methods)
		{
			for (auto param2_method : param_methods)
			{
				for (auto result1_method : result_methods)
				{
					for (auto result2_method : result_methods)
						TEST_ENTRY_4_DOUBLE_GENERATOR(block, opcode, size, param1_val, param2_val, result1_val, result2_val, carry, flags, param1_method, param2_method, result1_method, result2_method, FLAG_COMBOS[comboidx]);
				}
			}
		}
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_4_TRIPLE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uint64_t param2_val, uint64_t param3_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_4_TRIPLE_GENERATOR(block, opcode, size, param1_val, param2_val, param3_val, result_val, carry, flags, METHOD_REG, METHOD_REG, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param1_method : param_methods)
		{
			for (auto param2_method : param_methods)
			{
				for (auto param3_method : param_methods)
				{
					for (auto result_method : result_methods)
						TEST_ENTRY_4_TRIPLE_GENERATOR(block, opcode, size, param1_val, param2_val, param3_val, result_val, carry, flags, param1_method, param2_method, param3_method, result_method, FLAG_COMBOS[comboidx]);
				}
			}
		}
#endif
	}
}


void drctester_cpu_device::TEST_ENTRY_4_QUAD(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t result_in, uint64_t param1_val, uint64_t param2_val, uint64_t param3_val, uint64_t carry, uint64_t flags)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
#ifdef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_4_QUAD_GENERATOR(block, opcode, size, param1_val, param2_val, param3_val, result_in, result_val, carry, flags, METHOD_REG, METHOD_REG, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx]);
#else
		for (auto param1_method : param_methods)
		{
			for (auto param2_method : param_methods)
			{
				for (auto param3_method : param_methods)
				{
					for (auto result_method : result_methods)
						TEST_ENTRY_4_QUAD_GENERATOR(block, opcode, size, param1_val, param2_val, param3_val, result_in, result_val, carry, flags, param1_method, param2_method, param3_method, result_method, FLAG_COMBOS[comboidx]);
				}
			}
		}
#endif
	}
}


void drctester_cpu_device::TEST_ENTRY_FLOAT_2(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param_val, uint64_t carry, uint64_t flags, uint64_t result_format, uint64_t param_format)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
		TEST_ENTRY_FLOAT_2_GENERATOR(block, opcode, size, param_val, result_val, carry, flags, METHOD_REG, METHOD_REG, param_format, result_format, FLAG_COMBOS[comboidx]);
#ifndef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_FLOAT_2_GENERATOR(block, opcode, size, param_val, result_val, carry, flags, METHOD_MEM, METHOD_MEM, param_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_2_GENERATOR(block, opcode, size, param_val, result_val, carry, flags, METHOD_MEM, METHOD_REG, param_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_2_GENERATOR(block, opcode, size, param_val, result_val, carry, flags, METHOD_REG, METHOD_MEM, param_format, result_format, FLAG_COMBOS[comboidx]);
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_FLOAT_CMP(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t param1_format, uint64_t param2_format)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 1; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
		TEST_ENTRY_FLOAT_CMP_GENERATOR(block, opcode, size, param1_val, param2_val, carry, flags, METHOD_REG, METHOD_REG, param1_format, param2_format, FLAG_COMBOS[comboidx]);
#ifndef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_FLOAT_CMP_GENERATOR(block, opcode, size, param1_val, param2_val, carry, flags, METHOD_MEM, METHOD_MEM, param1_format, param2_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_CMP_GENERATOR(block, opcode, size, param1_val, param2_val, carry, flags, METHOD_MEM, METHOD_REG, param1_format, param2_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_CMP_GENERATOR(block, opcode, size, param1_val, param2_val, carry, flags, METHOD_REG, METHOD_MEM, param1_format, param2_format, FLAG_COMBOS[comboidx]);
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_FLOAT_3(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uint64_t param2_val, uint64_t carry, uint64_t flags, uint64_t result_format, uint64_t param1_format, uint64_t param2_format)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
		TEST_ENTRY_FLOAT_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_REG, METHOD_REG, METHOD_REG, param1_format, param2_format, result_format, FLAG_COMBOS[comboidx]);
#ifndef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_FLOAT_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_MEM, METHOD_MEM, METHOD_MEM, param1_format, param2_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_MEM, METHOD_MEM, METHOD_REG, param1_format, param2_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_MEM, METHOD_REG, METHOD_MEM, param1_format, param2_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_MEM, METHOD_REG, METHOD_REG, param1_format, param2_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_REG, METHOD_MEM, METHOD_MEM, param1_format, param2_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_REG, METHOD_MEM, METHOD_REG, param1_format, param2_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_3_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_REG, METHOD_REG, METHOD_MEM, param1_format, param2_format, result_format, FLAG_COMBOS[comboidx]);
#endif
	}
}

///////

void drctester_cpu_device::TEST_ENTRY_MOV_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t param_method, uint64_t result_method, uint32_t flag_combo, uml::condition_t conditional_flags)
{
	// if (conditional_flags == uml::COND_NZ && (flag_combo & uml::FLAG_Z))
	//  UML_BREAK(block);

	uml::parameter param = generate_set_param(block, param_val, param_method, uml::mem(&m_state->test_mem_value[0]), uml::I0);
	uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I1);
	const inout_desc inputs[] = { {param_val, param_method, param} };
	const inout_desc outputs[] = { {param_val, result_method, result} };
	generate_test_start(block, opcode, size, !!(flag_combo & uml::FLAG_C), 0, inputs, outputs, nullptr, nullptr, flag_combo, flag_combo);

	UML_MOV(block, uml::mem(&m_state->condition), uint32_t(conditional_flags));

	// Initialize the result field to something that isn't the initial value so we can be sure it actually did the move properly
	UML_DMOV(block, result, DEFAULT_PATTERN);

	block.append().configure(opcode, size, result, param, conditional_flags);

	generate_test_end(block, flag_combo);
}

void drctester_cpu_device::TEST_ENTRY_MOV(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val)
{
	for (auto condidx = 0; condidx < std::size(conditions); condidx++)
	{
		for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
		{
			TEST_ENTRY_MOV_GENERATOR(block, opcode, size, param_val, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx], conditions[condidx]);
#ifndef UML_BACKEND_TEST_SMALL
			TEST_ENTRY_MOV_GENERATOR(block, opcode, size, param_val, METHOD_REG, METHOD_MEM, FLAG_COMBOS[comboidx], conditions[condidx]);
			TEST_ENTRY_MOV_GENERATOR(block, opcode, size, param_val, METHOD_MEM, METHOD_REG, FLAG_COMBOS[comboidx], conditions[condidx]);
			TEST_ENTRY_MOV_GENERATOR(block, opcode, size, param_val, METHOD_MEM, METHOD_MEM, FLAG_COMBOS[comboidx], conditions[condidx]);
			TEST_ENTRY_MOV_GENERATOR(block, opcode, size, param_val, METHOD_IMM, METHOD_REG, FLAG_COMBOS[comboidx], conditions[condidx]);
			TEST_ENTRY_MOV_GENERATOR(block, opcode, size, param_val, METHOD_IMM, METHOD_MEM, FLAG_COMBOS[comboidx], conditions[condidx]);
#endif
		}
	}
}


///////

void drctester_cpu_device::TEST_ENTRY_FMOV_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val, uint64_t param_method, uint64_t result_method, uint32_t flag_combo, uml::condition_t conditional_flags)
{
	const auto format = size == 4 ? uml::SIZE_SHORT : uml::SIZE_DOUBLE;

	uml::parameter param = generate_set_fparam(block, param_val, param_method, format, uml::mem(&m_state->test_mem_value[0]), uml::F0, uml::I0);
	uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::F1);
	const inout_desc inputs[] = { {param_val, param_method, param} };
	const uint64_t input_formats[] = {format};
	const inout_desc outputs[] = { {param_val, result_method, result} };
	const uint64_t output_formats[] = {format};
	generate_test_start(block, opcode, size, !!(flag_combo & uml::FLAG_C), 0, inputs, outputs, input_formats, output_formats, flag_combo, flag_combo);

	UML_MOV(block, uml::mem(&m_state->condition), uint32_t(conditional_flags));

	UML_DMOV(block, uml::I0, DEFAULT_PATTERN);
	UML_FDCOPYI(block, uml::F1, uml::I0);
	UML_DMOV(block, uml::mem(&m_state->test_mem_result_value[0]), DEFAULT_PATTERN);

	block.append().configure(opcode, size, result, param, conditional_flags);

	generate_test_end(block, flag_combo);
}

void drctester_cpu_device::TEST_ENTRY_FMOV(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t param_val)
{
	for (auto condidx = 0; condidx < std::size(conditions); condidx++)
	{
		for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
		{
			TEST_ENTRY_FMOV_GENERATOR(block, opcode, size, param_val, METHOD_REG, METHOD_REG, FLAG_COMBOS[comboidx], conditions[condidx]);
#ifndef UML_BACKEND_TEST_SMALL
			TEST_ENTRY_FMOV_GENERATOR(block, opcode, size, param_val, METHOD_REG, METHOD_MEM, FLAG_COMBOS[comboidx], conditions[condidx]);
			TEST_ENTRY_FMOV_GENERATOR(block, opcode, size, param_val, METHOD_MEM, METHOD_REG, FLAG_COMBOS[comboidx], conditions[condidx]);
			TEST_ENTRY_FMOV_GENERATOR(block, opcode, size, param_val, METHOD_MEM, METHOD_MEM, FLAG_COMBOS[comboidx], conditions[condidx]);
#endif
		}
	}
}

void drctester_cpu_device::TEST_ENTRY_FLOAT_3_SIZE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uml::operand_size param2_val, uint64_t carry, uint64_t flags, uint64_t result_format, uint64_t param1_format)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
		TEST_ENTRY_FLOAT_3_SIZE_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_REG, METHOD_REG, param1_format, result_format, FLAG_COMBOS[comboidx]);
#ifndef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_FLOAT_3_SIZE_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_REG, METHOD_MEM, param1_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_3_SIZE_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_MEM, METHOD_REG, param1_format, result_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_3_SIZE_GENERATOR(block, opcode, size, param1_val, param2_val, result_val, carry, flags, METHOD_MEM, METHOD_MEM, param1_format, result_format, FLAG_COMBOS[comboidx]);
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_FLOAT_4_SIZE(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uint64_t result_val, uint64_t param1_val, uml::operand_size param2_val, uml::float_rounding_mode param3_val, uint64_t carry, uint64_t flags, uint64_t param1_format)
{
	const auto clobbered = uml::instruction::get_modflags(opcode) & ~uml::instruction::get_outflags(opcode);
	for (auto comboidx = 0; comboidx < std::size(FLAG_COMBOS); comboidx++)
	{
		if (FLAG_COMBOS[comboidx] & clobbered)
			continue;
		TEST_ENTRY_FLOAT_4_SIZE_GENERATOR(block, opcode, size, param1_val, param2_val, param3_val, result_val, carry, flags, METHOD_REG, METHOD_REG, param1_format, FLAG_COMBOS[comboidx]);
#ifndef UML_BACKEND_TEST_SMALL
		TEST_ENTRY_FLOAT_4_SIZE_GENERATOR(block, opcode, size, param1_val, param2_val, param3_val, result_val, carry, flags, METHOD_REG, METHOD_MEM, param1_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_4_SIZE_GENERATOR(block, opcode, size, param1_val, param2_val, param3_val, result_val, carry, flags, METHOD_MEM, METHOD_REG, param1_format, FLAG_COMBOS[comboidx]);
		TEST_ENTRY_FLOAT_4_SIZE_GENERATOR(block, opcode, size, param1_val, param2_val, param3_val, result_val, carry, flags, METHOD_MEM, METHOD_MEM, param1_format, FLAG_COMBOS[comboidx]);
#endif
	}
}

void drctester_cpu_device::TEST_ENTRY_COND_GENERATOR(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uml::condition_t cond, uint64_t result_val, uint64_t result_method, uint32_t flag_combo)
{
	uml::parameter result = generate_set_result(block, result_method, uml::mem(&m_state->test_mem_result_value[0]), uml::I0);
	const inout_desc inputs[] = { {cond, METHOD_IMM, cond} };
	const inout_desc outputs[] = { {result_val, result_method, result} };
	generate_test_start(block, opcode, size, -1, flag_combo, inputs, outputs, nullptr, nullptr, flag_combo, flag_combo);
	block.append().configure(opcode, size, result, cond);
	generate_test_end(block, flag_combo);
}

void drctester_cpu_device::TEST_ENTRY_COND(drcuml_block &block, uml::opcode_t opcode, uint8_t size, uml::condition_t cond, uint64_t result_val, uint64_t initial_flags)
{
	TEST_ENTRY_COND_GENERATOR(block, opcode, size, cond, result_val, METHOD_REG, initial_flags & uml::FLAGS_ALL);
	TEST_ENTRY_COND_GENERATOR(block, opcode, size, cond, result_val, METHOD_MEM, initial_flags & uml::FLAGS_ALL);
}

void drctester_cpu_device::TEST_MAPVAR_CONSTANT(drcuml_block &block, uml::parameter mapvar, uint32_t value)
{
	uml::parameter result = uml::mem(&m_state->test_mem_result_value[0]);
	const inout_desc inputs[] = { {value, METHOD_MAPVAR, mapvar} };
	const inout_desc outputs[] = { {value, METHOD_MAPVAR, result} };
	generate_test_start(block, uml::OP_MAPVAR, 4, -1, 0, inputs, outputs, nullptr, nullptr, 0, 0);

	UML_MAPVAR(block, mapvar, value);
	UML_ADD(block, uml::I0, mapvar, 0);

	UML_DMOV(block, result, uml::I0);

	generate_test_end(block, 0);
}

void drctester_cpu_device::TEST_MAPVAR_RECOVER(drcuml_block &block, uml::parameter mapvar, uint32_t value1, uint32_t value2, uint32_t value3)
{
	const inout_desc outputs[] = { {value1, METHOD_REG, uml::I0}, {value2, METHOD_REG, uml::I1}, {value3, METHOD_REG, uml::I2}, {value1, METHOD_REG, uml::I3}, {value3, METHOD_REG, uml::I4} };
	generate_test_start(block, uml::OP_MAPVAR, 4, -1, 0, nullptr, outputs, nullptr, nullptr, 0, 0);

	uml::code_handle *const subroutine = m_drcuml->handle_alloc(util::string_format("recover%u", m_labelnum).c_str());
	uint32_t exit;

	UML_MAPVAR(block, mapvar, value1);
	UML_MOV(block, I0, mapvar);
	UML_CALLH(block, *subroutine);
	UML_MAPVAR(block, mapvar, value2);
	UML_MOV(block, I1, mapvar);
	UML_MAPVAR(block, mapvar, value3);
	UML_JMP(block, exit = m_labelnum++);

	UML_HANDLE(block, *subroutine);
	UML_MOV(block, I2, mapvar);
	UML_RECOVER(block, I3, mapvar);
	UML_RET(block);

	UML_LABEL(block, exit);
	UML_MOV(block, I4, mapvar);

	generate_test_end(block, 0);
}
