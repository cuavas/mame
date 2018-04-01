// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    AES/MVS cartridge interface

    Based on logical spaces seen by host system:
    * P ROM read: (512k*16)
      address on A19-A1
	  data on D15-D0
	  strobed with ROMOE, ROMOEL, ROMOEU
	  mapped to 68k 0x000000-0x0fffff read
    * PORT read/write: (512k*16)
	  address on A19-A1
	  data on D15-D0
	  strobed with PORTADRS, PORTOEL, PORTOEU, PORTWEL, PORTWEU
	  mapped to 68k 0x200000-0x2fffff read/write
	* M1 ROM: (62k*8)
	  address on SDA15-SDA0
	  data on SDD7..SDD0
	  strobed with SDROM, SDMRD
	  mapped to Z80 0x0000-0xf7ff memory read
	* M1 I/O read: (8*8 with twelve address mirror bits)
	  address on SDA15-SDA0
	  data on SDD7-SDD0
	  strobed with SDRD0, SDRD1
	  mapped to Z80 0x0008-0x000f I/O read mirror 0xfff0
	* C ROM: (32M*32)
	  address on P23-P20:P15-P0:CA4:P19-P16
	  data on CR31-CR0
	  strobed with PCK1B
	  pixels serialisd with PRO-CT0 or NEO-ZMC2
	* S ROM: (128k*8)
	  address on P11-P0:P15:2H1:P14-P12
	  data on FIX07-FIX00
	  strobed with PCK2B
	* V1 ROM: (16M*8)
	  strobed with SDPMPX, SDPOE
	  address on SDRA23-SDRA20:SDRA9-SDRA8:SDRAD7-SDRAD0
	  data on SDRAD7-SDRAD0
	  direct connection to YM2610 ADPCM-A bus
	* V2 ROM: (16M*8)
	  address on SDPA11-SDPA8:SDPAD7-SDPAD0
	  data on SDPAD7-SDPAD0
	  strobed with SDPMPX, SDPOE
	  direct connection to YM2610 ADPCM-B bus

	Some compromises made to work with MAME in its current state:
	* We pretend C ROM is 8 bits wide and deliver it in the weird order
	  the sprite device wants.
	* YM2610 regions are exposed as pointers because the MAME device
	  uses dumb memory regions rather than address spaces.

***************************************************************************/
#ifndef MAME_BUS_NEOGEO_CART_CART_H
#define MAME_BUS_NEOGEO_CART_CART_H

#pragma once

#include <functional>


namespace bus { namespace neogeo { namespace cart {

class device_cart_interface : public device_slot_card_interface
{
protected:
	// useful constants
	enum : offs_t
	{
		P_MASK      = (offs_t(1) << 19) - 1,
		PORT_MASK   = (offs_t(1) << 19) - 1,
		M1_MASK     = (offs_t(1) << 16) - 1,
		C_MASK      = (offs_t(1) << 25) - 1,
		S_MASK      = (offs_t(1) << 17) - 1,
		V1_MASK     = (offs_t(1) << 24) - 1,
		V2_MASK     = (offs_t(1) << 24) - 1
	};

	// construction/destruction
	device_cart_interface(machine_config const &mconfig, device_t &device);

public:
	// callback types
	using get_feature_func = std::function<char const * (char const *name)>;

	// software loading
	virtual image_init_result configure_software(get_feature_func const &get_feature) = 0;

	// 16-bit 68k ROM and I/O
	virtual void install_p_rom(address_space &space, offs_t base);
	virtual void install_port_r(address_space &space, offs_t base);
	virtual void install_port_w(address_space &space, offs_t base);

	// 8-bit Z80 ROM and I/O
	virtual void install_m1_rom(address_space &space, offs_t base);
	virtual void install_m1_io(address_space &space, offs_t base);

	// 32-bit and 8-bit graphics ROM
	virtual void get_c_rom_info(u8 const *&base, offs_t &length, offs_t &mirror);
	virtual void get_s_rom_info(u8 const *&base, offs_t &length, offs_t &mirror);

	// 8-bit ADPCM ROM
	virtual void get_v1_rom_info(u8 const *&base, offs_t &length, offs_t &mirror);
	virtual void get_v2_rom_info(u8 const *&base, offs_t &length, offs_t &mirror);

	// input signals
	virtual DECLARE_WRITE_LINE_MEMBER(slotcs_w);
};


class slot_device : public device_t, public device_slot_interface, public device_image_interface
{
public:
	// construction/destruction
	slot_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

	// 16-bit 68k ROM and I/O
	void install_p_rom(address_space &space, offs_t base) { if (m_cart) m_cart->install_p_rom(space, base); }
	void install_port_r(address_space &space, offs_t base) { if (m_cart) m_cart->install_port_r(space, base); }
	void install_port_w(address_space &space, offs_t base) { if (m_cart) m_cart->install_port_w(space, base); }

	// 8-bit Z80 ROM and I/O
	void install_m1_rom(address_space &space, offs_t base) { if (m_cart) m_cart->install_m1_rom(space, base); }
	void install_m1_io(address_space &space, offs_t base) { if (m_cart) m_cart->install_m1_io(space, base); }

	// 32-bit and 8-bit graphics ROM
	void get_c_rom_info(u8 const *&base, offs_t &length, offs_t &mirror);
	void get_s_rom_info(u8 const *&base, offs_t &length, offs_t &mirror);

	// 8-bit ADPCM ROM
	void get_v1_rom_info(u8 const *&base, offs_t &length, offs_t &mirror);
	void get_v2_rom_info(u8 const *&base, offs_t &length, offs_t &mirror);

	// input signals
	DECLARE_WRITE_LINE_MEMBER(slotcs_w) { if (m_cart) m_cart->slotcs_w(state); }

protected:
	// device_t implementation
	virtual void device_validity_check(validity_checker &valid) const override ATTR_COLD;
	virtual void device_start() override;

	// device_slot_interface implementation
	virtual std::string get_default_card_software(get_default_card_software_hook &hook) const override;

	// device_image_interface implementation
	virtual image_init_result call_load() override;
	virtual void call_unload() override;
	virtual iodevice_t image_type() const override { return IO_CARTSLOT; }
	virtual bool is_readable() const override { return true; }
	virtual bool is_writeable() const override { return false; }
	virtual bool is_creatable() const override { return false; }
	virtual bool must_be_loaded() const override { return false; }
	virtual bool is_reset_on_load() const override { return true; }
	virtual char const *image_interface() const override { return "neo_cart"; }
	virtual char const *file_extensions() const override { return "rpk"; }
	virtual software_list_loader const &get_software_list_loader() const override;

private:
	device_cart_interface *m_cart;
};

} } } // namespace bus::neogeo::cart


DECLARE_DEVICE_TYPE_NS(NG_CART_SLOT, bus::neogeo::cart, slot_device)

SLOT_INTERFACE_EXTERN(neogeo_carts);

#endif // MAME_BUS_NEOGEO_CART_CART_H
