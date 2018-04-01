// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    AES/MVS COM MCU cartridges

    Riding Hero, League Bowling, and Thrash Rally support linking up to
    four cabinets to increase the number of players.  The link hardware
    is contained entirely in the cartridge.  The cartridge has a pair of
    3.5mm TRS jacks labelled IN and OUT, however they're wired in
    parallel and functionally identical.

    The MCU is an HD6301V1P with P20, P21 and P22 (PC0/PC1/PC2) tied
    high via a 10kΩ resistor to select single-chip mode.  The /STBY,
    /NMI and /IRQ1 pins are all tied high.  The EXTAL pin is driven by
    the 4MB clock (4MHz) with a 2.2kΩ pullup resistor.  The /RES pin is
    driven by the RESET signal.

    The link itself uses RS-485 signalling, with A on the tip, B on the
    ring, and GND on the sleeve.  Framing is 1 start bit, 8 data bits,
    and 1 stop bit, with NRZ signalling.  The line transceiver input and
    output are connected to P23 and P24.  The driver is enabled by P10,
    while the receiver is always enabled.

    To make operation simpler, each has a 680Ω resistor connected from A
    to B and a 2.2kΩ resistor connected from B to GND.  This makes
    terminating the ends of the bus unnecessary, but places a practical
    limit on the number of machines that can be linked.  The games
    support linking four machines, so the load will vary between 680Ω
    and 170Ω across A and B, and between 2.2kΩ and 550Ω from B to GND.
    This is within the limits for the SN75176 driver.

    To send a message the MCU, the main CPU can write to the low byte of
    any word in the PORT range (0x200000-0x2fffff).  The data appears on
    P37-P30 and SC1 (/IS3) is strobed.  The MCU latches the value and
    raises an interrupt on the falling edge of the strobe.  The SLOTCS
    signal from the host system is available to the MCU on P11.

    To read back status from the MCU the main CPU can read from any
    address in the PORT range (0x200000-0x2fffff).  P47-P40 appears on
    D7-D0 if the low byte is read, and P17-P4 appears inverted on D11-D8
    if the high byte is read.  The MCU is not aware if/when the main CPU
    reads status.

    PWAIT1 is tied low to enable wait state generation for PORT access
    (0x200000-0x2fffff).

***************************************************************************/

#include "emu.h"
#include "commcu.h"

#include "cpu/m6800/m6801.h"

//#define VERBOSE 1
//#define LOG_OUTPUT_STREAM std::cerr
#include "logmacro.h"


DEFINE_DEVICE_TYPE_NS(NG_CART_COMMCU, bus::neogeo::cart, com_mcu_device, "ng_cart_commcu", "Neo-Geo AES/MVS COM MCU Cartridge")


namespace bus { namespace neogeo { namespace cart {

//----------------------------------
// construction/destruction
//----------------------------------

com_mcu_device::com_mcu_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock)
	: flat_rom_device(mconfig, NG_CART_COMMCU, tag, owner, clock)
	, device_serial_interface(mconfig, *this)
	, m_com_mcu(*this, "com")
	, m_link(*this, "link")
	, m_com_rom(*this, "^com", 0x2000)
	, m_poll(nullptr)
	, m_p1_in(0xff)
	, m_p1_out(0xff)
	, m_p3_in(0xff)
	, m_p4_out(0xff)
	, m_de(1)
	, m_d(1)
	, m_r(1)
{
}


//----------------------------------
// software loading
//----------------------------------

image_init_result com_mcu_device::configure_software(get_feature_func const &get_feature)
{
	// handle main ROM regions
	image_init_result result(flat_rom_device::configure_software(get_feature));

	// deal with the MCU ROM
	if (m_com_rom)
	{
		if (m_com_rom.bytes() != 0x2000)
		{
			osd_printf_error("[%s] COM ROM size 0x%X unsupported\n", tag(), m_com_rom.bytes());
			result = image_init_result::FAIL;
		}
		else
		{
			m_com_mcu->space(AS_PROGRAM).install_rom(0xe000, 0xffff, &m_com_rom[0]);
		}
	}

	return result;
}


//----------------------------------
// 16-bit 68k I/O
//----------------------------------

void com_mcu_device::install_port_r(address_space &space, offs_t base)
{
	space.unmap_read(base, base | 0x0fffff);
	space.install_read_handler(base | 0x000000, base | 0x0fffff, read8_delegate(FUNC(com_mcu_device::com_lower_r), this), 0x00ff);
	space.install_read_handler(base | 0x000000, base | 0x0fffff, read8_delegate(FUNC(com_mcu_device::com_upper_r), this), 0xff00);
}

void com_mcu_device::install_port_w(address_space &space, offs_t base)
{
	space.unmap_write(base, base | 0x0fffff);
	space.install_write_handler(base | 0x000000, base | 0x0fffff, write8_delegate(FUNC(com_mcu_device::com_lower_w), this), 0x00ff);
}


//----------------------------------------------
// input signals
//----------------------------------------------

WRITE_LINE_MEMBER(com_mcu_device::slotcs_w)
{
	LOG("SLOTCS = %d\n", state);
	m_p1_in = (m_p1_in & 0xfd) | (state ? 0x02 : 0x00);
}


//----------------------------------
// device_t implementation
//----------------------------------

MACHINE_CONFIG_START(com_mcu_device::device_add_mconfig)
	// HD6301V1 in single-chip mode
	MCFG_CPU_ADD("com", HD6301, 4'000'000) // FIXME: should be derived
	MCFG_CPU_PROGRAM_MAP(com_mcu_mem);
	MCFG_CPU_IO_MAP(com_mcu_io);

