//==================================================================================
// Description  : Object defining a (global) icosahedral grid, that
//                uses gnomonic projections to locate element vertices.
// Copyright    : Copyright 2018. Colorado State University. All rights reserved
// Derived From : GGrid.
//==================================================================================

#include <cstdlib>
#include <memory>
#include <cmath>
//#include "omp.h"  // Are we calling API functions ?
#include "gutils.hpp"
#include "gspecbdy_factory.hpp"
#include "gelem_base.hpp"
#include "ggrid_icos.hpp"
#include "gtpoint.hpp"

using namespace std;

//**********************************************************************************
//**********************************************************************************
// METHOD : Constructor method (1)
// DESC   : Instantiate for 2d grid
// ARGS   : ptree: main property tree
//          b     : vector of basis pointers, of size at least ndim=2.Determies 
//                  dimensionality
//          comm  : communicator
// RETURNS: none
//**********************************************************************************
GGridIcos::GGridIcos(const geoflow::tbox::PropertyTree &ptree, GTVector<GNBasis<GCTYPE,GFTYPE>*> &b, GC_COMM &comm)
:                 GGrid(ptree, b, comm),
ilevel_                             (0),
nrows_                              (0),
ndim_                            (GDIM),
radiusi_                          (0.0),
radiuso_                          (0.0), // don't need this one in 2d
gdd_                          (NULLPTR),
lshapefcn_                    (NULLPTR)
{
  assert(b.size() == GDIM && "Basis has incorrect dimensionality");
  
  GString gname   = ptree.getValue<GString>("grid_type");
  assert(gname == "grid_icos" || gname == "grid_sphere");
  geoflow::tbox::PropertyTree gridptree = ptree.getPropertyTree(gname);

  gbasis_.resize(b.size());
  gbasis_ = b;
  lshapefcn_ = new GShapeFcn_linear<GTICOS>();
  ilevel_  = gridptree.getValue<GINT>("ilevel");
  sreftype_= gridptree.getValue<GString>("refine_type","GICOS_LAGRANGIAN");
  
  if ( ndim_ == 2 ) {
    assert(GDIM == 2 && "GDIM must be 2");
    radiusi_ = gridptree.getValue<GFTYPE>("radius");
    init2d();
  }
  else if ( ndim_ == 3 ) {
    assert(GDIM == 3 && "GDIM must be 3");
    std::vector<GINT> ne(3);
    radiusi_   = gridptree.getValue<GFTYPE>("radiusi");
    radiuso_   = gridptree.getValue<GFTYPE>("radiuso");
    nradelem_  = gridptree.getValue<GINT>("num_radial_elems");
    init3d();
  }
  else {
    assert(FALSE && "Invalid dimensionality");
  }

} // end of constructor method (1)



//**********************************************************************************
//**********************************************************************************
// METHOD : Destructor method 
// DESC   : 
// ARGS   : none
// RETURNS: none
//**********************************************************************************
GGridIcos::~GGridIcos()
{
  if ( lshapefcn_ != NULLPTR ) delete lshapefcn_;
  if ( gdd_       != NULLPTR ) delete gdd_;
} // end, destructor


//**********************************************************************************
//**********************************************************************************
// METHOD :  << operator method (1)
// DESC   : output stream operator
// ARGS   :
// RETURNS: ostream &
//**********************************************************************************
std::ostream &operator<<(std::ostream &str, GGridIcos &e)
{
  
  str << " radiusi: " << e.radiusi_;
  str << " radiusi: " << e.radiuso_;
  str << " level  : " << e.ilevel_;
  str << " nrows  : " << e.nrows_;
  str << std::endl << " Centroids: " ;
  for ( GSIZET i=0; i<e.ftcentroids_.size(); i++ )
     str << (e.ftcentroids_[i]) << " " ;
  str << std::endl;

  return str;
} // end of operator <<


//**********************************************************************************
//**********************************************************************************
// METHOD : set_partitioner
// DESC   : Set domain decomposition object
// ARGS   : GDD_base pointer
// RETURNS: none
//**********************************************************************************
void GGridIcos::set_partitioner(GDD_base<GTICOS> *gdd)
{

  gdd_ = gdd;

} // end of method set_partitioner


