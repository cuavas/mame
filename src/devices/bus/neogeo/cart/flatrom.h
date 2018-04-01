// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    AES/MVS flat ROM cartridges

    NEO-MVS PROG-8MB:
    * 8 × 74LS174 hex D-type flip-flop (latch V1 and V2 ROM addresses)
    * 1 × 74LS04 hex inverter (latch phasing, select ROM chips)
    * J1/J2 tie ROMWAIT high/low
    * J3/J4 tie P1 /CE to GND or A19
    * PWAIT0/PWAIT1/PDTACT tied high
    * 4M P ROM as 1×256k×16 or low half of 1×512k×16 (P1)
    * 5M P ROM as 1×256k×16 plus 1×64k×16 mirrored every 128k×16 (P1/P2)
    * 8M P ROM as 1×512k×16 (P1)
    * 16M V1 ROM as 21M×8 (V11/V12) mirrored every 2M×8
    * 16M V2 ROM as 21M×8 (V21/V22) mirrored every 2M×8

    NEO-MVS PROG-EP:
    * 8 × 74LS174 hex D-type flip-flop (latch V1 and V2 ROM addresses)
    * 1 × 74LS139 dual 2-to-4 decoder (select V1 and V2 ROM chips)
    * 1 × 74LS04 hex inverter (latch phasing)
    * J1/J2 tie ROMWAIT high/low
    * J3/J4 enable/disable V2 low 2M mirror (for V22 as 256k×8)
    * J5/J6 enable/disable V1 low 2M mirror (for V21 as 256k×8)
    * PWAIT0/PWAIT1/PDTACT tied high
    * 8M P ROM as 2×256k×16 (P1/P2) or 2×512×8 (PODD/PEVEN)
    * 16M V1 ROM as 4×512k×8 (V11/V12/V13/V14) mirrored every 2M×8
    * 16M V2 ROM as 4×512k×8 (V21/V22/V23/V24) mirrored every 2M×8

    NEO-MVS PROG-NAM:
    * 8 × 74LS174 hex D-type flip-flop (latch V1 and V2 ROM addresses)
    * 1 × 74LS139 dual 2-to-4 decoder (select P and V2 ROM chips)
    * 1 × 74LS04 hex inverter (latch phasing)
    * J1/J2 select 2×256k×16/4×128k×16 P ROM
    * J3/J4 tie ROMWAIT high/low
    * PWAIT0/PWAIT1/PDTACT tied high
    * 8M P ROM as 2×256k×16 (P1/P2) or 4×128k×16 (P1/P3/P2/P4)
    * 4M V1 ROM as 1×512k×8 (V11) mirrored every 512k×8
    * 12M V2 ROM as 3×512k×8 (V21/V22/V23) mirrored every 2M×8

    NEO-MVS PROG42G:
    * PCM (V ROM address/data latch/buffer, provides inverted P A19)
    * 4.7kΩ pullup resistor between A19 and VCC
    * PWAIT0/PWAIT1/PDTACK tied high
    * NO WAIT/1 WAIT tie ROMWAIT high/low
    * 1M/5M tie P2 /CE to GND or /A19
    * 8M/4M.5M tie P1 /CE to GND or A19
    * PWAIT0/PWAIT1/PDTACT tied high
    * 1M P ROM as 1×64k×16 mirrored every 128k×16 (P1)
    * 4M P ROM as 1×256k×16 or low half of 1×512k×16 (P1)
    * 5M P ROM as 1×256k×16 plus 1×64k×16 mirrored every 128k×16 (P1/P2)
    * 8M P ROM as 1×512k×16 (P1)
    * 32M V ROM as 4×1M×8 (V1/V2/V3/V4) mirrored every 4M×8?

    NEO-MVS PROG42G-1:
    * Only two of four VCC contacts present on each side of PCB
    * Same active components and jumpers as NEO-MVS PROG42G
    * Space for 4.7kΩ pullup resistor packs on YM2610 lines
    * Space for 330pF suppression capacitors on YM2610 address/data lines
    * Additional components may not be populated
    * May use new layout with ROMs perpendicular to edge connector

    NEO-MVS PROG8M42:
    * 7 × 74LS174 hex D-type flip-flop (latch V1 and V2 ROM addresses)
    * 1 × 74LS175 quad D-type flip-flop (latch V ROM chip selects)
    * 1 × 74LS04 hex inverter (latch phasing and P ROM chip select)
    * 4.7kΩ pullup resistor between A19 and VCC (may be unpopulated)
    * PWAIT0/PWAIT1/PDTACK tied high
    * NO WAIT/1 WAIT tie ROMWAIT high/low
    * 1M/5M tie P2 /CE to GND or /A19
    * 4M.8M/5M tie P1 /CE to GND or A19
    * 1M P ROM as 1×64k×16 mirrored every 128k×16 (P1)
    * 4M P ROM as 1×256k×16 or low half of 1×512k×16 (P1)
    * 5M P ROM as 1×256k×16 plus 1×64k×16 mirrored every 128k×16 (P1/P2)
    * 8M P ROM as 1×512k×16 (P1)
    * 16M V1 ROM as 2×1M×8 (V11/V12) mirrored every 2M×8
    * 16M V2 ROM as 2×1M×8 (V21/V22) mirrored every 2M×8

    NEO-MVS CHA-32:
    * 4 × 74LS273 octal D-type flip-flop (latch C and S ROM addresses)
    * 1 × 74LS174 hex D-type flip-flop (latch C ROM low addres bits)
    * 1 × 74LS139 dual 2-to-4 decoder (select C ROM chips)
    * J1/J2 tie M1 pin 24 to SDMRD/GND (27C010/27C301)
    * J3/J4 tie M1 pin 2 to GND/SDMRD (27C010/27C301)
    * 32M C ROM as 8×256k×16 (C1/C2/C3/C4/C5/C6/C7/C8) mirrored every 1M×32
    * 1M S ROM as 128k×8 (S1)
    * 512k M1 ROM as 64k×8 (supports oversize ROM up to 256k×8)

    NEO-MVS CHA-8M:
    * 4 × 74LS273 octal D-type flip-flop (latch C and S ROM addresses)
    * 1 × 74LS175 quad D-type flip-flop (latch C ROM chip selects)
    * J1/J2 tie M1 pin 24 to SDMRD/GND (27C010/27C301)
    * J3/J4 tie M1 pin 2 to GND/SDMRD (27C010/27C301)
    * 32M C ROM as 4×512k×16 (C1/C2/C3/C4) mirrored every 1M×32
    * 1M S ROM as 128k×8 (S1) - supports ROM or 27C301 EPROM
    * 512k M1 ROM as 64k×8 (supports oversize ROM up to 256k×8)

    J3/J4 and J5/J6 on PROG-EP work by tying pin 32 of V22/V21 to VCC or
    the A18 latch.  On a 256k×8 chip this is NC (ROM) or /P ([E]PROM),
    while on a 512k×8 chip it's A18.  J3/J5 disable programming for
    256k×8 [E]PROMs, and cause the high 2M (256k×8) to be mirrored for
    512k×8 ROMs.  J4/J6 enable full use of 512k×8 ROMs
    but may result in damage to 256k×8 [E]PROMs.

    J1/J2 and J3/J4 on CHA-32 and CHA-8MB select the M1 ROM type.  The
    27C301 (128k×8) has an unusual pinout with /OE on pin 2 and A16 on
    pin 24.  J2 and J4 should be bridged for 27C301 or equivalent; J1
    and J3 should be bridged for 27C512 (64k×8), 27C010 (128k×8),
    27C2001 (256k×8), or equivalents.

