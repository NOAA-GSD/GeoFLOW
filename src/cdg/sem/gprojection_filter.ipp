//==================================================================================
// Module       : gprojection_filter.hpp
// Date         : 4/10/21 (DLR)
// Description  : Computes a projectioon filter for stabilization.
//                Taken from Deville, Fischer & Mund "High-Order
//                Methods for Incompressible Flow"
//                 
//                Define interpolation matrices,
//                    I_N^M(i,j) = h_N,j(xi_M,i)
//                where h_N is the Lagrange interpolating polynomial
//                of order N, evaluated at nodes, x_M,i from the Mth 
//                polynomial. Then define
//                    P_N^M = I_M^N I_N^M.
//                The filter is then defined in 1d as
//                    F = alpha Pi_N^M + 1-alpha) I_N^N
//                where
//                    I_N^N 
//                is the Nth-order identify matrix. Filter, F, is then 
//                applied in tensor product form.
// Copyright    : Copyright 2021. Colorado State University. All rights reserved.
// Derived From : FilterBase
//==================================================================================

//**********************************************************************************
//**********************************************************************************
// METHOD : Constructor method (1)
// DESC   : Default constructor
// ARGS   : grid    : Grid object
//          ifilter : starting mode for filtering
//          mufilter: filter factor (by which to truncatte)
// RETURNS: none
//**********************************************************************************
template<typename TypePack>
GProjectionFilter<TypePack>::GProjectionFilter(Traits &traits, Grid &grid)
:
bInit_               (FALSE),
traits_              (traits),
grid_                (&grid)
{

//assert(grid_->ntype().multiplicity(0) == GE_MAX-1 
//      && "Only a single element type allowed on grid");
  assert(grid_->ispconst()); // order must not vary 
  assert(traits_.plow.size(() >= GDIM && traits_.alpha.size() >= GDIM);
} // end of constructor method (1)


//**********************************************************************************
//**********************************************************************************
// METHOD : Destructor method 
// DESC   : 
// ARGS   : none
// RETURNS: none
//**********************************************************************************
template<typename TypePack>
GProjectionFilter<TypePack>::~GProjectionFilter()
{
} // end, destructor


//**********************************************************************************
//**********************************************************************************
// METHOD : apply_impl
// DESC   : Compute application of this filter to input vector
//           
// ARGS   : t   : Time
//          u   : input vector field
//          utmp: array of tmp arrays; not used here
//          uo  : output (result) vector
//             
// RETURNS:  none
//**********************************************************************************
template<typename TypePack>
void GProjectionFilter<TypePack>::apply_impl(const Time &t, StateComp &u, State &utmp, StateComp &uo) 
{

  GSIZET           ibeg, iend; // beg, end indices in global array
  GTVector<Ftype>  tmp;
  typename TypePack::GElemList       *gelems=&grid_->elems();

  if ( !bInit_ ) init();

  for ( auto e=0; e<gelems->size(); e++ ) {
    ibeg = (*gelems)[e]->igbeg(); iend = (*gelems)[e]->igend();
    u .range(ibeg, iend); // restrict global vecs to local range
    uo.range(ibeg, iend); 
#if defined(_G_IS2D)
    GMTK::D2_X_D1<Ftype>(*F_[0], *FT_[1], u, tmp_, uo);
#elif defined(_G_IS3D)
    GMTK::D3_X_D2_X_D1<Ftype>(*F_[0], *FT_[1], *FT_[2],  u, tmp_, uo);
#endif
  }
  u .range_reset(); 
  uo.range_reset(); 

} // end of method apply_impl


//**********************************************************************************
//**********************************************************************************
// METHOD : apply_impl
// DESC   : In-place application of this filter to input vector
//           
// ARGS   : t   : Time
//          u   : input vector field
//          utmp: array of tmp arrays
//             
// RETURNS:  none
//**********************************************************************************
template<typename TypePack>
void GProjectionFilter<TypePack>::apply_impl(const Time &t, StateComp &u, State &utmp) 
{

  assert( utmp.size() >= 1
       && "Insufficient temp space provided");

  apply_impl(t, u, utmp, *utmp[utmp.size()-1]); 
  u = *utmp[utmp.size()-1];

} // end of method apply_impl


//**********************************************************************************
//**********************************************************************************
// METHOD : init
// DESC   : Initilize operators
// ARGS   : none.
// RETURNS:  none
//**********************************************************************************
template<typename TypePack>
void GProjectionFilter<TypePack>::init()
{

  GINT             nnodes;
  Ftype            a, b, xi, xf0, xN;
  GTVector<GINT>   Nhi(GDIM), Nlow(GDIM);
  GTVector<Ftype>  *xihi, xilow[GDIM];
  GTVector<GNBasis<GCTYPE,Ftype>*> *bhi, blow(GDIM); 
  GTMatrix<Ftype>  Id, Ihi, Ilow, M;
  typename TypePack::GElemList  *gelems=&grid_->elems();

  // For now, let's assume this filter only works
  // when order is constant among elements.

  // First, compute I_M^N I_N^M;
  bhi   = &(*gelems)[0]->gbasis();
  Nhi   = (*gelems)[0]->size();
  for ( auto j=0; j<GDIM; j++ ) { // allocate matrices
    // Limit new p to be in [pold-1, pold/2]:
    assert(plow[j] >= 1 && plow[j] < (Nhi[j]-1)/2); 
    xihi     = &(*bhi)[j].getXiNodes();
    Nlow[j]  = Nhi[j] - plow[j];
    blow[j]  .resize(Nlow[j]-1); // create low order bases
    blow[j]  .getXiNodes(xilow[j]);
    Ilow     .resize(Nlow[j],Nhi[j]); // interp to low order basis
    Ihi      .resize(Nhi[j],Nlow[j]); // interp to high order basis
    F_  [j]  .resize(Nhi[j],Nhi[j]);  // 1d filter matrices
    FT_ [j]  .resize(Nhi[j],Nhi[j]);  // 1d filter matrix transposes
    M        .resize(Nhi[j],Nhi[j]);  // tmp matrix
    Id       .resize(Nhi[j],Nhi[j]); Id.createIdentity()(;
    (*bhi)[j].evalBasis(xilow[j],Ihi[j]);
    blow[j].evalBasis(*xihi,Ilow[j]);

    // Compute 1d filter matrices: F = alpha Ihi Ilow + (1-alpha) I;
    M        = Ihi * Ilow;
    F_  [j]  = M * traits_.alpha[j] 
    F_  [j] += ( Id * (1.0-alpha[j]) );
    F_  [j]  .transpose(FT_[j]);
  }

  bInit_ = TRUE;

} // end of method init