//**********************************************************************************
//**********************************************************************************
// METHOD : init2d
// DESC   : Initialize base state/base icosahedron
// ARGS   : none.
// RETURNS: none.
//**********************************************************************************
void GGridIcos::init2d()
{
  GString serr = "GridIcos::init2d: ";

  GFTYPE phi = (1.0+sqrt(5.0))/2.0;  // Golden ratio


  // Base vertex list:
  fv0_.resize(12,3);
#if 0
  // Following give orientation s.t. edge lies at the top:
  fv0_(0 ,0) = 0  ; fv0_(0 ,1) = 1  ; fv0_(0 ,2) = phi;
  fv0_(1 ,0) = 0  ; fv0_(1 ,1) =-1  ; fv0_(1 ,2) = phi;
  fv0_(2 ,0) = 0  ; fv0_(2 ,1) = 1  ; fv0_(2 ,2) =-phi;
  fv0_(3 ,0) = 0  ; fv0_(3 ,1) =-1  ; fv0_(3 ,2) =-phi;
  fv0_(4 ,0) = 1  ; fv0_(4 ,1) = phi; fv0_(4 ,2) = 0;
  fv0_(5 ,0) =-1  ; fv0_(5 ,1) = phi; fv0_(5 ,2) = 0;
  fv0_(6 ,0) = 1  ; fv0_(6 ,1) =-phi; fv0_(6 ,2) = 0;
  fv0_(7 ,0) =-1  ; fv0_(7 ,1) =-phi; fv0_(7 ,2) = 0;
  fv0_(8 ,0) = phi; fv0_(8 ,1) = 0  ; fv0_(8 ,2) = 1;
  fv0_(9 ,0) =-phi; fv0_(9 ,1) = 0  ; fv0_(9 ,2) = 1;
  fv0_(10,0) = phi; fv0_(10,1) = 0  ; fv0_(10,2) =-1;
  fv0_(11,0) =-phi; fv0_(11,1) = 0  ; fv0_(11,2) =-1;
#endif

// Following give orientation s.t. vertex is at top:
fv0_(0 ,0) =  0.000000000000000; fv0_(0 ,1) =  0.000000000000000; fv0_(0 ,2) =  1.000000000000000;
fv0_(1 ,0) =  0.894427190999916; fv0_(1 ,1) =  0.000000000000000; fv0_(1 ,2) =  0.447213595499958;
fv0_(2 ,0) = -0.894427190999916; fv0_(2 ,1) =  0.000000000000000; fv0_(2 ,2) = -0.447213595499958;
fv0_(3 ,0) =  0.000000000000000; fv0_(3 ,1) =  0.000000000000000; fv0_(3 ,2) = -1.000000000000000;
fv0_(4 ,0) = -0.723606797749979; fv0_(4 ,1) =  0.525731112119134; fv0_(4 ,2) =  0.447213595499958;
fv0_(5 ,0) = -0.723606797749979; fv0_(5 ,1) = -0.525731112119134; fv0_(5 ,2) =  0.447213595499958;
fv0_(6 ,0) =  0.723606797749979; fv0_(6 ,1) =  0.525731112119134; fv0_(6 ,2) = -0.447213595499958;
fv0_(7 ,0) =  0.723606797749979; fv0_(7 ,1) = -0.525731112119134; fv0_(7 ,2) = -0.447213595499958;
fv0_(8 ,0) =  0.276393202250021; fv0_(8 ,1) =  0.850650808352040; fv0_(8 ,2) =  0.447213595499958;
fv0_(9 ,0) =  0.276393202250021; fv0_(9 ,1) = -0.850650808352040; fv0_(9 ,2) =  0.447213595499958;
fv0_(10,0) = -0.276393202250021; fv0_(10,1) =  0.850650808352040; fv0_(10,2) = -0.447213595499958;
fv0_(11,0) = -0.276393202250021; fv0_(11,1) = -0.850650808352040; fv0_(11,2) = -0.447213595499958;

  
  // Normalize vertices to be at 1.0:
  GFTYPE fact = 1.0/sqrt(phi*phi + 1.0);
  fv0_ *= fact;

  // Points in verts array that make up 
  // each face of base icosahedron:
  ifv0_.resize(20,3);
  ifv0_(0 ,0) = 0 ; ifv0_(0 ,1) = 1 ; ifv0_(0 ,2) = 8 ;
  ifv0_(1 ,0) = 0 ; ifv0_(1 ,1) = 8 ; ifv0_(1 ,2) = 4 ;
  ifv0_(2 ,0) = 0 ; ifv0_(2 ,1) = 4 ; ifv0_(2 ,2) = 5 ;
  ifv0_(3 ,0) = 0 ; ifv0_(3 ,1) = 5 ; ifv0_(3 ,2) = 9 ;
  ifv0_(4 ,0) = 0 ; ifv0_(4 ,1) = 9 ; ifv0_(4 ,2) = 1 ;
  ifv0_(5 ,0) = 1 ; ifv0_(5 ,1) = 6 ; ifv0_(5 ,2) = 8 ;
  ifv0_(6 ,0) = 8 ; ifv0_(6 ,1) = 6 ; ifv0_(6 ,2) = 10;
  ifv0_(7 ,0) = 8 ; ifv0_(7 ,1) = 10; ifv0_(7 ,2) = 4 ;
  ifv0_(8 ,0) = 4 ; ifv0_(8 ,1) = 10; ifv0_(8 ,2) = 2 ;
  ifv0_(9 ,0) = 4 ; ifv0_(9 ,1) = 2 ; ifv0_(9 ,2) = 5 ;
  ifv0_(10,0) = 5 ; ifv0_(10,1) = 2 ; ifv0_(10,2) = 11;
  ifv0_(11,0) = 5 ; ifv0_(11,1) = 11; ifv0_(11,2) = 9 ;
  ifv0_(12,0) = 9 ; ifv0_(12,1) = 11; ifv0_(12,2) = 7 ;
  ifv0_(13,0) = 9 ; ifv0_(13,1) = 7 ; ifv0_(13,2) = 1 ;
  ifv0_(14,0) = 1 ; ifv0_(14,1) = 7 ; ifv0_(14,2) = 6 ;
  ifv0_(15,0) = 3 ; ifv0_(15,1) = 6 ; ifv0_(15,2) = 7 ;
  ifv0_(16,0) = 3 ; ifv0_(16,1) = 7 ; ifv0_(16,2) = 11;
  ifv0_(17,0) = 3 ; ifv0_(17,1) = 11; ifv0_(17,2) = 2 ;
  ifv0_(18,0) = 3 ; ifv0_(18,1) = 2 ; ifv0_(18,2) = 10;
  ifv0_(19,0) = 3 ; ifv0_(19,1) = 10; ifv0_(19,2) = 6 ;

  // Copy base data to new structure:
  GTPoint<GTICOS> pt;
  tbase_.resize(ifv0_.size(1));
  for ( GSIZET j=0; j<tbase_.size(); j++ ) tbase_[j].resize(3);
  for ( GSIZET i=0; i<ifv0_.size(1); i++ ) { // for all triangles:
    for ( GSIZET j=0; j<ifv0_.size(2); j++ ) { // for each vertex:
      for ( GSIZET k=0; k<3; k++ ) pt[k] = fv0_(ifv0_(i,j),k);
      *tbase_[i].v[j] = pt;
    }
  }

  lagrefine();

} // end of method init2d


//**********************************************************************************
//**********************************************************************************
// METHOD : init3d
// DESC   : Initialize for 3d elements
// ARGS   : none.
// RETURNS: none.
//**********************************************************************************
void GGridIcos::init3d()
{
  GString serr = "GridIcos::init3d: ";

  init2d();

} // end, method init3d


