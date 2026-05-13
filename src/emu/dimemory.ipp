// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    dimemory.ipp

    Device memory interfaces.

***************************************************************************/

#ifndef MAME_EMU_DIMEMORY_IPP
#define MAME_EMU_DIMEMORY_IPP

#pragma once

#include "dimemory.h"

#include "mconfig.h"


template <typename T, typename Ret, typename... Params>
inline void device_memory_interface::set_addrmap(int spacenum, Ret (T::*func)(Params... args))
{
	device_t &dev(device().mconfig().current_device());
	if constexpr (is_related_class<device_t, T>::value)
		set_addrmap(spacenum, address_map_constructor(func, dev.tag(), &downcast<T &>(dev)));
	else
		set_addrmap(spacenum, address_map_constructor(func, dev.tag(), &dynamic_cast<T &>(dev)));
}

#endif // MAME_EMU_DIMEMORY_IPP