	// cooked communication link
	MCFG_DEVICE_ADD("link", BITBANGER, 0)
MACHINE_CONFIG_END

void com_mcu_device::device_start()
{
	flat_rom_device::device_start();

	m_poll = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(com_mcu_device::poll_link), this));
	poll_link(nullptr, 0);

	save_item(NAME(m_p1_in));
	save_item(NAME(m_p1_out));
	save_item(NAME(m_p3_in));
	save_item(NAME(m_p4_out));
	save_item(NAME(m_de));
	save_item(NAME(m_d));
	save_item(NAME(m_r));
}

void com_mcu_device::device_reset()
{
	flat_rom_device::device_reset();

	// Baud divisors:
	// * /6 divider for 4MB clock
	// * /4 MCU internal clock divider
	// * /16 MCU programmed SCI clock divider
	set_data_frame(1, 8, PARITY_NONE, STOP_BITS_1);
	set_rate(24'000'000 / 6 / 4 / 16); // TODO: should be a derived clock
	receive_register_reset();
	transmit_register_reset();
}


//----------------------------------
// serial handlers
//----------------------------------

void com_mcu_device::tra_callback()
{
	m_r = transmit_register_get_data_bit();
}

void com_mcu_device::tra_complete()
{
	poll_link(nullptr, 0);
}

void com_mcu_device::rcv_complete()
{
	receive_register_extract();
	u8 const byte(get_received_char());
	LOG("MCU transmit 0x%02X\n", byte);
	m_link->output(byte);
}


//----------------------------------
// host communication handlers
//----------------------------------

READ8_MEMBER(com_mcu_device::com_lower_r)
{
	LOG("Host read P47-P40 = 0x%02X\n", m_p4_out);
	return m_p4_out;
}

READ8_MEMBER(com_mcu_device::com_upper_r)
{
	LOG("Host read P17-P14 = 0x%01X\n", (m_p1_out >> 4) & 0x0f);
	return ((space.unmap() >> 8) & 0xf0) | (~(m_p1_out >> 4) & 0x0f);
}

WRITE8_MEMBER(com_mcu_device::com_lower_w)
{
	LOG("Host write P37-P30 = 0x%02X\n", data);
	m_p3_in = data;
	m_com_mcu->set_input_line(M6801_SC1_LINE, HOLD_LINE);
	m_com_mcu->set_input_line(M6801_IRQ_LINE, HOLD_LINE); // FIXME: massive hack - the MCU should take an interrupt on SC1 strobe
}


//----------------------------------
// MCU I/O handlers
//----------------------------------

READ8_MEMBER(com_mcu_device::mcu_p1_r)
{
	LOG("MCU read P1 = 0x%02X\n", m_p1_in);
	return m_p1_in;
}

READ8_MEMBER(com_mcu_device::mcu_p2_r)
{
	u8 const data = 0xf7 | ((!m_r || (m_de && !m_d))  ? 0x00 : 0x08);
	LOG("MCU read P2 = 0x%02X\n", data);
	return data;
}

READ8_MEMBER(com_mcu_device::mcu_p3_r)
{
	LOG("MCU read P3 = 0x%02X\n", m_p3_in);
	return m_p3_in;
}

WRITE8_MEMBER(com_mcu_device::mcu_p1_w)
{
	LOG("MCU write P1 = 0x%02X\n", data);
	machine().scheduler().synchronize(timer_expired_delegate(FUNC(com_mcu_device::set_p1_out), this), unsigned(data));
	m_de = BIT(data, 0);
	rx_w((m_de && !m_d) ? 0 : 1);
}

WRITE8_MEMBER(com_mcu_device::mcu_p2_w)
{
	LOG("MCU write P2 = 0x%02X\n", data);
	m_d = BIT(data, 4);
	rx_w((m_de && !m_d) ? 0 : 1);
}

WRITE8_MEMBER(com_mcu_device::mcu_p4_w)
{
	LOG("MCU write P4 = 0x%02X\n", data);
	machine().scheduler().synchronize(timer_expired_delegate(FUNC(com_mcu_device::set_p4_out), this), unsigned(data));
}

TIMER_CALLBACK_MEMBER(com_mcu_device::poll_link)
{
	u8 byte;
	if (m_link->input(&byte, sizeof(byte)))
	{
		LOG("Link receive 0x%02X\n", byte);
		transmit_register_setup(byte);
	}
	else
	{
		m_poll->adjust(attotime::from_hz(24'000'000 / 6 / 4 / 16)); // TODO: should be a derived clock
	}
}