//**********************************************************************************
//**********************************************************************************
// METHOD : lagrefine
// DESC   : Use 'Lagrangian polynomial'-like placement of 'nodes' in 
//          base face/triangle to refine, before doing projection of 
//          vertices. An alternative might be, say, a self-similar 
//          (recursive) refinement of every triangle into 4 triangles.
// ARGS   : none.
// RETURNS: none.
//**********************************************************************************
void GGridIcos::lagrefine()
{
  GString serr = "GridIcos::lagrefine: ";
   
  GLLONG ibeg, j, l, m, n, t;

  if      ( "GICOS_BISECTION" == sreftype_ ) {
    // interpret nrows_ as bisection count:
    nrows_ = pow(2,ilevel_)-1; 
  }
  else if ( "GICOS_LAGRANGIAN" == sreftype_ ) {
    // interpret nrows_ as # 'Lagrangian' subdivisions:
    nrows_ = ilevel_;
  }
  else {
    assert(FALSE && "Invalid subdivision type (GICOS_LAGRANGIAN or GICOS_BISECTION");
  }

  // Re-dimension mesh points to be 3d: Expect
  // 20 * (ileve+1)^2, and 20 * (ilevel+1)^2 * 3 quads
  tmesh_.resize(20*(nrows_*(nrows_+2)+1)); // refined triangular mesh
  for ( j=0; j<tmesh_.size(); j++ ) tmesh_[j].resize(3);

  GTPoint<GTICOS> a(3), b(3), c(3); // base vertices

  n = 0;
  GTVector<GTPoint<GTICOS>> R0(2*(nrows_+1)-1), R1(2*(nrows_+1));
  GTVector<GTPoint<GTICOS>> Rz(2*(nrows_+1)+1); // interleave R0, R1


#pragma omp parallel private (ibeg,l,m,t,a,b,c,R0,R1,Rz) reduction(+: n)
  R0.resize(2*(nrows_+1)-1);
  R1.resize(2*(nrows_+1));
  Rz.resize(2*(nrows_+1)+1);

  // Do refinement of base mesh triangles:
#pragma omp for
  for ( t=0; t<tbase_.size(); t++ ) { // for each base triangle 
    a = tbase_[t].v1; b = tbase_[t].v2; c = tbase_[t].v3;
    for ( l=0; l<nrows_+1; l++ ) { // for each triangle 'row'
      lagvert<GTICOS>(a,b,c,l   ,R0); // get previous row of points
      lagvert<GTICOS>(a,b,c,l+1 ,R1); // get current row of points
      interleave<GTICOS>(R0, R1, l, Rz); // zig-zag from R1 to R0, to R1, etc
      for ( m=0; m<2*l+1; m++ ) { // loop over all tri's on this row
        ibeg = m;
        tmesh_[n].v1 = Rz[ibeg];
        tmesh_[n].v2 = Rz[ibeg+1];
        tmesh_[n].v3 = Rz[ibeg+2];
        n++;
      }
    } // end, l-loop
  } // end, t-loop

  
  // Project all vertices to unit sphere:
  project2sphere<GTICOS>(tmesh_,1.0);

  // Order triangles (set iup_ flags):
  order_triangles<GTICOS>(tmesh_);

  // Compute centroids of all triangles:
  ftcentroids_.clear();
  ftcentroids_.resize(tmesh_.size());
  GTICOS fact = 1.0/3.0;
  for ( j=0; j<tmesh_.size(); j++ ) { // for each triangle
    a =  tmesh_[j].v1 + tmesh_[j].v2;
    a += tmesh_[j].v3;
    a *= fact;
    ftcentroids_[j] = a;
  }

} // end of method lagrefine


//**********************************************************************************
//**********************************************************************************
// METHOD : do_elems (1)
// DESC   : Public entry point for grid computation
// ARGS   : 
//          rank: MPI rank or partition id
// RETURNS: none.
//**********************************************************************************
void GGridIcos::do_elems()
{
  if ( ndim_ == 2 ) do_elems2d(irank_);
  if ( ndim_ == 3 ) do_elems3d(irank_);

} // end, method do_elems (1)


//**********************************************************************************
//**********************************************************************************
// METHOD : do_elems (2)
// DESC   : Public entry point for grid element computation, for restart
// ARGS   : p     : matrix of size the number of elements X GDIM containing 
//                  the poly expansion order in each direction
//          xnodes: vector of GDIM vectors containing Cartesian coords of elements
//                  for each node point
// RETURNS: none.
//**********************************************************************************
void GGridIcos::do_elems(GTMatrix<GINT> &p,
                        GTVector<GTVector<GFTYPE>> &xnodes)
{
  if ( ndim_ == 2 ) do_elems2d(p, xnodes);
  if ( ndim_ == 3 ) do_elems3d(p, xnodes);

} // end, method do_elems (2)


