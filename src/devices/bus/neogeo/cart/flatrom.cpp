// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    AES/MVS flat ROM cartridges

    TODO:
    * Does the PCM chip decode the most significant address bits?  It
      only provides enables for four V ROMs, so it could mirror every
      4M*8 or it could fully decode the address and leave the rest of
      the space unmapped.

***************************************************************************/

#include "emu.h"
#include "flatrom.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <string>

//#define VERBOSE 1
//#define LOG_OUTPUT_STREAM std::cerr
#include "logmacro.h"


DEFINE_DEVICE_TYPE_NS(NG_CART_FLATROM, bus::neogeo::cart, flat_rom_device, "ng_cart_flatrom", "Neo-Geo AES/MVS Flat ROM Cartridge")


namespace bus { namespace neogeo { namespace cart {

//----------------------------------
// construction/destruction
//----------------------------------

flat_rom_device::flat_rom_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock)
	: flat_rom_device(mconfig, NG_CART_FLATROM, tag, owner, clock)
{
}

flat_rom_device::flat_rom_device(machine_config const &mconfig, device_type type, char const *tag, device_t *owner, u32 clock)
	: device_t(mconfig, type, tag, owner, clock)
	, device_cart_interface(mconfig, *this)
	, m_p_rom(*this, "^p"), m_v1_rom(*this, "^v1"), m_v2_rom(*this, "^v2"), m_v_rom(*this, "^v")
	, m_c_rom(*this, "^c"), m_s_rom(*this, "^s"), m_m1_rom(*this, "^m1")
	, m_prog_pcb(nullptr), m_cha_pcb(nullptr)
{
}


//----------------------------------
// software loading
//----------------------------------

image_init_result flat_rom_device::configure_software(get_feature_func const &get_feature)
{
	char const *const prog_feature(get_feature("prog"));
	char const *const cha_feature(get_feature("cha"));
	if (!prog_feature || !*prog_feature)
	{
		osd_printf_error("[%s] cannot load software: required feature \"prog\" not provided\n", tag());
		return image_init_result::FAIL;
	}
	if (!cha_feature || !*cha_feature)
	{
		osd_printf_error("[%s] cannot load software: required feature \"cha\" not provided\n", tag());
		return image_init_result::FAIL;
	}

	m_prog_pcb = config_prog(prog_feature, get_feature);
	m_cha_pcb = config_cha(cha_feature, get_feature);

	if (!m_prog_pcb || !m_cha_pcb)
		return image_init_result::FAIL;

	if (m_p_rom && (m_p_rom.length() > m_prog_pcb->p_rom_max))
	{
		osd_printf_error(
				"[%s] cannot load software: P ROM size 0x%X exceeds maximum 0x%X for %s\n",
				tag(), m_p_rom.length(), m_prog_pcb->p_rom_max, prog_feature);
		return image_init_result::FAIL;
	}
	if (m_v1_rom && (m_v1_rom.length() > m_prog_pcb->v1_rom_max))
	{
		osd_printf_error(
				"[%s] cannot load software: V1 ROM size 0x%X exceeds maximum 0x%X for %s\n",
				tag(), m_v1_rom.length(), m_prog_pcb->v1_rom_max, prog_feature);
		return image_init_result::FAIL;
	}
	if (m_v_rom && (m_v_rom.length() > m_prog_pcb->v1_rom_max))
	{
		osd_printf_error(
				"[%s] cannot load software: V1 ROM size 0x%X exceeds maximum 0x%X for %s\n",
				tag(), m_v_rom.length(), m_prog_pcb->v1_rom_max, prog_feature);
		return image_init_result::FAIL;
	}
	if (m_v2_rom && (m_v2_rom.length() > m_prog_pcb->v2_rom_max))
	{
		osd_printf_error(
				"[%s] cannot load software: V2 ROM size 0x%X exceeds maximum 0x%X for %s\n",
				tag(), m_v2_rom.length(), m_prog_pcb->v2_rom_max, prog_feature);
		return image_init_result::FAIL;
	}
	if (m_v_rom && (m_v_rom.length() > m_prog_pcb->v2_rom_max))
	{
		osd_printf_error(
				"[%s] cannot load software: V2 ROM size 0x%X exceeds maximum 0x%X for %s\n",
				tag(), m_v_rom.length(), m_prog_pcb->v2_rom_max, prog_feature);
		return image_init_result::FAIL;
	}
	if (m_c_rom && ((m_c_rom.length() >> 2) > m_cha_pcb->c_rom_max))
	{
		osd_printf_error(
				"[%s] cannot load software: C ROM size 0x%X exceeds maximum 0x%X for %s\n",
				tag(), (m_c_rom.length() >> 2), m_cha_pcb->c_rom_max, cha_feature);
		return image_init_result::FAIL;
	}
	if (m_s_rom && (m_s_rom.length() > m_cha_pcb->s_rom_max))
	{
		osd_printf_error(
				"[%s] cannot load software: S ROM size 0x%X exceeds maximum 0x%X for %s\n",
				tag(), m_s_rom.length(), m_cha_pcb->s_rom_max, cha_feature);
		return image_init_result::FAIL;
	}
	if (m_m1_rom && (m_m1_rom.length() > m_cha_pcb->m1_rom_max))
	{
		osd_printf_error(
				"[%s] cannot load software: M1 ROM size 0x%X exceeds maximum 0x%X for %s\n",
				tag(), m_m1_rom.length(), m_cha_pcb->m1_rom_max, cha_feature);
		return image_init_result::FAIL;
	}

	return image_init_result::PASS;
}


//----------------------------------
// 16-bit 68k ROM and I/O
//----------------------------------

void flat_rom_device::install_p_rom(address_space &space, offs_t base)
{
	install_rom<1>(m_p_rom, m_prog_pcb->p_rom_limit, m_prog_pcb->p_rom_mask, m_prog_pcb->p_rom_offset, space, base, "P ROM");
}


//----------------------------------
// 8-bit Z80 ROM
//----------------------------------

void flat_rom_device::install_m1_rom(address_space &space, offs_t base)
{
	install_rom<0>(m_m1_rom, m_cha_pcb->m1_rom_limit, m_cha_pcb->m1_rom_mask, m_cha_pcb->m1_rom_offset, space, base, "M1 ROM");
}


//----------------------------------
// 32-bit and 8-bit graphics ROM
//----------------------------------

void flat_rom_device::get_c_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	if (m_c_rom)
	{
		base = &m_c_rom[0];
		length = m_c_rom.bytes() >> 2;
		mirror = C_MASK & ~m_cha_pcb->c_rom_mask;
	}
	else
	{
		device_cart_interface::get_c_rom_info(base, length, mirror);
	}
}

void flat_rom_device::get_s_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	if (m_s_rom)
	{
		base = &m_s_rom[0];
		length = m_s_rom.bytes();
		mirror = S_MASK & ~m_cha_pcb->s_rom_mask;
	}
	else
	{
		device_cart_interface::get_s_rom_info(base, length, mirror);
	}
}


