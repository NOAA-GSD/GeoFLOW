//==================================================================================
// Module       : ginitstate_factory
// Date         : 7/11/19 (DLR)
// Description  : GeoFLOW state initialization factory
// Copyright    : Copyright 2020. Colorado State University. All rights reserved.
// Derived From : none.
//==================================================================================

//**********************************************************************************
//**********************************************************************************
// METHOD : spec
// DESC   : Do specification of terrain
// ARGS   : ptree  : main property tree
//          grid   : grid object
//          utmp   : tmp arrays
//          xb     : terrain coordinates (of size of a State vector)
//          bterr  : flag telling if terrain was loaded
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GSpecTerrainFactory<Types>::spec(const PropertyTree& ptree, GGrid &grid, State &utmp, State &xb, GBOOL &bterr)
{
  GBOOL         bret = FALSE;
  GString       stype;  

  // Get type of initialization: by-name or by-block:
  stype = ptree.getValue<GString>("terrain_type","");
  if ( "none"   == stype 
    || ""       == stype ) {
    bterr = FALSE;
    return TRUE;
  }
  else if ( std::is_same<grid,GGridBox>::value ) {
    bret = spec_box   (ptree, grid, utmp, stype, xb);
  }
  else if ( std::is_same<grid,GGridIcos>::value ) {
    bret = spec_sphere(ptree, grid, utmp, stype, xb);
  }
  else {
    assert(FALSE && "Invalid specification class or grid type");
  }

  if ( bret ) bterr = TRUE;
  return bret;

} // end, init method


//**********************************************************************************
//**********************************************************************************
// METHOD : spec_box
// DESC   : Do terrain specification for box grid types
// ARGS   : ptree  : main property tree
//          grid   : grid object
//          utmp   : tmp arrays
//          stype  : terrain type name
//          xb     : terrain coordinates
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GSpecTerrainFactory<Types>::spec_box(const PropertyTree& ptree, GGrid &grid, State &utmp, GString stype, State &xb)
{
  GBOOL         bret    = FALSE;

  if      ( "none"          == stype 
         || ""              == stype ) {
    return TRUE;
  }
  else if ( "gauss_range"   == stype ) {
    bret = gterrain_specbox::impl_gauss_range   (ptree, grid, utmp, xb);
  }
  else if ( "poly_range"    == stype ) {
    bret = gterrain_specbox::impl_poly_range    (ptree, grid, utmp, xb);
  }
  else if ( "schar_range"   == stype ) {
    bret = gterrain_specbox::impl_schar_range   (ptree, grid, utmp, xb);
  }
  else                                        {
    assert(FALSE && "Terrain specification method unknown");

  }

  return bret;

} // end, spec_box method


//**********************************************************************************
//**********************************************************************************
// METHOD : spec_sphere
// DESC   : Do terrain specification for sphere grid types
// ARGS   : ptree  : main property tree
//          grid   : grid object
//          utmp   : tmp arrays
//          stype  : terrain type name
//          xb     : terrain coordinates
// RETURNS: TRUE on success; else FALSE
//**********************************************************************************
template<typename Types>
GBOOL GSpecTerrainFactory<Types>::spec_sphere(const PropertyTree& ptree, GGrid &grid, State &utmp, GString stype, State &xb)
{
  GBOOL         bret    = FALSE;

  if      ( "none"          == stype 
         || ""              == stype ) {
    return TRUE;
  }
  else if ( "gauss_range"   == stype ) {
    bret = gterrain_specsph::impl_gauss_range   (ptree, grid, utmp, xb);
  }
  else if ( "poly_range"    == stype ) {
    bret = gterrain_specsph::impl_poly_range    (ptree, grid, utmp, xb);
  }
  else                                        {
    assert(FALSE && "Terrain specification method unknown");

  }

  return bret;

} // end, spec_box method