//**********************************************************************************
//**********************************************************************************
// METHOD : do_elems2d (1)
// DESC   : Build 2d elemental grid on base mesh
// ARGS   : 
//          irank: MPI rank or partition id
// RETURNS: none.
//**********************************************************************************
void GGridIcos::do_elems2d(GINT irank)
{
  GString           serr = "GridIcos::do_elems2d (1): ";
  GFTYPE            fact, xlatc, xlongc;
  GTVector<GTPoint<GTICOS>> cverts(4), gverts(4), tverts(4);
  GTPoint<GTICOS>    c(3), ct(3), v1(3), v2(3), v3(3); // 3d points
  GElem_base        *pelem;
  GTVector<GINT>    iind;
  GTVector<GINT>    I(1);
  GTVector<GTICOS>  Ni;
  GTVector<GTVector<GFTYPE>>   *xNodes;
  GTVector<GTVector<GFTYPE>*>  *xiNodes;
  GTVector<GTVector<GTICOS>>    xd;
  GTVector<GTVector<GTICOS>>    xid;
  GTVector<GTVector<GTICOS>*>   pxid;
  GTVector<GTVector<GTICOS>>    xgtmp(3);

  // Do eveything on unit sphere, then project to radiusi_
  // as a final step.

  assert(gbasis_.size()>0 && "Must set basis first");

  if ( gdd_ == NULLPTR ) gdd_ = new GDD_base<GTICOS>(nprocs_);

  // Resize points to appropriate size:
  for ( GSIZET j=0; j<tmesh_.size(); j++ ) tmesh_[j].resize(3);
  for ( GSIZET j=0; j<4; j++ ) {
    cverts[j].resize(3); // is a 3d point
    gverts[j].resize(2); // is only a 2d point
    tverts[j].resize(2); // is only a 2d point
  }

  // Get indirection indices to create elements
  // only for this task:
  gdd_->doDD(ftcentroids_, irank, iind);


  GTVector<GSIZET> isort;


  // When setting elements, must first construct Cartesian
  // coordinates at interior node points. This is done in
  // following steps:
  //   (0) find element vertices from triangular mesh
  //   (1) find centroid of element
  //   (2) use centroid to find gnomonic (2d) vertices of element
  //   (3) construct interior node points from gnomonic vertices and basis
  //   (4) transform inter node coords back to 3D Cartesian space from gnomonic space
  //   (5) project the node coords to sphere in Cart. coords 
  fact = 1.0/3.0;
  GSIZET i;
  GSIZET nfnodes;   // no. face nodes
  GSIZET icurr = 0; // current global index
  GSIZET fcurr = 0; // current global face index
  // For each triangle in base mesh owned by this rank...
  for ( GSIZET n=0; n<iind.size(); n++ ) { 
    i = iind[n];
    copycast<GTICOS,GTICOS>(*tmesh_[i].v[0], v1);
    copycast<GTICOS,GTICOS>(*tmesh_[i].v[1], v2);
    copycast<GTICOS,GTICOS>(*tmesh_[i].v[2], v3);
    ct = (v1 + (v2 + v3)) * fact;  // triangle centroid; don't overwrite later on
    // Compute element vertices:
    // NOTE: Is this ordering compatible with shape functions 
    // (placement of +/-1 with physical point)?
    for ( GSIZET j=0; j<3; j++ ) { // 3 new elements for each triangle
      pelem = new GElem_base(GE_2DEMBEDDED, gbasis_);
      cverts[0] = v1; cverts[1] = (v1+v2)*0.5; cverts[2] = ct; cverts[3] = (v1+v3)*0.5;
#if 0
      order_latlong2d<GTICOS>(cverts);
#else
      if ( iup_[i] == 1 ) {
      switch (j) {
      case 0: 
      cverts[0] = v1; cverts[1] = (v1+v2)*0.5; cverts[2] = ct; cverts[3] = (v1+v3)*0.5; break;
      case 1: 
      cverts[0] = (v1+v2)*0.5; cverts[1] = v2; cverts[2] = (v2+v3)*0.5; cverts[3] = ct; break;
      case 2: 
      cverts[0] = ct; cverts[1] = (v2+v3)*0.5; cverts[2] = v3; cverts[3] = (v1+v3)*0.5; break;
      }
      }
      else {
      switch (j) {
      case 0: 
      cverts[0] = v1; cverts[1] = (v1+v2)*0.5; cverts[2] = ct; cverts[3] = (v1+v3)*0.5; break;
      case 1: 
      cverts[0] = ct; cverts[1] = (v1+v2)*0.5; cverts[2] = v2; cverts[3] = (v2+v3)*0.5; break;
      case 2: 
      cverts[0] = (v1+v3)*0.5; cverts[1] = ct; cverts[2] = (v2+v3)*0.5; cverts[3] = v3; break;
      }
      }
#endif
      
      xNodes  = &pelem->xNodes();  // node spatial data
      xiNodes = &pelem->xiNodes(); // node ref interval data
      xd .resize(xNodes->size());
      xid.resize(xiNodes->size());
      pxid.resize(xiNodes->size());
      for ( auto l=0; l<xid.size(); l++ ) pxid[l] = &xid[l];
      copycast<GFTYPE,GTICOS>(*xNodes, xd);
      copycast<GFTYPE,GTICOS>(*xiNodes, pxid);
      
      Ni.resize(pelem->nnodes());

      project2sphere<GTICOS>(cverts, 1.0); // project verts to unit sphere     
      c = (cverts[0] + cverts[1] + cverts[2] + cverts[3])*0.25; // elem centroid
      xlatc  = asin(c.x3)         ; // reference lat/long
      xlongc = atan2(c.x2,c.x1);
      xlongc = xlongc < 0.0 ? 2*PI+xlongc : xlongc;

      cart2gnomonic<GTICOS>(cverts, 1.0, xlatc, xlongc, tverts); // gnomonic vertices of quads
      reorderverts2d<GTICOS>(tverts, isort, gverts); // reorder vertices consistenet with shape fcn
      for ( GSIZET l=0; l<2; l++ ) { // loop over available gnomonic coords
        xgtmp[l].resizem(pelem->nnodes());
        xgtmp[l] = 0.0;
        for ( GSIZET m=0; m<4; m++ ) { // loop over gnomonic vertices
          I[0] = m;
          lshapefcn_->Ni(I, pxid, Ni);
          xgtmp[l] += (Ni * (gverts[m][l]*0.25)); // gnomonic node coordinate
        }
      }
      gnomonic2cart<GTICOS>(xgtmp, 1.0, xlatc, xlongc, xd); //
      project2sphere<GTICOS>(xd, radiusi_);
      for ( auto l=0; l<xd.size(); l++ ) {
        for ( auto k=0; k<xd[l].size(); k++ ) (*xNodes)[l][k] = xd[l][k];
      }
      pelem->init(*xNodes);

      nfnodes = 0;
      for ( GSIZET j=0; j<pelem->nfaces(); j++ )  // get # face nodes
        nfnodes += pelem->face_indices(j).size();
      pelem->igbeg() = icurr;      // beginning global index
      pelem->igend() = icurr+pelem->nnodes()-1; // end global index
      pelem->ifbeg() = fcurr;
      pelem->ifend() = fcurr+nfnodes-1; // end global face index
      icurr += pelem->nnodes();
      fcurr += nfnodes;

      gelems_.push_back(pelem);

    } // end of element loop for this triangle
  } // end of triangle base mesh loop

} // end of method do_elems2d (1)