//----------------------------------
// 8-bit ADPCM ROM
//----------------------------------

void flat_rom_device::get_v1_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	if (m_v1_rom || m_v_rom)
	{
		base = &(m_v1_rom ? m_v1_rom : m_v_rom)[0];
		length = (m_v1_rom ? m_v1_rom : m_v_rom).bytes();
		mirror = V1_MASK & ~(m_prog_pcb->v1_rom_mask | (m_prog_pcb->v1_rom_mask - 1));
	}
	else
	{
		device_cart_interface::get_v1_rom_info(base, length, mirror);
	}
}


void flat_rom_device::get_v2_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	if (m_v2_rom || m_v_rom)
	{
		base = &(m_v2_rom ? m_v2_rom : m_v_rom)[0];
		length = (m_v2_rom ? m_v2_rom : m_v_rom).bytes();
		mirror = V2_MASK & ~(m_prog_pcb->v2_rom_mask | (m_prog_pcb->v2_rom_mask - 1));
	}
	else
	{
		device_cart_interface::get_v2_rom_info(base, length, mirror);
	}
}


//----------------------------------
// device_t implementation
//----------------------------------

void flat_rom_device::device_start()
{
}

void flat_rom_device::device_stop()
{
	m_prog_pcb.reset();
	m_cha_pcb.reset();
}


//----------------------------------
// mapping helper
//----------------------------------

template <unsigned Shift, typename T>
void flat_rom_device::install_rom(
		optional_region_ptr<T> &region,
		offs_t decode_limit,
		offs_t decode_mask,
		offs_t decode_offset,
		address_space &space,
		offs_t base,
		char const *name)
{
	assert(!(decode_limit & base));
	if (region)
	{
		offs_t const length(region.bytes() >> Shift);
		for (offs_t remain = length, start = 0; remain && (start <= decode_limit); )
		{
			unsigned const msb(31 - count_leading_zeros(u32(remain)));
			offs_t const chunk(offs_t(1) << msb);
			offs_t range((chunk - 1) & decode_limit);
			offs_t mirror(decode_limit & ~decode_mask & ~range);

			// TODO: deal with small offsets - probably requires breaking into smaller chunks with mirror set
			if (decode_offset & range)
				throw emu_fatalerror("Can't deal with offset/size combination\n");

			range = (range << Shift) | ((offs_t(1) << Shift) - 1);
			mirror <<= Shift;
			offs_t const begin(start << Shift);
			offs_t const end(begin | range);
			offs_t const src(start | (decode_offset & (chunk - 1)));

			LOG("Install %s 0x%X-0x%X at 0x%X-0x%X mirror 0x%X\n", name, src << Shift, (src << Shift) | range, base | begin, base | end, mirror);
			space.unmap_read(base | begin, base | end, mirror);
			space.install_rom(base | begin, base | end, mirror, &region[src]);

			remain ^= chunk;
			start ^= chunk;
		}
	}
}


//----------------------------------
// PCB selection
//----------------------------------

std::unique_ptr<flat_rom_device::prog_desc const> flat_rom_device::config_prog(char const *name, get_feature_func const &get_feature)
{
	if (!std::strcmp(name, "NEO-MVS PROG-8MB"))
		return config_prog_8mb(name, get_feature);
	else if (!std::strcmp(name, "NEO-MVS PROG-EP"))
		return config_prog_ep(name, get_feature);
	else if (!std::strcmp(name, "NEO-MVS PROG-NAM"))
		return config_prog_nam(name, get_feature);
	else if (!std::strcmp(name, "NEO-MVS PROG42G") || !std::strcmp(name, "NEO-MVS PROG42G-1"))
		return config_prog42g(name, get_feature);
	else if (!std::strcmp(name, "NEO-MVS PROG8M42"))
		return config_prog8m42(name, get_feature);

	osd_printf_error("[%s] cannot load software: unsupported PCB \"%s\"\n", tag(), name);
	return nullptr;
}

