// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    AES/MVS cartridge interface

***************************************************************************/

#include "emu.h"
#include "cart.h"

#include "emuopts.h"
#include "softlist_dev.h"

#include <cstring>

//#define VERBOSE 1
//#define LOG_OUTPUT_STREAM std::cerr
#include "logmacro.h"


DEFINE_DEVICE_TYPE_NS(NG_CART_SLOT, bus::neogeo::cart, slot_device, "neogeo_cart_new", "Neo-Geo AES/MVS Cartridge Slot")


namespace bus { namespace neogeo { namespace cart {

/***********************************************************************
    Cartridge device interface
***********************************************************************/

//----------------------------------
// construction/destruction
//----------------------------------

device_cart_interface::device_cart_interface(machine_config const &mconfig, device_t &device)
	: device_slot_card_interface(mconfig, device)
{
}


//----------------------------------
// 16-bit 68k ROM and I/O
//----------------------------------

void device_cart_interface::install_p_rom(address_space &space, offs_t base)
{
}

void device_cart_interface::install_port_r(address_space &space, offs_t base)
{
}

void device_cart_interface::install_port_w(address_space &space, offs_t base)
{
}


//----------------------------------
// 8-bit Z80 ROM and I/O
//----------------------------------

void device_cart_interface::install_m1_rom(address_space &space, offs_t base)
{
}

void device_cart_interface::install_m1_io(address_space &space, offs_t base)
{
}


//----------------------------------
// 32-bit and 8-bit graphics ROM
//----------------------------------

void device_cart_interface::get_c_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	base = nullptr;
	length = 0U;
	mirror = C_MASK;
}

void device_cart_interface::get_s_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	base = nullptr;
	length = 0U;
	mirror = S_MASK;
}


//----------------------------------
// 8-bit ADPCM ROM
//----------------------------------

void device_cart_interface::get_v1_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	base = nullptr;
	length = 0U;
	mirror = V1_MASK;
}

void device_cart_interface::get_v2_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	base = nullptr;
	length = 0U;
	mirror = V2_MASK;
}


//----------------------------------------------
// input signals
//----------------------------------------------

WRITE_LINE_MEMBER(device_cart_interface::slotcs_w)
{
}



/***********************************************************************
    Cartridge slot device
***********************************************************************/

//----------------------------------
// construction/destruction
//----------------------------------

slot_device::slot_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock)
	: device_t(mconfig, NG_CART_SLOT, tag, owner, clock)
	, device_slot_interface(mconfig, *this)
	, device_image_interface(mconfig, *this)
	, m_cart(nullptr)
{
}


//----------------------------------------------
// 32-bit and 8-bit graphics ROM
//----------------------------------------------

void slot_device::get_c_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	if (m_cart)
	{
		m_cart->get_c_rom_info(base, length, mirror);
	}
	else
	{
		base = nullptr;
		length = 0U;
		mirror = (offs_t(1) << 25) - 1;
	}
}

void slot_device::get_s_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	if (m_cart)
	{
		m_cart->get_s_rom_info(base, length, mirror);
	}
	else
	{
		base = nullptr;
		length = 0U;
		mirror = (offs_t(1) << 17) - 1;
	}
}


//----------------------------------------------
// 8-bit ADPCM ROM
//----------------------------------------------

void slot_device::get_v1_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	if (m_cart)
	{
		m_cart->get_v1_rom_info(base, length, mirror);
	}
	else
	{
		base = nullptr;
		length = 0U;
		mirror = (offs_t(1) << 24) - 1;
	}
}

void slot_device::get_v2_rom_info(u8 const *&base, offs_t &length, offs_t &mirror)
{
	if (m_cart)
	{
		m_cart->get_v2_rom_info(base, length, mirror);
	}
	else
	{
		base = nullptr;
		length = 0U;
		mirror = (offs_t(1) << 24) - 1;
	}
}


//----------------------------------------------
// device_t implementation
//----------------------------------------------

void slot_device::device_validity_check(validity_checker &valid) const
{
    device_t *const card(get_card_device());
	if (card && !dynamic_cast<device_cart_interface *>(card))
		osd_printf_error("Card device %s (%s) does not implement device_cart_interface\n", card->tag(), card->name());
}

void slot_device::device_start()
{
    device_t *const card(get_card_device());
	m_cart = dynamic_cast<device_cart_interface *>(card);
	if (card && !m_cart)
		throw emu_fatalerror("Card device %s (%s) does not implement device_cart_interface\n", card->tag(), card->name());
}


//----------------------------------------------
// device_slot_interface implementation
//----------------------------------------------

std::string slot_device::get_default_card_software(get_default_card_software_hook &hook) const
{
	std::string const &selection(mconfig().options().image_option(instance_name()).value());
	if (hook.image_file())
	{
		// FIXME: add RPK support, which of course requires the ability to open the file as an archive
		return "flatrom";
	}
	else
	{
		// probe the software list entry
		software_part const *const part(find_software_item(selection, true));
		if (part)
		{
			char const *const prog(part->feature("prog"));
			char const *const cha(part->feature("cha"));
			if (prog && !std::strcmp(prog, "NEO-MVS PROG-HERO"))
				return "commcu";
			else if (cha && (!std::strcmp(cha, "NEO-MVS CHA42G") || !std::strcmp(cha, "NEO-MVS CHA42G-1")))
				return "m1zmc";
			else
				return "flatrom";
		}
	}
	return "";
}


//----------------------------------------------
// device_image_interface implementation
//----------------------------------------------

image_init_result slot_device::call_load()
{
	if (m_cart)
	{
		if (loaded_through_softlist())
		{
			return m_cart->configure_software([this] (char const *name) { return get_feature(name); });
		}
		else
		{
			// FIXME: add RPK support
		}
	}
	return image_init_result::PASS;
}

void slot_device::call_unload()
{
}

software_list_loader const &slot_device::get_software_list_loader() const
{
	return rom_software_list_loader::instance();
}

} } } // namespace bus::neogeo::cart


#include "commcu.h"
#include "flatrom.h"
#include "m1zmc.h"

/***********************************************************************
    Cartridge types
***********************************************************************/

SLOT_INTERFACE_START(neogeo_carts)
	SLOT_INTERFACE_INTERNAL("commcu", NG_CART_COMMCU)
	SLOT_INTERFACE_INTERNAL("flatrom", NG_CART_FLATROM)
	SLOT_INTERFACE_INTERNAL("m1zmc", NG_CART_M1ZMC)
SLOT_INTERFACE_END