//**********************************************************************************
//**********************************************************************************
// METHOD : do_elems3d (1)
// DESC   : Build 3d elemental grid. It's assumed that init3d has been called
//          prior to entry, and that the icos base grid has been set.
// ARGS   : 
//          irank: MPI rank or partition id
// RETURNS: none.
//**********************************************************************************
void GGridIcos::do_elems3d(GINT irank)
{
  GString           serr = "GridIcos::do_elems3d (1): ";
  GSIZET            nxy;
  GTICOS            fact, r0, rdelta, xlatc, xlongc;
  GTVector<GTPoint<GTICOS>>
                    cverts(4), gverts(4), tverts(4);
  GTPoint<GTICOS>   c(3), ct(3), v1(3), v2(3), v3(3); // 3d points
  GElem_base        *pelem;
  GElem_base        *pelem2d;
  GTVector<GINT>    iind;
  GTVector<GINT>    I(1);
  GTVector<GTICOS>  Ni;
  GTVector<GTVector<GFTYPE>>  *xNodes;
  GTVector<GTVector<GFTYPE>>   xNodes2d(2);
  GTVector<GTVector<GFTYPE>*>  xiNodes2d(2);
  GTVector<GFTYPE>            *xiNodesr;
  GTVector<GTVector<GTICOS>>   xd, xd2d;
  GTVector<GTVector<GTICOS>>   xid, xid2d;
  GTVector<GTVector<GTICOS>*>  pxid;
  GTVector<GTVector<GTICOS>>   xgtmp(3);

  // Do eveything on unit sphere, then project to radiusi_
  // as a final step.

  assert(gbasis_.size()>0 && "Must set basis first");

  if ( gdd_ == NULLPTR ) gdd_ = new GDD_base<GTICOS>(nprocs_);

  // Resize points to appropriate size:
  for ( GSIZET j=0; j<tmesh_.size(); j++ ) tmesh_[j].resize(3);
  for ( GSIZET j=0; j<4; j++ ) {
    cverts[j].resize(3); // is a 3d point
    gverts[j].resize(2); // is only a 2d point
    tverts[j].resize(2); // is only a 2d point
  }

  // Get indirection indices to create elements
  // only for this task:
  gdd_->doDD(ftcentroids_, irank, iind);

  GTVector<GSIZET> isort;

  // Make radial dimension of elements the same:
  rdelta = (radiuso_ - radiusi_)/nradelem_;


  // When setting elements, must first construct Cartesian
  // coordinates at interior node points. This is done in
  // following steps:
  //   (0) find element vertices from triangular mesh
  //   (1) find centroid of element
  //   (2) use centroid to find gnomonic (2d) vertices of element
  //   (3) construct interior node points from gnomonic vertices and basis
  //   (4) transform inter node coords back to 3D Cartesian space from gnomonic space
  //   (5) project the node coords to sphere in sph. coords 
  //   (6) extend 'patch' in radial direction for all nodes in each
  //       radial element, transforming each element to Cartesian coords
  fact = 1.0/3.0;
  GSIZET i;
  GSIZET nfnodes;   // no. face nodes
  GSIZET icurr = 0; // current global index
  GSIZET fcurr = 0; // current global face index
  // For each triangle in base mesh owned by this rank...
  for ( GSIZET n=0; n<iind.size(); n++ ) { 
    i = iind[n];
    copycast<GTICOS,GTICOS>(*tmesh_[i].v[0], v1);
    copycast<GTICOS,GTICOS>(*tmesh_[i].v[1], v2);
    copycast<GTICOS,GTICOS>(*tmesh_[i].v[2], v3);
    ct = (v1 + (v2 + v3)) * fact;  // triangle centroid; don't overwrite later on
    // Compute element vertices:
    // NOTE: Is this ordering compatible with shape functions 
    // (placement of +/-1 with physical point)?
    for ( GSIZET j=0; j<3; j++ ) { // 3 new elements for each triangle
      pelem2d = new GElem_base(GE_2DEMBEDDED, gbasis_);
      cverts[0] = v1; cverts[1] = (v1+v2)*0.5; cverts[2] = ct; cverts[3] = (v1+v3)*0.5;
      if ( iup_[i] == 1 ) {
      switch (j) {
      case 0: 
      cverts[0] = v1; cverts[1] = (v1+v2)*0.5; cverts[2] = ct; cverts[3] = (v1+v3)*0.5; break;
      case 1: 
      cverts[0] = (v1+v2)*0.5; cverts[1] = v2; cverts[2] = (v2+v3)*0.5; cverts[3] = ct; break;
      case 2: 
      cverts[0] = ct; cverts[1] = (v2+v3)*0.5; cverts[2] = v3; cverts[3] = (v1+v3)*0.5; break;
      }
      }
      else {
      switch (j) {
      case 0: 
      cverts[0] = v1; cverts[1] = (v1+v2)*0.5; cverts[2] = ct; cverts[3] = (v1+v3)*0.5; break;
      case 1: 
      cverts[0] = ct; cverts[1] = (v1+v2)*0.5; cverts[2] = v2; cverts[3] = (v2+v3)*0.5; break;
      case 2: 
      cverts[0] = (v1+v3)*0.5; cverts[1] = ct; cverts[2] = (v2+v3)*0.5; cverts[3] = v3; break;
      }
      }
      
      nxy     = pelem2d->nnodes();
      Ni.resize(nxy);
      for ( GINT m=0; m<2; m++ ) {
        xiNodes2d[m] = &pelem2d->xiNodes(m);
        xNodes2d [m].resizem(nxy);
      }
      xd2d .resize(xNodes2d.size());
      xid2d.resize(xiNodes2d.size());
      pxid .resize(xiNodes2d.size());
      for ( auto l=0; l<xid.size(); l++ ) pxid[l] = &xid2d[l];
      copycast<GFTYPE,GTICOS>(xNodes2d, xd2d);
      copycast<GFTYPE,GTICOS>(xiNodes2d, pxid);

      project2sphere<GTICOS>(cverts, 1.0); // project verts to unit sphere     
      c = (cverts[0] + cverts[1] + cverts[2] + cverts[3])*0.25; // elem centroid
      xlatc  = asin(c.x3)         ; // reference lat/long
      xlongc = atan2(c.x2,c.x1);
      xlongc = xlongc < 0.0 ? 2*PI+xlongc : xlongc;

      cart2gnomonic<GTICOS>(cverts, 1.0, xlatc, xlongc, tverts); // gnomonic vertices of quads
      reorderverts2d<GTICOS>(tverts, isort, gverts); // reorder vertices consistenet with shape fcn
      for ( GINT l=0; l<2; l++ ) { // loop over available gnomonic coords
        xgtmp[l].resizem(nxy);
        xgtmp[l] = 0.0;
        for ( GSIZET m=0; m<4; m++ ) { // loop over gnomonic vertices
          I[0] = m;
          lshapefcn_->Ni(I, pxid, Ni);
          xgtmp[l] += (Ni * (gverts[m][l]*0.25)); // gnomonic node coordinate
        }
      }

      // Convert 2d plane back to Cart coords and project to 
      // surface of inner sphere:
      gnomonic2cart<GTICOS>(xgtmp, 1.0, xlatc, xlongc, xd2d); 
      project2sphere<GTICOS>(xd2d, radiusi_);
      xyz2spherical<GTICOS>(xd2d);

      // Loop over radial elements and build all elements 
      // based on this patch (we're still in sph coords here):
      for ( GSIZET e=0; e<nradelem_; e++ ) {
        pelem = new GElem_base(GE_DEFORMED, gbasis_);
        xiNodesr = &pelem->xiNodes(2); // get radial reference nodes
        xNodes   = &pelem->xNodes();  // node spatial data
        r0       = radiusi_ + e*rdelta;
        for ( GSIZET m=0; m<pelem->size(2); m++ ) { // for each radial node
          for ( GSIZET n=0; n<nxy; n++ ) { // find sph coords for each horizontal node
            (*xNodes)[0][n+m*nxy] =  0.5*rdelta*((*xiNodesr)[m] + 1.0);
            (*xNodes)[1][n+m*nxy] =  xd2d[1][n];
            (*xNodes)[2][n+m*nxy] =  xd2d[2][n];
          }
        }
        spherical2xyz<GFTYPE>(*xNodes);

        pelem->init(*xNodes);

        nfnodes = 0;
        for ( GSIZET j=0; j<pelem->nfaces(); j++ )  // get # face nodes
          nfnodes += pelem->face_indices(j).size();
        pelem->igbeg() = icurr;      // beginning global index
        pelem->igend() = icurr+pelem->nnodes()-1; // end global index
        pelem->ifbeg() = fcurr;
        pelem->ifend() = fcurr+nfnodes-1; // end global face index
        icurr += pelem->nnodes();
        fcurr += nfnodes;

        gelems_.push_back(pelem);
      }
      delete pelem2d;

    } // end of element loop for this triangle
  } // end of triangle base mesh loop

} // end of method do_elems3d (1)


