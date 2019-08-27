//==================================================================================
// Module       : ginitforce_direct_user.hpp
// Date         : 7/10/19 (DLR)
// Description  : Direct user force initialization function implementations. These
//                methods are called directly during configuration, and can set 
//                forcing for entire state (v+b+s) etc. The 'component' types
//                in which component groups (v, b, s, etc) are set individually
//                are contained in separate namespaces.
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================
#if !defined(_GINITFORCE_USER_HPP)
#define _GINITFORCE_USER_HPP

#include "tbox/property_tree.hpp"
#include "gtypes.h"
#include "gtvector.hpp"
#include "ggrid.hpp"
#include "gcomm.hpp"


namespace ginitforce
{

GBOOL impl_null    (const PropertyTree &ftree, const Time &t, State &u, State &uf);
GBOOL impl_rand    (const PropertyTree &ftree, const Time &t, State &u, State &uf);

};


#endif
