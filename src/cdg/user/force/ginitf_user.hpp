//==================================================================================
// Module       : ginitf_user.hpp
// Date         : 7/10/19 (DLR)
// Description  : User force initialization function implementations
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================
#if !defined(_GINITF_USER_HPP)
#define _GINITF_USER_HPP

#include "tbox/property_tree.hpp"
#include "gtypes.h"
#include "gtvector.hpp"
#include "ggrid.hpp"
#include "gcomm.hpp"


namespace ginitf
{

GBOOL impl_null    (const PropertyTree &ftree, const Time &t, State &u, State &uf);
GBOOL impl_rand    (const PropertyTree &ftree, const Time &t, State &u, State &uf);

};

#include "ginitf_null.ipp"
#include "ginitf_rand.ipp"

#endif