//**********************************************************************************
//**********************************************************************************
// METHOD : do_elems2d (2)
// DESC   : Build 2d elemental grid on base mesh
// ARGS   : p      : matrix of size the number of elements X GDIM containing 
//                   the poly expansion order in each direction
//          gxnodes: vector of GDIM vectors containing Cartesian coords of elements
// RETURNS: none.
//**********************************************************************************
void GGridIcos::do_elems2d(GTMatrix<GINT> &p,
                           GTVector<GTVector<GFTYPE>> &gxnodes)
{
  GString                     serr = "GridIcos::do_elems2d (2): ";
  GElem_base                  *pelem;
  GTVector<GTVector<GFTYPE>>  *xNodes;
  GTVector<GTVector<GFTYPE>*> *xiNodes;
  GTVector<GNBasis<GCTYPE,GFTYPE>*>
                               gb(GDIM);
  GTVector<GINT>               ppool(gbasis_.size());

  // Now, treat the gbasis_ as a pool that we search
  // to find bases we need:
  for ( GSIZET j=0; j<ppool.size(); j++ ) ppool[j] = gbasis_[j]->getOrder();


  // Set element internal dof from input data:
  GSIZET iwhere ;
  GSIZET nvnodes;   // no. vol nodes
  GSIZET nfnodes;   // no. face nodes
  GSIZET icurr = 0; // current global index
  GSIZET fcurr = 0; // current global face index
  // For each triangle in base mesh owned by this rank...
  for ( GSIZET i=0; i<p.size(1); i++ ) { 
    nvnodes = 1;
    for ( GSIZET j=0; j<GDIM; j++ ) { // set basis from pool
      assert(ppool.contains(p(i,j),iwhere) && "Expansion order not found");
      gb[j] = gbasis_[iwhere];
      nvnodes *= (p(i,j) + 1);
    }
    pelem = new GElem_base(GE_2DEMBEDDED, gb);
    xNodes  = &pelem->xNodes();  // node spatial data
    xiNodes = &pelem->xiNodes(); // node ref interval data
#if 0
    bdy_ind = &pelem->bdy_indices(); // get bdy indices data member
    bdy_typ = &pelem->bdy_types  (); // get bdy types data member
    bdy_ind->clear(); bdy_typ->clear();
#endif

    // Set internal node positions from input data.
    // Note that gxnodes are 'global' and xNodes is
    // element-local:
    for ( GSIZET j=0; j<GDIM; j++ ) {
       gxnodes[j].range(icurr, icurr+nvnodes-1);
      (*xNodes)[j] = gxnodes[j];
    }
    for ( GSIZET j=0; j<GDIM; j++ ) gxnodes[j].range_reset();

    pelem->init(*xNodes);
    gelems_.push_back(pelem);

    assert(nvnodes == gelems_[i]->nnodes() && "Incompatible node count");
    nfnodes = gelems_[i]->nfnodes();
    pelem->igbeg() = icurr;      // beginning global index
    pelem->igend() = icurr+nvnodes-1; // end global index
    pelem->ifbeg() = fcurr;
    pelem->ifend() = fcurr+nfnodes-1; // end global face index
    icurr += nvnodes;
    fcurr += nfnodes;
  } // end of triangle base mesh loop

} // end of method do_elems2d (2)


