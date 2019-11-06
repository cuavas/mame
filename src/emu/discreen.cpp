// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Vas Crabb

#include "emu.h"
#include "discreen.h"

#include "rendutil.h"


std::pair<unsigned, unsigned> device_screen_interface::physical_aspect() const noexcept
{
	assert(device().configured());

	// treat a ratio with zero in it as unconfigured
	std::pair<unsigned, unsigned> result(m_phys_aspect);
	if (!result.first || !result.second)
		result = screen_default_aspect();

	// magic value used to mean square pixels
	if ((~0U == result.first) && (~0U == result.second))
	{
		result.first = visible_area().width();
		result.second = visible_area().height();
	}

	// always return in reduced form
	util::reduce_fraction(result.first, result.second);
	return result;
}


device_screen_interface::device_screen_interface(machine_config const &mconfig, device_t &device)
	: device_interface(device, "screen")
	, m_orientation(ROT0)
	, m_phys_aspect(0U, 0U)
	, m_container(nullptr)
	, m_vis_area(0, 99, 0, 99)
{
}


void device_screen_interface::interface_config_complete()
{
	// combine screen orientation with machine orientation
	m_orientation = orientation_add(m_orientation, device().mconfig().gamedrv().flags & machine_flags::MASK_ORIENTATION);
}


void device_screen_interface::interface_pre_start()
{
	if (!m_container)
		throw emu_fatalerror("Screen %s render container not set before device start time\n", device().tag());
}


void device_screen_interface::interface_post_start()
{
	device().save_item(NAME(m_vis_area.min_x));
	device().save_item(NAME(m_vis_area.max_x));
	device().save_item(NAME(m_vis_area.min_y));
	device().save_item(NAME(m_vis_area.max_y));
}


void device_screen_interface::interface_post_stop()
{
	m_container = nullptr;
}


std::pair<unsigned, unsigned> device_screen_interface::screen_default_aspect() const noexcept
{
	return std::make_pair(~0U, ~0U);
}