std::unique_ptr<flat_rom_device::cha_desc const> flat_rom_device::config_cha(char const *name, get_feature_func const &get_feature)
{
	if (!std::strcmp(name, "NEO-MVS CHA-32"))
		return config_cha_32(name, get_feature);
	else if (!std::strcmp(name, "NEO-MVS CHA-8M"))
		return config_cha_8mb(name, get_feature);

	osd_printf_error("[%s] cannot load software: unsupported PCB \"%s\"\n", tag(), name);
	return nullptr;
}


//----------------------------------
// PROG PCB configuration
//----------------------------------

std::unique_ptr<flat_rom_device::prog_desc const> flat_rom_device::config_prog_8mb(char const *name, get_feature_func const &get_feature)
{
	std::unique_ptr<prog_desc> desc(new prog_desc);

	desc->p_rom_max = 0x80000;
	desc->p_rom_offset = 0x00000;
	desc->p_rom_mask = 0x40000;

	desc->v1_rom_max = 0x0200000;
	desc->v1_rom_offset = 0x0000000;
	desc->v1_rom_mask = 0x100000;

	desc->v2_rom_max = 0x0200000;
	desc->v2_rom_offset = 0x0000000,
	desc->v2_rom_mask = 0x100000;

	// get jumper values
	bool badjumpers(false);
	bool j1valid, j2valid, j3valid, j4valid;
	bool const j1(get_jumper(name, get_feature, "prog.J1", j1valid));
	bool const j2(get_jumper(name, get_feature, "prog.J2", j2valid));
	bool const j3(get_jumper(name, get_feature, "prog.J3", j3valid));
	bool const j4(get_jumper(name, get_feature, "prog.J4", j4valid));

	// ROMWAIT jumpers
	if (!set_romwait(name, j1, j2, j1valid, j2valid))
		badjumpers = true;

	// P ROM configuration jumpers
	bool have_p_rom(m_p_rom && (m_p_rom.bytes() > (offs_t(1) << 16)));
	bool p1global(false);
	if (j3valid && j4valid)
	{
		if (j3 && j4)
		{
			osd_printf_error("[%s] %s: A19 shorted to GND - check jumpers\n", tag(), name);
			badjumpers = true;
		}
		else if (!j3 && !j4)
		{
			osd_printf_warning("[%s] %s: P1 /CE floating - check jumpers\n", tag(), name);
			p1global = true;
		}
		else
		{
			p1global = j4;
		}
	}
	else if (j3valid)
	{
		p1global = !j3;
	}
	else if (j4valid)
	{
		p1global = j4;
	}

	// adjust address decoding for P ROM configuration and check size
	if (p1global)
	{
		desc->p_rom_mask = 0x00000;
	}
	else if (have_p_rom && ((m_p_rom.bytes() >> 1) >= (offs_t(1) << 19)))
	{
		desc->p_rom_limit >>= 1;
	}

	// check P ROM size
	if (have_p_rom)
	{
		offs_t const words(m_p_rom.bytes() >> 1);
		offs_t const excess(words & ((offs_t(1) << 18) - 1));
		if (p1global)
		{
			if (m_p_rom.bytes() & (m_p_rom.bytes() - 1))
			{
				osd_printf_error("[%s] %s: P ROM size 0x%X unsupported when jumpered for P1 only\n", tag(), name, m_p_rom.bytes());
				badjumpers = true;
			}
		}
		else
		{
			if ((excess > (offs_t(1) << 16)) || (excess & (excess - 1)))
			{
				osd_printf_error("[%s] %s: P ROM size 0x%X unsupported\n", tag(), name, m_p_rom.bytes());
				badjumpers = true;
			}
		}
	}

	// no unified V ROM
	if (m_v_rom)
	{
		osd_printf_error("[%s] %s: unified V ROM unsupported\n", tag(), name);
		badjumpers = true;
	}

	// check V1 ROM size
	if (m_v1_rom && (m_v1_rom.bytes() <= desc->v1_rom_max))
	{
		offs_t const excess(m_v1_rom.bytes() & ((offs_t(1) << 20) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: V1 ROM size 0x%X unsupported\n", tag(), name, m_v1_rom.bytes());
			badjumpers = true;
		}
	}

	// check V2 ROM size
	if (m_v2_rom && (m_v2_rom.bytes() <= desc->v2_rom_max))
	{
		offs_t const excess(m_v2_rom.bytes() & ((offs_t(1) << 20) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: V2 ROM size 0x%X unsupported\n", tag(), name, m_v2_rom.bytes());
			badjumpers = true;
		}
	}

	return badjumpers ? nullptr : std::move(desc);
}

std::unique_ptr<flat_rom_device::prog_desc const> flat_rom_device::config_prog_ep(char const *name, get_feature_func const &get_feature)
{
	std::unique_ptr<prog_desc> desc(new prog_desc);

	desc->p_rom_max = 0x80000;
	desc->p_rom_offset = 0x00000;
	desc->p_rom_mask = 0x40000;

	desc->v1_rom_max = 0x0200000;
	desc->v1_rom_offset = 0x0000000;
	desc->v1_rom_mask = 0x180000;

	desc->v2_rom_max = 0x0200000;
	desc->v2_rom_offset = 0x0000000,
	desc->v2_rom_mask = 0x180000;

	// get jumper values
	bool badjumpers(false);
	bool j1valid, j2valid, j3valid, j4valid, j5valid, j6valid;
	bool const j1(get_jumper(name, get_feature, "prog.J1", j1valid));
	bool const j2(get_jumper(name, get_feature, "prog.J2", j2valid));
	bool const j3(get_jumper(name, get_feature, "prog.J3", j3valid));
	bool const j4(get_jumper(name, get_feature, "prog.J4", j4valid));
	bool const j5(get_jumper(name, get_feature, "prog.J5", j5valid));
	bool const j6(get_jumper(name, get_feature, "prog.J6", j6valid));

	// ROMWAIT jumpers
	if (!set_romwait(name, j1, j2, j1valid, j2valid))
		badjumpers = true;

	// V ROM mirroring jumpers
	set_2m_mirror(name, "V2", m_v2_rom, j3, j4, j3valid, j4valid);
	set_2m_mirror(name, "V1", m_v1_rom, j5, j6, j5valid, j6valid);

	// check P ROM size
	if (m_p_rom && ((m_p_rom.bytes() >> 1) <= desc->p_rom_max))
	{
		offs_t const words(m_p_rom.bytes() >> 1);
		offs_t const excess(words & ((offs_t(1) << 18) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: P ROM size 0x%X unsupported", tag(), name, m_p_rom.bytes());
			badjumpers = true;
		}
	}

	// no unified V ROM
	if (m_v_rom)
	{
		osd_printf_error("[%s] %s: unified V ROM unsupported\n", tag(), name);
		badjumpers = true;
	}

	// check V1 ROM size
	if (m_v1_rom && (m_v1_rom.bytes() <= desc->v1_rom_max))
	{
		offs_t const excess(m_v1_rom.bytes() & ((offs_t(1) << 19) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: V1 ROM size 0x%X unsupported\n", tag(), name, m_v1_rom.bytes());
			badjumpers = true;
		}
	}

	// check V2 ROM size
	if (m_v2_rom && (m_v2_rom.bytes() <= desc->v2_rom_max))
	{
		offs_t const excess(m_v2_rom.bytes() & ((offs_t(1) << 19) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: V2 ROM size 0x%X unsupported\n", tag(), name, m_v2_rom.bytes());
			badjumpers = true;
		}
	}

	return badjumpers ? nullptr : std::move(desc);
}

std::unique_ptr<flat_rom_device::prog_desc const> flat_rom_device::config_prog_nam(char const *name, get_feature_func const &get_feature)
{
	std::unique_ptr<prog_desc> desc(new prog_desc);

	desc->p_rom_max = 0x80000;
	desc->p_rom_offset = 0x00000;
	desc->p_rom_mask = 0x40000;

	desc->v1_rom_max = 0x0080000;
	desc->v1_rom_offset = 0x0000000;
	desc->v1_rom_mask = 0x000000;

	desc->v2_rom_max = 0x0180000;
	desc->v2_rom_offset = 0x0000000;
	desc->v2_rom_mask = 0x180000;

	// get jumper values
	bool badjumpers(false);
	bool j1valid, j2valid, j3valid, j4valid;
	bool const j1(get_jumper(name, get_feature, "prog.J1", j1valid));
	bool const j2(get_jumper(name, get_feature, "prog.J2", j2valid));
	bool const j3(get_jumper(name, get_feature, "prog.J3", j3valid));
	bool const j4(get_jumper(name, get_feature, "prog.J4", j4valid));

	// P1 ROM size jumpers
	bool large(true);
	if (j1valid && j2valid)
	{
		if (j1 && j2)
		{
			osd_printf_error("[%s] %s: A18 shorted to GND - check jumpers\n", tag(), name);
			badjumpers = true;
		}
		else if (!j1 && !j2)
		{
			osd_printf_error("[%s] %s: P ROM select A floating - check jumpers\n", tag(), name);
		}
		else
		{
			large = j2;
		}
	}
	else if (j1valid)
	{
		large = !j1;
	}
	else if (j2valid)
	{
		large = !j2;
	}
	if (m_p_rom && ((m_p_rom.bytes() >> 1) <= desc->p_rom_max))
	{
		offs_t const words(m_p_rom.bytes() >> 1);
		offs_t const excess(words & ((offs_t(1) << (large ? 18 : 17)) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error(
					"[%s] %s: P ROM size 0x%X unsupported when jumpered for %uk*16 ROMs\n",
					tag(), name, m_p_rom.bytes(), large ? 256U : 128U);
			badjumpers = true;
		}
	}

	// ROMWAIT jumpers
	if (!set_romwait(name, j3, j4, j3valid, j4valid))
		badjumpers = true;

	// no unified V ROM
	if (m_v_rom)
	{
		osd_printf_error("[%s] %s: unified V ROM unsupported\n", tag(), name);
		badjumpers = true;
	}

	// check V1 ROM size
	if (m_v1_rom && (m_v1_rom.bytes() <= desc->v1_rom_max))
	{
		if (m_v1_rom.bytes() & (m_v1_rom.bytes() - 1))
		{
			osd_printf_error("[%s] %s: V1 ROM size 0x%X unsupported\n", tag(), name, m_v1_rom.bytes());
			badjumpers = true;
		}
	}

	// check V2 ROM size
	if (m_v2_rom && (m_v2_rom.bytes() <= desc->v2_rom_max))
	{
		offs_t const excess(m_v2_rom.bytes() & ((offs_t(1) << 19) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: V2 ROM size 0x%X unsupported\n", tag(), name, m_v2_rom.bytes());
			badjumpers = true;
		}
	}

	return badjumpers ? nullptr : std::move(desc);
}

std::unique_ptr<flat_rom_device::prog_desc const> flat_rom_device::config_prog42g(char const *name, get_feature_func const &get_feature)
{
	std::unique_ptr<prog_desc> desc(new prog_desc);

	desc->p_rom_max = 0x80000;
	desc->p_rom_offset = 0x00000;
	desc->p_rom_mask = 0x40000;

	desc->v1_rom_max = desc->v2_rom_max = 0x0400000;
	desc->v1_rom_offset = desc->v2_rom_offset = 0x0000000;
	desc->v1_rom_mask = desc->v2_rom_mask = 0x300000;

	// get jumper values
	bool badjumpers(false);
	bool nowaitvalid, waitvalid, j1mvalid, j5mvalid, j8mvalid, j4m5mvalid;
	bool const nowait(get_jumper(name, get_feature, "prog.NO WAIT", nowaitvalid));
	bool const wait(get_jumper(name, get_feature, "prog.1 WAIT", waitvalid));
	bool const j1m(get_jumper(name, get_feature, "prog.1M", j1mvalid));
	bool const j5m(get_jumper(name, get_feature, "prog.5M", j5mvalid));
	bool const j8m(get_jumper(name, get_feature, "prog.8M", j8mvalid));
	bool const j4m5m(get_jumper(name, get_feature, "prog.4M.5M", j4m5mvalid));

	// ROMWAIT jumpers
	if (!set_romwait(name, nowait, wait, nowaitvalid, waitvalid))
		badjumpers = true;

	// P ROM configuration jumpers
	bool have_p_rom(m_p_rom && (m_p_rom.bytes() > (offs_t(1) << 16)));
	bool p1global(false);
	bool p2global(false);
	if (j1mvalid && j5mvalid)
	{
		if (j1m && j5m)
		{
			osd_printf_error("[%s] %s: /A19 shorted to GND - check jumpers\n", tag(), name);
			badjumpers = true;
		}
		else if (!j1m && !j5m)
		{
			if (have_p_rom && (m_p_rom.bytes() & (m_p_rom.bytes() - 1)))
			{
				osd_printf_error("[%s] %s: P2 /CE floating - check jumpers\n", tag(), name);
				badjumpers = true;
			}
			p2global = true;
		}
		else
		{
			p2global = j1m;
		}
	}
	else if (j1mvalid)
	{
		p2global = j1m;
	}
	else if (j5mvalid)
	{
		p2global = !j5m;
	}
	if (j8mvalid && j4m5mvalid)
	{
		if (j8m && j4m5m)
		{
			osd_printf_error("[%s] %s: A19 shorted to GND - check jumpers\n", tag(), name);
			badjumpers = true;
		}
		else if (!j8m && !j4m5m)
		{
			if (have_p_rom && ((m_p_rom.bytes() >> 1) > (offs_t(1) << 16)))
				osd_printf_warning("[%s] %s: P1 /CE floating - check jumpers\n", tag(), name);
			p1global = true;
		}
		else
		{
			p1global = j8m;
		}
	}
	else if (j8mvalid)
	{
		p1global = j8m;
	}
	else if (j4m5mvalid)
	{
		p1global = !j4m5m;
	}

	// adjust address decoding for P ROM configuration and check size
	if (p1global)
	{
		desc->p_rom_mask = 0x00000;
	}
	else if (have_p_rom)
	{
		if (p2global && ((m_p_rom.bytes() >> 1) <= (offs_t(1) << 16)))
			desc->p_rom_mask = 0x00000;
		else if ((m_p_rom.bytes() >> 1) >= (offs_t(1) << 19))
			desc->p_rom_limit >>= 1;
	}

	// check P ROM size
	if (have_p_rom)
	{
		offs_t const words(m_p_rom.bytes() >> 1);
		offs_t const excess(words & ((offs_t(1) << 18) - 1));
		if (p1global || p2global)
		{
			if (m_p_rom.bytes() & (m_p_rom.bytes() - 1))
			{
				osd_printf_error(
						"[%s] %s: P ROM size 0x%X unsupported when jumpered for %s only\n",
						tag(), name, m_p_rom.bytes(), p1global ? "P1" : "P2");
				badjumpers = true;
			}
		}
		else
		{
			if ((excess > (offs_t(1) << 16)) || (excess & (excess - 1)))
			{
				osd_printf_error("[%s] %s: P ROM size 0x%X unsupported\n", tag(), name, m_p_rom.bytes());
				badjumpers = true;
			}
		}
	}

	// no separate V1/V2 ROM
	if (m_v1_rom || m_v2_rom)
	{
		osd_printf_error("[%s] %s: separate V1/V2 ROM unsupported\n", tag(), name);
		badjumpers = true;
	}

	// check V ROM size
	if (m_v_rom && (m_v_rom.bytes() <= desc->v1_rom_max))
	{
		offs_t const excess(m_v_rom.bytes() & ((offs_t(1) << 20) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: V ROM size 0x%X unsupported\n", tag(), name, m_v_rom.bytes());
			badjumpers = true;
		}
	}

	return badjumpers ? nullptr : std::move(desc);
}

std::unique_ptr<flat_rom_device::prog_desc const> flat_rom_device::config_prog8m42(char const *name, get_feature_func const &get_feature)
{
	std::unique_ptr<prog_desc> desc(new prog_desc);

	desc->p_rom_max = 0x80000;
	desc->p_rom_offset = 0x00000;
	desc->p_rom_mask = 0x40000;

	desc->v1_rom_max = 0x0200000;
	desc->v1_rom_offset = 0x0000000;
	desc->v1_rom_mask = 0x100000;

	desc->v2_rom_max = 0x0200000;
	desc->v2_rom_offset = 0x0000000;
	desc->v2_rom_mask = 0x100000;

	// get jumper values
	bool badjumpers(false);
	bool nowaitvalid, waitvalid, j1mvalid, j5mavalid, j4m8mvalid, j5mbvalid;
	bool const nowait(get_jumper(name, get_feature, "prog.NO WAIT", nowaitvalid));
	bool const wait(get_jumper(name, get_feature, "prog.1 WAIT", waitvalid));
	bool const j1m(get_jumper(name, get_feature, "prog.1M", j1mvalid));
	bool const j5ma(get_jumper(name, get_feature, "prog.5Ma", j5mavalid));
	bool const j4m8m(get_jumper(name, get_feature, "prog.4M.8M", j4m8mvalid));
	bool const j5mb(get_jumper(name, get_feature, "prog.5Mb", j5mbvalid));

	// ROMWAIT jumpers
	if (!set_romwait(name, nowait, wait, nowaitvalid, waitvalid))
		badjumpers = true;

	// P ROM configuration jumpers
	bool have_p_rom(m_p_rom && (m_p_rom.bytes() > (offs_t(1) << 16)));
	bool p1global(false);
	bool p2global(false);
	if (j1mvalid && j5mavalid)
	{
		if (j1m && j5ma)
		{
			osd_printf_error("[%s] %s: /A19 shorted to GND - check jumpers\n", tag(), name);
			badjumpers = true;
		}
		else if (!j1m && !j5ma)
		{
			if (have_p_rom && (m_p_rom.bytes() & (m_p_rom.bytes() - 1)))
			{
				osd_printf_error("[%s] %s: P2 /CE floating - check jumpers\n", tag(), name);
				badjumpers = true;
			}
			p2global = true;
		}
		else
		{
			p2global = j1m;
		}
	}
	else if (j1mvalid)
	{
		p2global = j1m;
	}
	else if (j5mavalid)
	{
		p2global = !j5ma;
	}
	if (j4m8mvalid && j5mbvalid)
	{
		if (j4m8m && j5mb)
		{
			osd_printf_error("[%s] %s: A19 shorted to GND - check jumpers\n", tag(), name);
			badjumpers = true;
		}
		else if (!j4m8m && !j5mb)
		{
			if (have_p_rom && ((m_p_rom.bytes() >> 1) > (offs_t(1) << 16)))
				osd_printf_warning("[%s] %s: P1 /CE floating - check jumpers\n", tag(), name);
			p1global = true;
		}
		else
		{
			p1global = j4m8m;
		}
	}
	else if (j4m8mvalid)
	{
		p1global = j4m8m;
	}
	else if (j5mbvalid)
	{
		p1global = !j5mb;
	}

	// adjust address decoding for P ROM configuration and check size
	if (p1global)
	{
		desc->p_rom_mask = 0x00000;
	}
	else if (have_p_rom)
	{
		if (p2global && ((m_p_rom.bytes() >> 1) <= (offs_t(1) << 16)))
			desc->p_rom_mask = 0x00000;
		else if ((m_p_rom.bytes() >> 1) >= (offs_t(1) << 19))
			desc->p_rom_limit >>= 1;
	}

	// check P ROM size
	if (have_p_rom)
	{
		offs_t const words(m_p_rom.bytes() >> 1);
		offs_t const excess(words & ((offs_t(1) << 18) - 1));
		if (p1global || p2global)
		{
			if (m_p_rom.bytes() & (m_p_rom.bytes() - 1))
			{
				osd_printf_error(
						"[%s] %s: P ROM size 0x%X unsupported when jumpered for %s only\n",
						tag(), name, m_p_rom.bytes(), p1global ? "P1" : "P2");
				badjumpers = true;
			}
		}
		else
		{
			if ((excess > (offs_t(1) << 16)) || (excess & (excess - 1)))
			{
				osd_printf_error("[%s] %s: P ROM size 0x%X unsupported\n", tag(), name, m_p_rom.bytes());
				badjumpers = true;
			}
		}
	}

	// no unified V ROM
	if (m_v_rom)
	{
		osd_printf_error("[%s] %s: unified V ROM unsupported\n", tag(), name);
		badjumpers = true;
	}

	// check V1 ROM size
	if (m_v1_rom && (m_v1_rom.bytes() <= desc->v1_rom_max))
	{
		offs_t const excess(m_v1_rom.bytes() & ((offs_t(1) << 20) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: V1 ROM size 0x%X unsupported\n", tag(), name, m_v1_rom.bytes());
			badjumpers = true;
		}
	}

	// check V2 ROM size
	if (m_v2_rom && (m_v2_rom.bytes() <= desc->v2_rom_max))
	{
		offs_t const excess(m_v2_rom.bytes() & ((offs_t(1) << 20) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: V2 ROM size 0x%X unsupported\n", tag(), name, m_v2_rom.bytes());
			badjumpers = true;
		}
	}

	return badjumpers ? nullptr : std::move(desc);
}


//----------------------------------
// CHA PCB configuration
//----------------------------------

std::unique_ptr<flat_rom_device::cha_desc const> flat_rom_device::config_cha_32(char const *name, get_feature_func const &get_feature)
{
	std::unique_ptr<cha_desc> desc(new cha_desc);

	desc->c_rom_max = 0x0100000;
	desc->c_rom_offset = 0x0000000;
	desc->c_rom_mask = 0x00c0000;

	desc->s_rom_max = 0x20000;
	desc->s_rom_offset = 0x00000;
	desc->s_rom_mask = 0x00000;

	desc->m1_rom_max = 0x40000;
	desc->m1_rom_offset = 0x20000;
	desc->m1_rom_mask = 0x0000;

	bool badjumpers(false);
	bool j1valid, j2valid, j3valid, j4valid;
	bool const j1(get_jumper(name, get_feature, "cha.J1", j1valid));
	bool const j2(get_jumper(name, get_feature, "cha.J2", j2valid));
	bool const j3(get_jumper(name, get_feature, "cha.J3", j3valid));
	bool const j4(get_jumper(name, get_feature, "cha.J4", j4valid));

	// check for shorts with the M1 configuration jumpers
	bool pin2gnd(false), pin2float(false);
	bool pin24gnd(false), pin24float(false);
	if (j2valid)
	{
		pin24gnd = j2;
		if (j1valid)
		{
			if (j1 && j2)
			{
				osd_printf_error("[%s] %s: SDMRD shorted to GND - check jumpers\n", tag(), name);
				badjumpers = true;
			}
			else if (!j1 && !j2)
			{
				pin24float = true;
			}
		}
	}
	else if (j1valid)
	{
		pin24gnd = !j1;
	}
	if (j3valid)
	{
		pin2gnd = j3;
		if (j4valid)
		{
			if (j3 && j4)
			{
				if (!j1valid || !j2valid || !j1 || !j2)
					osd_printf_error("[%s] %s: SDMRD shorted to GND - check jumpers\n", tag(), name);
				badjumpers = true;
			}
			else if (!j3 && !j4)
			{
				pin2float = true;
			}
		}
	}
	else if (j4valid)
	{
		pin2gnd = !j4;
	}

	// check whether the jumpers are sane
	if (pin24gnd && ((m_m1_rom && (m_m1_rom.bytes() != (offs_t(1) << 17))) || pin2gnd))
	{
		osd_printf_error("[%s] %s: M1 /OE always asserted - may cause damage, check jumpers\n", tag(), name);
		badjumpers = true;
	}
	if (pin24float && ((m_m1_rom && (m_m1_rom.bytes() != (offs_t(1) << 17))) || pin2float))
	{
		osd_printf_error("[%s] %s: M1 /OE floating - may cause damage, check jumpers\n", tag(), name);
		badjumpers = true;
	}
	if (pin2float && m_m1_rom && (m_m1_rom.bytes() > (offs_t(1) << 17)))
		osd_printf_warning("[%s] %s: M1 A16 floating - check jumpers\n", tag(), name);

	// check C ROM size
	if (m_c_rom && ((m_c_rom.bytes() >> 2) <= desc->c_rom_max))
	{
		offs_t const words(m_c_rom.bytes() >> 2);
		offs_t const excess(words & ((offs_t(1) << 18) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: C ROM size 0x%X unsupported\n", tag(), name, m_c_rom.bytes());
			badjumpers = true;
		}
	}

	// check S ROM size
	if (m_s_rom && (m_s_rom.bytes() <= desc->s_rom_max))
	{
		if (m_s_rom.bytes() & (m_s_rom.bytes() - 1))
		{
			osd_printf_error("[%s] %s: S ROM size 0x%X unsupported\n", tag(), name, m_s_rom.bytes());
			badjumpers = true;
		}
	}

	// check M1 ROM size
	if (m_m1_rom && (m_m1_rom.bytes() <= desc->m1_rom_max))
	{
		if (m_m1_rom.bytes() & (m_m1_rom.bytes() - 1))
		{
			osd_printf_error("[%s] %s: M1 ROM size 0x%X unsupported\n", tag(), name, m_m1_rom.bytes());
			badjumpers = true;
		}
	}

	return badjumpers ? nullptr : std::move(desc);
}

std::unique_ptr<flat_rom_device::cha_desc const> flat_rom_device::config_cha_8mb(char const *name, get_feature_func const &get_feature)
{
	std::unique_ptr<cha_desc> desc(new cha_desc);

	desc->c_rom_max = 0x0100000;
	desc->c_rom_offset = 0x0000000;
	desc->c_rom_mask = 0x0080000;

	desc->s_rom_max = 0x20000;
	desc->s_rom_offset = 0x00000;
	desc->s_rom_mask = 0x00000;

	desc->m1_rom_max = 0x40000;
	desc->m1_rom_offset = 0x20000;
	desc->m1_rom_mask = 0x0000;

	bool badjumpers(false);
	bool j1valid, j2valid, j3valid, j4valid;
	bool const j1(get_jumper(name, get_feature, "cha.J1", j1valid));
	bool const j2(get_jumper(name, get_feature, "cha.J2", j2valid));
	bool const j3(get_jumper(name, get_feature, "cha.J3", j3valid));
	bool const j4(get_jumper(name, get_feature, "cha.J4", j4valid));

	// check for shorts with the M1 configuration jumpers
	bool pin2gnd(false), pin2float(false);
	bool pin24gnd(false), pin24float(false);
	if (j2valid)
	{
		pin24gnd = j2;
		if (j1valid)
		{
			if (j1 && j2)
			{
				osd_printf_error("[%s] %s: SDMRD shorted to GND - check jumpers\n", tag(), name);
				badjumpers = true;
			}
			else if (!j1 && !j2)
			{
				pin24float = true;
			}
		}
	}
	else if (j1valid)
	{
		pin24gnd = !j1;
	}
	if (j3valid)
	{
		pin2gnd = j3;
		if (j4valid)
		{
			if (j3 && j4)
			{
				if (!j1valid || !j2valid || !j1 || !j2)
					osd_printf_error("[%s] %s: SDMRD shorted to GND - check jumpers\n", tag(), name);
				badjumpers = true;
			}
			else if (!j3 && !j4)
			{
				pin2float = true;
			}
		}
	}
	else if (j4valid)
	{
		pin2gnd = !j4;
	}

	// check whether the jumpers are sane
	if (pin24gnd && ((m_m1_rom && (m_m1_rom.bytes() != (offs_t(1) << 17))) || pin2gnd))
	{
		osd_printf_error("[%s] %s: M1 /OE always asserted - may cause damage, check jumpers\n", tag(), name);
		badjumpers = true;
	}
	if (pin24float && ((m_m1_rom && (m_m1_rom.bytes() != (offs_t(1) << 17))) || pin2float))
	{
		osd_printf_error("[%s] %s: M1 /OE floating - may cause damage, check jumpers\n", tag(), name);
		badjumpers = true;
	}
	if (pin2float && m_m1_rom && (m_m1_rom.bytes() > (offs_t(1) << 17)))
		osd_printf_warning("[%s] %s: M1 A16 floating - check jumpers\n", tag(), name);

	// check C ROM size
	if (m_c_rom && ((m_c_rom.bytes() >> 2) <= desc->c_rom_max))
	{
		offs_t const words(m_c_rom.bytes() >> 2);
		offs_t const excess(words & ((offs_t(1) << 19) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] %s: C ROM size 0x%X unsupported\n", tag(), name, m_c_rom.bytes());
			badjumpers = true;
		}
	}

	// check S ROM size
	if (m_s_rom && (m_s_rom.bytes() <= desc->s_rom_max))
	{
		if (m_s_rom.bytes() & (m_s_rom.bytes() - 1))
		{
			osd_printf_error("[%s] %s: S ROM size 0x%X unsupported\n", tag(), name, m_s_rom.bytes());
			badjumpers = true;
		}
	}

	// check M1 ROM size
	if (m_m1_rom && (m_m1_rom.bytes() <= desc->m1_rom_max))
	{
		if (m_m1_rom.bytes() & (m_m1_rom.bytes() - 1))
		{
			osd_printf_error("[%s] %s: M1 ROM size 0x%X unsupported\n", tag(), name, m_m1_rom.bytes());
			badjumpers = true;
		}
	}

	return badjumpers ? nullptr : std::move(desc);
}


//----------------------------------
// configuration helpers
//----------------------------------

bool flat_rom_device::set_2m_mirror(
		char const *name,
		char const *part,
		optional_region_ptr<u8> &region,
		bool high,
		bool live,
		bool high_valid,
		bool live_valid)
{
	bool mirror(false);
	bool need_a18(region && (region.bytes() >= (offs_t(1) << 19)));
	bool need_p(region && (region.bytes() == (offs_t(1) << 18)));

	if (high_valid && live_valid)
	{
		if (high && live)
		{
			osd_printf_error("[%s] %s: %s A18 latch shorted to VCC - check jumpers\n", tag(), name, part);
			return false;
		}
		else if (!high && !live)
		{
			if (need_a18)
				osd_printf_warning("[%s] %s: %s1 A18 floating - check jumpers\n", tag(), name, part);
			else if (need_p)
				osd_printf_warning("[%s] %s: %s1 /P floating - check jumpers\n", tag(), name, part);
			mirror = true;
		}
		else
		{
			mirror = high;
		}
	}
	else if (high_valid)
	{
		mirror = high;
	}
	else if (live_valid)
	{
		mirror = !live;
	}

	if (need_p && !mirror)
		osd_printf_warning("[%s] %s: %s1 /P tied to A18 latch - may cause damage, check jumpers\n", tag(), name, part);

	if (need_a18 && mirror)
	{
		LOG("%s: apply %s low 2M mirror\n", name, part);
		std::copy_n(&region[1 << 18], 1 << 18, &region[0]);
	}

	return true;
}

bool flat_rom_device::set_romwait(char const *name, bool high, bool low, bool high_valid, bool low_valid)
{
	bool romwait(false);

	if (high_valid && low_valid)
	{
		if (high && low)
		{
			osd_printf_error("[%s] %s: VCC shorted to GND - check jumpers\n", tag(), name);
			return false;
		}
		else if (!high && !low)
		{
			osd_printf_warning("[%s] %s: ROMWAIT floating - check jumpers\n", tag(), name);
		}
		else
		{
			romwait = low;
		}
	}
	else if (high_valid)
	{
		romwait = !high;
	}
	else if (low_valid)
	{
		romwait = low;
	}

	if (romwait)
	{
		// TODO: P ROM wait state support
		osd_printf_warning("[%s] %s: P ROM wait states not supported\n", tag(), name);
	}

	return true;
}

bool flat_rom_device::get_jumper(char const *name, get_feature_func const &get_feature, char const *jumper, bool &valid)
{
	valid = false;
	char const *const feature(get_feature(jumper));
	if (!feature || !*feature)
	{
		osd_printf_warning("[%s] %s: jumper %s value not specified\n", tag(), name, jumper);
		return false;
	}

	try
	{
		int const value(std::stoi(feature));
		if ((0 == value) || (1 == value))
		{
			valid = true;
			return bool(value);
		}
	}
	catch (std::exception const &)
	{
	}
	osd_printf_warning("[%s] %s: jumper %s value \"%s\" is invalid\n", tag(), name, jumper, feature);
	return false;
}

} } } // namespace bus::neogeo::cart