***************************************************************************/
#ifndef MAME_BUS_NEOGEO_CART_FLATROM_H
#define MAME_BUS_NEOGEO_CART_FLATROM_H

#pragma once

#include "cart.h"

#include <memory>


namespace bus { namespace neogeo { namespace cart {

class flat_rom_device : public device_t, public device_cart_interface
{
public:
	// construction/destruction
	flat_rom_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

	// software loading
	virtual image_init_result configure_software(get_feature_func const &get_feature) override;

	// 16-bit 68k ROM and I/O
	virtual void install_p_rom(address_space &space, offs_t base) override;

	// 8-bit Z80 ROM
	virtual void install_m1_rom(address_space &space, offs_t base) override;

	// 32-bit and 8-bit graphics ROM
	virtual void get_c_rom_info(u8 const *&base, offs_t &length, offs_t &mirror) override;
	virtual void get_s_rom_info(u8 const *&base, offs_t &length, offs_t &mirror) override;

	// 8-bit ADPCM ROM
	virtual void get_v1_rom_info(u8 const *&base, offs_t &length, offs_t &mirror) override;
	virtual void get_v2_rom_info(u8 const *&base, offs_t &length, offs_t &mirror) override;

protected:
	// board descriptions
	struct prog_desc
	{
		offs_t p_rom_limit = P_MASK;
		offs_t p_rom_max = 0U;
		offs_t p_rom_offset = 0U;
		offs_t p_rom_mask = 0U;
		offs_t v1_rom_limit = V1_MASK;
		offs_t v1_rom_max = 0U;
		offs_t v1_rom_offset = 0U;
		offs_t v1_rom_mask = 0U;
		offs_t v2_rom_limit = V2_MASK;
		offs_t v2_rom_max = 0U;
		offs_t v2_rom_offset = 0U;
		offs_t v2_rom_mask = 0U;
	};
	struct cha_desc
	{
		offs_t c_rom_limit = C_MASK;
		offs_t c_rom_max = 0U;
		offs_t c_rom_offset = 0U;
		offs_t c_rom_mask = 0U;
		offs_t s_rom_limit = S_MASK;
		offs_t s_rom_max = 0U;
		offs_t s_rom_offset = 0U;
		offs_t s_rom_mask = 0U;
		offs_t m1_rom_limit = M1_MASK;
		offs_t m1_rom_max = 0U;
		offs_t m1_rom_offset = 0U;
		offs_t m1_rom_mask = 0U;
	};

