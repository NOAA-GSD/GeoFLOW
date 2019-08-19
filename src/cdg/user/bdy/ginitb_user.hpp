//==================================================================================
// Module       : ginitb_user.hpp
// Date         : 7/11/19 (DLR)
// Description  : Boundary initialization function implementations provided
//                by user
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================
#if !defined(_GINITB_USER_HPP)
#define _GINITB_USER_HPP

#include "tbox/property_tree.hpp"
#include "gtypes.h"
#include "gtvector.hpp"
#include "ggrid.hpp"


using namespace geoflow;
using namespace pdeint;

namespace ginitb
{

GBOOL impl_mybdyinit        (const PropertyTree& stree, GGrid &grid,  Time &time, State &utmp, State &u, State &ub);
};

#include "ginitb_user.ipp"

#endif