//**********************************************************************************
//**********************************************************************************
// METHOD : do_elems3d (2)
// DESC   : Build 3d elemental grid. It's assumed that init3d has been called
//          prior to entry, and that the base icos grid has beed set. 
// ARGS   : p      : matrix of size the number of elements X GDIM containing 
//                   the poly expansion order in each direction
//          gxnodes: vector of GDIM vectors containing Cartesian coords of elements
// RETURNS: none.
//**********************************************************************************
void GGridIcos::do_elems3d(GTMatrix<GINT> &p,
                           GTVector<GTVector<GFTYPE>> &gxnodes)
{
  GString                      serr = "GridIcos::do_elems3d (2): ";
  GElem_base                  *pelem;
  GTVector<GTVector<GFTYPE>>  *xNodes;
  GTVector<GTVector<GFTYPE>*> *xiNodes;
  GTVector<GNBasis<GCTYPE,GFTYPE>*>
                               gb(GDIM);
  GTVector<GINT>               ppool(gbasis_.size());

  // Now, treat the gbasis_ as a pool that we search
  // to find bases we need:
  for ( GSIZET j=0; j<ppool.size(); j++ ) ppool[j] = gbasis_[j]->getOrder();


  // Set element internal dof from input data:
  GSIZET iwhere ;
  GSIZET nvnodes;   // no. vol nodes
  GSIZET nfnodes;   // no. face nodes
  GSIZET icurr = 0; // current global index
  GSIZET fcurr = 0; // current global face index
  // For each triangle in base mesh owned by this rank...
  for ( GSIZET i=0; i<p.size(1); i++ ) { 
    nvnodes = 1;
    for ( GSIZET j=0; j<GDIM; j++ ) { // set basis from pool
      assert(ppool.contains(p(i,j),iwhere) && "Expansion order not found");
      gb[j] = gbasis_[iwhere];
      nvnodes *= (p(i,j) + 1);
    }
    pelem = new GElem_base(GE_DEFORMED, gb);
    xNodes  = &pelem->xNodes();  // node spatial data
    xiNodes = &pelem->xiNodes(); // node ref interval data
#if 0
    bdy_ind = &pelem->bdy_indices(); // get bdy indices data member
    bdy_typ = &pelem->bdy_types  (); // get bdy types data member
    bdy_ind->clear(); bdy_typ->clear();
#endif

    // Set internal node positions from input data.
    // Note that gxnodes are 'global' and xNodes is
    // element-local:
    for ( GSIZET j=0; j<GDIM; j++ ) {
       gxnodes[j].range(icurr, icurr+nvnodes-1);
      (*xNodes)[j] = gxnodes[j];
    }
    for ( GSIZET j=0; j<GDIM; j++ ) gxnodes[j].range_reset();

    pelem->init(*xNodes);
    gelems_.push_back(pelem);

    assert(nvnodes == gelems_[i]->nnodes() && "Incompatible node count");
    nfnodes = gelems_[i]->nfnodes();
    pelem->igbeg() = icurr;      // beginning global index
    pelem->igend() = icurr+nvnodes-1; // end global index
    pelem->ifbeg() = fcurr;
    pelem->ifend() = fcurr+nfnodes-1; // end global face index
    icurr += nvnodes;
    fcurr += nfnodes;
  } // end of triangle base mesh loop

} // end of method do_elems3d (2)


//**********************************************************************************
//**********************************************************************************
// METHOD : print
// DESC   : Print final (global triangular) base mesh. It is often the case 
//          that we will print the mesh to a file for visualization, so this is a 
//          utility that allows us to do this easily. A stream operator is 
//          still provided to print in a completely formatted way.
// ARGS   : filename: filename to print to
//          icoord  : GCOORDSYST: GICOS_CART or GICOS_LATLONG, for cartesian or
//                    lat-long where appropriate
// RETURNS: none.
//**********************************************************************************
void GGridIcos::print(const GString &filename, GCOORDSYST icoord)
{
  GString serr = "GridIcos::print: ";
  std::ofstream ios;

  GTPoint<GTICOS> pt;
  GTICOS          r, xlat, xlong;

  ios.open(filename);
  if ( icoord == GICOS_LATLONG) { // print in lat-long
    for ( GSIZET i=0; i<tmesh_.size(); i++ ) { // for each triangle
      for ( GSIZET j=0; j<3; j++ ) { // for each vertex of triangle
        pt = *tmesh_[i].v[j];
        r = sqrt(pt.x1*pt.x1 + pt.x2*pt.x2 + pt.x3*pt.x3);
        xlat  = asin(pt.x3/r);
        xlong = atan2(pt.x2,pt.x1);
        xlong = xlong < 0.0 ? 2*PI+xlong : xlong;
#if defined(_G_IS2D)
        ios << xlat << " " <<  xlong << std::endl;
#elif defined(_G_IS3D)
        ios << r << " " << xlat << " " <<  xlong << std::endl;
#endif
      }
    }
  }
  else if ( icoord == GICOS_CART ) { // print in Cartesian
    for ( GSIZET i=0; i<tmesh_.size(); i++ ) { // for each triangle
      for ( GSIZET j=0; j<3; j++ ) { // for each vertex of triangle
        pt = *tmesh_[i].v[j];
        ios << pt.x1 << " " << pt.x2 << " " << pt.x3 << std::endl ;
      }
    }
  }

  ios.close();

} // end of method print


