// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    AES/MVS banked M1 ROM cartridges

    The NEO-ZMC (Z80 Memory Controller?) chip expands the Z80's sixteen
    address lines to nineteen, increasing the M1 ROM capacity from 496k
    (62k×8 because RAM covers 2k×8) to 4M (512k×8).  The memory space is
    split into six regions according to the number of leading one bits:
    * 0x0000-0x7fff (32k×8) always addresses low 32k×8 of ROM
    * 0x8000-0xbfff (16k×8) addresses one of 32 pages
    * 0xc000-0xdfff (8k×8) addresses one of 64 pages
    * 0xe000-0xefff (4k×8) addresses one of 128 pages
    * 0xf000-0xf7ff (2k×8) addresses one of 256 pages
    * 0xf800-0xf800 would mirror 0xf000-0xf7ff if RAM wasn't there

    Note that the banked regions and number of latch bits are arranged
    so that any part of the ROM can be mapped into any of the banked
    regions, including parts that are always accessible in the fixed
    area.

    Z80 I/O addresses are decoded on the system board (by the NEO-D0 on
    first-generation chipsets).  Two strobes are provided for reads from
    0x0008-0x000b (SDRD0) and reads from 0x000c-0x000f (SDRD1).  In both
    cases the address bits 0xfffc are ignored.  Since the NEO-ZMC can
    only respond to reads, the least significant bits of the address are
    used to specify the banked region to set, and the upper byte of the
    address is used to specify a page:
    * 0x8000-0xbfff: ---ppppp ----1011
    * 0xc000-0xdfff: --pppppp ----1010
    * 0xe000-0xefff: -ppppppp ----1001
    * 0xf000-0xf7ff: pppppppp ----1000

    To switch pages from Z80 code, store the page number in B, the
    region selector in C, and exectute a register-indirect IN
    instruction (e.g. IN A,(C) which won't affect flags).  Despite
    appearances, B appears on the most significant byte of the address
    bus.  The NEO-ZMC can't drive the data bus, so the result will be an
    open-bus read.

    The NEO-ZMC has no reset input and no access to the SLOTCS line, so
    the selected pages are not affected by resetting the system or
    switching games.

***************************************************************************/

#include "emu.h"
#include "m1zmc.h"

#include <utility>

//#define VERBOSE 1
//#define LOG_OUTPUT_STREAM std::cerr
#include "logmacro.h"


DEFINE_DEVICE_TYPE_NS(NG_CART_M1ZMC, bus::neogeo::cart, m1_zmc_device, "ng_cart_m1_zmc", "Neo-Geo AES/MVS Banked M1 ROM Cartridge")


namespace bus { namespace neogeo { namespace cart {

//----------------------------------
// construction/destruction
//----------------------------------

m1_zmc_device::m1_zmc_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock)
	: flat_rom_device(mconfig, NG_CART_M1ZMC, tag, owner, clock)
	, device_memory_interface(mconfig, *this)
	, m_space_config("sdrom", ENDIANNESS_LITTLE, 8, 16, 0, address_map_constructor(FUNC(m1_zmc_device::sdrom_map), this))
	, m_m1_banks(*this, "^m1bank%u", 0U)
{
}


//----------------------------------
// software loading
//----------------------------------

image_init_result m1_zmc_device::configure_software(get_feature_func const &get_feature)
{
	// handle main ROM regions
	image_init_result result(flat_rom_device::configure_software(get_feature));
	if (image_init_result::PASS != result)
		return result;

	// set up pages
	if (m_m1_rom)
	{
		offs_t const total(m_m1_rom.bytes());
		unsigned bits(8);
		for (unsigned bank = 0; m_m1_banks.size() > bank; ++bank, --bits)
		{
			offs_t const stride(offs_t(1) << (19 - bits));
			offs_t const rep(total / stride);
			for (offs_t start = 0; (offs_t(1) << bits) > start; start += rep)
			{
				LOG("Bank %u map %u-%u stride %04X\n", bank, start, start + rep - 1, stride);
				m_m1_banks[bank]->configure_entries(start, rep, &m_m1_rom[0], stride);
			}
		}
	}

	return result;
}


//----------------------------------
// 8-bit Z80 ROM and I/O
//----------------------------------

void m1_zmc_device::install_m1_rom(address_space &space, offs_t base)
{
	if (m_m1_rom)
	{
		space.unmap_read(base, base | 0xffff);
		space.install_rom(0x0000, 0x7fff, &m_m1_rom[0]);
		space.install_read_bank(0x8000, 0xbfff, m_m1_banks[3]);
		space.install_read_bank(0xc000, 0xdfff, m_m1_banks[2]);
		space.install_read_bank(0xe000, 0xefff, m_m1_banks[1]);
		space.install_read_bank(0xf000, 0xf7ff, 0x0800, m_m1_banks[0]);
	}
}

void m1_zmc_device::install_m1_io(address_space &space, offs_t base)
{
	space.unmap_read(base, base | 0x0003, 0xfff0);
	space.install_read_handler(base, base | 0x0003, 0x0000, 0x00f0, 0xff00, read8_delegate(FUNC(m1_zmc_device::zmc_r), this));
}


//----------------------------------
// device_memory_interface
//----------------------------------

device_memory_interface::space_config_vector m1_zmc_device::memory_space_config() const
{
	return space_config_vector{ std::make_pair(AS_PROGRAM, &m_space_config) };
}


//----------------------------------
// Z80 I/O handlers
//----------------------------------

READ8_MEMBER(m1_zmc_device::zmc_r)
{
	if (!machine().side_effects_disabled())
	{
		unsigned const bank(offset & 0x0003);
		unsigned const page((offset >> 8) & ((offs_t(1) << (8 - bank)) - 1));
		LOG(
				"Bank %u (0x%04X) select page %u (0x%05X & 0x%05X)\n",
				bank,
				~((1U << (12 + bank)) - 1) & 0xffff,
				page,
				page << (11 + bank), m_m1_rom ? (m_m1_rom.bytes() - 1) : 0);
		m_m1_banks[bank]->set_entry(page);
	}
	return space.unmap();
}


//----------------------------------
// dummy memory map
//----------------------------------

void m1_zmc_device::sdrom_map(address_map &map)
{
	map(0x8000, 0xbfff).bankr("m1bank3");
	map(0xc000, 0xdfff).bankr("m1bank2");
	map(0xe000, 0xefff).bankr("m1bank1");
	map(0xf000, 0xf7ff).mirror(0x0800).bankr("m1bank0");
}


//----------------------------------
// PCB configuration
//----------------------------------

std::unique_ptr<flat_rom_device::cha_desc const> m1_zmc_device::config_cha(char const *name, get_feature_func const &get_feature)
{
	if (!std::strcmp(name, "NEO-MVS CHA42G") || !std::strcmp(name, "NEO-MVS CHA42G-1"))
		return config_cha42g(name, get_feature);

	osd_printf_error("[%s] cannot load software: unsupported PCB \"%s\"\n", tag(), name);
	return nullptr;
}

std::unique_ptr<flat_rom_device::cha_desc const> m1_zmc_device::config_cha42g(char const *name, get_feature_func const &get_feature)
{
	std::unique_ptr<cha_desc> desc(new cha_desc);

	desc->c_rom_max = 0x0100000;
	desc->c_rom_offset = 0x0000000;
	desc->c_rom_mask = 0x0080000;

	desc->s_rom_max = 0x20000;
	desc->s_rom_offset = 0x00000;
	desc->s_rom_mask = 0x00000;

	desc->m1_rom_max = 0x80000;
	desc->m1_rom_offset = 0x00000;
	desc->m1_rom_mask = 0x0000;

	bool badjumpers(false);

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
		// it's possible but completely pointless to use smaller ROMs - mirroring within pages isn't worth the effort
		if ((m_m1_rom.bytes() & (m_m1_rom.bytes() - 1)) || (m_m1_rom.bytes() < (offs_t(1) << 14)))
		{
			osd_printf_error("[%s] %s: M1 ROM size 0x%X unsupported\n", tag(), name, m_m1_rom.bytes());
			badjumpers = true;
		}
	}

	return badjumpers ? nullptr : std::move(desc);
}

} } } // namespace bus::neogeo::cart
