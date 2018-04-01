// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    AES/MVS COM MCU cartridges

    NEO-MVS PROG-HERO:
    * 8 × 74LS174 hex D-type flip-flop (latch V1 and V2 ROM addresses)
    * 1 × 74LS139 dual 2-to-4 decoder (select V1 and V2 ROM chips)
    * 1 × 74LS245 octal bus trasceiver (drive D7-D0 with P47-P40 on PORTOEL)
    * 1 × 74LS368 hex 3-state inverter (drive D11-D8 with P17-P14 on PORTOEU)
    * 1 × SN75176 RS422/RS485 trasceiver (communication)
    * OUT/IN jacks connected to transceiver A/B/GND
    * 680Ω resistor across transceiver A/B
    * 2.2kΩ resistor from transciver B to GND
    * J1/J2 tie ROMWAIT high/low
    * J3/J4 enable/disable V2 low 2M mirror (for V22 as 256k×8)
    * J5/J6 enable/disable V1 low 2M mirror (for V21 as 256k×8)
    * PWAIT0 tied high, PWAIT1 tied low, PDTACT tied high
    * 4M P ROM as 1×256k×16 (P1) mirrored every 256k×16
    * 16M V1 ROM as 4×512k×8 (V11/V12/V13/V14) mirrored every 2M×8
    * 16M V2 ROM as 4×512k×8 (V21/V22/V23/V24) mirrored every 2M×8

    J3/J4 and J5/J6 on PROG-HERO work by tying pin 32 of V22/V21 to VCC
    or the A18 latch.  On a 256k×8 chip this is NC (ROM) or /P
    ([E]PROM), while on a 512k×8 chip it's A18.  J3/J5 disable
    programming for 256k×8 [E]PROMs, and cause the high 2M (256k×8) to
    be mirrored for 512k×8 ROMs.  J4/J6 enable full use of 512k×8 ROMs
    but may result in damage to 256k×8 [E]PROMs.

***************************************************************************/
#ifndef MAME_BUS_NEOGEO_CART_COMMCU_H
#define MAME_BUS_NEOGEO_CART_COMMCU_H

#pragma once

#include "flatrom.h"

#include "imagedev/bitbngr.h"

#include <memory>


namespace bus { namespace neogeo { namespace cart {

class com_mcu_device : public flat_rom_device, public device_serial_interface
{
public:
	// construction/destruction
	com_mcu_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

	// software loading
	virtual image_init_result configure_software(get_feature_func const &get_feature) override;

	// 16-bit 68k I/O
	virtual void install_port_r(address_space &space, offs_t base) override;
	virtual void install_port_w(address_space &space, offs_t base) override;

	// input signals
	virtual DECLARE_WRITE_LINE_MEMBER(slotcs_w) override;

protected:
	// device_t implementation
	virtual void device_add_mconfig(machine_config &config) override;
	virtual void device_start() override;
	virtual void device_reset() override;

	// serial handlers
	virtual void tra_callback() override;
	virtual void tra_complete() override;
	virtual void rcv_complete() override;

	// host communication handlers
	DECLARE_READ8_MEMBER(com_lower_r);
	DECLARE_READ8_MEMBER(com_upper_r);
	DECLARE_WRITE8_MEMBER(com_lower_w);

	// MCU I/O handlers
	DECLARE_READ8_MEMBER(mcu_p1_r);
	DECLARE_READ8_MEMBER(mcu_p2_r);
	DECLARE_READ8_MEMBER(mcu_p3_r);
	DECLARE_WRITE8_MEMBER(mcu_p1_w);
	DECLARE_WRITE8_MEMBER(mcu_p2_w);
	DECLARE_WRITE8_MEMBER(mcu_p4_w);
	TIMER_CALLBACK_MEMBER(poll_link);

	// synchronisation callbacks
	void set_p1_out(void *ptr, s32 param);
	void set_p4_out(void *ptr, s32 param);

	// MCU address maps
	void com_mcu_mem(address_map &map);
	void com_mcu_io(address_map &map);

	// PCB configuration
	virtual std::unique_ptr<prog_desc const> config_prog(char const *name, get_feature_func const &get_feature) override;
	std::unique_ptr<prog_desc const> config_prog_hero(get_feature_func const &get_feature);

private:
	// object finders
	required_device<cpu_device>         m_com_mcu;
	required_device<bitbanger_device>   m_link;
	optional_region_ptr<u8>             m_com_rom;

	// MAME resources
	emu_timer   *m_poll;

	// communication ports
	u8  m_p1_in;
	u8  m_p1_out;
	u8  m_p3_in;
	u8  m_p4_out;

	// serial line state
	u8  m_de;
	u8  m_d;
	u8  m_r;
};

} } } // namespace bus::neogeo::cart


DECLARE_DEVICE_TYPE_NS(NG_CART_COMMCU, bus::neogeo::cart, com_mcu_device)

#endif // MAME_BUS_NEOGEO_CART_COMMCU_H