//**********************************************************************************
//**********************************************************************************
// METHOD : config_bdy
// DESC   : Configure 3d spherical boundaries from ptree
// ARGS   : 
//          ptree : main prop tree 
//          igbdy : For each natural/canonical global boundary face,
//                  gives vector of global bdy ids
//          igbdyt: bdy type ids for each index in igbdy
// RETURNS: none.
//**********************************************************************************
void GGridIcos::config_bdy(const PropertyTree &ptree, 
                           GTVector<GTVector<GSIZET>> &igbdy, 
                           GTVector<GTVector<GBdyType>> &igbdyt)
{
  // Cycle over all geometric boundaries, and configure:

  GBOOL              bret, buniform=FALSE;
  GSIZET             iwhere;
  GTVector<GBOOL>    uniform(2);
  GTVector<GBdyType> bdytype(2);
  GTVector<GBdyType> btmp;
  GTVector<GSIZET>   itmp;
  GTVector<GFTYPE>   rbdy(2);
  GTVector<GString>  bdynames(2);
  GTVector<GString>  confmthd (2);
  GTVector<GString>  bdyupdate(2);
  GString            gname, sbdy, bdyclass;
  PropertyTree       bdytree, gridptree, spectree;

  // Clear input arrays:
  igbdy .clear();
  igbdyt.clear();

  if ( ndim_ == 2 ) return; // no boundaries to configure
 
  bdynames[0] = "bdy_inner";
  bdynames[1] = "bdy_outer";

  gname     = ptree.getValue<GString>("grid_type");
  gridptree = ptree.getPropertyTree(gname);


  bdyupdate = gridptree.getValue<GString>("update_method","");
  rbdy[0] = radiusi_;
  rbdy[1] = radiuso_;

  // Get properties from the main prop tree. 
  // Note: bdys are configured by way of geometry's
  //       natural decomposition: here, by inner and
  //       outer spherical surfaces. But the bdy indices 
  //       and types returned on exist contain info for all bdys:
  for ( auto j=0; j<2; j++ ) { // cycle over 2 spherical surfaces
    sbdy         = gridptree.getValue<GString>(bdynames[j]);
    bdytree      = gridptree.getPropertyTree(sbdy);
    bdyclass     = bdytree.getValue<GString>("bdy_class", "uniform");
    bdytype  [j] = geoflow::str2bdytype(bdytree.getValue<GString>("base_type", "GBDY_NONE"));
    assert(bdytype  [j] == GBDY_PERIODIC && "Invalid boundary condition");
    uniform  [j] = bdyclass == "uniform" ? TRUE : FALSE;
    confmthd [j] = bdytree.getValue<GString>("bdy_config_method","");
    buniform     = buniform || uniform[j];
  }

  // Handle non-uniform (user-configured) bdy types first;
  // Note: If "uniform" not specified for a boundary, then
  //       user MUST supply a method to configure it.
  //       Also, each natural face may be configured independently,
  //       but the bdy indices & corresp. types are concatenated into 
  //       single arrays:
  for ( auto j=0; j<2; j++ ) { 
    // First, find global bdy indices:
    if ( uniform[j] ) continue;
    find_bdy_ind3d(rbdy[j], itmp); 
    spectree  = ptree.getPropertyTree(confmthd[j]);
    bret = GSpecBdyFactory::dospec(spectree, *this, j, itmp, btmp); // get user-defined bdy spec
    assert(bret && "Boundary specification failed");
    igbdy [j].resize(itmp.size()); igbdy [j] = itmp;
    igbdyt[j].resize(itmp.size()); igbdyt[j] = btmp;

  }
  
  // Fill in uniform bdy types:
  for ( auto j=0; j<2; j++ ) { 
    if ( !uniform[j] ) continue;
    // First, find global bdy indices:
    find_bdy_ind3d(rbdy[j], itmp); 
    // Set type for each bdy index:
    for ( auto i=0; i<itmp.size(); i++ ) {
      btmp[i] = bdytype[j]; 
    }
    igbdy [j].resize(itmp.size()); igbdy [j] = itmp;
    igbdyt[j].resize(itmp.size()); igbdyt[j] = btmp;
  }


} // end of method config_bdy


//**********************************************************************************
//**********************************************************************************
// METHOD : find_bdy_ind3d
// DESC   : Find global bdy indices (indices into xNodes_ arrays) that
//          corresp to specified radius
// ARGS   : radius   : radius
//          ibdy     : array of indices into xNodes that comprise this boundary
// RETURNS: none.
//**********************************************************************************
void GGridIcos::find_bdy_ind3d(GFTYPE radius, GTVector<GSIZET> &ibdy)
{

  GFTYPE          eps, r;
  GTPoint<GFTYPE> pt(ndim_);

  ibdy.clear();
  eps = 100*std::numeric_limits<GFTYPE>::epsilon();

  for ( GSIZET i=0; i<xNodes_[0].size(); i++ ) { // face 0
      r = sqrt(pow(xNodes_[0][i],2)+pow(xNodes_[1][i],2)+pow(xNodes_[2][i],2));
      if ( FUZZYEQ(r, radius, eps) ) {
        ibdy.push_back(i);
      }
  }

} // end, method find_bdy_ind3d


//**********************************************************************************
//**********************************************************************************
// METHOD : do_face_normals
// DESC   : Compute normals to each element face
// ARGS   : none 
// RETURNS: none
//**********************************************************************************
void GGridIcos::do_face_normals()
{

  #if defined(_G_IS2D)
    do_face_normals2d();
  #elif defined(_G_IS3D)
    do_face_normals3d();
  #else
    #error Invalid problem dimensionality
  #endif

} // end, method do_face_normals


//**********************************************************************************
//**********************************************************************************
// METHOD : do_face_normals2d
// DESC   : Compute normals to each element face in 2d
// ARGS   : none 
// RETURNS: none
//**********************************************************************************
void GGridIcos::do_face_normals2d()
{

  // Cycle through local elem face indices to set
  // normals. Taken in order, these should correspond


} // end, method do_bdy_normals2d


//**********************************************************************************
//**********************************************************************************
// METHOD : do_face_normals3d
// DESC   : Compute normals to each element face in 3d
// ARGS   : none 
// RETURNS: none
//**********************************************************************************
void GGridIcos::do_face_normals3d()
{


} // end, method do_bdy_normals3d



//**********************************************************************************
//**********************************************************************************
// METHOD : do_bdy_normals
// DESC   : Compute normals to each domain bdy
// ARGS   : none 
// RETURNS: none
//**********************************************************************************
void GGridIcos::do_bdy_normals()
{

  #if defined(_G_IS2D)
    return;
  #elif defined(_G_IS3D)
    do_bdy_normals3d();
  #else
    #error Invalid problem dimensionality
  #endif

} // end, method do_bdy_normals


//**********************************************************************************
//**********************************************************************************
// METHOD : do_bdy_normals3d
// DESC   : Compute normals to each domain bdy in 3d
// ARGS   : none 
// RETURNS: none
//**********************************************************************************
void GGridIcos::do_bdy_normals3d()
{

} // end, method do_bdy_normals3d