	// device_t implementation
	flat_rom_device(machine_config const &mconfig, device_type type, char const *tag, device_t *owner, u32 clock);
	virtual void device_start() override;
	virtual void device_stop() override;

	// mapping helper
	template <unsigned Shift, typename T>
	void install_rom(
			optional_region_ptr<T> &region,
			offs_t decode_limit,
			offs_t decode_mask,
			offs_t decode_offset,
			address_space &space,
			offs_t base,
			char const *name);

	// PCB selection
	virtual std::unique_ptr<prog_desc const> config_prog(char const *name, get_feature_func const &get_feature);
	virtual std::unique_ptr<cha_desc const> config_cha(char const *name, get_feature_func const &get_feature);

	// PROG PCB configuration
	std::unique_ptr<prog_desc const> config_prog_8mb(char const *name, get_feature_func const &get_feature);
	std::unique_ptr<prog_desc const> config_prog_ep(char const *name, get_feature_func const &get_feature);
	std::unique_ptr<prog_desc const> config_prog_nam(char const *name, get_feature_func const &get_feature);
	std::unique_ptr<prog_desc const> config_prog42g(char const *name, get_feature_func const &get_feature);
	std::unique_ptr<prog_desc const> config_prog8m42(char const *name, get_feature_func const &get_feature);

	// CHA PCB configuration
	std::unique_ptr<cha_desc const> config_cha_32(char const *name, get_feature_func const &get_feature);
	std::unique_ptr<cha_desc const> config_cha_8mb(char const *name, get_feature_func const &get_feature);

	// configuration helpers
	bool set_2m_mirror(char const *name, char const *part, optional_region_ptr<u8> &region, bool high, bool live, bool high_valid, bool live_valid);
	bool set_romwait(char const *name, bool high, bool low, bool high_valid, bool low_valid);
	bool get_jumper(char const *name, get_feature_func const &get_feature, char const *jumper, bool &valid);

	// PROG
	optional_region_ptr<u16>            m_p_rom;
	optional_region_ptr<u8>             m_v1_rom;
	optional_region_ptr<u8>             m_v2_rom;
	optional_region_ptr<u8>             m_v_rom;

	// CHA
	optional_region_ptr<u8>             m_c_rom;
	optional_region_ptr<u8>             m_s_rom;
	optional_region_ptr<u8>             m_m1_rom;

private:
	// info
	std::unique_ptr<prog_desc const>    m_prog_pcb;
	std::unique_ptr<cha_desc const>     m_cha_pcb;
};

} } } // namespace bus::neogeo::cart


DECLARE_DEVICE_TYPE_NS(NG_CART_FLATROM, bus::neogeo::cart, flat_rom_device)

#endif // MAME_BUS_NEOGEO_CART_FLATROM_H