//----------------------------------
// synchronisation callbacks
//----------------------------------

void com_mcu_device::set_p1_out(void *ptr, s32 param)
{
	m_p1_out = u8(u32(param));
}

void com_mcu_device::set_p4_out(void *ptr, s32 param)
{
	m_p4_out = u8(u32(param));
}


//----------------------------------
// MCU address maps
//----------------------------------

void com_mcu_device::com_mcu_mem(address_map &map)
{
	map(0x0000, 0x001f).rw("com", FUNC(hd6301_cpu_device::m6801_io_r), FUNC(hd6301_cpu_device::m6801_io_w));
	map(0x0080, 0x00ff).ram();
}

void com_mcu_device::com_mcu_io(address_map &map)
{
	map.unmap_value_high();
	map(M6801_PORT1, M6801_PORT1).rw(this, FUNC(com_mcu_device::mcu_p1_r), FUNC(com_mcu_device::mcu_p1_w));
	map(M6801_PORT2, M6801_PORT2).rw(this, FUNC(com_mcu_device::mcu_p2_r), FUNC(com_mcu_device::mcu_p2_w));
	map(M6801_PORT3, M6801_PORT3).r(this, FUNC(com_mcu_device::mcu_p3_r));
	map(M6801_PORT4, M6801_PORT4).w(this, FUNC(com_mcu_device::mcu_p4_w));
}


//----------------------------------
// PCB configuration
//----------------------------------

std::unique_ptr<flat_rom_device::prog_desc const> com_mcu_device::config_prog(char const *name, get_feature_func const &get_feature)
{
	if (!std::strcmp(name, "NEO-MVS PROG-HERO"))
	{
		return config_prog_hero(get_feature);
	}
	else
	{
		osd_printf_error("[%s] cannot load software: unsupported PCB \"%s\"\n", tag(), name);
		return nullptr;
	}
}

std::unique_ptr<flat_rom_device::prog_desc const> com_mcu_device::config_prog_hero(get_feature_func const &get_feature)
{
	std::unique_ptr<prog_desc> desc(new prog_desc);

	desc->p_rom_max = 0x40000;
	desc->p_rom_offset = 0x00000;
	desc->p_rom_mask = 0x00000;

	desc->v1_rom_max = 0x0200000;
	desc->v1_rom_offset = 0x0000000;
	desc->v1_rom_mask = 0x180000;

	desc->v2_rom_max = 0x0200000;
	desc->v2_rom_offset = 0x0000000,
	desc->v2_rom_mask = 0x180000;

	// get jumper values
	bool badjumpers(false);
	bool j1valid, j2valid, j3valid, j4valid, j5valid, j6valid;
	bool const j1(get_jumper("PROG-EP", get_feature, "prog.J1", j1valid));
	bool const j2(get_jumper("PROG-EP", get_feature, "prog.J2", j2valid));
	bool const j3(get_jumper("PROG-EP", get_feature, "prog.J3", j3valid));
	bool const j4(get_jumper("PROG-EP", get_feature, "prog.J4", j4valid));
	bool const j5(get_jumper("PROG-EP", get_feature, "prog.J5", j5valid));
	bool const j6(get_jumper("PROG-EP", get_feature, "prog.J6", j6valid));

	// ROMWAIT jumpers
	if (!set_romwait("PROG-EP", j1, j2, j1valid, j2valid))
		badjumpers = true;

	// V ROM mirroring jumpers
	set_2m_mirror("PROG-EP", "V2", m_v2_rom, j3, j4, j3valid, j4valid);
	set_2m_mirror("PROG-EP", "V1", m_v1_rom, j5, j6, j5valid, j6valid);

	// check P ROM size
	if (m_p_rom && ((m_p_rom.bytes() >> 1) <= desc->p_rom_max))
	{
		if (m_p_rom.bytes() & (m_p_rom.bytes() - 1))
		{
			osd_printf_error("[%s] PROG-EP: P ROM size 0x%X unsupported", tag(), m_p_rom.bytes());
			badjumpers = true;
		}
	}

	// check V1 ROM size
	if (m_v1_rom && (m_v1_rom.bytes() <= desc->v1_rom_max))
	{
		offs_t const excess(m_v1_rom.bytes() & ((offs_t(1) << 19) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] PROG-NAM: V1 ROM size 0x%X unsupported\n", tag(), m_v1_rom.bytes());
			badjumpers = true;
		}
	}

	// check V2 ROM size
	if (m_v2_rom && (m_v2_rom.bytes() <= desc->v2_rom_max))
	{
		offs_t const excess(m_v2_rom.bytes() & ((offs_t(1) << 19) - 1));
		if (excess & (excess - 1))
		{
			osd_printf_error("[%s] PROG-NAM: V2 ROM size 0x%X unsupported\n", tag(), m_v2_rom.bytes());
			badjumpers = true;
		}
	}

	return std::move(desc);
}

} } } // namespace bus::neogeo::cart
