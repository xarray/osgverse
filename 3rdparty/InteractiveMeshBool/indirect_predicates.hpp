/****************************************************************************
* Indirect predicates for geometric constructions					        *
*                                                                           *
* Consiglio Nazionale delle Ricerche                                        *
* Istituto di Matematica Applicata e Tecnologie Informatiche                *
* Sezione di Genova                                                         * 
* IMATI-GE / CNR                                                            * 
*                                                                           *
* Authors: Marco Attene                                                     * 
* Copyright(C) 2019: IMATI-GE / CNR                                         * 
* All rights reserved.                                                      * 
*                                                                           *
* This program is FreeDoubles software; you can redistribute it and/or modify      * 
* it under the terms of the GNU Lesser General Public License as published  * 
* by the Free Software Foundation; either version 3 of the License, or (at  * 
* your option) any later version.                                           * 
*                                                                           *
* This program is distributed in the hope that it will be useful, but       * 
* WITHOUT ANY WARRANTY; without even the implied warranty of                * 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser  * 
* General Public License for more details.                                  * 
*                                                                           *
* You should have received a copy of the GNU Lesser General Public License  * 
* along with this program.  If not, see http://www.gnu.org/licenses/.       *
*                                                                           *
****************************************************************************/ 

/* This code was generated automatically. Do not edit unless you exactly   */
/* know what you are doing!                                                */

#include "implicit_point.h"

#pragma intrinsic(fabs)

// Uncomment the following to activate overflow/underflow checks
#define CHECK_FOR_XYZERFLOWS

inline int dotProductSign2D_filtered(double px, double py, double rx, double ry, double qx, double qy)
{
   const double lx = px - qx;
   const double ly = py - qy;
   const double gx = rx - qx;
   const double gy = ry - qy;
   const double dx = lx * gx;
   const double dy = ly * gy;
   const double d = dx + dy;

   double _tmp_fabs;

   double max_var = 0.0;
   if ((_tmp_fabs = fabs(lx)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(ly)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(gx)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(gy)) > max_var) max_var = _tmp_fabs;
   double epsilon = max_var;
   epsilon *= epsilon;
   epsilon *= 8.881784197001253e-16;
   if (d > epsilon) return IP_Sign::POSITIVE;
   if (-d > epsilon) return IP_Sign::NEGATIVE;
   return Filtered_Sign::UNCERTAIN;
}

inline int dotProductSign2D_interval(interval_number px, interval_number py, interval_number rx, interval_number ry, interval_number qx, interval_number qy)
{
   setFPUModeToRoundUP();
   const interval_number lx(px - qx);
   const interval_number ly(py - qy);
   const interval_number gx(rx - qx);
   const interval_number gy(ry - qy);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number d(dx + dy);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign2D_bigfloat(bigfloat px, bigfloat py, bigfloat rx, bigfloat ry, bigfloat qx, bigfloat qy)
{
   const bigfloat lx(px - qx);
   const bigfloat ly(py - qy);
   const bigfloat gx(rx - qx);
   const bigfloat gy(ry - qy);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat d(dx + dy);
   return sgn(d);
}

inline int dotProductSign2D_exact(double px, double py, double rx, double ry, double qx, double qy)
{
   
   double lx[2];
   expansionObject::two_Diff(px, qx, lx);
   double ly[2];
   expansionObject::two_Diff(py, qy, ly);
   double gx[2];
   expansionObject::two_Diff(rx, qx, gx);
   double gy[2];
   expansionObject::two_Diff(ry, qy, gy);
   double dx[8];
   int dx_len = expansionObject::Gen_Product(2, lx, 2, gx, dx);
   double dy[8];
   int dy_len = expansionObject::Gen_Product(2, ly, 2, gy, dy);
   double d[16];
   int d_len = expansionObject::Gen_Sum(dx_len, dx, dy_len, dy, d);

   double return_value = d[d_len - 1];

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign2D(double px, double py, double rx, double ry, double qx, double qy)
{
   int ret;
   ret = dotProductSign2D_filtered(px, py, rx, ry, qx, qy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   ret = dotProductSign2D_interval(px, py, rx, ry, qx, qy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign2D_exact(px, py, rx, ry, qx, qy);
}

inline int dotProductSign3D_filtered(double px, double py, double pz, double rx, double ry, double rz, double qx, double qy, double qz)
{
   const double lx = px - qx;
   const double ly = py - qy;
   const double lz = pz - qz;
   const double gx = rx - qx;
   const double gy = ry - qy;
   const double gz = rz - qz;
   const double dx = lx * gx;
   const double dy = ly * gy;
   const double dz = lz * gz;
   const double d1 = dx + dy;
   const double d = d1 + dz;

   double _tmp_fabs;

   double max_var = 0.0;
   if ((_tmp_fabs = fabs(lx)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(ly)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(lz)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(gx)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(gy)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(gz)) > max_var) max_var = _tmp_fabs;
   double epsilon = max_var;
   epsilon *= epsilon;
   epsilon *= 1.443289932012704e-15;
   if (d > epsilon) return IP_Sign::POSITIVE;
   if (-d > epsilon) return IP_Sign::NEGATIVE;
   return Filtered_Sign::UNCERTAIN;
}

inline int dotProductSign3D_interval(interval_number px, interval_number py, interval_number pz, interval_number rx, interval_number ry, interval_number rz, interval_number qx, interval_number qy, interval_number qz)
{
   setFPUModeToRoundUP();
   const interval_number lx(px - qx);
   const interval_number ly(py - qy);
   const interval_number lz(pz - qz);
   const interval_number gx(rx - qx);
   const interval_number gy(ry - qy);
   const interval_number gz(rz - qz);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number dz(lz * gz);
   const interval_number d1(dx + dy);
   const interval_number d(d1 + dz);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign3D_bigfloat(bigfloat px, bigfloat py, bigfloat pz, bigfloat rx, bigfloat ry, bigfloat rz, bigfloat qx, bigfloat qy, bigfloat qz)
{
   const bigfloat lx(px - qx);
   const bigfloat ly(py - qy);
   const bigfloat lz(pz - qz);
   const bigfloat gx(rx - qx);
   const bigfloat gy(ry - qy);
   const bigfloat gz(rz - qz);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat dz(lz * gz);
   const bigfloat d1(dx + dy);
   const bigfloat d(d1 + dz);
   return sgn(d);
}

inline int dotProductSign3D_exact(double px, double py, double pz, double rx, double ry, double rz, double qx, double qy, double qz)
{
   
   double lx[2];
   expansionObject::two_Diff(px, qx, lx);
   double ly[2];
   expansionObject::two_Diff(py, qy, ly);
   double lz[2];
   expansionObject::two_Diff(pz, qz, lz);
   double gx[2];
   expansionObject::two_Diff(rx, qx, gx);
   double gy[2];
   expansionObject::two_Diff(ry, qy, gy);
   double gz[2];
   expansionObject::two_Diff(rz, qz, gz);
   double dx[8];
   int dx_len = expansionObject::Gen_Product(2, lx, 2, gx, dx);
   double dy[8];
   int dy_len = expansionObject::Gen_Product(2, ly, 2, gy, dy);
   double dz[8];
   int dz_len = expansionObject::Gen_Product(2, lz, 2, gz, dz);
   double d1[16];
   int d1_len = expansionObject::Gen_Sum(dx_len, dx, dy_len, dy, d1);
   double d[24];
   int d_len = expansionObject::Gen_Sum(d1_len, d1, dz_len, dz, d);

   double return_value = d[d_len - 1];

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign3D(double px, double py, double pz, double rx, double ry, double rz, double qx, double qy, double qz)
{
   int ret;
   ret = dotProductSign3D_filtered(px, py, pz, rx, ry, rz, qx, qy, qz);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   ret = dotProductSign3D_interval(px, py, pz, rx, ry, rz, qx, qy, qz);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign3D_exact(px, py, pz, rx, ry, rz, qx, qy, qz);
}

inline int incircle_filtered(double pax, double pay, double pbx, double pby, double pcx, double pcy, double pdx, double pdy)
{
   const double adx = pax - pdx;
   const double ady = pay - pdy;
   const double bdx = pbx - pdx;
   const double bdy = pby - pdy;
   const double cdx = pcx - pdx;
   const double cdy = pcy - pdy;
   const double abdeta = adx * bdy;
   const double abdetb = bdx * ady;
   const double abdet = abdeta - abdetb;
   const double bcdeta = bdx * cdy;
   const double bcdetb = cdx * bdy;
   const double bcdet = bcdeta - bcdetb;
   const double cadeta = cdx * ady;
   const double cadetb = adx * cdy;
   const double cadet = cadeta - cadetb;
   const double alifta = adx * adx;
   const double aliftb = ady * ady;
   const double alift = alifta + aliftb;
   const double blifta = bdx * bdx;
   const double bliftb = bdy * bdy;
   const double blift = blifta + bliftb;
   const double clifta = cdx * cdx;
   const double cliftb = cdy * cdy;
   const double clift = clifta + cliftb;
   const double la = alift * bcdet;
   const double lb = blift * cadet;
   const double lc = clift * abdet;
   const double lab = la + lb;
   const double L = lab + lc;

   double _tmp_fabs;

   double max_var = 0.0;
   if ((_tmp_fabs = fabs(adx)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(ady)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(bdx)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(bdy)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(cdx)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(cdy)) > max_var) max_var = _tmp_fabs;
   double epsilon = max_var;
   epsilon *= epsilon;
   epsilon *= epsilon;
   epsilon *= 1.376676550535195e-14;
   if (L > epsilon) return IP_Sign::POSITIVE;
   if (-L > epsilon) return IP_Sign::NEGATIVE;
   return Filtered_Sign::UNCERTAIN;
}

inline int incircle_interval(interval_number pax, interval_number pay, interval_number pbx, interval_number pby, interval_number pcx, interval_number pcy, interval_number pdx, interval_number pdy)
{
   setFPUModeToRoundUP();
   const interval_number adx(pax - pdx);
   const interval_number ady(pay - pdy);
   const interval_number bdx(pbx - pdx);
   const interval_number bdy(pby - pdy);
   const interval_number cdx(pcx - pdx);
   const interval_number cdy(pcy - pdy);
   const interval_number abdeta(adx * bdy);
   const interval_number abdetb(bdx * ady);
   const interval_number abdet(abdeta - abdetb);
   const interval_number bcdeta(bdx * cdy);
   const interval_number bcdetb(cdx * bdy);
   const interval_number bcdet(bcdeta - bcdetb);
   const interval_number cadeta(cdx * ady);
   const interval_number cadetb(adx * cdy);
   const interval_number cadet(cadeta - cadetb);
   const interval_number alifta(adx * adx);
   const interval_number aliftb(ady * ady);
   const interval_number alift(alifta + aliftb);
   const interval_number blifta(bdx * bdx);
   const interval_number bliftb(bdy * bdy);
   const interval_number blift(blifta + bliftb);
   const interval_number clifta(cdx * cdx);
   const interval_number cliftb(cdy * cdy);
   const interval_number clift(clifta + cliftb);
   const interval_number la(alift * bcdet);
   const interval_number lb(blift * cadet);
   const interval_number lc(clift * abdet);
   const interval_number lab(la + lb);
   const interval_number L(lab + lc);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int incircle_bigfloat(bigfloat pax, bigfloat pay, bigfloat pbx, bigfloat pby, bigfloat pcx, bigfloat pcy, bigfloat pdx, bigfloat pdy)
{
   const bigfloat adx(pax - pdx);
   const bigfloat ady(pay - pdy);
   const bigfloat bdx(pbx - pdx);
   const bigfloat bdy(pby - pdy);
   const bigfloat cdx(pcx - pdx);
   const bigfloat cdy(pcy - pdy);
   const bigfloat abdeta(adx * bdy);
   const bigfloat abdetb(bdx * ady);
   const bigfloat abdet(abdeta - abdetb);
   const bigfloat bcdeta(bdx * cdy);
   const bigfloat bcdetb(cdx * bdy);
   const bigfloat bcdet(bcdeta - bcdetb);
   const bigfloat cadeta(cdx * ady);
   const bigfloat cadetb(adx * cdy);
   const bigfloat cadet(cadeta - cadetb);
   const bigfloat alifta(adx * adx);
   const bigfloat aliftb(ady * ady);
   const bigfloat alift(alifta + aliftb);
   const bigfloat blifta(bdx * bdx);
   const bigfloat bliftb(bdy * bdy);
   const bigfloat blift(blifta + bliftb);
   const bigfloat clifta(cdx * cdx);
   const bigfloat cliftb(cdy * cdy);
   const bigfloat clift(clifta + cliftb);
   const bigfloat la(alift * bcdet);
   const bigfloat lb(blift * cadet);
   const bigfloat lc(clift * abdet);
   const bigfloat lab(la + lb);
   const bigfloat L(lab + lc);
   return sgn(L);
}

inline int incircle_exact(double pax, double pay, double pbx, double pby, double pcx, double pcy, double pdx, double pdy)
{
   
   double adx[2];
   expansionObject::two_Diff(pax, pdx, adx);
   double ady[2];
   expansionObject::two_Diff(pay, pdy, ady);
   double bdx[2];
   expansionObject::two_Diff(pbx, pdx, bdx);
   double bdy[2];
   expansionObject::two_Diff(pby, pdy, bdy);
   double cdx[2];
   expansionObject::two_Diff(pcx, pdx, cdx);
   double cdy[2];
   expansionObject::two_Diff(pcy, pdy, cdy);
   double abdeta[8];
   int abdeta_len = expansionObject::Gen_Product(2, adx, 2, bdy, abdeta);
   double abdetb[8];
   int abdetb_len = expansionObject::Gen_Product(2, bdx, 2, ady, abdetb);
   double abdet[16];
   int abdet_len = expansionObject::Gen_Diff(abdeta_len, abdeta, abdetb_len, abdetb, abdet);
   double bcdeta[8];
   int bcdeta_len = expansionObject::Gen_Product(2, bdx, 2, cdy, bcdeta);
   double bcdetb[8];
   int bcdetb_len = expansionObject::Gen_Product(2, cdx, 2, bdy, bcdetb);
   double bcdet[16];
   int bcdet_len = expansionObject::Gen_Diff(bcdeta_len, bcdeta, bcdetb_len, bcdetb, bcdet);
   double cadeta[8];
   int cadeta_len = expansionObject::Gen_Product(2, cdx, 2, ady, cadeta);
   double cadetb[8];
   int cadetb_len = expansionObject::Gen_Product(2, adx, 2, cdy, cadetb);
   double cadet[16];
   int cadet_len = expansionObject::Gen_Diff(cadeta_len, cadeta, cadetb_len, cadetb, cadet);
   double alifta[8];
   int alifta_len = expansionObject::Gen_Product(2, adx, 2, adx, alifta);
   double aliftb[8];
   int aliftb_len = expansionObject::Gen_Product(2, ady, 2, ady, aliftb);
   double alift[16];
   int alift_len = expansionObject::Gen_Sum(alifta_len, alifta, aliftb_len, aliftb, alift);
   double blifta[8];
   int blifta_len = expansionObject::Gen_Product(2, bdx, 2, bdx, blifta);
   double bliftb[8];
   int bliftb_len = expansionObject::Gen_Product(2, bdy, 2, bdy, bliftb);
   double blift[16];
   int blift_len = expansionObject::Gen_Sum(blifta_len, blifta, bliftb_len, bliftb, blift);
   double clifta[8];
   int clifta_len = expansionObject::Gen_Product(2, cdx, 2, cdx, clifta);
   double cliftb[8];
   int cliftb_len = expansionObject::Gen_Product(2, cdy, 2, cdy, cliftb);
   double clift[16];
   int clift_len = expansionObject::Gen_Sum(clifta_len, clifta, cliftb_len, cliftb, clift);
   double la_p[128], *la = la_p;
   int la_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcdet_len, bcdet, &la, 128);
   double lb_p[128], *lb = lb_p;
   int lb_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cadet_len, cadet, &lb, 128);
   double lc_p[128], *lc = lc_p;
   int lc_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, abdet_len, abdet, &lc, 128);
   double lab_p[128], *lab = lab_p;
   int lab_len = expansionObject::Gen_Sum_With_PreAlloc(la_len, la, lb_len, lb, &lab, 128);
   double L_p[128], *L = L_p;
   int L_len = expansionObject::Gen_Sum_With_PreAlloc(lab_len, lab, lc_len, lc, &L, 128);

   double return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (lab_p != lab) FreeDoubles(lab);
   if (lc_p != lc) FreeDoubles(lc);
   if (lb_p != lb) FreeDoubles(lb);
   if (la_p != la) FreeDoubles(la);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int incircle(double pax, double pay, double pbx, double pby, double pcx, double pcy, double pdx, double pdy)
{
   int ret;
   ret = incircle_filtered(pax, pay, pbx, pby, pcx, pcy, pdx, pdy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   ret = incircle_interval(pax, pay, pbx, pby, pcx, pcy, pdx, pdy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return incircle_exact(pax, pay, pbx, pby, pcx, pcy, pdx, pdy);
}

inline int inSphere_filtered(double pax, double pay, double paz, double pbx, double pby, double pbz, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez)
{
   const double aex = pax - pex;
   const double aey = pay - pey;
   const double aez = paz - pez;
   const double bex = pbx - pex;
   const double bey = pby - pey;
   const double bez = pbz - pez;
   const double cex = pcx - pex;
   const double cey = pcy - pey;
   const double cez = pcz - pez;
   const double dex = pdx - pex;
   const double dey = pdy - pey;
   const double dez = pdz - pez;
   const double aexbey = aex * bey;
   const double bexaey = bex * aey;
   const double ab = aexbey - bexaey;
   const double bexcey = bex * cey;
   const double cexbey = cex * bey;
   const double bc = bexcey - cexbey;
   const double cexdey = cex * dey;
   const double dexcey = dex * cey;
   const double cd = cexdey - dexcey;
   const double dexaey = dex * aey;
   const double aexdey = aex * dey;
   const double da = dexaey - aexdey;
   const double aexcey = aex * cey;
   const double cexaey = cex * aey;
   const double ac = aexcey - cexaey;
   const double bexdey = bex * dey;
   const double dexbey = dex * bey;
   const double bd = bexdey - dexbey;
   const double abc1 = aez * bc;
   const double abc2 = bez * ac;
   const double abc3 = cez * ab;
   const double abc4 = abc1 + abc3;
   const double abc = abc4 - abc2;
   const double bcd1 = bez * cd;
   const double bcd2 = cez * bd;
   const double bcd3 = dez * bc;
   const double bcd4 = bcd1 + bcd3;
   const double bcd = bcd4 - bcd2;
   const double cda1 = cez * da;
   const double cda2 = dez * ac;
   const double cda3 = aez * cd;
   const double cda4 = cda1 + cda3;
   const double cda = cda4 + cda2;
   const double dab1 = dez * ab;
   const double dab2 = aez * bd;
   const double dab3 = bez * da;
   const double dab4 = dab1 + dab3;
   const double dab = dab4 + dab2;
   const double al1 = aex * aex;
   const double al2 = aey * aey;
   const double al3 = aez * aez;
   const double al4 = al1 + al2;
   const double alift = al4 + al3;
   const double bl1 = bex * bex;
   const double bl2 = bey * bey;
   const double bl3 = bez * bez;
   const double bl4 = bl1 + bl2;
   const double blift = bl4 + bl3;
   const double cl1 = cex * cex;
   const double cl2 = cey * cey;
   const double cl3 = cez * cez;
   const double cl4 = cl1 + cl2;
   const double clift = cl4 + cl3;
   const double dl1 = dex * dex;
   const double dl2 = dey * dey;
   const double dl3 = dez * dez;
   const double dl4 = dl1 + dl2;
   const double dlift = dl4 + dl3;
   const double ds1 = dlift * abc;
   const double ds2 = clift * dab;
   const double dl = ds2 - ds1;
   const double dr1 = blift * cda;
   const double dr2 = alift * bcd;
   const double dr = dr2 - dr1;
   const double det = dl + dr;

   double _tmp_fabs;

   double max_var = 0.0;
   if ((_tmp_fabs = fabs(aex)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(aey)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(aez)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(bex)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(bey)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(bez)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(cex)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(cey)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(cez)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(dex)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(dey)) > max_var) max_var = _tmp_fabs;
   if ((_tmp_fabs = fabs(dez)) > max_var) max_var = _tmp_fabs;
   double epsilon = max_var;
   epsilon *= epsilon;
   epsilon *= epsilon;
   epsilon *= max_var;
   epsilon *= 1.145750161413163e-13;
   if (det > epsilon) return IP_Sign::POSITIVE;
   if (-det > epsilon) return IP_Sign::NEGATIVE;
   return Filtered_Sign::UNCERTAIN;
}

inline int inSphere_interval(interval_number pax, interval_number pay, interval_number paz, interval_number pbx, interval_number pby, interval_number pbz, interval_number pcx, interval_number pcy, interval_number pcz, interval_number pdx, interval_number pdy, interval_number pdz, interval_number pex, interval_number pey, interval_number pez)
{
   setFPUModeToRoundUP();
   const interval_number aex(pax - pex);
   const interval_number aey(pay - pey);
   const interval_number aez(paz - pez);
   const interval_number bex(pbx - pex);
   const interval_number bey(pby - pey);
   const interval_number bez(pbz - pez);
   const interval_number cex(pcx - pex);
   const interval_number cey(pcy - pey);
   const interval_number cez(pcz - pez);
   const interval_number dex(pdx - pex);
   const interval_number dey(pdy - pey);
   const interval_number dez(pdz - pez);
   const interval_number aexbey(aex * bey);
   const interval_number bexaey(bex * aey);
   const interval_number ab(aexbey - bexaey);
   const interval_number bexcey(bex * cey);
   const interval_number cexbey(cex * bey);
   const interval_number bc(bexcey - cexbey);
   const interval_number cexdey(cex * dey);
   const interval_number dexcey(dex * cey);
   const interval_number cd(cexdey - dexcey);
   const interval_number dexaey(dex * aey);
   const interval_number aexdey(aex * dey);
   const interval_number da(dexaey - aexdey);
   const interval_number aexcey(aex * cey);
   const interval_number cexaey(cex * aey);
   const interval_number ac(aexcey - cexaey);
   const interval_number bexdey(bex * dey);
   const interval_number dexbey(dex * bey);
   const interval_number bd(bexdey - dexbey);
   const interval_number abc1(aez * bc);
   const interval_number abc2(bez * ac);
   const interval_number abc3(cez * ab);
   const interval_number abc4(abc1 + abc3);
   const interval_number abc(abc4 - abc2);
   const interval_number bcd1(bez * cd);
   const interval_number bcd2(cez * bd);
   const interval_number bcd3(dez * bc);
   const interval_number bcd4(bcd1 + bcd3);
   const interval_number bcd(bcd4 - bcd2);
   const interval_number cda1(cez * da);
   const interval_number cda2(dez * ac);
   const interval_number cda3(aez * cd);
   const interval_number cda4(cda1 + cda3);
   const interval_number cda(cda4 + cda2);
   const interval_number dab1(dez * ab);
   const interval_number dab2(aez * bd);
   const interval_number dab3(bez * da);
   const interval_number dab4(dab1 + dab3);
   const interval_number dab(dab4 + dab2);
   const interval_number al1(aex * aex);
   const interval_number al2(aey * aey);
   const interval_number al3(aez * aez);
   const interval_number al4(al1 + al2);
   const interval_number alift(al4 + al3);
   const interval_number bl1(bex * bex);
   const interval_number bl2(bey * bey);
   const interval_number bl3(bez * bez);
   const interval_number bl4(bl1 + bl2);
   const interval_number blift(bl4 + bl3);
   const interval_number cl1(cex * cex);
   const interval_number cl2(cey * cey);
   const interval_number cl3(cez * cez);
   const interval_number cl4(cl1 + cl2);
   const interval_number clift(cl4 + cl3);
   const interval_number dl1(dex * dex);
   const interval_number dl2(dey * dey);
   const interval_number dl3(dez * dez);
   const interval_number dl4(dl1 + dl2);
   const interval_number dlift(dl4 + dl3);
   const interval_number ds1(dlift * abc);
   const interval_number ds2(clift * dab);
   const interval_number dl(ds2 - ds1);
   const interval_number dr1(blift * cda);
   const interval_number dr2(alift * bcd);
   const interval_number dr(dr2 - dr1);
   const interval_number det(dl + dr);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int inSphere_bigfloat(bigfloat pax, bigfloat pay, bigfloat paz, bigfloat pbx, bigfloat pby, bigfloat pbz, bigfloat pcx, bigfloat pcy, bigfloat pcz, bigfloat pdx, bigfloat pdy, bigfloat pdz, bigfloat pex, bigfloat pey, bigfloat pez)
{
   const bigfloat aex(pax - pex);
   const bigfloat aey(pay - pey);
   const bigfloat aez(paz - pez);
   const bigfloat bex(pbx - pex);
   const bigfloat bey(pby - pey);
   const bigfloat bez(pbz - pez);
   const bigfloat cex(pcx - pex);
   const bigfloat cey(pcy - pey);
   const bigfloat cez(pcz - pez);
   const bigfloat dex(pdx - pex);
   const bigfloat dey(pdy - pey);
   const bigfloat dez(pdz - pez);
   const bigfloat aexbey(aex * bey);
   const bigfloat bexaey(bex * aey);
   const bigfloat ab(aexbey - bexaey);
   const bigfloat bexcey(bex * cey);
   const bigfloat cexbey(cex * bey);
   const bigfloat bc(bexcey - cexbey);
   const bigfloat cexdey(cex * dey);
   const bigfloat dexcey(dex * cey);
   const bigfloat cd(cexdey - dexcey);
   const bigfloat dexaey(dex * aey);
   const bigfloat aexdey(aex * dey);
   const bigfloat da(dexaey - aexdey);
   const bigfloat aexcey(aex * cey);
   const bigfloat cexaey(cex * aey);
   const bigfloat ac(aexcey - cexaey);
   const bigfloat bexdey(bex * dey);
   const bigfloat dexbey(dex * bey);
   const bigfloat bd(bexdey - dexbey);
   const bigfloat abc1(aez * bc);
   const bigfloat abc2(bez * ac);
   const bigfloat abc3(cez * ab);
   const bigfloat abc4(abc1 + abc3);
   const bigfloat abc(abc4 - abc2);
   const bigfloat bcd1(bez * cd);
   const bigfloat bcd2(cez * bd);
   const bigfloat bcd3(dez * bc);
   const bigfloat bcd4(bcd1 + bcd3);
   const bigfloat bcd(bcd4 - bcd2);
   const bigfloat cda1(cez * da);
   const bigfloat cda2(dez * ac);
   const bigfloat cda3(aez * cd);
   const bigfloat cda4(cda1 + cda3);
   const bigfloat cda(cda4 + cda2);
   const bigfloat dab1(dez * ab);
   const bigfloat dab2(aez * bd);
   const bigfloat dab3(bez * da);
   const bigfloat dab4(dab1 + dab3);
   const bigfloat dab(dab4 + dab2);
   const bigfloat al1(aex * aex);
   const bigfloat al2(aey * aey);
   const bigfloat al3(aez * aez);
   const bigfloat al4(al1 + al2);
   const bigfloat alift(al4 + al3);
   const bigfloat bl1(bex * bex);
   const bigfloat bl2(bey * bey);
   const bigfloat bl3(bez * bez);
   const bigfloat bl4(bl1 + bl2);
   const bigfloat blift(bl4 + bl3);
   const bigfloat cl1(cex * cex);
   const bigfloat cl2(cey * cey);
   const bigfloat cl3(cez * cez);
   const bigfloat cl4(cl1 + cl2);
   const bigfloat clift(cl4 + cl3);
   const bigfloat dl1(dex * dex);
   const bigfloat dl2(dey * dey);
   const bigfloat dl3(dez * dez);
   const bigfloat dl4(dl1 + dl2);
   const bigfloat dlift(dl4 + dl3);
   const bigfloat ds1(dlift * abc);
   const bigfloat ds2(clift * dab);
   const bigfloat dl(ds2 - ds1);
   const bigfloat dr1(blift * cda);
   const bigfloat dr2(alift * bcd);
   const bigfloat dr(dr2 - dr1);
   const bigfloat det(dl + dr);
   return sgn(det);
}

inline int inSphere_exact(double pax, double pay, double paz, double pbx, double pby, double pbz, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez)
{
   
   double aex[2];
   expansionObject::two_Diff(pax, pex, aex);
   double aey[2];
   expansionObject::two_Diff(pay, pey, aey);
   double aez[2];
   expansionObject::two_Diff(paz, pez, aez);
   double bex[2];
   expansionObject::two_Diff(pbx, pex, bex);
   double bey[2];
   expansionObject::two_Diff(pby, pey, bey);
   double bez[2];
   expansionObject::two_Diff(pbz, pez, bez);
   double cex[2];
   expansionObject::two_Diff(pcx, pex, cex);
   double cey[2];
   expansionObject::two_Diff(pcy, pey, cey);
   double cez[2];
   expansionObject::two_Diff(pcz, pez, cez);
   double dex[2];
   expansionObject::two_Diff(pdx, pex, dex);
   double dey[2];
   expansionObject::two_Diff(pdy, pey, dey);
   double dez[2];
   expansionObject::two_Diff(pdz, pez, dez);
   double aexbey[8];
   int aexbey_len = expansionObject::Gen_Product(2, aex, 2, bey, aexbey);
   double bexaey[8];
   int bexaey_len = expansionObject::Gen_Product(2, bex, 2, aey, bexaey);
   double ab[16];
   int ab_len = expansionObject::Gen_Diff(aexbey_len, aexbey, bexaey_len, bexaey, ab);
   double bexcey[8];
   int bexcey_len = expansionObject::Gen_Product(2, bex, 2, cey, bexcey);
   double cexbey[8];
   int cexbey_len = expansionObject::Gen_Product(2, cex, 2, bey, cexbey);
   double bc[16];
   int bc_len = expansionObject::Gen_Diff(bexcey_len, bexcey, cexbey_len, cexbey, bc);
   double cexdey[8];
   int cexdey_len = expansionObject::Gen_Product(2, cex, 2, dey, cexdey);
   double dexcey[8];
   int dexcey_len = expansionObject::Gen_Product(2, dex, 2, cey, dexcey);
   double cd[16];
   int cd_len = expansionObject::Gen_Diff(cexdey_len, cexdey, dexcey_len, dexcey, cd);
   double dexaey[8];
   int dexaey_len = expansionObject::Gen_Product(2, dex, 2, aey, dexaey);
   double aexdey[8];
   int aexdey_len = expansionObject::Gen_Product(2, aex, 2, dey, aexdey);
   double da[16];
   int da_len = expansionObject::Gen_Diff(dexaey_len, dexaey, aexdey_len, aexdey, da);
   double aexcey[8];
   int aexcey_len = expansionObject::Gen_Product(2, aex, 2, cey, aexcey);
   double cexaey[8];
   int cexaey_len = expansionObject::Gen_Product(2, cex, 2, aey, cexaey);
   double ac[16];
   int ac_len = expansionObject::Gen_Diff(aexcey_len, aexcey, cexaey_len, cexaey, ac);
   double bexdey[8];
   int bexdey_len = expansionObject::Gen_Product(2, bex, 2, dey, bexdey);
   double dexbey[8];
   int dexbey_len = expansionObject::Gen_Product(2, dex, 2, bey, dexbey);
   double bd[16];
   int bd_len = expansionObject::Gen_Diff(bexdey_len, bexdey, dexbey_len, dexbey, bd);
   double abc1_p[32], *abc1 = abc1_p;
   int abc1_len = expansionObject::Gen_Product_With_PreAlloc(2, aez, bc_len, bc, &abc1, 32);
   double abc2_p[32], *abc2 = abc2_p;
   int abc2_len = expansionObject::Gen_Product_With_PreAlloc(2, bez, ac_len, ac, &abc2, 32);
   double abc3_p[32], *abc3 = abc3_p;
   int abc3_len = expansionObject::Gen_Product_With_PreAlloc(2, cez, ab_len, ab, &abc3, 32);
   double abc4_p[32], *abc4 = abc4_p;
   int abc4_len = expansionObject::Gen_Sum_With_PreAlloc(abc1_len, abc1, abc3_len, abc3, &abc4, 32);
   double abc_p[32], *abc = abc_p;
   int abc_len = expansionObject::Gen_Diff_With_PreAlloc(abc4_len, abc4, abc2_len, abc2, &abc, 32);
   double bcd1_p[32], *bcd1 = bcd1_p;
   int bcd1_len = expansionObject::Gen_Product_With_PreAlloc(2, bez, cd_len, cd, &bcd1, 32);
   double bcd2_p[32], *bcd2 = bcd2_p;
   int bcd2_len = expansionObject::Gen_Product_With_PreAlloc(2, cez, bd_len, bd, &bcd2, 32);
   double bcd3_p[32], *bcd3 = bcd3_p;
   int bcd3_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, bc_len, bc, &bcd3, 32);
   double bcd4_p[32], *bcd4 = bcd4_p;
   int bcd4_len = expansionObject::Gen_Sum_With_PreAlloc(bcd1_len, bcd1, bcd3_len, bcd3, &bcd4, 32);
   double bcd_p[32], *bcd = bcd_p;
   int bcd_len = expansionObject::Gen_Diff_With_PreAlloc(bcd4_len, bcd4, bcd2_len, bcd2, &bcd, 32);
   double cda1_p[32], *cda1 = cda1_p;
   int cda1_len = expansionObject::Gen_Product_With_PreAlloc(2, cez, da_len, da, &cda1, 32);
   double cda2_p[32], *cda2 = cda2_p;
   int cda2_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, ac_len, ac, &cda2, 32);
   double cda3_p[32], *cda3 = cda3_p;
   int cda3_len = expansionObject::Gen_Product_With_PreAlloc(2, aez, cd_len, cd, &cda3, 32);
   double cda4_p[32], *cda4 = cda4_p;
   int cda4_len = expansionObject::Gen_Sum_With_PreAlloc(cda1_len, cda1, cda3_len, cda3, &cda4, 32);
   double cda_p[32], *cda = cda_p;
   int cda_len = expansionObject::Gen_Sum_With_PreAlloc(cda4_len, cda4, cda2_len, cda2, &cda, 32);
   double dab1_p[32], *dab1 = dab1_p;
   int dab1_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, ab_len, ab, &dab1, 32);
   double dab2_p[32], *dab2 = dab2_p;
   int dab2_len = expansionObject::Gen_Product_With_PreAlloc(2, aez, bd_len, bd, &dab2, 32);
   double dab3_p[32], *dab3 = dab3_p;
   int dab3_len = expansionObject::Gen_Product_With_PreAlloc(2, bez, da_len, da, &dab3, 32);
   double dab4_p[32], *dab4 = dab4_p;
   int dab4_len = expansionObject::Gen_Sum_With_PreAlloc(dab1_len, dab1, dab3_len, dab3, &dab4, 32);
   double dab_p[32], *dab = dab_p;
   int dab_len = expansionObject::Gen_Sum_With_PreAlloc(dab4_len, dab4, dab2_len, dab2, &dab, 32);
   double al1[8];
   int al1_len = expansionObject::Gen_Product(2, aex, 2, aex, al1);
   double al2[8];
   int al2_len = expansionObject::Gen_Product(2, aey, 2, aey, al2);
   double al3[8];
   int al3_len = expansionObject::Gen_Product(2, aez, 2, aez, al3);
   double al4[16];
   int al4_len = expansionObject::Gen_Sum(al1_len, al1, al2_len, al2, al4);
   double alift[24];
   int alift_len = expansionObject::Gen_Sum(al4_len, al4, al3_len, al3, alift);
   double bl1[8];
   int bl1_len = expansionObject::Gen_Product(2, bex, 2, bex, bl1);
   double bl2[8];
   int bl2_len = expansionObject::Gen_Product(2, bey, 2, bey, bl2);
   double bl3[8];
   int bl3_len = expansionObject::Gen_Product(2, bez, 2, bez, bl3);
   double bl4[16];
   int bl4_len = expansionObject::Gen_Sum(bl1_len, bl1, bl2_len, bl2, bl4);
   double blift[24];
   int blift_len = expansionObject::Gen_Sum(bl4_len, bl4, bl3_len, bl3, blift);
   double cl1[8];
   int cl1_len = expansionObject::Gen_Product(2, cex, 2, cex, cl1);
   double cl2[8];
   int cl2_len = expansionObject::Gen_Product(2, cey, 2, cey, cl2);
   double cl3[8];
   int cl3_len = expansionObject::Gen_Product(2, cez, 2, cez, cl3);
   double cl4[16];
   int cl4_len = expansionObject::Gen_Sum(cl1_len, cl1, cl2_len, cl2, cl4);
   double clift[24];
   int clift_len = expansionObject::Gen_Sum(cl4_len, cl4, cl3_len, cl3, clift);
   double dl1[8];
   int dl1_len = expansionObject::Gen_Product(2, dex, 2, dex, dl1);
   double dl2[8];
   int dl2_len = expansionObject::Gen_Product(2, dey, 2, dey, dl2);
   double dl3[8];
   int dl3_len = expansionObject::Gen_Product(2, dez, 2, dez, dl3);
   double dl4[16];
   int dl4_len = expansionObject::Gen_Sum(dl1_len, dl1, dl2_len, dl2, dl4);
   double dlift[24];
   int dlift_len = expansionObject::Gen_Sum(dl4_len, dl4, dl3_len, dl3, dlift);
   double ds1_p[32], *ds1 = ds1_p;
   int ds1_len = expansionObject::Gen_Product_With_PreAlloc(dlift_len, dlift, abc_len, abc, &ds1, 32);
   double ds2_p[32], *ds2 = ds2_p;
   int ds2_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, dab_len, dab, &ds2, 32);
   double dl_p[32], *dl = dl_p;
   int dl_len = expansionObject::Gen_Diff_With_PreAlloc(ds2_len, ds2, ds1_len, ds1, &dl, 32);
   double dr1_p[32], *dr1 = dr1_p;
   int dr1_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cda_len, cda, &dr1, 32);
   double dr2_p[32], *dr2 = dr2_p;
   int dr2_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcd_len, bcd, &dr2, 32);
   double dr_p[32], *dr = dr_p;
   int dr_len = expansionObject::Gen_Diff_With_PreAlloc(dr2_len, dr2, dr1_len, dr1, &dr, 32);
   double det_p[32], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dl_len, dl, dr_len, dr, &det, 32);

   double return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (dr_p != dr) FreeDoubles(dr);
   if (dr2_p != dr2) FreeDoubles(dr2);
   if (dr1_p != dr1) FreeDoubles(dr1);
   if (dl_p != dl) FreeDoubles(dl);
   if (ds2_p != ds2) FreeDoubles(ds2);
   if (ds1_p != ds1) FreeDoubles(ds1);
   if (dab_p != dab) FreeDoubles(dab);
   if (dab4_p != dab4) FreeDoubles(dab4);
   if (dab3_p != dab3) FreeDoubles(dab3);
   if (dab2_p != dab2) FreeDoubles(dab2);
   if (dab1_p != dab1) FreeDoubles(dab1);
   if (cda_p != cda) FreeDoubles(cda);
   if (cda4_p != cda4) FreeDoubles(cda4);
   if (cda3_p != cda3) FreeDoubles(cda3);
   if (cda2_p != cda2) FreeDoubles(cda2);
   if (cda1_p != cda1) FreeDoubles(cda1);
   if (bcd_p != bcd) FreeDoubles(bcd);
   if (bcd4_p != bcd4) FreeDoubles(bcd4);
   if (bcd3_p != bcd3) FreeDoubles(bcd3);
   if (bcd2_p != bcd2) FreeDoubles(bcd2);
   if (bcd1_p != bcd1) FreeDoubles(bcd1);
   if (abc_p != abc) FreeDoubles(abc);
   if (abc4_p != abc4) FreeDoubles(abc4);
   if (abc3_p != abc3) FreeDoubles(abc3);
   if (abc2_p != abc2) FreeDoubles(abc2);
   if (abc1_p != abc1) FreeDoubles(abc1);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int inSphere(double pax, double pay, double paz, double pbx, double pby, double pbz, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez)
{
   int ret;
   ret = inSphere_filtered(pax, pay, paz, pbx, pby, pbz, pcx, pcy, pcz, pdx, pdy, pdz, pex, pey, pez);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   ret = inSphere_interval(pax, pay, paz, pbx, pby, pbz, pcx, pcy, pcz, pdx, pdy, pdz, pex, pey, pez);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return inSphere_exact(pax, pay, paz, pbx, pby, pbz, pcx, pcy, pcz, pdx, pdy, pdz, pex, pey, pez);
}

inline int dotProductSign2D_EEI_interval(const genericPoint& q, interval_number px, interval_number py, interval_number rx, interval_number ry)
{
   interval_number lqx, lqy, dq;
   if (
   !q.getIntervalLambda(lqx, lqy, dq)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pxq(px * dq);
   const interval_number pyq(py * dq);
   const interval_number rxq(rx * dq);
   const interval_number ryq(ry * dq);
   const interval_number lx(pxq - lqx);
   const interval_number ly(pyq - lqy);
   const interval_number gx(rxq - lqx);
   const interval_number gy(ryq - lqy);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number d(dx + dy);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign2D_EEI_bigfloat(const genericPoint& q, bigfloat px, bigfloat py, bigfloat rx, bigfloat ry)
{
   bigfloat lqx, lqy, dq;
   q.getBigfloatLambda(lqx, lqy, dq);
   const bigfloat pxq(px * dq);
   const bigfloat pyq(py * dq);
   const bigfloat rxq(rx * dq);
   const bigfloat ryq(ry * dq);
   const bigfloat lx(pxq - lqx);
   const bigfloat ly(pyq - lqy);
   const bigfloat gx(rxq - lqx);
   const bigfloat gy(ryq - lqy);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat d(dx + dy);
   return sgn(d);
}

inline int dotProductSign2D_EEI_exact(const genericPoint& q, double px, double py, double rx, double ry)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lqx_p[128], *lqx = lqx_p, lqy_p[128], *lqy = lqy_p, dq_p[128], *dq = dq_p;
 int lqx_len = 128, lqy_len = 128, dq_len = 128;
 q.getExactLambda(&lqx, lqx_len, &lqy, lqy_len, &dq, dq_len);
 if ((dq[dq_len - 1] != 0))
 {
   
   double pxq_p[128], *pxq = pxq_p;
   int pxq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, px, &pxq, 128);
   double pyq_p[128], *pyq = pyq_p;
   int pyq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, py, &pyq, 128);
   double rxq_p[128], *rxq = rxq_p;
   int rxq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, rx, &rxq, 128);
   double ryq_p[128], *ryq = ryq_p;
   int ryq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, ry, &ryq, 128);
   double lx_p[128], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(pxq_len, pxq, lqx_len, lqx, &lx, 128);
   double ly_p[128], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(pyq_len, pyq, lqy_len, lqy, &ly, 128);
   double gx_p[128], *gx = gx_p;
   int gx_len = expansionObject::Gen_Diff_With_PreAlloc(rxq_len, rxq, lqx_len, lqx, &gx, 128);
   double gy_p[128], *gy = gy_p;
   int gy_len = expansionObject::Gen_Diff_With_PreAlloc(ryq_len, ryq, lqy_len, lqy, &gy, 128);
   double dx_p[128], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, gx_len, gx, &dx, 128);
   double dy_p[128], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, gy_len, gy, &dy, 128);
   double d_p[128], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d, 128);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (gy_p != gy) FreeDoubles(gy);
   if (gx_p != gx) FreeDoubles(gx);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (ryq_p != ryq) FreeDoubles(ryq);
   if (rxq_p != rxq) FreeDoubles(rxq);
   if (pyq_p != pyq) FreeDoubles(pyq);
   if (pxq_p != pxq) FreeDoubles(pxq);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign2D_EEI_bigfloat(q, px, py, rx, ry);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign2D_EEI(const genericPoint& q, double px, double py, double rx, double ry)
{
   int ret;
   ret = dotProductSign2D_EEI_interval(q, px, py, rx, ry);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign2D_EEI_exact(q, px, py, rx, ry);
}

inline int dotProductSign2D_IEE_interval(const genericPoint& p, interval_number rx, interval_number ry, interval_number qx, interval_number qy)
{
   interval_number lpx, lpy, dp;
   if (
   !p.getIntervalLambda(lpx, lpy, dp)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number qxd(qx * dp);
   const interval_number qyd(qy * dp);
   const interval_number lx(lpx - qxd);
   const interval_number ly(lpy - qyd);
   const interval_number gx(rx - qx);
   const interval_number gy(ry - qy);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number d(dx + dy);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign2D_IEE_bigfloat(const genericPoint& p, bigfloat rx, bigfloat ry, bigfloat qx, bigfloat qy)
{
   bigfloat lpx, lpy, dp;
   p.getBigfloatLambda(lpx, lpy, dp);
   const bigfloat qxd(qx * dp);
   const bigfloat qyd(qy * dp);
   const bigfloat lx(lpx - qxd);
   const bigfloat ly(lpy - qyd);
   const bigfloat gx(rx - qx);
   const bigfloat gy(ry - qy);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat d(dx + dy);
   return sgn(d);
}

inline int dotProductSign2D_IEE_exact(const genericPoint& p, double rx, double ry, double qx, double qy)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lpx_p[128], *lpx = lpx_p, lpy_p[128], *lpy = lpy_p, dp_p[128], *dp = dp_p;
 int lpx_len = 128, lpy_len = 128, dp_len = 128;
 p.getExactLambda(&lpx, lpx_len, &lpy, lpy_len, &dp, dp_len);
 if ((dp[dp_len - 1] != 0))
 {
   
   double qxd_p[128], *qxd = qxd_p;
   int qxd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qx, &qxd, 128);
   double qyd_p[128], *qyd = qyd_p;
   int qyd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qy, &qyd, 128);
   double lx_p[128], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(lpx_len, lpx, qxd_len, qxd, &lx, 128);
   double ly_p[128], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(lpy_len, lpy, qyd_len, qyd, &ly, 128);
   double gx[2];
   expansionObject::two_Diff(rx, qx, gx);
   double gy[2];
   expansionObject::two_Diff(ry, qy, gy);
   double dx_p[128], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, 2, gx, &dx, 128);
   double dy_p[128], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, 2, gy, &dy, 128);
   double d_p[128], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d, 128);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (qyd_p != qyd) FreeDoubles(qyd);
   if (qxd_p != qxd) FreeDoubles(qxd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign2D_IEE_bigfloat(p, rx, ry, qx, qy);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign2D_IEE(const genericPoint& p, double rx, double ry, double qx, double qy)
{
   int ret;
   ret = dotProductSign2D_IEE_interval(p, rx, ry, qx, qy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign2D_IEE_exact(p, rx, ry, qx, qy);
}

inline int dotProductSign2D_IEI_interval(const genericPoint& p, const genericPoint& q, interval_number rx, interval_number ry)
{
   interval_number lpx, lpy, dp, lqx, lqy, dq;
   if (
   !p.getIntervalLambda(lpx, lpy, dp)
   || !q.getIntervalLambda(lqx, lqy, dq)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number dqp(dq * dp);
   const interval_number pxq(lpx * dqp);
   const interval_number pyq(lpy * dqp);
   const interval_number rxq(rx * dq);
   const interval_number ryq(ry * dq);
   const interval_number lqxd(lqx * dp);
   const interval_number lqyd(lqy * dp);
   const interval_number lx(pxq - lqxd);
   const interval_number ly(pyq - lqyd);
   const interval_number gx(rxq - lqx);
   const interval_number gy(ryq - lqy);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number d(dx + dy);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign2D_IEI_bigfloat(const genericPoint& p, const genericPoint& q, bigfloat rx, bigfloat ry)
{
   bigfloat lpx, lpy, dp, lqx, lqy, dq;
   p.getBigfloatLambda(lpx, lpy, dp);
   q.getBigfloatLambda(lqx, lqy, dq);
   const bigfloat dqp(dq * dp);
   const bigfloat pxq(lpx * dqp);
   const bigfloat pyq(lpy * dqp);
   const bigfloat rxq(rx * dq);
   const bigfloat ryq(ry * dq);
   const bigfloat lqxd(lqx * dp);
   const bigfloat lqyd(lqy * dp);
   const bigfloat lx(pxq - lqxd);
   const bigfloat ly(pyq - lqyd);
   const bigfloat gx(rxq - lqx);
   const bigfloat gy(ryq - lqy);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat d(dx + dy);
   return sgn(d);
}

inline int dotProductSign2D_IEI_exact(const genericPoint& p, const genericPoint& q, double rx, double ry)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lpx_p[64], *lpx = lpx_p, lpy_p[64], *lpy = lpy_p, dp_p[64], *dp = dp_p, lqx_p[64], *lqx = lqx_p, lqy_p[64], *lqy = lqy_p, dq_p[64], *dq = dq_p;
 int lpx_len = 64, lpy_len = 64, dp_len = 64, lqx_len = 64, lqy_len = 64, dq_len = 64;
 p.getExactLambda(&lpx, lpx_len, &lpy, lpy_len, &dp, dp_len);
 q.getExactLambda(&lqx, lqx_len, &lqy, lqy_len, &dq, dq_len);
 if ((dp[dp_len - 1] != 0) && (dq[dq_len - 1] != 0))
 {
   
   double dqp_p[64], *dqp = dqp_p;
   int dqp_len = expansionObject::Gen_Product_With_PreAlloc(dq_len, dq, dp_len, dp, &dqp, 64);
   double pxq_p[64], *pxq = pxq_p;
   int pxq_len = expansionObject::Gen_Product_With_PreAlloc(lpx_len, lpx, dqp_len, dqp, &pxq, 64);
   double pyq_p[64], *pyq = pyq_p;
   int pyq_len = expansionObject::Gen_Product_With_PreAlloc(lpy_len, lpy, dqp_len, dqp, &pyq, 64);
   double rxq_p[64], *rxq = rxq_p;
   int rxq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, rx, &rxq, 64);
   double ryq_p[64], *ryq = ryq_p;
   int ryq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, ry, &ryq, 64);
   double lqxd_p[64], *lqxd = lqxd_p;
   int lqxd_len = expansionObject::Gen_Product_With_PreAlloc(lqx_len, lqx, dp_len, dp, &lqxd, 64);
   double lqyd_p[64], *lqyd = lqyd_p;
   int lqyd_len = expansionObject::Gen_Product_With_PreAlloc(lqy_len, lqy, dp_len, dp, &lqyd, 64);
   double lx_p[64], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(pxq_len, pxq, lqxd_len, lqxd, &lx, 64);
   double ly_p[64], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(pyq_len, pyq, lqyd_len, lqyd, &ly, 64);
   double gx_p[64], *gx = gx_p;
   int gx_len = expansionObject::Gen_Diff_With_PreAlloc(rxq_len, rxq, lqx_len, lqx, &gx, 64);
   double gy_p[64], *gy = gy_p;
   int gy_len = expansionObject::Gen_Diff_With_PreAlloc(ryq_len, ryq, lqy_len, lqy, &gy, 64);
   double dx_p[64], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, gx_len, gx, &dx, 64);
   double dy_p[64], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, gy_len, gy, &dy, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d, 64);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (gy_p != gy) FreeDoubles(gy);
   if (gx_p != gx) FreeDoubles(gx);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (lqyd_p != lqyd) FreeDoubles(lqyd);
   if (lqxd_p != lqxd) FreeDoubles(lqxd);
   if (ryq_p != ryq) FreeDoubles(ryq);
   if (rxq_p != rxq) FreeDoubles(rxq);
   if (pyq_p != pyq) FreeDoubles(pyq);
   if (pxq_p != pxq) FreeDoubles(pxq);
   if (dqp_p != dqp) FreeDoubles(dqp);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign2D_IEI_bigfloat(p, q, rx, ry);
#endif

 if (lpx_p != lpx) FreeDoubles(lpx);
 if (lpy_p != lpy) FreeDoubles(lpy);
 if (dp_p != dp) FreeDoubles(dp);
 if (lqx_p != lqx) FreeDoubles(lqx);
 if (lqy_p != lqy) FreeDoubles(lqy);
 if (dq_p != dq) FreeDoubles(dq);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign2D_IEI(const genericPoint& p, const genericPoint& q, double rx, double ry)
{
   int ret;
   ret = dotProductSign2D_IEI_interval(p, q, rx, ry);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign2D_IEI_exact(p, q, rx, ry);
}

inline int dotProductSign2D_IIE_interval(const genericPoint& p, const genericPoint& r, interval_number qx, interval_number qy)
{
   interval_number lpx, lpy, dp, lrx, lry, dr;
   if (
   !p.getIntervalLambda(lpx, lpy, dp)
   || !r.getIntervalLambda(lrx, lry, dr)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number qxd(qx * dp);
   const interval_number qyd(qy * dp);
   const interval_number lx(lpx - qxd);
   const interval_number ly(lpy - qyd);
   const interval_number qxr(qx * dr);
   const interval_number qyr(qy * dr);
   const interval_number gx(lrx - qxr);
   const interval_number gy(lry - qyr);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number d(dx + dy);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign2D_IIE_bigfloat(const genericPoint& p, const genericPoint& r, bigfloat qx, bigfloat qy)
{
   bigfloat lpx, lpy, dp, lrx, lry, dr;
   p.getBigfloatLambda(lpx, lpy, dp);
   r.getBigfloatLambda(lrx, lry, dr);
   const bigfloat qxd(qx * dp);
   const bigfloat qyd(qy * dp);
   const bigfloat lx(lpx - qxd);
   const bigfloat ly(lpy - qyd);
   const bigfloat qxr(qx * dr);
   const bigfloat qyr(qy * dr);
   const bigfloat gx(lrx - qxr);
   const bigfloat gy(lry - qyr);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat d(dx + dy);
   return sgn(d);
}

inline int dotProductSign2D_IIE_exact(const genericPoint& p, const genericPoint& r, double qx, double qy)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lpx_p[64], *lpx = lpx_p, lpy_p[64], *lpy = lpy_p, dp_p[64], *dp = dp_p, lrx_p[64], *lrx = lrx_p, lry_p[64], *lry = lry_p, dr_p[64], *dr = dr_p;
 int lpx_len = 64, lpy_len = 64, dp_len = 64, lrx_len = 64, lry_len = 64, dr_len = 64;
 p.getExactLambda(&lpx, lpx_len, &lpy, lpy_len, &dp, dp_len);
 r.getExactLambda(&lrx, lrx_len, &lry, lry_len, &dr, dr_len);
 if ((dp[dp_len - 1] != 0) && (dr[dr_len - 1] != 0))
 {
   
   double qxd_p[64], *qxd = qxd_p;
   int qxd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qx, &qxd, 64);
   double qyd_p[64], *qyd = qyd_p;
   int qyd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qy, &qyd, 64);
   double lx_p[64], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(lpx_len, lpx, qxd_len, qxd, &lx, 64);
   double ly_p[64], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(lpy_len, lpy, qyd_len, qyd, &ly, 64);
   double qxr_p[64], *qxr = qxr_p;
   int qxr_len = expansionObject::Gen_Scale_With_PreAlloc(dr_len, dr, qx, &qxr, 64);
   double qyr_p[64], *qyr = qyr_p;
   int qyr_len = expansionObject::Gen_Scale_With_PreAlloc(dr_len, dr, qy, &qyr, 64);
   double gx_p[64], *gx = gx_p;
   int gx_len = expansionObject::Gen_Diff_With_PreAlloc(lrx_len, lrx, qxr_len, qxr, &gx, 64);
   double gy_p[64], *gy = gy_p;
   int gy_len = expansionObject::Gen_Diff_With_PreAlloc(lry_len, lry, qyr_len, qyr, &gy, 64);
   double dx_p[64], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, gx_len, gx, &dx, 64);
   double dy_p[64], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, gy_len, gy, &dy, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d, 64);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (gy_p != gy) FreeDoubles(gy);
   if (gx_p != gx) FreeDoubles(gx);
   if (qyr_p != qyr) FreeDoubles(qyr);
   if (qxr_p != qxr) FreeDoubles(qxr);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (qyd_p != qyd) FreeDoubles(qyd);
   if (qxd_p != qxd) FreeDoubles(qxd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign2D_IIE_bigfloat(p, r, qx, qy);
#endif

 if (lpx_p != lpx) FreeDoubles(lpx);
 if (lpy_p != lpy) FreeDoubles(lpy);
 if (dp_p != dp) FreeDoubles(dp);
 if (lrx_p != lrx) FreeDoubles(lrx);
 if (lry_p != lry) FreeDoubles(lry);
 if (dr_p != dr) FreeDoubles(dr);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign2D_IIE(const genericPoint& p, const genericPoint& r, double qx, double qy)
{
   int ret;
   ret = dotProductSign2D_IIE_interval(p, r, qx, qy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign2D_IIE_exact(p, r, qx, qy);
}

inline int dotProductSign2D_III_interval(const genericPoint& p, const genericPoint& r, const genericPoint& q)
{
   interval_number lpx, lpy, dp, lrx, lry, dr, lqx, lqy, dq;
   if (
   !p.getIntervalLambda(lpx, lpy, dp)
   || !r.getIntervalLambda(lrx, lry, dr)
   || !q.getIntervalLambda(lqx, lqy, dq)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number qxd(lqx * dp);
   const interval_number qyd(lqy * dp);
   const interval_number lpxq(lpx * dq);
   const interval_number lpyq(lpy * dq);
   const interval_number lx(lpxq - qxd);
   const interval_number ly(lpyq - qyd);
   const interval_number qxr(lqx * dr);
   const interval_number qyr(lqy * dr);
   const interval_number lrxq(lrx * dq);
   const interval_number lryq(lry * dq);
   const interval_number gx(lrxq - qxr);
   const interval_number gy(lryq - qyr);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number d(dx + dy);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign2D_III_bigfloat(const genericPoint& p, const genericPoint& r, const genericPoint& q)
{
   bigfloat lpx, lpy, dp, lrx, lry, dr, lqx, lqy, dq;
   p.getBigfloatLambda(lpx, lpy, dp);
   r.getBigfloatLambda(lrx, lry, dr);
   q.getBigfloatLambda(lqx, lqy, dq);
   const bigfloat qxd(lqx * dp);
   const bigfloat qyd(lqy * dp);
   const bigfloat lpxq(lpx * dq);
   const bigfloat lpyq(lpy * dq);
   const bigfloat lx(lpxq - qxd);
   const bigfloat ly(lpyq - qyd);
   const bigfloat qxr(lqx * dr);
   const bigfloat qyr(lqy * dr);
   const bigfloat lrxq(lrx * dq);
   const bigfloat lryq(lry * dq);
   const bigfloat gx(lrxq - qxr);
   const bigfloat gy(lryq - qyr);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat d(dx + dy);
   return sgn(d);
}

inline int dotProductSign2D_III_exact(const genericPoint& p, const genericPoint& r, const genericPoint& q)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lpx_p[64], *lpx = lpx_p, lpy_p[64], *lpy = lpy_p, dp_p[64], *dp = dp_p, lrx_p[64], *lrx = lrx_p, lry_p[64], *lry = lry_p, dr_p[64], *dr = dr_p, lqx_p[64], *lqx = lqx_p, lqy_p[64], *lqy = lqy_p, dq_p[64], *dq = dq_p;
 int lpx_len = 64, lpy_len = 64, dp_len = 64, lrx_len = 64, lry_len = 64, dr_len = 64, lqx_len = 64, lqy_len = 64, dq_len = 64;
 p.getExactLambda(&lpx, lpx_len, &lpy, lpy_len, &dp, dp_len);
 r.getExactLambda(&lrx, lrx_len, &lry, lry_len, &dr, dr_len);
 q.getExactLambda(&lqx, lqx_len, &lqy, lqy_len, &dq, dq_len);
 if ((dp[dp_len - 1] != 0) && (dr[dr_len - 1] != 0) && (dq[dq_len - 1] != 0))
 {
   
   double qxd_p[64], *qxd = qxd_p;
   int qxd_len = expansionObject::Gen_Product_With_PreAlloc(lqx_len, lqx, dp_len, dp, &qxd, 64);
   double qyd_p[64], *qyd = qyd_p;
   int qyd_len = expansionObject::Gen_Product_With_PreAlloc(lqy_len, lqy, dp_len, dp, &qyd, 64);
   double lpxq_p[64], *lpxq = lpxq_p;
   int lpxq_len = expansionObject::Gen_Product_With_PreAlloc(lpx_len, lpx, dq_len, dq, &lpxq, 64);
   double lpyq_p[64], *lpyq = lpyq_p;
   int lpyq_len = expansionObject::Gen_Product_With_PreAlloc(lpy_len, lpy, dq_len, dq, &lpyq, 64);
   double lx_p[64], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(lpxq_len, lpxq, qxd_len, qxd, &lx, 64);
   double ly_p[64], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(lpyq_len, lpyq, qyd_len, qyd, &ly, 64);
   double qxr_p[64], *qxr = qxr_p;
   int qxr_len = expansionObject::Gen_Product_With_PreAlloc(lqx_len, lqx, dr_len, dr, &qxr, 64);
   double qyr_p[64], *qyr = qyr_p;
   int qyr_len = expansionObject::Gen_Product_With_PreAlloc(lqy_len, lqy, dr_len, dr, &qyr, 64);
   double lrxq_p[64], *lrxq = lrxq_p;
   int lrxq_len = expansionObject::Gen_Product_With_PreAlloc(lrx_len, lrx, dq_len, dq, &lrxq, 64);
   double lryq_p[64], *lryq = lryq_p;
   int lryq_len = expansionObject::Gen_Product_With_PreAlloc(lry_len, lry, dq_len, dq, &lryq, 64);
   double gx_p[64], *gx = gx_p;
   int gx_len = expansionObject::Gen_Diff_With_PreAlloc(lrxq_len, lrxq, qxr_len, qxr, &gx, 64);
   double gy_p[64], *gy = gy_p;
   int gy_len = expansionObject::Gen_Diff_With_PreAlloc(lryq_len, lryq, qyr_len, qyr, &gy, 64);
   double dx_p[64], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, gx_len, gx, &dx, 64);
   double dy_p[64], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, gy_len, gy, &dy, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d, 64);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (gy_p != gy) FreeDoubles(gy);
   if (gx_p != gx) FreeDoubles(gx);
   if (lryq_p != lryq) FreeDoubles(lryq);
   if (lrxq_p != lrxq) FreeDoubles(lrxq);
   if (qyr_p != qyr) FreeDoubles(qyr);
   if (qxr_p != qxr) FreeDoubles(qxr);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (lpyq_p != lpyq) FreeDoubles(lpyq);
   if (lpxq_p != lpxq) FreeDoubles(lpxq);
   if (qyd_p != qyd) FreeDoubles(qyd);
   if (qxd_p != qxd) FreeDoubles(qxd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign2D_III_bigfloat(p, r, q);
#endif

 if (lpx_p != lpx) FreeDoubles(lpx);
 if (lpy_p != lpy) FreeDoubles(lpy);
 if (dp_p != dp) FreeDoubles(dp);
 if (lrx_p != lrx) FreeDoubles(lrx);
 if (lry_p != lry) FreeDoubles(lry);
 if (dr_p != dr) FreeDoubles(dr);
 if (lqx_p != lqx) FreeDoubles(lqx);
 if (lqy_p != lqy) FreeDoubles(lqy);
 if (dq_p != dq) FreeDoubles(dq);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign2D_III(const genericPoint& p, const genericPoint& r, const genericPoint& q)
{
   int ret;
   ret = dotProductSign2D_III_interval(p, r, q);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign2D_III_exact(p, r, q);
}

inline int dotProductSign3D_EEI_interval(const genericPoint& q, interval_number px, interval_number py, interval_number pz, interval_number rx, interval_number ry, interval_number rz)
{
   interval_number lqx, lqy, lqz, dq;
   if (
   !q.getIntervalLambda(lqx, lqy, lqz, dq)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pxq(px * dq);
   const interval_number pyq(py * dq);
   const interval_number pzq(pz * dq);
   const interval_number rxq(rx * dq);
   const interval_number ryq(ry * dq);
   const interval_number rzq(rz * dq);
   const interval_number lx(pxq - lqx);
   const interval_number ly(pyq - lqy);
   const interval_number lz(pzq - lqz);
   const interval_number gx(rxq - lqx);
   const interval_number gy(ryq - lqy);
   const interval_number gz(rzq - lqz);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number dz(lz * gz);
   const interval_number d1(dx + dy);
   const interval_number d(d1 + dz);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign3D_EEI_bigfloat(const genericPoint& q, bigfloat px, bigfloat py, bigfloat pz, bigfloat rx, bigfloat ry, bigfloat rz)
{
   bigfloat lqx, lqy, lqz, dq;
   q.getBigfloatLambda(lqx, lqy, lqz, dq);
   const bigfloat pxq(px * dq);
   const bigfloat pyq(py * dq);
   const bigfloat pzq(pz * dq);
   const bigfloat rxq(rx * dq);
   const bigfloat ryq(ry * dq);
   const bigfloat rzq(rz * dq);
   const bigfloat lx(pxq - lqx);
   const bigfloat ly(pyq - lqy);
   const bigfloat lz(pzq - lqz);
   const bigfloat gx(rxq - lqx);
   const bigfloat gy(ryq - lqy);
   const bigfloat gz(rzq - lqz);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat dz(lz * gz);
   const bigfloat d1(dx + dy);
   const bigfloat d(d1 + dz);
   return sgn(d);
}

inline int dotProductSign3D_EEI_exact(const genericPoint& q, double px, double py, double pz, double rx, double ry, double rz)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lqx_p[64], *lqx = lqx_p, lqy_p[64], *lqy = lqy_p, lqz_p[64], *lqz = lqz_p, dq_p[64], *dq = dq_p;
 int lqx_len = 64, lqy_len = 64, lqz_len = 64, dq_len = 64;
 q.getExactLambda(&lqx, lqx_len, &lqy, lqy_len, &lqz, lqz_len, &dq, dq_len);
 if ((dq[dq_len - 1] != 0))
 {
   
   double pxq_p[64], *pxq = pxq_p;
   int pxq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, px, &pxq, 64);
   double pyq_p[64], *pyq = pyq_p;
   int pyq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, py, &pyq, 64);
   double pzq_p[64], *pzq = pzq_p;
   int pzq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, pz, &pzq, 64);
   double rxq_p[64], *rxq = rxq_p;
   int rxq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, rx, &rxq, 64);
   double ryq_p[64], *ryq = ryq_p;
   int ryq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, ry, &ryq, 64);
   double rzq_p[64], *rzq = rzq_p;
   int rzq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, rz, &rzq, 64);
   double lx_p[64], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(pxq_len, pxq, lqx_len, lqx, &lx, 64);
   double ly_p[64], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(pyq_len, pyq, lqy_len, lqy, &ly, 64);
   double lz_p[64], *lz = lz_p;
   int lz_len = expansionObject::Gen_Diff_With_PreAlloc(pzq_len, pzq, lqz_len, lqz, &lz, 64);
   double gx_p[64], *gx = gx_p;
   int gx_len = expansionObject::Gen_Diff_With_PreAlloc(rxq_len, rxq, lqx_len, lqx, &gx, 64);
   double gy_p[64], *gy = gy_p;
   int gy_len = expansionObject::Gen_Diff_With_PreAlloc(ryq_len, ryq, lqy_len, lqy, &gy, 64);
   double gz_p[64], *gz = gz_p;
   int gz_len = expansionObject::Gen_Diff_With_PreAlloc(rzq_len, rzq, lqz_len, lqz, &gz, 64);
   double dx_p[64], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, gx_len, gx, &dx, 64);
   double dy_p[64], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, gy_len, gy, &dy, 64);
   double dz_p[64], *dz = dz_p;
   int dz_len = expansionObject::Gen_Product_With_PreAlloc(lz_len, lz, gz_len, gz, &dz, 64);
   double d1_p[64], *d1 = d1_p;
   int d1_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d1, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(d1_len, d1, dz_len, dz, &d, 64);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (d1_p != d1) FreeDoubles(d1);
   if (dz_p != dz) FreeDoubles(dz);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (gz_p != gz) FreeDoubles(gz);
   if (gy_p != gy) FreeDoubles(gy);
   if (gx_p != gx) FreeDoubles(gx);
   if (lz_p != lz) FreeDoubles(lz);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (rzq_p != rzq) FreeDoubles(rzq);
   if (ryq_p != ryq) FreeDoubles(ryq);
   if (rxq_p != rxq) FreeDoubles(rxq);
   if (pzq_p != pzq) FreeDoubles(pzq);
   if (pyq_p != pyq) FreeDoubles(pyq);
   if (pxq_p != pxq) FreeDoubles(pxq);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign3D_EEI_bigfloat(q, px, py, pz, rx, ry, rz);
#endif

 if (lqx_p != lqx) FreeDoubles(lqx);
 if (lqy_p != lqy) FreeDoubles(lqy);
 if (lqz_p != lqz) FreeDoubles(lqz);
 if (dq_p != dq) FreeDoubles(dq);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign3D_EEI(const genericPoint& q, double px, double py, double pz, double rx, double ry, double rz)
{
   int ret;
   ret = dotProductSign3D_EEI_interval(q, px, py, pz, rx, ry, rz);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign3D_EEI_exact(q, px, py, pz, rx, ry, rz);
}

inline int dotProductSign3D_IEE_interval(const genericPoint& p, interval_number rx, interval_number ry, interval_number rz, interval_number qx, interval_number qy, interval_number qz)
{
   interval_number lpx, lpy, lpz, dp;
   if (
   !p.getIntervalLambda(lpx, lpy, lpz, dp)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number qxd(qx * dp);
   const interval_number qyd(qy * dp);
   const interval_number qzd(qz * dp);
   const interval_number lx(lpx - qxd);
   const interval_number ly(lpy - qyd);
   const interval_number lz(lpz - qzd);
   const interval_number gx(rx - qx);
   const interval_number gy(ry - qy);
   const interval_number gz(rz - qz);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number dz(lz * gz);
   const interval_number d1(dx + dy);
   const interval_number d(d1 + dz);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign3D_IEE_bigfloat(const genericPoint& p, bigfloat rx, bigfloat ry, bigfloat rz, bigfloat qx, bigfloat qy, bigfloat qz)
{
   bigfloat lpx, lpy, lpz, dp;
   p.getBigfloatLambda(lpx, lpy, lpz, dp);
   const bigfloat qxd(qx * dp);
   const bigfloat qyd(qy * dp);
   const bigfloat qzd(qz * dp);
   const bigfloat lx(lpx - qxd);
   const bigfloat ly(lpy - qyd);
   const bigfloat lz(lpz - qzd);
   const bigfloat gx(rx - qx);
   const bigfloat gy(ry - qy);
   const bigfloat gz(rz - qz);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat dz(lz * gz);
   const bigfloat d1(dx + dy);
   const bigfloat d(d1 + dz);
   return sgn(d);
}

inline int dotProductSign3D_IEE_exact(const genericPoint& p, double rx, double ry, double rz, double qx, double qy, double qz)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lpx_p[128], *lpx = lpx_p, lpy_p[128], *lpy = lpy_p, lpz_p[128], *lpz = lpz_p, dp_p[128], *dp = dp_p;
 int lpx_len = 128, lpy_len = 128, lpz_len = 128, dp_len = 128;
 p.getExactLambda(&lpx, lpx_len, &lpy, lpy_len, &lpz, lpz_len, &dp, dp_len);
 if ((dp[dp_len - 1] != 0))
 {
   
   double qxd_p[128], *qxd = qxd_p;
   int qxd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qx, &qxd, 128);
   double qyd_p[128], *qyd = qyd_p;
   int qyd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qy, &qyd, 128);
   double qzd_p[128], *qzd = qzd_p;
   int qzd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qz, &qzd, 128);
   double lx_p[128], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(lpx_len, lpx, qxd_len, qxd, &lx, 128);
   double ly_p[128], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(lpy_len, lpy, qyd_len, qyd, &ly, 128);
   double lz_p[128], *lz = lz_p;
   int lz_len = expansionObject::Gen_Diff_With_PreAlloc(lpz_len, lpz, qzd_len, qzd, &lz, 128);
   double gx[2];
   expansionObject::two_Diff(rx, qx, gx);
   double gy[2];
   expansionObject::two_Diff(ry, qy, gy);
   double gz[2];
   expansionObject::two_Diff(rz, qz, gz);
   double dx_p[128], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, 2, gx, &dx, 128);
   double dy_p[128], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, 2, gy, &dy, 128);
   double dz_p[128], *dz = dz_p;
   int dz_len = expansionObject::Gen_Product_With_PreAlloc(lz_len, lz, 2, gz, &dz, 128);
   double d1_p[128], *d1 = d1_p;
   int d1_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d1, 128);
   double d_p[128], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(d1_len, d1, dz_len, dz, &d, 128);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (d1_p != d1) FreeDoubles(d1);
   if (dz_p != dz) FreeDoubles(dz);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (lz_p != lz) FreeDoubles(lz);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (qzd_p != qzd) FreeDoubles(qzd);
   if (qyd_p != qyd) FreeDoubles(qyd);
   if (qxd_p != qxd) FreeDoubles(qxd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign3D_IEE_bigfloat(p, rx, ry, rz, qx, qy, qz);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign3D_IEE(const genericPoint& p, double rx, double ry, double rz, double qx, double qy, double qz)
{
   int ret;
   ret = dotProductSign3D_IEE_interval(p, rx, ry, rz, qx, qy, qz);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign3D_IEE_exact(p, rx, ry, rz, qx, qy, qz);
}

inline int dotProductSign3D_IEI_interval(const genericPoint& p, const genericPoint& q, interval_number rx, interval_number ry, interval_number rz)
{
   interval_number lpx, lpy, lpz, dp, lqx, lqy, lqz, dq;
   if (
   !p.getIntervalLambda(lpx, lpy, lpz, dp)
   || !q.getIntervalLambda(lqx, lqy, lqz, dq)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number dqp(dq * dp);
   const interval_number pxq(lpx * dqp);
   const interval_number pyq(lpy * dqp);
   const interval_number pzq(lpz * dqp);
   const interval_number rxq(rx * dq);
   const interval_number ryq(ry * dq);
   const interval_number rzq(rz * dq);
   const interval_number lqxd(lqx * dp);
   const interval_number lqyd(lqy * dp);
   const interval_number lqzd(lqz * dp);
   const interval_number lx(pxq - lqxd);
   const interval_number ly(pyq - lqyd);
   const interval_number lz(pzq - lqzd);
   const interval_number gx(rxq - lqx);
   const interval_number gy(ryq - lqy);
   const interval_number gz(rzq - lqz);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number dz(lz * gz);
   const interval_number d1(dx + dy);
   const interval_number d(d1 + dz);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign3D_IEI_bigfloat(const genericPoint& p, const genericPoint& q, bigfloat rx, bigfloat ry, bigfloat rz)
{
   bigfloat lpx, lpy, lpz, dp, lqx, lqy, lqz, dq;
   p.getBigfloatLambda(lpx, lpy, lpz, dp);
   q.getBigfloatLambda(lqx, lqy, lqz, dq);
   const bigfloat dqp(dq * dp);
   const bigfloat pxq(lpx * dqp);
   const bigfloat pyq(lpy * dqp);
   const bigfloat pzq(lpz * dqp);
   const bigfloat rxq(rx * dq);
   const bigfloat ryq(ry * dq);
   const bigfloat rzq(rz * dq);
   const bigfloat lqxd(lqx * dp);
   const bigfloat lqyd(lqy * dp);
   const bigfloat lqzd(lqz * dp);
   const bigfloat lx(pxq - lqxd);
   const bigfloat ly(pyq - lqyd);
   const bigfloat lz(pzq - lqzd);
   const bigfloat gx(rxq - lqx);
   const bigfloat gy(ryq - lqy);
   const bigfloat gz(rzq - lqz);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat dz(lz * gz);
   const bigfloat d1(dx + dy);
   const bigfloat d(d1 + dz);
   return sgn(d);
}

inline int dotProductSign3D_IEI_exact(const genericPoint& p, const genericPoint& q, double rx, double ry, double rz)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lpx_p[64], *lpx = lpx_p, lpy_p[64], *lpy = lpy_p, lpz_p[64], *lpz = lpz_p, dp_p[64], *dp = dp_p, lqx_p[64], *lqx = lqx_p, lqy_p[64], *lqy = lqy_p, lqz_p[64], *lqz = lqz_p, dq_p[64], *dq = dq_p;
 int lpx_len = 64, lpy_len = 64, lpz_len = 64, dp_len = 64, lqx_len = 64, lqy_len = 64, lqz_len = 64, dq_len = 64;
 p.getExactLambda(&lpx, lpx_len, &lpy, lpy_len, &lpz, lpz_len, &dp, dp_len);
 q.getExactLambda(&lqx, lqx_len, &lqy, lqy_len, &lqz, lqz_len, &dq, dq_len);
 if ((dp[dp_len - 1] != 0) && (dq[dq_len - 1] != 0))
 {
   
   double dqp_p[64], *dqp = dqp_p;
   int dqp_len = expansionObject::Gen_Product_With_PreAlloc(dq_len, dq, dp_len, dp, &dqp, 64);
   double pxq_p[64], *pxq = pxq_p;
   int pxq_len = expansionObject::Gen_Product_With_PreAlloc(lpx_len, lpx, dqp_len, dqp, &pxq, 64);
   double pyq_p[64], *pyq = pyq_p;
   int pyq_len = expansionObject::Gen_Product_With_PreAlloc(lpy_len, lpy, dqp_len, dqp, &pyq, 64);
   double pzq_p[64], *pzq = pzq_p;
   int pzq_len = expansionObject::Gen_Product_With_PreAlloc(lpz_len, lpz, dqp_len, dqp, &pzq, 64);
   double rxq_p[64], *rxq = rxq_p;
   int rxq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, rx, &rxq, 64);
   double ryq_p[64], *ryq = ryq_p;
   int ryq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, ry, &ryq, 64);
   double rzq_p[64], *rzq = rzq_p;
   int rzq_len = expansionObject::Gen_Scale_With_PreAlloc(dq_len, dq, rz, &rzq, 64);
   double lqxd_p[64], *lqxd = lqxd_p;
   int lqxd_len = expansionObject::Gen_Product_With_PreAlloc(lqx_len, lqx, dp_len, dp, &lqxd, 64);
   double lqyd_p[64], *lqyd = lqyd_p;
   int lqyd_len = expansionObject::Gen_Product_With_PreAlloc(lqy_len, lqy, dp_len, dp, &lqyd, 64);
   double lqzd_p[64], *lqzd = lqzd_p;
   int lqzd_len = expansionObject::Gen_Product_With_PreAlloc(lqz_len, lqz, dp_len, dp, &lqzd, 64);
   double lx_p[64], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(pxq_len, pxq, lqxd_len, lqxd, &lx, 64);
   double ly_p[64], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(pyq_len, pyq, lqyd_len, lqyd, &ly, 64);
   double lz_p[64], *lz = lz_p;
   int lz_len = expansionObject::Gen_Diff_With_PreAlloc(pzq_len, pzq, lqzd_len, lqzd, &lz, 64);
   double gx_p[64], *gx = gx_p;
   int gx_len = expansionObject::Gen_Diff_With_PreAlloc(rxq_len, rxq, lqx_len, lqx, &gx, 64);
   double gy_p[64], *gy = gy_p;
   int gy_len = expansionObject::Gen_Diff_With_PreAlloc(ryq_len, ryq, lqy_len, lqy, &gy, 64);
   double gz_p[64], *gz = gz_p;
   int gz_len = expansionObject::Gen_Diff_With_PreAlloc(rzq_len, rzq, lqz_len, lqz, &gz, 64);
   double dx_p[64], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, gx_len, gx, &dx, 64);
   double dy_p[64], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, gy_len, gy, &dy, 64);
   double dz_p[64], *dz = dz_p;
   int dz_len = expansionObject::Gen_Product_With_PreAlloc(lz_len, lz, gz_len, gz, &dz, 64);
   double d1_p[64], *d1 = d1_p;
   int d1_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d1, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(d1_len, d1, dz_len, dz, &d, 64);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (d1_p != d1) FreeDoubles(d1);
   if (dz_p != dz) FreeDoubles(dz);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (gz_p != gz) FreeDoubles(gz);
   if (gy_p != gy) FreeDoubles(gy);
   if (gx_p != gx) FreeDoubles(gx);
   if (lz_p != lz) FreeDoubles(lz);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (lqzd_p != lqzd) FreeDoubles(lqzd);
   if (lqyd_p != lqyd) FreeDoubles(lqyd);
   if (lqxd_p != lqxd) FreeDoubles(lqxd);
   if (rzq_p != rzq) FreeDoubles(rzq);
   if (ryq_p != ryq) FreeDoubles(ryq);
   if (rxq_p != rxq) FreeDoubles(rxq);
   if (pzq_p != pzq) FreeDoubles(pzq);
   if (pyq_p != pyq) FreeDoubles(pyq);
   if (pxq_p != pxq) FreeDoubles(pxq);
   if (dqp_p != dqp) FreeDoubles(dqp);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign3D_IEI_bigfloat(p, q, rx, ry, rz);
#endif

 if (lpx_p != lpx) FreeDoubles(lpx);
 if (lpy_p != lpy) FreeDoubles(lpy);
 if (lpz_p != lpz) FreeDoubles(lpz);
 if (dp_p != dp) FreeDoubles(dp);
 if (lqx_p != lqx) FreeDoubles(lqx);
 if (lqy_p != lqy) FreeDoubles(lqy);
 if (lqz_p != lqz) FreeDoubles(lqz);
 if (dq_p != dq) FreeDoubles(dq);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign3D_IEI(const genericPoint& p, const genericPoint& q, double rx, double ry, double rz)
{
   int ret;
   ret = dotProductSign3D_IEI_interval(p, q, rx, ry, rz);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign3D_IEI_exact(p, q, rx, ry, rz);
}

inline int dotProductSign3D_IIE_interval(const genericPoint& p, const genericPoint& r, interval_number qx, interval_number qy, interval_number qz)
{
   interval_number lpx, lpy, lpz, dp, lrx, lry, lrz, dr;
   if (
   !p.getIntervalLambda(lpx, lpy, lpz, dp)
   || !r.getIntervalLambda(lrx, lry, lrz, dr)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number qxd(qx * dp);
   const interval_number qyd(qy * dp);
   const interval_number qzd(qz * dp);
   const interval_number lx(lpx - qxd);
   const interval_number ly(lpy - qyd);
   const interval_number lz(lpz - qzd);
   const interval_number qxr(qx * dr);
   const interval_number qyr(qy * dr);
   const interval_number qzr(qz * dr);
   const interval_number gx(lrx - qxr);
   const interval_number gy(lry - qyr);
   const interval_number gz(lrz - qzr);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number dz(lz * gz);
   const interval_number d1(dx + dy);
   const interval_number d(d1 + dz);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign3D_IIE_bigfloat(const genericPoint& p, const genericPoint& r, bigfloat qx, bigfloat qy, bigfloat qz)
{
   bigfloat lpx, lpy, lpz, dp, lrx, lry, lrz, dr;
   p.getBigfloatLambda(lpx, lpy, lpz, dp);
   r.getBigfloatLambda(lrx, lry, lrz, dr);
   const bigfloat qxd(qx * dp);
   const bigfloat qyd(qy * dp);
   const bigfloat qzd(qz * dp);
   const bigfloat lx(lpx - qxd);
   const bigfloat ly(lpy - qyd);
   const bigfloat lz(lpz - qzd);
   const bigfloat qxr(qx * dr);
   const bigfloat qyr(qy * dr);
   const bigfloat qzr(qz * dr);
   const bigfloat gx(lrx - qxr);
   const bigfloat gy(lry - qyr);
   const bigfloat gz(lrz - qzr);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat dz(lz * gz);
   const bigfloat d1(dx + dy);
   const bigfloat d(d1 + dz);
   return sgn(d);
}

inline int dotProductSign3D_IIE_exact(const genericPoint& p, const genericPoint& r, double qx, double qy, double qz)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lpx_p[64], *lpx = lpx_p, lpy_p[64], *lpy = lpy_p, lpz_p[64], *lpz = lpz_p, dp_p[64], *dp = dp_p, lrx_p[64], *lrx = lrx_p, lry_p[64], *lry = lry_p, lrz_p[64], *lrz = lrz_p, dr_p[64], *dr = dr_p;
 int lpx_len = 64, lpy_len = 64, lpz_len = 64, dp_len = 64, lrx_len = 64, lry_len = 64, lrz_len = 64, dr_len = 64;
 p.getExactLambda(&lpx, lpx_len, &lpy, lpy_len, &lpz, lpz_len, &dp, dp_len);
 r.getExactLambda(&lrx, lrx_len, &lry, lry_len, &lrz, lrz_len, &dr, dr_len);
 if ((dp[dp_len - 1] != 0) && (dr[dr_len - 1] != 0))
 {
   
   double qxd_p[64], *qxd = qxd_p;
   int qxd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qx, &qxd, 64);
   double qyd_p[64], *qyd = qyd_p;
   int qyd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qy, &qyd, 64);
   double qzd_p[64], *qzd = qzd_p;
   int qzd_len = expansionObject::Gen_Scale_With_PreAlloc(dp_len, dp, qz, &qzd, 64);
   double lx_p[64], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(lpx_len, lpx, qxd_len, qxd, &lx, 64);
   double ly_p[64], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(lpy_len, lpy, qyd_len, qyd, &ly, 64);
   double lz_p[64], *lz = lz_p;
   int lz_len = expansionObject::Gen_Diff_With_PreAlloc(lpz_len, lpz, qzd_len, qzd, &lz, 64);
   double qxr_p[64], *qxr = qxr_p;
   int qxr_len = expansionObject::Gen_Scale_With_PreAlloc(dr_len, dr, qx, &qxr, 64);
   double qyr_p[64], *qyr = qyr_p;
   int qyr_len = expansionObject::Gen_Scale_With_PreAlloc(dr_len, dr, qy, &qyr, 64);
   double qzr_p[64], *qzr = qzr_p;
   int qzr_len = expansionObject::Gen_Scale_With_PreAlloc(dr_len, dr, qz, &qzr, 64);
   double gx_p[64], *gx = gx_p;
   int gx_len = expansionObject::Gen_Diff_With_PreAlloc(lrx_len, lrx, qxr_len, qxr, &gx, 64);
   double gy_p[64], *gy = gy_p;
   int gy_len = expansionObject::Gen_Diff_With_PreAlloc(lry_len, lry, qyr_len, qyr, &gy, 64);
   double gz_p[64], *gz = gz_p;
   int gz_len = expansionObject::Gen_Diff_With_PreAlloc(lrz_len, lrz, qzr_len, qzr, &gz, 64);
   double dx_p[64], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, gx_len, gx, &dx, 64);
   double dy_p[64], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, gy_len, gy, &dy, 64);
   double dz_p[64], *dz = dz_p;
   int dz_len = expansionObject::Gen_Product_With_PreAlloc(lz_len, lz, gz_len, gz, &dz, 64);
   double d1_p[64], *d1 = d1_p;
   int d1_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d1, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(d1_len, d1, dz_len, dz, &d, 64);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (d1_p != d1) FreeDoubles(d1);
   if (dz_p != dz) FreeDoubles(dz);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (gz_p != gz) FreeDoubles(gz);
   if (gy_p != gy) FreeDoubles(gy);
   if (gx_p != gx) FreeDoubles(gx);
   if (qzr_p != qzr) FreeDoubles(qzr);
   if (qyr_p != qyr) FreeDoubles(qyr);
   if (qxr_p != qxr) FreeDoubles(qxr);
   if (lz_p != lz) FreeDoubles(lz);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (qzd_p != qzd) FreeDoubles(qzd);
   if (qyd_p != qyd) FreeDoubles(qyd);
   if (qxd_p != qxd) FreeDoubles(qxd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign3D_IIE_bigfloat(p, r, qx, qy, qz);
#endif

 if (lpx_p != lpx) FreeDoubles(lpx);
 if (lpy_p != lpy) FreeDoubles(lpy);
 if (lpz_p != lpz) FreeDoubles(lpz);
 if (dp_p != dp) FreeDoubles(dp);
 if (lrx_p != lrx) FreeDoubles(lrx);
 if (lry_p != lry) FreeDoubles(lry);
 if (lrz_p != lrz) FreeDoubles(lrz);
 if (dr_p != dr) FreeDoubles(dr);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign3D_IIE(const genericPoint& p, const genericPoint& r, double qx, double qy, double qz)
{
   int ret;
   ret = dotProductSign3D_IIE_interval(p, r, qx, qy, qz);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign3D_IIE_exact(p, r, qx, qy, qz);
}

inline int dotProductSign3D_III_interval(const genericPoint& p, const genericPoint& r, const genericPoint& q)
{
   interval_number lpx, lpy, lpz, dp, lrx, lry, lrz, dr, lqx, lqy, lqz, dq;
   if (
   !p.getIntervalLambda(lpx, lpy, lpz, dp)
   || !r.getIntervalLambda(lrx, lry, lrz, dr)
   || !q.getIntervalLambda(lqx, lqy, lqz, dq)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number qxd(lqx * dp);
   const interval_number qyd(lqy * dp);
   const interval_number qzd(lqz * dp);
   const interval_number lpxq(lpx * dq);
   const interval_number lpyq(lpy * dq);
   const interval_number lpzq(lpz * dq);
   const interval_number lx(lpxq - qxd);
   const interval_number ly(lpyq - qyd);
   const interval_number lz(lpzq - qzd);
   const interval_number qxr(lqx * dr);
   const interval_number qyr(lqy * dr);
   const interval_number qzr(lqz * dr);
   const interval_number lrxq(lrx * dq);
   const interval_number lryq(lry * dq);
   const interval_number lrzq(lrz * dq);
   const interval_number gx(lrxq - qxr);
   const interval_number gy(lryq - qyr);
   const interval_number gz(lrzq - qzr);
   const interval_number dx(lx * gx);
   const interval_number dy(ly * gy);
   const interval_number dz(lz * gz);
   const interval_number d1(dx + dy);
   const interval_number d(d1 + dz);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int dotProductSign3D_III_bigfloat(const genericPoint& p, const genericPoint& r, const genericPoint& q)
{
   bigfloat lpx, lpy, lpz, dp, lrx, lry, lrz, dr, lqx, lqy, lqz, dq;
   p.getBigfloatLambda(lpx, lpy, lpz, dp);
   r.getBigfloatLambda(lrx, lry, lrz, dr);
   q.getBigfloatLambda(lqx, lqy, lqz, dq);
   const bigfloat qxd(lqx * dp);
   const bigfloat qyd(lqy * dp);
   const bigfloat qzd(lqz * dp);
   const bigfloat lpxq(lpx * dq);
   const bigfloat lpyq(lpy * dq);
   const bigfloat lpzq(lpz * dq);
   const bigfloat lx(lpxq - qxd);
   const bigfloat ly(lpyq - qyd);
   const bigfloat lz(lpzq - qzd);
   const bigfloat qxr(lqx * dr);
   const bigfloat qyr(lqy * dr);
   const bigfloat qzr(lqz * dr);
   const bigfloat lrxq(lrx * dq);
   const bigfloat lryq(lry * dq);
   const bigfloat lrzq(lrz * dq);
   const bigfloat gx(lrxq - qxr);
   const bigfloat gy(lryq - qyr);
   const bigfloat gz(lrzq - qzr);
   const bigfloat dx(lx * gx);
   const bigfloat dy(ly * gy);
   const bigfloat dz(lz * gz);
   const bigfloat d1(dx + dy);
   const bigfloat d(d1 + dz);
   return sgn(d);
}

inline int dotProductSign3D_III_exact(const genericPoint& p, const genericPoint& r, const genericPoint& q)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double lpx_p[32], *lpx = lpx_p, lpy_p[32], *lpy = lpy_p, lpz_p[32], *lpz = lpz_p, dp_p[32], *dp = dp_p, lrx_p[32], *lrx = lrx_p, lry_p[32], *lry = lry_p, lrz_p[32], *lrz = lrz_p, dr_p[32], *dr = dr_p, lqx_p[32], *lqx = lqx_p, lqy_p[32], *lqy = lqy_p, lqz_p[32], *lqz = lqz_p, dq_p[32], *dq = dq_p;
 int lpx_len = 32, lpy_len = 32, lpz_len = 32, dp_len = 32, lrx_len = 32, lry_len = 32, lrz_len = 32, dr_len = 32, lqx_len = 32, lqy_len = 32, lqz_len = 32, dq_len = 32;
 p.getExactLambda(&lpx, lpx_len, &lpy, lpy_len, &lpz, lpz_len, &dp, dp_len);
 r.getExactLambda(&lrx, lrx_len, &lry, lry_len, &lrz, lrz_len, &dr, dr_len);
 q.getExactLambda(&lqx, lqx_len, &lqy, lqy_len, &lqz, lqz_len, &dq, dq_len);
 if ((dp[dp_len - 1] != 0) && (dr[dr_len - 1] != 0) && (dq[dq_len - 1] != 0))
 {
   
   double qxd_p[32], *qxd = qxd_p;
   int qxd_len = expansionObject::Gen_Product_With_PreAlloc(lqx_len, lqx, dp_len, dp, &qxd, 32);
   double qyd_p[32], *qyd = qyd_p;
   int qyd_len = expansionObject::Gen_Product_With_PreAlloc(lqy_len, lqy, dp_len, dp, &qyd, 32);
   double qzd_p[32], *qzd = qzd_p;
   int qzd_len = expansionObject::Gen_Product_With_PreAlloc(lqz_len, lqz, dp_len, dp, &qzd, 32);
   double lpxq_p[32], *lpxq = lpxq_p;
   int lpxq_len = expansionObject::Gen_Product_With_PreAlloc(lpx_len, lpx, dq_len, dq, &lpxq, 32);
   double lpyq_p[32], *lpyq = lpyq_p;
   int lpyq_len = expansionObject::Gen_Product_With_PreAlloc(lpy_len, lpy, dq_len, dq, &lpyq, 32);
   double lpzq_p[32], *lpzq = lpzq_p;
   int lpzq_len = expansionObject::Gen_Product_With_PreAlloc(lpz_len, lpz, dq_len, dq, &lpzq, 32);
   double lx_p[32], *lx = lx_p;
   int lx_len = expansionObject::Gen_Diff_With_PreAlloc(lpxq_len, lpxq, qxd_len, qxd, &lx, 32);
   double ly_p[32], *ly = ly_p;
   int ly_len = expansionObject::Gen_Diff_With_PreAlloc(lpyq_len, lpyq, qyd_len, qyd, &ly, 32);
   double lz_p[32], *lz = lz_p;
   int lz_len = expansionObject::Gen_Diff_With_PreAlloc(lpzq_len, lpzq, qzd_len, qzd, &lz, 32);
   double qxr_p[32], *qxr = qxr_p;
   int qxr_len = expansionObject::Gen_Product_With_PreAlloc(lqx_len, lqx, dr_len, dr, &qxr, 32);
   double qyr_p[32], *qyr = qyr_p;
   int qyr_len = expansionObject::Gen_Product_With_PreAlloc(lqy_len, lqy, dr_len, dr, &qyr, 32);
   double qzr_p[32], *qzr = qzr_p;
   int qzr_len = expansionObject::Gen_Product_With_PreAlloc(lqz_len, lqz, dr_len, dr, &qzr, 32);
   double lrxq_p[32], *lrxq = lrxq_p;
   int lrxq_len = expansionObject::Gen_Product_With_PreAlloc(lrx_len, lrx, dq_len, dq, &lrxq, 32);
   double lryq_p[32], *lryq = lryq_p;
   int lryq_len = expansionObject::Gen_Product_With_PreAlloc(lry_len, lry, dq_len, dq, &lryq, 32);
   double lrzq_p[32], *lrzq = lrzq_p;
   int lrzq_len = expansionObject::Gen_Product_With_PreAlloc(lrz_len, lrz, dq_len, dq, &lrzq, 32);
   double gx_p[32], *gx = gx_p;
   int gx_len = expansionObject::Gen_Diff_With_PreAlloc(lrxq_len, lrxq, qxr_len, qxr, &gx, 32);
   double gy_p[32], *gy = gy_p;
   int gy_len = expansionObject::Gen_Diff_With_PreAlloc(lryq_len, lryq, qyr_len, qyr, &gy, 32);
   double gz_p[32], *gz = gz_p;
   int gz_len = expansionObject::Gen_Diff_With_PreAlloc(lrzq_len, lrzq, qzr_len, qzr, &gz, 32);
   double dx_p[32], *dx = dx_p;
   int dx_len = expansionObject::Gen_Product_With_PreAlloc(lx_len, lx, gx_len, gx, &dx, 32);
   double dy_p[32], *dy = dy_p;
   int dy_len = expansionObject::Gen_Product_With_PreAlloc(ly_len, ly, gy_len, gy, &dy, 32);
   double dz_p[32], *dz = dz_p;
   int dz_len = expansionObject::Gen_Product_With_PreAlloc(lz_len, lz, gz_len, gz, &dz, 32);
   double d1_p[32], *d1 = d1_p;
   int d1_len = expansionObject::Gen_Sum_With_PreAlloc(dx_len, dx, dy_len, dy, &d1, 32);
   double d_p[32], *d = d_p;
   int d_len = expansionObject::Gen_Sum_With_PreAlloc(d1_len, d1, dz_len, dz, &d, 32);

   return_value = d[d_len - 1];
   if (d_p != d) FreeDoubles(d);
   if (d1_p != d1) FreeDoubles(d1);
   if (dz_p != dz) FreeDoubles(dz);
   if (dy_p != dy) FreeDoubles(dy);
   if (dx_p != dx) FreeDoubles(dx);
   if (gz_p != gz) FreeDoubles(gz);
   if (gy_p != gy) FreeDoubles(gy);
   if (gx_p != gx) FreeDoubles(gx);
   if (lrzq_p != lrzq) FreeDoubles(lrzq);
   if (lryq_p != lryq) FreeDoubles(lryq);
   if (lrxq_p != lrxq) FreeDoubles(lrxq);
   if (qzr_p != qzr) FreeDoubles(qzr);
   if (qyr_p != qyr) FreeDoubles(qyr);
   if (qxr_p != qxr) FreeDoubles(qxr);
   if (lz_p != lz) FreeDoubles(lz);
   if (ly_p != ly) FreeDoubles(ly);
   if (lx_p != lx) FreeDoubles(lx);
   if (lpzq_p != lpzq) FreeDoubles(lpzq);
   if (lpyq_p != lpyq) FreeDoubles(lpyq);
   if (lpxq_p != lpxq) FreeDoubles(lpxq);
   if (qzd_p != qzd) FreeDoubles(qzd);
   if (qyd_p != qyd) FreeDoubles(qyd);
   if (qxd_p != qxd) FreeDoubles(qxd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = dotProductSign3D_III_bigfloat(p, r, q);
#endif

 if (lpx_p != lpx) FreeDoubles(lpx);
 if (lpy_p != lpy) FreeDoubles(lpy);
 if (lpz_p != lpz) FreeDoubles(lpz);
 if (dp_p != dp) FreeDoubles(dp);
 if (lrx_p != lrx) FreeDoubles(lrx);
 if (lry_p != lry) FreeDoubles(lry);
 if (lrz_p != lrz) FreeDoubles(lrz);
 if (dr_p != dr) FreeDoubles(dr);
 if (lqx_p != lqx) FreeDoubles(lqx);
 if (lqy_p != lqy) FreeDoubles(lqy);
 if (lqz_p != lqz) FreeDoubles(lqz);
 if (dq_p != dq) FreeDoubles(dq);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int dotProductSign3D_III(const genericPoint& p, const genericPoint& r, const genericPoint& q)
{
   int ret;
   ret = dotProductSign3D_III_interval(p, r, q);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return dotProductSign3D_III_exact(p, r, q);
}

inline int incirclexy_indirect_IEEE_interval(const genericPoint& p1, interval_number pbx, interval_number pby, interval_number pcx, interval_number pcy, interval_number pdx, interval_number pdy)
{
   interval_number l1x, l1y, l1z, d1;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pdxt(pdx * d1);
   const interval_number pdyt(pdy * d1);
   const interval_number adx(l1x - pdxt);
   const interval_number ady(l1y - pdyt);
   const interval_number bdx(pbx - pdx);
   const interval_number bdy(pby - pdy);
   const interval_number cdx(pcx - pdx);
   const interval_number cdy(pcy - pdy);
   const interval_number abdeta(adx * bdy);
   const interval_number abdetb(bdx * ady);
   const interval_number abdet(abdeta - abdetb);
   const interval_number bcdeta(bdx * cdy);
   const interval_number bcdetb(cdx * bdy);
   const interval_number bcdet(bcdeta - bcdetb);
   const interval_number cadeta(cdx * ady);
   const interval_number cadetb(adx * cdy);
   const interval_number cadet(cadeta - cadetb);
   const interval_number alifta(adx * adx);
   const interval_number aliftb(ady * ady);
   const interval_number alift(alifta + aliftb);
   const interval_number blifta(bdx * bdx);
   const interval_number bliftb(bdy * bdy);
   const interval_number blift(blifta + bliftb);
   const interval_number clifta(cdx * cdx);
   const interval_number cliftb(cdy * cdy);
   const interval_number clift(clifta + cliftb);
   const interval_number la(alift * bcdet);
   const interval_number lbt(blift * cadet);
   const interval_number lb(lbt * d1);
   const interval_number lct(clift * abdet);
   const interval_number lc(lct * d1);
   const interval_number lab(la + lb);
   const interval_number L(lab + lc);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int incirclexy_indirect_IEEE_bigfloat(const genericPoint& p1, bigfloat pbx, bigfloat pby, bigfloat pcx, bigfloat pcy, bigfloat pdx, bigfloat pdy)
{
   bigfloat l1x, l1y, l1z, d1;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   const bigfloat pdxt(pdx * d1);
   const bigfloat pdyt(pdy * d1);
   const bigfloat adx(l1x - pdxt);
   const bigfloat ady(l1y - pdyt);
   const bigfloat bdx(pbx - pdx);
   const bigfloat bdy(pby - pdy);
   const bigfloat cdx(pcx - pdx);
   const bigfloat cdy(pcy - pdy);
   const bigfloat abdeta(adx * bdy);
   const bigfloat abdetb(bdx * ady);
   const bigfloat abdet(abdeta - abdetb);
   const bigfloat bcdeta(bdx * cdy);
   const bigfloat bcdetb(cdx * bdy);
   const bigfloat bcdet(bcdeta - bcdetb);
   const bigfloat cadeta(cdx * ady);
   const bigfloat cadetb(adx * cdy);
   const bigfloat cadet(cadeta - cadetb);
   const bigfloat alifta(adx * adx);
   const bigfloat aliftb(ady * ady);
   const bigfloat alift(alifta + aliftb);
   const bigfloat blifta(bdx * bdx);
   const bigfloat bliftb(bdy * bdy);
   const bigfloat blift(blifta + bliftb);
   const bigfloat clifta(cdx * cdx);
   const bigfloat cliftb(cdy * cdy);
   const bigfloat clift(clifta + cliftb);
   const bigfloat la(alift * bcdet);
   const bigfloat lbt(blift * cadet);
   const bigfloat lb(lbt * d1);
   const bigfloat lct(clift * abdet);
   const bigfloat lc(lct * d1);
   const bigfloat lab(la + lb);
   const bigfloat L(lab + lc);
   return sgn(L);
}

inline int incirclexy_indirect_IEEE_exact(const genericPoint& p1, double pbx, double pby, double pcx, double pcy, double pdx, double pdy)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[64], *l1x = l1x_p, l1y_p[64], *l1y = l1y_p, l1z_p[64], *l1z = l1z_p, d1_p[64], *d1 = d1_p;
 int l1x_len = 64, l1y_len = 64, l1z_len = 64, d1_len = 64;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double pdxt_p[64], *pdxt = pdxt_p;
   int pdxt_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdx, &pdxt, 64);
   double pdyt_p[64], *pdyt = pdyt_p;
   int pdyt_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdy, &pdyt, 64);
   double adx_p[64], *adx = adx_p;
   int adx_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pdxt_len, pdxt, &adx, 64);
   double ady_p[64], *ady = ady_p;
   int ady_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, pdyt_len, pdyt, &ady, 64);
   double bdx[2];
   expansionObject::two_Diff(pbx, pdx, bdx);
   double bdy[2];
   expansionObject::two_Diff(pby, pdy, bdy);
   double cdx[2];
   expansionObject::two_Diff(pcx, pdx, cdx);
   double cdy[2];
   expansionObject::two_Diff(pcy, pdy, cdy);
   double abdeta_p[64], *abdeta = abdeta_p;
   int abdeta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, 2, bdy, &abdeta, 64);
   double abdetb_p[64], *abdetb = abdetb_p;
   int abdetb_len = expansionObject::Gen_Product_With_PreAlloc(2, bdx, ady_len, ady, &abdetb, 64);
   double abdet_p[64], *abdet = abdet_p;
   int abdet_len = expansionObject::Gen_Diff_With_PreAlloc(abdeta_len, abdeta, abdetb_len, abdetb, &abdet, 64);
   double bcdeta[8];
   int bcdeta_len = expansionObject::Gen_Product(2, bdx, 2, cdy, bcdeta);
   double bcdetb[8];
   int bcdetb_len = expansionObject::Gen_Product(2, cdx, 2, bdy, bcdetb);
   double bcdet[16];
   int bcdet_len = expansionObject::Gen_Diff(bcdeta_len, bcdeta, bcdetb_len, bcdetb, bcdet);
   double cadeta_p[64], *cadeta = cadeta_p;
   int cadeta_len = expansionObject::Gen_Product_With_PreAlloc(2, cdx, ady_len, ady, &cadeta, 64);
   double cadetb_p[64], *cadetb = cadetb_p;
   int cadetb_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, 2, cdy, &cadetb, 64);
   double cadet_p[64], *cadet = cadet_p;
   int cadet_len = expansionObject::Gen_Diff_With_PreAlloc(cadeta_len, cadeta, cadetb_len, cadetb, &cadet, 64);
   double alifta_p[64], *alifta = alifta_p;
   int alifta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, adx_len, adx, &alifta, 64);
   double aliftb_p[64], *aliftb = aliftb_p;
   int aliftb_len = expansionObject::Gen_Product_With_PreAlloc(ady_len, ady, ady_len, ady, &aliftb, 64);
   double alift_p[64], *alift = alift_p;
   int alift_len = expansionObject::Gen_Sum_With_PreAlloc(alifta_len, alifta, aliftb_len, aliftb, &alift, 64);
   double blifta[8];
   int blifta_len = expansionObject::Gen_Product(2, bdx, 2, bdx, blifta);
   double bliftb[8];
   int bliftb_len = expansionObject::Gen_Product(2, bdy, 2, bdy, bliftb);
   double blift[16];
   int blift_len = expansionObject::Gen_Sum(blifta_len, blifta, bliftb_len, bliftb, blift);
   double clifta[8];
   int clifta_len = expansionObject::Gen_Product(2, cdx, 2, cdx, clifta);
   double cliftb[8];
   int cliftb_len = expansionObject::Gen_Product(2, cdy, 2, cdy, cliftb);
   double clift[16];
   int clift_len = expansionObject::Gen_Sum(clifta_len, clifta, cliftb_len, cliftb, clift);
   double la_p[64], *la = la_p;
   int la_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcdet_len, bcdet, &la, 64);
   double lbt_p[64], *lbt = lbt_p;
   int lbt_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cadet_len, cadet, &lbt, 64);
   double lb_p[64], *lb = lb_p;
   int lb_len = expansionObject::Gen_Product_With_PreAlloc(lbt_len, lbt, d1_len, d1, &lb, 64);
   double lct_p[64], *lct = lct_p;
   int lct_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, abdet_len, abdet, &lct, 64);
   double lc_p[64], *lc = lc_p;
   int lc_len = expansionObject::Gen_Product_With_PreAlloc(lct_len, lct, d1_len, d1, &lc, 64);
   double lab_p[64], *lab = lab_p;
   int lab_len = expansionObject::Gen_Sum_With_PreAlloc(la_len, la, lb_len, lb, &lab, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Sum_With_PreAlloc(lab_len, lab, lc_len, lc, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (lab_p != lab) FreeDoubles(lab);
   if (lc_p != lc) FreeDoubles(lc);
   if (lct_p != lct) FreeDoubles(lct);
   if (lb_p != lb) FreeDoubles(lb);
   if (lbt_p != lbt) FreeDoubles(lbt);
   if (la_p != la) FreeDoubles(la);
   if (alift_p != alift) FreeDoubles(alift);
   if (aliftb_p != aliftb) FreeDoubles(aliftb);
   if (alifta_p != alifta) FreeDoubles(alifta);
   if (cadet_p != cadet) FreeDoubles(cadet);
   if (cadetb_p != cadetb) FreeDoubles(cadetb);
   if (cadeta_p != cadeta) FreeDoubles(cadeta);
   if (abdet_p != abdet) FreeDoubles(abdet);
   if (abdetb_p != abdetb) FreeDoubles(abdetb);
   if (abdeta_p != abdeta) FreeDoubles(abdeta);
   if (ady_p != ady) FreeDoubles(ady);
   if (adx_p != adx) FreeDoubles(adx);
   if (pdyt_p != pdyt) FreeDoubles(pdyt);
   if (pdxt_p != pdxt) FreeDoubles(pdxt);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = incirclexy_indirect_IEEE_bigfloat(p1, pbx, pby, pcx, pcy, pdx, pdy);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int incirclexy_indirect_IEEE(const genericPoint& p1, double pbx, double pby, double pcx, double pcy, double pdx, double pdy)
{
   int ret;
   ret = incirclexy_indirect_IEEE_interval(p1, pbx, pby, pcx, pcy, pdx, pdy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return incirclexy_indirect_IEEE_exact(p1, pbx, pby, pcx, pcy, pdx, pdy);
}

inline int incirclexy_indirect_IIEE_interval(const genericPoint& p1, const genericPoint& p2, interval_number pcx, interval_number pcy, interval_number pdx, interval_number pdy)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pdx1(pdx * d1);
   const interval_number pdy1(pdy * d1);
   const interval_number adx(l1x - pdx1);
   const interval_number ady(l1y - pdy1);
   const interval_number pdx2(pdx * d2);
   const interval_number pdy2(pdy * d2);
   const interval_number bdx(l2x - pdx2);
   const interval_number bdy(l2y - pdy2);
   const interval_number cdx(pcx - pdx);
   const interval_number cdy(pcy - pdy);
   const interval_number abdeta(adx * bdy);
   const interval_number abdetb(bdx * ady);
   const interval_number abdet(abdeta - abdetb);
   const interval_number bcdeta(bdx * cdy);
   const interval_number bcdetb(cdx * bdy);
   const interval_number bcdet(bcdeta - bcdetb);
   const interval_number cadeta(cdx * ady);
   const interval_number cadetb(adx * cdy);
   const interval_number cadet(cadeta - cadetb);
   const interval_number alifta(adx * adx);
   const interval_number aliftb(ady * ady);
   const interval_number aliftt(alifta + aliftb);
   const interval_number alift(aliftt * d2);
   const interval_number blifta(bdx * bdx);
   const interval_number bliftb(bdy * bdy);
   const interval_number blift(blifta + bliftb);
   const interval_number clifta(cdx * cdx);
   const interval_number cliftb(cdy * cdy);
   const interval_number cliftt(clifta + cliftb);
   const interval_number clift(cliftt * d2);
   const interval_number la(alift * bcdet);
   const interval_number lb(blift * cadet);
   const interval_number lc(clift * abdet);
   const interval_number lab(lc + lb);
   const interval_number lab2(lab * d1);
   const interval_number L(lab2 + la);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int incirclexy_indirect_IIEE_bigfloat(const genericPoint& p1, const genericPoint& p2, bigfloat pcx, bigfloat pcy, bigfloat pdx, bigfloat pdy)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   const bigfloat pdx1(pdx * d1);
   const bigfloat pdy1(pdy * d1);
   const bigfloat adx(l1x - pdx1);
   const bigfloat ady(l1y - pdy1);
   const bigfloat pdx2(pdx * d2);
   const bigfloat pdy2(pdy * d2);
   const bigfloat bdx(l2x - pdx2);
   const bigfloat bdy(l2y - pdy2);
   const bigfloat cdx(pcx - pdx);
   const bigfloat cdy(pcy - pdy);
   const bigfloat abdeta(adx * bdy);
   const bigfloat abdetb(bdx * ady);
   const bigfloat abdet(abdeta - abdetb);
   const bigfloat bcdeta(bdx * cdy);
   const bigfloat bcdetb(cdx * bdy);
   const bigfloat bcdet(bcdeta - bcdetb);
   const bigfloat cadeta(cdx * ady);
   const bigfloat cadetb(adx * cdy);
   const bigfloat cadet(cadeta - cadetb);
   const bigfloat alifta(adx * adx);
   const bigfloat aliftb(ady * ady);
   const bigfloat aliftt(alifta + aliftb);
   const bigfloat alift(aliftt * d2);
   const bigfloat blifta(bdx * bdx);
   const bigfloat bliftb(bdy * bdy);
   const bigfloat blift(blifta + bliftb);
   const bigfloat clifta(cdx * cdx);
   const bigfloat cliftb(cdy * cdy);
   const bigfloat cliftt(clifta + cliftb);
   const bigfloat clift(cliftt * d2);
   const bigfloat la(alift * bcdet);
   const bigfloat lb(blift * cadet);
   const bigfloat lc(clift * abdet);
   const bigfloat lab(lc + lb);
   const bigfloat lab2(lab * d1);
   const bigfloat L(lab2 + la);
   return sgn(L);
}

inline int incirclexy_indirect_IIEE_exact(const genericPoint& p1, const genericPoint& p2, double pcx, double pcy, double pdx, double pdy)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[32], *l1x = l1x_p, l1y_p[32], *l1y = l1y_p, l1z_p[32], *l1z = l1z_p, d1_p[32], *d1 = d1_p, l2x_p[32], *l2x = l2x_p, l2y_p[32], *l2y = l2y_p, l2z_p[32], *l2z = l2z_p, d2_p[32], *d2 = d2_p;
 int l1x_len = 32, l1y_len = 32, l1z_len = 32, d1_len = 32, l2x_len = 32, l2y_len = 32, l2z_len = 32, d2_len = 32;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double pdx1_p[32], *pdx1 = pdx1_p;
   int pdx1_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdx, &pdx1, 32);
   double pdy1_p[32], *pdy1 = pdy1_p;
   int pdy1_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdy, &pdy1, 32);
   double adx_p[32], *adx = adx_p;
   int adx_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pdx1_len, pdx1, &adx, 32);
   double ady_p[32], *ady = ady_p;
   int ady_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, pdy1_len, pdy1, &ady, 32);
   double pdx2_p[32], *pdx2 = pdx2_p;
   int pdx2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pdx, &pdx2, 32);
   double pdy2_p[32], *pdy2 = pdy2_p;
   int pdy2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pdy, &pdy2, 32);
   double bdx_p[32], *bdx = bdx_p;
   int bdx_len = expansionObject::Gen_Diff_With_PreAlloc(l2x_len, l2x, pdx2_len, pdx2, &bdx, 32);
   double bdy_p[32], *bdy = bdy_p;
   int bdy_len = expansionObject::Gen_Diff_With_PreAlloc(l2y_len, l2y, pdy2_len, pdy2, &bdy, 32);
   double cdx[2];
   expansionObject::two_Diff(pcx, pdx, cdx);
   double cdy[2];
   expansionObject::two_Diff(pcy, pdy, cdy);
   double abdeta_p[32], *abdeta = abdeta_p;
   int abdeta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, bdy_len, bdy, &abdeta, 32);
   double abdetb_p[32], *abdetb = abdetb_p;
   int abdetb_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, ady_len, ady, &abdetb, 32);
   double abdet_p[32], *abdet = abdet_p;
   int abdet_len = expansionObject::Gen_Diff_With_PreAlloc(abdeta_len, abdeta, abdetb_len, abdetb, &abdet, 32);
   double bcdeta_p[32], *bcdeta = bcdeta_p;
   int bcdeta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, 2, cdy, &bcdeta, 32);
   double bcdetb_p[32], *bcdetb = bcdetb_p;
   int bcdetb_len = expansionObject::Gen_Product_With_PreAlloc(2, cdx, bdy_len, bdy, &bcdetb, 32);
   double bcdet_p[32], *bcdet = bcdet_p;
   int bcdet_len = expansionObject::Gen_Diff_With_PreAlloc(bcdeta_len, bcdeta, bcdetb_len, bcdetb, &bcdet, 32);
   double cadeta_p[32], *cadeta = cadeta_p;
   int cadeta_len = expansionObject::Gen_Product_With_PreAlloc(2, cdx, ady_len, ady, &cadeta, 32);
   double cadetb_p[32], *cadetb = cadetb_p;
   int cadetb_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, 2, cdy, &cadetb, 32);
   double cadet_p[32], *cadet = cadet_p;
   int cadet_len = expansionObject::Gen_Diff_With_PreAlloc(cadeta_len, cadeta, cadetb_len, cadetb, &cadet, 32);
   double alifta_p[32], *alifta = alifta_p;
   int alifta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, adx_len, adx, &alifta, 32);
   double aliftb_p[32], *aliftb = aliftb_p;
   int aliftb_len = expansionObject::Gen_Product_With_PreAlloc(ady_len, ady, ady_len, ady, &aliftb, 32);
   double aliftt_p[32], *aliftt = aliftt_p;
   int aliftt_len = expansionObject::Gen_Sum_With_PreAlloc(alifta_len, alifta, aliftb_len, aliftb, &aliftt, 32);
   double alift_p[32], *alift = alift_p;
   int alift_len = expansionObject::Gen_Product_With_PreAlloc(aliftt_len, aliftt, d2_len, d2, &alift, 32);
   double blifta_p[32], *blifta = blifta_p;
   int blifta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, bdx_len, bdx, &blifta, 32);
   double bliftb_p[32], *bliftb = bliftb_p;
   int bliftb_len = expansionObject::Gen_Product_With_PreAlloc(bdy_len, bdy, bdy_len, bdy, &bliftb, 32);
   double blift_p[32], *blift = blift_p;
   int blift_len = expansionObject::Gen_Sum_With_PreAlloc(blifta_len, blifta, bliftb_len, bliftb, &blift, 32);
   double clifta[8];
   int clifta_len = expansionObject::Gen_Product(2, cdx, 2, cdx, clifta);
   double cliftb[8];
   int cliftb_len = expansionObject::Gen_Product(2, cdy, 2, cdy, cliftb);
   double cliftt[16];
   int cliftt_len = expansionObject::Gen_Sum(clifta_len, clifta, cliftb_len, cliftb, cliftt);
   double clift_p[32], *clift = clift_p;
   int clift_len = expansionObject::Gen_Product_With_PreAlloc(cliftt_len, cliftt, d2_len, d2, &clift, 32);
   double la_p[32], *la = la_p;
   int la_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcdet_len, bcdet, &la, 32);
   double lb_p[32], *lb = lb_p;
   int lb_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cadet_len, cadet, &lb, 32);
   double lc_p[32], *lc = lc_p;
   int lc_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, abdet_len, abdet, &lc, 32);
   double lab_p[32], *lab = lab_p;
   int lab_len = expansionObject::Gen_Sum_With_PreAlloc(lc_len, lc, lb_len, lb, &lab, 32);
   double lab2_p[32], *lab2 = lab2_p;
   int lab2_len = expansionObject::Gen_Product_With_PreAlloc(lab_len, lab, d1_len, d1, &lab2, 32);
   double L_p[32], *L = L_p;
   int L_len = expansionObject::Gen_Sum_With_PreAlloc(lab2_len, lab2, la_len, la, &L, 32);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (lab2_p != lab2) FreeDoubles(lab2);
   if (lab_p != lab) FreeDoubles(lab);
   if (lc_p != lc) FreeDoubles(lc);
   if (lb_p != lb) FreeDoubles(lb);
   if (la_p != la) FreeDoubles(la);
   if (clift_p != clift) FreeDoubles(clift);
   if (blift_p != blift) FreeDoubles(blift);
   if (bliftb_p != bliftb) FreeDoubles(bliftb);
   if (blifta_p != blifta) FreeDoubles(blifta);
   if (alift_p != alift) FreeDoubles(alift);
   if (aliftt_p != aliftt) FreeDoubles(aliftt);
   if (aliftb_p != aliftb) FreeDoubles(aliftb);
   if (alifta_p != alifta) FreeDoubles(alifta);
   if (cadet_p != cadet) FreeDoubles(cadet);
   if (cadetb_p != cadetb) FreeDoubles(cadetb);
   if (cadeta_p != cadeta) FreeDoubles(cadeta);
   if (bcdet_p != bcdet) FreeDoubles(bcdet);
   if (bcdetb_p != bcdetb) FreeDoubles(bcdetb);
   if (bcdeta_p != bcdeta) FreeDoubles(bcdeta);
   if (abdet_p != abdet) FreeDoubles(abdet);
   if (abdetb_p != abdetb) FreeDoubles(abdetb);
   if (abdeta_p != abdeta) FreeDoubles(abdeta);
   if (bdy_p != bdy) FreeDoubles(bdy);
   if (bdx_p != bdx) FreeDoubles(bdx);
   if (pdy2_p != pdy2) FreeDoubles(pdy2);
   if (pdx2_p != pdx2) FreeDoubles(pdx2);
   if (ady_p != ady) FreeDoubles(ady);
   if (adx_p != adx) FreeDoubles(adx);
   if (pdy1_p != pdy1) FreeDoubles(pdy1);
   if (pdx1_p != pdx1) FreeDoubles(pdx1);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = incirclexy_indirect_IIEE_bigfloat(p1, p2, pcx, pcy, pdx, pdy);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int incirclexy_indirect_IIEE(const genericPoint& p1, const genericPoint& p2, double pcx, double pcy, double pdx, double pdy)
{
   int ret;
   ret = incirclexy_indirect_IIEE_interval(p1, p2, pcx, pcy, pdx, pdy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return incirclexy_indirect_IIEE_exact(p1, p2, pcx, pcy, pdx, pdy);
}

inline int incirclexy_indirect_IIIE_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, interval_number pdx, interval_number pdy)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   || !p3.getIntervalLambda(l3x, l3y, l3z, d3)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pdx1(pdx * d1);
   const interval_number pdy1(pdy * d1);
   const interval_number adx(l1x - pdx1);
   const interval_number ady(l1y - pdy1);
   const interval_number pdx2(pdx * d2);
   const interval_number pdy2(pdy * d2);
   const interval_number bdx(l2x - pdx2);
   const interval_number bdy(l2y - pdy2);
   const interval_number pdx3(pdx * d3);
   const interval_number pdy3(pdy * d3);
   const interval_number cdx(l3x - pdx3);
   const interval_number cdy(l3y - pdy3);
   const interval_number abdeta(adx * bdy);
   const interval_number abdetb(bdx * ady);
   const interval_number abdet(abdeta - abdetb);
   const interval_number bcdeta(bdx * cdy);
   const interval_number bcdetb(cdx * bdy);
   const interval_number bcdet(bcdeta - bcdetb);
   const interval_number cadeta(cdx * ady);
   const interval_number cadetb(adx * cdy);
   const interval_number cadet(cadeta - cadetb);
   const interval_number alifta(adx * adx);
   const interval_number aliftb(ady * ady);
   const interval_number aliftt(alifta + aliftb);
   const interval_number alift2(aliftt * d2);
   const interval_number alift(alift2 * d3);
   const interval_number blifta(bdx * bdx);
   const interval_number bliftb(bdy * bdy);
   const interval_number bliftt(blifta + bliftb);
   const interval_number blift(bliftt * d3);
   const interval_number clifta(cdx * cdx);
   const interval_number cliftb(cdy * cdy);
   const interval_number cliftt(clifta + cliftb);
   const interval_number clift(cliftt * d2);
   const interval_number la(alift * bcdet);
   const interval_number lb(blift * cadet);
   const interval_number lc(clift * abdet);
   const interval_number lab2(lc + lb);
   const interval_number lab(lab2 * d1);
   const interval_number L(lab + la);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int incirclexy_indirect_IIIE_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, bigfloat pdx, bigfloat pdy)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   p3.getBigfloatLambda(l3x, l3y, l3z, d3);
   const bigfloat pdx1(pdx * d1);
   const bigfloat pdy1(pdy * d1);
   const bigfloat adx(l1x - pdx1);
   const bigfloat ady(l1y - pdy1);
   const bigfloat pdx2(pdx * d2);
   const bigfloat pdy2(pdy * d2);
   const bigfloat bdx(l2x - pdx2);
   const bigfloat bdy(l2y - pdy2);
   const bigfloat pdx3(pdx * d3);
   const bigfloat pdy3(pdy * d3);
   const bigfloat cdx(l3x - pdx3);
   const bigfloat cdy(l3y - pdy3);
   const bigfloat abdeta(adx * bdy);
   const bigfloat abdetb(bdx * ady);
   const bigfloat abdet(abdeta - abdetb);
   const bigfloat bcdeta(bdx * cdy);
   const bigfloat bcdetb(cdx * bdy);
   const bigfloat bcdet(bcdeta - bcdetb);
   const bigfloat cadeta(cdx * ady);
   const bigfloat cadetb(adx * cdy);
   const bigfloat cadet(cadeta - cadetb);
   const bigfloat alifta(adx * adx);
   const bigfloat aliftb(ady * ady);
   const bigfloat aliftt(alifta + aliftb);
   const bigfloat alift2(aliftt * d2);
   const bigfloat alift(alift2 * d3);
   const bigfloat blifta(bdx * bdx);
   const bigfloat bliftb(bdy * bdy);
   const bigfloat bliftt(blifta + bliftb);
   const bigfloat blift(bliftt * d3);
   const bigfloat clifta(cdx * cdx);
   const bigfloat cliftb(cdy * cdy);
   const bigfloat cliftt(clifta + cliftb);
   const bigfloat clift(cliftt * d2);
   const bigfloat la(alift * bcdet);
   const bigfloat lb(blift * cadet);
   const bigfloat lc(clift * abdet);
   const bigfloat lab2(lc + lb);
   const bigfloat lab(lab2 * d1);
   const bigfloat L(lab + la);
   return sgn(L);
}

inline int incirclexy_indirect_IIIE_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double pdx, double pdy)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[32], *l1x = l1x_p, l1y_p[32], *l1y = l1y_p, l1z_p[32], *l1z = l1z_p, d1_p[32], *d1 = d1_p, l2x_p[32], *l2x = l2x_p, l2y_p[32], *l2y = l2y_p, l2z_p[32], *l2z = l2z_p, d2_p[32], *d2 = d2_p, l3x_p[32], *l3x = l3x_p, l3y_p[32], *l3y = l3y_p, l3z_p[32], *l3z = l3z_p, d3_p[32], *d3 = d3_p;
 int l1x_len = 32, l1y_len = 32, l1z_len = 32, d1_len = 32, l2x_len = 32, l2y_len = 32, l2z_len = 32, d2_len = 32, l3x_len = 32, l3y_len = 32, l3z_len = 32, d3_len = 32;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &l3z, l3z_len, &d3, d3_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0))
 {
   
   double pdx1_p[32], *pdx1 = pdx1_p;
   int pdx1_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdx, &pdx1, 32);
   double pdy1_p[32], *pdy1 = pdy1_p;
   int pdy1_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdy, &pdy1, 32);
   double adx_p[32], *adx = adx_p;
   int adx_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pdx1_len, pdx1, &adx, 32);
   double ady_p[32], *ady = ady_p;
   int ady_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, pdy1_len, pdy1, &ady, 32);
   double pdx2_p[32], *pdx2 = pdx2_p;
   int pdx2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pdx, &pdx2, 32);
   double pdy2_p[32], *pdy2 = pdy2_p;
   int pdy2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pdy, &pdy2, 32);
   double bdx_p[32], *bdx = bdx_p;
   int bdx_len = expansionObject::Gen_Diff_With_PreAlloc(l2x_len, l2x, pdx2_len, pdx2, &bdx, 32);
   double bdy_p[32], *bdy = bdy_p;
   int bdy_len = expansionObject::Gen_Diff_With_PreAlloc(l2y_len, l2y, pdy2_len, pdy2, &bdy, 32);
   double pdx3_p[32], *pdx3 = pdx3_p;
   int pdx3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pdx, &pdx3, 32);
   double pdy3_p[32], *pdy3 = pdy3_p;
   int pdy3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pdy, &pdy3, 32);
   double cdx_p[32], *cdx = cdx_p;
   int cdx_len = expansionObject::Gen_Diff_With_PreAlloc(l3x_len, l3x, pdx3_len, pdx3, &cdx, 32);
   double cdy_p[32], *cdy = cdy_p;
   int cdy_len = expansionObject::Gen_Diff_With_PreAlloc(l3y_len, l3y, pdy3_len, pdy3, &cdy, 32);
   double abdeta_p[32], *abdeta = abdeta_p;
   int abdeta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, bdy_len, bdy, &abdeta, 32);
   double abdetb_p[32], *abdetb = abdetb_p;
   int abdetb_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, ady_len, ady, &abdetb, 32);
   double abdet_p[32], *abdet = abdet_p;
   int abdet_len = expansionObject::Gen_Diff_With_PreAlloc(abdeta_len, abdeta, abdetb_len, abdetb, &abdet, 32);
   double bcdeta_p[32], *bcdeta = bcdeta_p;
   int bcdeta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, cdy_len, cdy, &bcdeta, 32);
   double bcdetb_p[32], *bcdetb = bcdetb_p;
   int bcdetb_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, bdy_len, bdy, &bcdetb, 32);
   double bcdet_p[32], *bcdet = bcdet_p;
   int bcdet_len = expansionObject::Gen_Diff_With_PreAlloc(bcdeta_len, bcdeta, bcdetb_len, bcdetb, &bcdet, 32);
   double cadeta_p[32], *cadeta = cadeta_p;
   int cadeta_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, ady_len, ady, &cadeta, 32);
   double cadetb_p[32], *cadetb = cadetb_p;
   int cadetb_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, cdy_len, cdy, &cadetb, 32);
   double cadet_p[32], *cadet = cadet_p;
   int cadet_len = expansionObject::Gen_Diff_With_PreAlloc(cadeta_len, cadeta, cadetb_len, cadetb, &cadet, 32);
   double alifta_p[32], *alifta = alifta_p;
   int alifta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, adx_len, adx, &alifta, 32);
   double aliftb_p[32], *aliftb = aliftb_p;
   int aliftb_len = expansionObject::Gen_Product_With_PreAlloc(ady_len, ady, ady_len, ady, &aliftb, 32);
   double aliftt_p[32], *aliftt = aliftt_p;
   int aliftt_len = expansionObject::Gen_Sum_With_PreAlloc(alifta_len, alifta, aliftb_len, aliftb, &aliftt, 32);
   double alift2_p[32], *alift2 = alift2_p;
   int alift2_len = expansionObject::Gen_Product_With_PreAlloc(aliftt_len, aliftt, d2_len, d2, &alift2, 32);
   double alift_p[32], *alift = alift_p;
   int alift_len = expansionObject::Gen_Product_With_PreAlloc(alift2_len, alift2, d3_len, d3, &alift, 32);
   double blifta_p[32], *blifta = blifta_p;
   int blifta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, bdx_len, bdx, &blifta, 32);
   double bliftb_p[32], *bliftb = bliftb_p;
   int bliftb_len = expansionObject::Gen_Product_With_PreAlloc(bdy_len, bdy, bdy_len, bdy, &bliftb, 32);
   double bliftt_p[32], *bliftt = bliftt_p;
   int bliftt_len = expansionObject::Gen_Sum_With_PreAlloc(blifta_len, blifta, bliftb_len, bliftb, &bliftt, 32);
   double blift_p[32], *blift = blift_p;
   int blift_len = expansionObject::Gen_Product_With_PreAlloc(bliftt_len, bliftt, d3_len, d3, &blift, 32);
   double clifta_p[32], *clifta = clifta_p;
   int clifta_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, cdx_len, cdx, &clifta, 32);
   double cliftb_p[32], *cliftb = cliftb_p;
   int cliftb_len = expansionObject::Gen_Product_With_PreAlloc(cdy_len, cdy, cdy_len, cdy, &cliftb, 32);
   double cliftt_p[32], *cliftt = cliftt_p;
   int cliftt_len = expansionObject::Gen_Sum_With_PreAlloc(clifta_len, clifta, cliftb_len, cliftb, &cliftt, 32);
   double clift_p[32], *clift = clift_p;
   int clift_len = expansionObject::Gen_Product_With_PreAlloc(cliftt_len, cliftt, d2_len, d2, &clift, 32);
   double la_p[32], *la = la_p;
   int la_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcdet_len, bcdet, &la, 32);
   double lb_p[32], *lb = lb_p;
   int lb_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cadet_len, cadet, &lb, 32);
   double lc_p[32], *lc = lc_p;
   int lc_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, abdet_len, abdet, &lc, 32);
   double lab2_p[32], *lab2 = lab2_p;
   int lab2_len = expansionObject::Gen_Sum_With_PreAlloc(lc_len, lc, lb_len, lb, &lab2, 32);
   double lab_p[32], *lab = lab_p;
   int lab_len = expansionObject::Gen_Product_With_PreAlloc(lab2_len, lab2, d1_len, d1, &lab, 32);
   double L_p[32], *L = L_p;
   int L_len = expansionObject::Gen_Sum_With_PreAlloc(lab_len, lab, la_len, la, &L, 32);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (lab_p != lab) FreeDoubles(lab);
   if (lab2_p != lab2) FreeDoubles(lab2);
   if (lc_p != lc) FreeDoubles(lc);
   if (lb_p != lb) FreeDoubles(lb);
   if (la_p != la) FreeDoubles(la);
   if (clift_p != clift) FreeDoubles(clift);
   if (cliftt_p != cliftt) FreeDoubles(cliftt);
   if (cliftb_p != cliftb) FreeDoubles(cliftb);
   if (clifta_p != clifta) FreeDoubles(clifta);
   if (blift_p != blift) FreeDoubles(blift);
   if (bliftt_p != bliftt) FreeDoubles(bliftt);
   if (bliftb_p != bliftb) FreeDoubles(bliftb);
   if (blifta_p != blifta) FreeDoubles(blifta);
   if (alift_p != alift) FreeDoubles(alift);
   if (alift2_p != alift2) FreeDoubles(alift2);
   if (aliftt_p != aliftt) FreeDoubles(aliftt);
   if (aliftb_p != aliftb) FreeDoubles(aliftb);
   if (alifta_p != alifta) FreeDoubles(alifta);
   if (cadet_p != cadet) FreeDoubles(cadet);
   if (cadetb_p != cadetb) FreeDoubles(cadetb);
   if (cadeta_p != cadeta) FreeDoubles(cadeta);
   if (bcdet_p != bcdet) FreeDoubles(bcdet);
   if (bcdetb_p != bcdetb) FreeDoubles(bcdetb);
   if (bcdeta_p != bcdeta) FreeDoubles(bcdeta);
   if (abdet_p != abdet) FreeDoubles(abdet);
   if (abdetb_p != abdetb) FreeDoubles(abdetb);
   if (abdeta_p != abdeta) FreeDoubles(abdeta);
   if (cdy_p != cdy) FreeDoubles(cdy);
   if (cdx_p != cdx) FreeDoubles(cdx);
   if (pdy3_p != pdy3) FreeDoubles(pdy3);
   if (pdx3_p != pdx3) FreeDoubles(pdx3);
   if (bdy_p != bdy) FreeDoubles(bdy);
   if (bdx_p != bdx) FreeDoubles(bdx);
   if (pdy2_p != pdy2) FreeDoubles(pdy2);
   if (pdx2_p != pdx2) FreeDoubles(pdx2);
   if (ady_p != ady) FreeDoubles(ady);
   if (adx_p != adx) FreeDoubles(adx);
   if (pdy1_p != pdy1) FreeDoubles(pdy1);
   if (pdx1_p != pdx1) FreeDoubles(pdx1);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = incirclexy_indirect_IIIE_bigfloat(p1, p2, p3, pdx, pdy);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (d3_p != d3) FreeDoubles(d3);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int incirclexy_indirect_IIIE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double pdx, double pdy)
{
   int ret;
   ret = incirclexy_indirect_IIIE_interval(p1, p2, p3, pdx, pdy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return incirclexy_indirect_IIIE_exact(p1, p2, p3, pdx, pdy);
}

inline int incirclexy_indirect_IIII_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3, l4x, l4y, l4z, d4;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   || !p3.getIntervalLambda(l3x, l3y, l3z, d3)
   || !p4.getIntervalLambda(l4x, l4y, l4z, d4)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number l1xt(l1x * d4);
   const interval_number l1yt(l1y * d4);
   const interval_number l2xt(l2x * d4);
   const interval_number l2yt(l2y * d4);
   const interval_number l3xt(l3x * d4);
   const interval_number l3yt(l3y * d4);
   const interval_number l4x1(l4x * d1);
   const interval_number l4y1(l4y * d1);
   const interval_number adx(l1xt - l4x1);
   const interval_number ady(l1yt - l4y1);
   const interval_number l4x2(l4x * d2);
   const interval_number l4y2(l4y * d2);
   const interval_number bdx(l2xt - l4x2);
   const interval_number bdy(l2yt - l4y2);
   const interval_number l4x3(l4x * d3);
   const interval_number l4y3(l4y * d3);
   const interval_number cdx(l3xt - l4x3);
   const interval_number cdy(l3yt - l4y3);
   const interval_number abdeta(adx * bdy);
   const interval_number abdetb(bdx * ady);
   const interval_number abdet(abdeta - abdetb);
   const interval_number bcdeta(bdx * cdy);
   const interval_number bcdetb(cdx * bdy);
   const interval_number bcdet(bcdeta - bcdetb);
   const interval_number cadeta(cdx * ady);
   const interval_number cadetb(adx * cdy);
   const interval_number cadet(cadeta - cadetb);
   const interval_number alifta(adx * adx);
   const interval_number aliftb(ady * ady);
   const interval_number aliftt(alifta + aliftb);
   const interval_number alift2(aliftt * d2);
   const interval_number alift(alift2 * d3);
   const interval_number blifta(bdx * bdx);
   const interval_number bliftb(bdy * bdy);
   const interval_number bliftt(blifta + bliftb);
   const interval_number blift(bliftt * d3);
   const interval_number clifta(cdx * cdx);
   const interval_number cliftb(cdy * cdy);
   const interval_number cliftt(clifta + cliftb);
   const interval_number clift(cliftt * d2);
   const interval_number la(alift * bcdet);
   const interval_number lb(blift * cadet);
   const interval_number lc(clift * abdet);
   const interval_number lab2(lc + lb);
   const interval_number lab(lab2 * d1);
   const interval_number L(lab + la);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int incirclexy_indirect_IIII_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3, l4x, l4y, l4z, d4;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   p3.getBigfloatLambda(l3x, l3y, l3z, d3);
   p4.getBigfloatLambda(l4x, l4y, l4z, d4);
   const bigfloat l1xt(l1x * d4);
   const bigfloat l1yt(l1y * d4);
   const bigfloat l2xt(l2x * d4);
   const bigfloat l2yt(l2y * d4);
   const bigfloat l3xt(l3x * d4);
   const bigfloat l3yt(l3y * d4);
   const bigfloat l4x1(l4x * d1);
   const bigfloat l4y1(l4y * d1);
   const bigfloat adx(l1xt - l4x1);
   const bigfloat ady(l1yt - l4y1);
   const bigfloat l4x2(l4x * d2);
   const bigfloat l4y2(l4y * d2);
   const bigfloat bdx(l2xt - l4x2);
   const bigfloat bdy(l2yt - l4y2);
   const bigfloat l4x3(l4x * d3);
   const bigfloat l4y3(l4y * d3);
   const bigfloat cdx(l3xt - l4x3);
   const bigfloat cdy(l3yt - l4y3);
   const bigfloat abdeta(adx * bdy);
   const bigfloat abdetb(bdx * ady);
   const bigfloat abdet(abdeta - abdetb);
   const bigfloat bcdeta(bdx * cdy);
   const bigfloat bcdetb(cdx * bdy);
   const bigfloat bcdet(bcdeta - bcdetb);
   const bigfloat cadeta(cdx * ady);
   const bigfloat cadetb(adx * cdy);
   const bigfloat cadet(cadeta - cadetb);
   const bigfloat alifta(adx * adx);
   const bigfloat aliftb(ady * ady);
   const bigfloat aliftt(alifta + aliftb);
   const bigfloat alift2(aliftt * d2);
   const bigfloat alift(alift2 * d3);
   const bigfloat blifta(bdx * bdx);
   const bigfloat bliftb(bdy * bdy);
   const bigfloat bliftt(blifta + bliftb);
   const bigfloat blift(bliftt * d3);
   const bigfloat clifta(cdx * cdx);
   const bigfloat cliftb(cdy * cdy);
   const bigfloat cliftt(clifta + cliftb);
   const bigfloat clift(cliftt * d2);
   const bigfloat la(alift * bcdet);
   const bigfloat lb(blift * cadet);
   const bigfloat lc(clift * abdet);
   const bigfloat lab2(lc + lb);
   const bigfloat lab(lab2 * d1);
   const bigfloat L(lab + la);
   return sgn(L);
}

inline int incirclexy_indirect_IIII_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[16], *l1x = l1x_p, l1y_p[16], *l1y = l1y_p, l1z_p[16], *l1z = l1z_p, d1_p[16], *d1 = d1_p, l2x_p[16], *l2x = l2x_p, l2y_p[16], *l2y = l2y_p, l2z_p[16], *l2z = l2z_p, d2_p[16], *d2 = d2_p, l3x_p[16], *l3x = l3x_p, l3y_p[16], *l3y = l3y_p, l3z_p[16], *l3z = l3z_p, d3_p[16], *d3 = d3_p, l4x_p[16], *l4x = l4x_p, l4y_p[16], *l4y = l4y_p, l4z_p[16], *l4z = l4z_p, d4_p[16], *d4 = d4_p;
 int l1x_len = 16, l1y_len = 16, l1z_len = 16, d1_len = 16, l2x_len = 16, l2y_len = 16, l2z_len = 16, d2_len = 16, l3x_len = 16, l3y_len = 16, l3z_len = 16, d3_len = 16, l4x_len = 16, l4y_len = 16, l4z_len = 16, d4_len = 16;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &l3z, l3z_len, &d3, d3_len);
 p4.getExactLambda(&l4x, l4x_len, &l4y, l4y_len, &l4z, l4z_len, &d4, d4_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0) && (d4[d4_len - 1] != 0))
 {
   
   double l1xt_p[16], *l1xt = l1xt_p;
   int l1xt_len = expansionObject::Gen_Product_With_PreAlloc(l1x_len, l1x, d4_len, d4, &l1xt, 16);
   double l1yt_p[16], *l1yt = l1yt_p;
   int l1yt_len = expansionObject::Gen_Product_With_PreAlloc(l1y_len, l1y, d4_len, d4, &l1yt, 16);
   double l2xt_p[16], *l2xt = l2xt_p;
   int l2xt_len = expansionObject::Gen_Product_With_PreAlloc(l2x_len, l2x, d4_len, d4, &l2xt, 16);
   double l2yt_p[16], *l2yt = l2yt_p;
   int l2yt_len = expansionObject::Gen_Product_With_PreAlloc(l2y_len, l2y, d4_len, d4, &l2yt, 16);
   double l3xt_p[16], *l3xt = l3xt_p;
   int l3xt_len = expansionObject::Gen_Product_With_PreAlloc(l3x_len, l3x, d4_len, d4, &l3xt, 16);
   double l3yt_p[16], *l3yt = l3yt_p;
   int l3yt_len = expansionObject::Gen_Product_With_PreAlloc(l3y_len, l3y, d4_len, d4, &l3yt, 16);
   double l4x1_p[16], *l4x1 = l4x1_p;
   int l4x1_len = expansionObject::Gen_Product_With_PreAlloc(l4x_len, l4x, d1_len, d1, &l4x1, 16);
   double l4y1_p[16], *l4y1 = l4y1_p;
   int l4y1_len = expansionObject::Gen_Product_With_PreAlloc(l4y_len, l4y, d1_len, d1, &l4y1, 16);
   double adx_p[16], *adx = adx_p;
   int adx_len = expansionObject::Gen_Diff_With_PreAlloc(l1xt_len, l1xt, l4x1_len, l4x1, &adx, 16);
   double ady_p[16], *ady = ady_p;
   int ady_len = expansionObject::Gen_Diff_With_PreAlloc(l1yt_len, l1yt, l4y1_len, l4y1, &ady, 16);
   double l4x2_p[16], *l4x2 = l4x2_p;
   int l4x2_len = expansionObject::Gen_Product_With_PreAlloc(l4x_len, l4x, d2_len, d2, &l4x2, 16);
   double l4y2_p[16], *l4y2 = l4y2_p;
   int l4y2_len = expansionObject::Gen_Product_With_PreAlloc(l4y_len, l4y, d2_len, d2, &l4y2, 16);
   double bdx_p[16], *bdx = bdx_p;
   int bdx_len = expansionObject::Gen_Diff_With_PreAlloc(l2xt_len, l2xt, l4x2_len, l4x2, &bdx, 16);
   double bdy_p[16], *bdy = bdy_p;
   int bdy_len = expansionObject::Gen_Diff_With_PreAlloc(l2yt_len, l2yt, l4y2_len, l4y2, &bdy, 16);
   double l4x3_p[16], *l4x3 = l4x3_p;
   int l4x3_len = expansionObject::Gen_Product_With_PreAlloc(l4x_len, l4x, d3_len, d3, &l4x3, 16);
   double l4y3_p[16], *l4y3 = l4y3_p;
   int l4y3_len = expansionObject::Gen_Product_With_PreAlloc(l4y_len, l4y, d3_len, d3, &l4y3, 16);
   double cdx_p[16], *cdx = cdx_p;
   int cdx_len = expansionObject::Gen_Diff_With_PreAlloc(l3xt_len, l3xt, l4x3_len, l4x3, &cdx, 16);
   double cdy_p[16], *cdy = cdy_p;
   int cdy_len = expansionObject::Gen_Diff_With_PreAlloc(l3yt_len, l3yt, l4y3_len, l4y3, &cdy, 16);
   double abdeta_p[16], *abdeta = abdeta_p;
   int abdeta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, bdy_len, bdy, &abdeta, 16);
   double abdetb_p[16], *abdetb = abdetb_p;
   int abdetb_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, ady_len, ady, &abdetb, 16);
   double abdet_p[16], *abdet = abdet_p;
   int abdet_len = expansionObject::Gen_Diff_With_PreAlloc(abdeta_len, abdeta, abdetb_len, abdetb, &abdet, 16);
   double bcdeta_p[16], *bcdeta = bcdeta_p;
   int bcdeta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, cdy_len, cdy, &bcdeta, 16);
   double bcdetb_p[16], *bcdetb = bcdetb_p;
   int bcdetb_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, bdy_len, bdy, &bcdetb, 16);
   double bcdet_p[16], *bcdet = bcdet_p;
   int bcdet_len = expansionObject::Gen_Diff_With_PreAlloc(bcdeta_len, bcdeta, bcdetb_len, bcdetb, &bcdet, 16);
   double cadeta_p[16], *cadeta = cadeta_p;
   int cadeta_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, ady_len, ady, &cadeta, 16);
   double cadetb_p[16], *cadetb = cadetb_p;
   int cadetb_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, cdy_len, cdy, &cadetb, 16);
   double cadet_p[16], *cadet = cadet_p;
   int cadet_len = expansionObject::Gen_Diff_With_PreAlloc(cadeta_len, cadeta, cadetb_len, cadetb, &cadet, 16);
   double alifta_p[16], *alifta = alifta_p;
   int alifta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, adx_len, adx, &alifta, 16);
   double aliftb_p[16], *aliftb = aliftb_p;
   int aliftb_len = expansionObject::Gen_Product_With_PreAlloc(ady_len, ady, ady_len, ady, &aliftb, 16);
   double aliftt_p[16], *aliftt = aliftt_p;
   int aliftt_len = expansionObject::Gen_Sum_With_PreAlloc(alifta_len, alifta, aliftb_len, aliftb, &aliftt, 16);
   double alift2_p[16], *alift2 = alift2_p;
   int alift2_len = expansionObject::Gen_Product_With_PreAlloc(aliftt_len, aliftt, d2_len, d2, &alift2, 16);
   double alift_p[16], *alift = alift_p;
   int alift_len = expansionObject::Gen_Product_With_PreAlloc(alift2_len, alift2, d3_len, d3, &alift, 16);
   double blifta_p[16], *blifta = blifta_p;
   int blifta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, bdx_len, bdx, &blifta, 16);
   double bliftb_p[16], *bliftb = bliftb_p;
   int bliftb_len = expansionObject::Gen_Product_With_PreAlloc(bdy_len, bdy, bdy_len, bdy, &bliftb, 16);
   double bliftt_p[16], *bliftt = bliftt_p;
   int bliftt_len = expansionObject::Gen_Sum_With_PreAlloc(blifta_len, blifta, bliftb_len, bliftb, &bliftt, 16);
   double blift_p[16], *blift = blift_p;
   int blift_len = expansionObject::Gen_Product_With_PreAlloc(bliftt_len, bliftt, d3_len, d3, &blift, 16);
   double clifta_p[16], *clifta = clifta_p;
   int clifta_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, cdx_len, cdx, &clifta, 16);
   double cliftb_p[16], *cliftb = cliftb_p;
   int cliftb_len = expansionObject::Gen_Product_With_PreAlloc(cdy_len, cdy, cdy_len, cdy, &cliftb, 16);
   double cliftt_p[16], *cliftt = cliftt_p;
   int cliftt_len = expansionObject::Gen_Sum_With_PreAlloc(clifta_len, clifta, cliftb_len, cliftb, &cliftt, 16);
   double clift_p[16], *clift = clift_p;
   int clift_len = expansionObject::Gen_Product_With_PreAlloc(cliftt_len, cliftt, d2_len, d2, &clift, 16);
   double la_p[16], *la = la_p;
   int la_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcdet_len, bcdet, &la, 16);
   double lb_p[16], *lb = lb_p;
   int lb_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cadet_len, cadet, &lb, 16);
   double lc_p[16], *lc = lc_p;
   int lc_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, abdet_len, abdet, &lc, 16);
   double lab2_p[16], *lab2 = lab2_p;
   int lab2_len = expansionObject::Gen_Sum_With_PreAlloc(lc_len, lc, lb_len, lb, &lab2, 16);
   double lab_p[16], *lab = lab_p;
   int lab_len = expansionObject::Gen_Product_With_PreAlloc(lab2_len, lab2, d1_len, d1, &lab, 16);
   double L_p[16], *L = L_p;
   int L_len = expansionObject::Gen_Sum_With_PreAlloc(lab_len, lab, la_len, la, &L, 16);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (lab_p != lab) FreeDoubles(lab);
   if (lab2_p != lab2) FreeDoubles(lab2);
   if (lc_p != lc) FreeDoubles(lc);
   if (lb_p != lb) FreeDoubles(lb);
   if (la_p != la) FreeDoubles(la);
   if (clift_p != clift) FreeDoubles(clift);
   if (cliftt_p != cliftt) FreeDoubles(cliftt);
   if (cliftb_p != cliftb) FreeDoubles(cliftb);
   if (clifta_p != clifta) FreeDoubles(clifta);
   if (blift_p != blift) FreeDoubles(blift);
   if (bliftt_p != bliftt) FreeDoubles(bliftt);
   if (bliftb_p != bliftb) FreeDoubles(bliftb);
   if (blifta_p != blifta) FreeDoubles(blifta);
   if (alift_p != alift) FreeDoubles(alift);
   if (alift2_p != alift2) FreeDoubles(alift2);
   if (aliftt_p != aliftt) FreeDoubles(aliftt);
   if (aliftb_p != aliftb) FreeDoubles(aliftb);
   if (alifta_p != alifta) FreeDoubles(alifta);
   if (cadet_p != cadet) FreeDoubles(cadet);
   if (cadetb_p != cadetb) FreeDoubles(cadetb);
   if (cadeta_p != cadeta) FreeDoubles(cadeta);
   if (bcdet_p != bcdet) FreeDoubles(bcdet);
   if (bcdetb_p != bcdetb) FreeDoubles(bcdetb);
   if (bcdeta_p != bcdeta) FreeDoubles(bcdeta);
   if (abdet_p != abdet) FreeDoubles(abdet);
   if (abdetb_p != abdetb) FreeDoubles(abdetb);
   if (abdeta_p != abdeta) FreeDoubles(abdeta);
   if (cdy_p != cdy) FreeDoubles(cdy);
   if (cdx_p != cdx) FreeDoubles(cdx);
   if (l4y3_p != l4y3) FreeDoubles(l4y3);
   if (l4x3_p != l4x3) FreeDoubles(l4x3);
   if (bdy_p != bdy) FreeDoubles(bdy);
   if (bdx_p != bdx) FreeDoubles(bdx);
   if (l4y2_p != l4y2) FreeDoubles(l4y2);
   if (l4x2_p != l4x2) FreeDoubles(l4x2);
   if (ady_p != ady) FreeDoubles(ady);
   if (adx_p != adx) FreeDoubles(adx);
   if (l4y1_p != l4y1) FreeDoubles(l4y1);
   if (l4x1_p != l4x1) FreeDoubles(l4x1);
   if (l3yt_p != l3yt) FreeDoubles(l3yt);
   if (l3xt_p != l3xt) FreeDoubles(l3xt);
   if (l2yt_p != l2yt) FreeDoubles(l2yt);
   if (l2xt_p != l2xt) FreeDoubles(l2xt);
   if (l1yt_p != l1yt) FreeDoubles(l1yt);
   if (l1xt_p != l1xt) FreeDoubles(l1xt);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = incirclexy_indirect_IIII_bigfloat(p1, p2, p3, p4);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (d3_p != d3) FreeDoubles(d3);
 if (l4x_p != l4x) FreeDoubles(l4x);
 if (l4y_p != l4y) FreeDoubles(l4y);
 if (l4z_p != l4z) FreeDoubles(l4z);
 if (d4_p != d4) FreeDoubles(d4);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int incirclexy_indirect_IIII(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
   int ret;
   ret = incirclexy_indirect_IIII_interval(p1, p2, p3, p4);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return incirclexy_indirect_IIII_exact(p1, p2, p3, p4);
}

inline int incircle_indirect_IEEE_interval(const genericPoint& p1, interval_number pbx, interval_number pby, interval_number pcx, interval_number pcy, interval_number pdx, interval_number pdy)
{
   interval_number l1x, l1y, d1;
   if (
   !p1.getIntervalLambda(l1x, l1y, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pdxt(pdx * d1);
   const interval_number pdyt(pdy * d1);
   const interval_number adx(l1x - pdxt);
   const interval_number ady(l1y - pdyt);
   const interval_number bdx(pbx - pdx);
   const interval_number bdy(pby - pdy);
   const interval_number cdx(pcx - pdx);
   const interval_number cdy(pcy - pdy);
   const interval_number abdeta(adx * bdy);
   const interval_number abdetb(bdx * ady);
   const interval_number abdet(abdeta - abdetb);
   const interval_number bcdeta(bdx * cdy);
   const interval_number bcdetb(cdx * bdy);
   const interval_number bcdet(bcdeta - bcdetb);
   const interval_number cadeta(cdx * ady);
   const interval_number cadetb(adx * cdy);
   const interval_number cadet(cadeta - cadetb);
   const interval_number alifta(adx * adx);
   const interval_number aliftb(ady * ady);
   const interval_number alift(alifta + aliftb);
   const interval_number blifta(bdx * bdx);
   const interval_number bliftb(bdy * bdy);
   const interval_number blift(blifta + bliftb);
   const interval_number clifta(cdx * cdx);
   const interval_number cliftb(cdy * cdy);
   const interval_number clift(clifta + cliftb);
   const interval_number la(alift * bcdet);
   const interval_number lbt(blift * cadet);
   const interval_number lb(lbt * d1);
   const interval_number lct(clift * abdet);
   const interval_number lc(lct * d1);
   const interval_number lab(la + lb);
   const interval_number L(lab + lc);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int incircle_indirect_IEEE_bigfloat(const genericPoint& p1, bigfloat pbx, bigfloat pby, bigfloat pcx, bigfloat pcy, bigfloat pdx, bigfloat pdy)
{
   bigfloat l1x, l1y, d1;
   p1.getBigfloatLambda(l1x, l1y, d1);
   const bigfloat pdxt(pdx * d1);
   const bigfloat pdyt(pdy * d1);
   const bigfloat adx(l1x - pdxt);
   const bigfloat ady(l1y - pdyt);
   const bigfloat bdx(pbx - pdx);
   const bigfloat bdy(pby - pdy);
   const bigfloat cdx(pcx - pdx);
   const bigfloat cdy(pcy - pdy);
   const bigfloat abdeta(adx * bdy);
   const bigfloat abdetb(bdx * ady);
   const bigfloat abdet(abdeta - abdetb);
   const bigfloat bcdeta(bdx * cdy);
   const bigfloat bcdetb(cdx * bdy);
   const bigfloat bcdet(bcdeta - bcdetb);
   const bigfloat cadeta(cdx * ady);
   const bigfloat cadetb(adx * cdy);
   const bigfloat cadet(cadeta - cadetb);
   const bigfloat alifta(adx * adx);
   const bigfloat aliftb(ady * ady);
   const bigfloat alift(alifta + aliftb);
   const bigfloat blifta(bdx * bdx);
   const bigfloat bliftb(bdy * bdy);
   const bigfloat blift(blifta + bliftb);
   const bigfloat clifta(cdx * cdx);
   const bigfloat cliftb(cdy * cdy);
   const bigfloat clift(clifta + cliftb);
   const bigfloat la(alift * bcdet);
   const bigfloat lbt(blift * cadet);
   const bigfloat lb(lbt * d1);
   const bigfloat lct(clift * abdet);
   const bigfloat lc(lct * d1);
   const bigfloat lab(la + lb);
   const bigfloat L(lab + lc);
   return sgn(L);
}

inline int incircle_indirect_IEEE_exact(const genericPoint& p1, double pbx, double pby, double pcx, double pcy, double pdx, double pdy)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[64], *l1x = l1x_p, l1y_p[64], *l1y = l1y_p, d1_p[64], *d1 = d1_p;
 int l1x_len = 64, l1y_len = 64, d1_len = 64;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double pdxt_p[64], *pdxt = pdxt_p;
   int pdxt_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdx, &pdxt, 64);
   double pdyt_p[64], *pdyt = pdyt_p;
   int pdyt_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdy, &pdyt, 64);
   double adx_p[64], *adx = adx_p;
   int adx_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pdxt_len, pdxt, &adx, 64);
   double ady_p[64], *ady = ady_p;
   int ady_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, pdyt_len, pdyt, &ady, 64);
   double bdx[2];
   expansionObject::two_Diff(pbx, pdx, bdx);
   double bdy[2];
   expansionObject::two_Diff(pby, pdy, bdy);
   double cdx[2];
   expansionObject::two_Diff(pcx, pdx, cdx);
   double cdy[2];
   expansionObject::two_Diff(pcy, pdy, cdy);
   double abdeta_p[64], *abdeta = abdeta_p;
   int abdeta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, 2, bdy, &abdeta, 64);
   double abdetb_p[64], *abdetb = abdetb_p;
   int abdetb_len = expansionObject::Gen_Product_With_PreAlloc(2, bdx, ady_len, ady, &abdetb, 64);
   double abdet_p[64], *abdet = abdet_p;
   int abdet_len = expansionObject::Gen_Diff_With_PreAlloc(abdeta_len, abdeta, abdetb_len, abdetb, &abdet, 64);
   double bcdeta[8];
   int bcdeta_len = expansionObject::Gen_Product(2, bdx, 2, cdy, bcdeta);
   double bcdetb[8];
   int bcdetb_len = expansionObject::Gen_Product(2, cdx, 2, bdy, bcdetb);
   double bcdet[16];
   int bcdet_len = expansionObject::Gen_Diff(bcdeta_len, bcdeta, bcdetb_len, bcdetb, bcdet);
   double cadeta_p[64], *cadeta = cadeta_p;
   int cadeta_len = expansionObject::Gen_Product_With_PreAlloc(2, cdx, ady_len, ady, &cadeta, 64);
   double cadetb_p[64], *cadetb = cadetb_p;
   int cadetb_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, 2, cdy, &cadetb, 64);
   double cadet_p[64], *cadet = cadet_p;
   int cadet_len = expansionObject::Gen_Diff_With_PreAlloc(cadeta_len, cadeta, cadetb_len, cadetb, &cadet, 64);
   double alifta_p[64], *alifta = alifta_p;
   int alifta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, adx_len, adx, &alifta, 64);
   double aliftb_p[64], *aliftb = aliftb_p;
   int aliftb_len = expansionObject::Gen_Product_With_PreAlloc(ady_len, ady, ady_len, ady, &aliftb, 64);
   double alift_p[64], *alift = alift_p;
   int alift_len = expansionObject::Gen_Sum_With_PreAlloc(alifta_len, alifta, aliftb_len, aliftb, &alift, 64);
   double blifta[8];
   int blifta_len = expansionObject::Gen_Product(2, bdx, 2, bdx, blifta);
   double bliftb[8];
   int bliftb_len = expansionObject::Gen_Product(2, bdy, 2, bdy, bliftb);
   double blift[16];
   int blift_len = expansionObject::Gen_Sum(blifta_len, blifta, bliftb_len, bliftb, blift);
   double clifta[8];
   int clifta_len = expansionObject::Gen_Product(2, cdx, 2, cdx, clifta);
   double cliftb[8];
   int cliftb_len = expansionObject::Gen_Product(2, cdy, 2, cdy, cliftb);
   double clift[16];
   int clift_len = expansionObject::Gen_Sum(clifta_len, clifta, cliftb_len, cliftb, clift);
   double la_p[64], *la = la_p;
   int la_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcdet_len, bcdet, &la, 64);
   double lbt_p[64], *lbt = lbt_p;
   int lbt_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cadet_len, cadet, &lbt, 64);
   double lb_p[64], *lb = lb_p;
   int lb_len = expansionObject::Gen_Product_With_PreAlloc(lbt_len, lbt, d1_len, d1, &lb, 64);
   double lct_p[64], *lct = lct_p;
   int lct_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, abdet_len, abdet, &lct, 64);
   double lc_p[64], *lc = lc_p;
   int lc_len = expansionObject::Gen_Product_With_PreAlloc(lct_len, lct, d1_len, d1, &lc, 64);
   double lab_p[64], *lab = lab_p;
   int lab_len = expansionObject::Gen_Sum_With_PreAlloc(la_len, la, lb_len, lb, &lab, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Sum_With_PreAlloc(lab_len, lab, lc_len, lc, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (lab_p != lab) FreeDoubles(lab);
   if (lc_p != lc) FreeDoubles(lc);
   if (lct_p != lct) FreeDoubles(lct);
   if (lb_p != lb) FreeDoubles(lb);
   if (lbt_p != lbt) FreeDoubles(lbt);
   if (la_p != la) FreeDoubles(la);
   if (alift_p != alift) FreeDoubles(alift);
   if (aliftb_p != aliftb) FreeDoubles(aliftb);
   if (alifta_p != alifta) FreeDoubles(alifta);
   if (cadet_p != cadet) FreeDoubles(cadet);
   if (cadetb_p != cadetb) FreeDoubles(cadetb);
   if (cadeta_p != cadeta) FreeDoubles(cadeta);
   if (abdet_p != abdet) FreeDoubles(abdet);
   if (abdetb_p != abdetb) FreeDoubles(abdetb);
   if (abdeta_p != abdeta) FreeDoubles(abdeta);
   if (ady_p != ady) FreeDoubles(ady);
   if (adx_p != adx) FreeDoubles(adx);
   if (pdyt_p != pdyt) FreeDoubles(pdyt);
   if (pdxt_p != pdxt) FreeDoubles(pdxt);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = incircle_indirect_IEEE_bigfloat(p1, pbx, pby, pcx, pcy, pdx, pdy);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (d1_p != d1) FreeDoubles(d1);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int incircle_indirect_IEEE(const genericPoint& p1, double pbx, double pby, double pcx, double pcy, double pdx, double pdy)
{
   int ret;
   ret = incircle_indirect_IEEE_interval(p1, pbx, pby, pcx, pcy, pdx, pdy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return incircle_indirect_IEEE_exact(p1, pbx, pby, pcx, pcy, pdx, pdy);
}

inline int incircle_indirect_IIEE_interval(const genericPoint& p1, const genericPoint& p2, interval_number pcx, interval_number pcy, interval_number pdx, interval_number pdy)
{
   interval_number l1x, l1y, d1, l2x, l2y, d2;
   if (
   !p1.getIntervalLambda(l1x, l1y, d1)
   || !p2.getIntervalLambda(l2x, l2y, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pdx1(pdx * d1);
   const interval_number pdy1(pdy * d1);
   const interval_number adx(l1x - pdx1);
   const interval_number ady(l1y - pdy1);
   const interval_number pdx2(pdx * d2);
   const interval_number pdy2(pdy * d2);
   const interval_number bdx(l2x - pdx2);
   const interval_number bdy(l2y - pdy2);
   const interval_number cdx(pcx - pdx);
   const interval_number cdy(pcy - pdy);
   const interval_number abdeta(adx * bdy);
   const interval_number abdetb(bdx * ady);
   const interval_number abdet(abdeta - abdetb);
   const interval_number bcdeta(bdx * cdy);
   const interval_number bcdetb(cdx * bdy);
   const interval_number bcdet(bcdeta - bcdetb);
   const interval_number cadeta(cdx * ady);
   const interval_number cadetb(adx * cdy);
   const interval_number cadet(cadeta - cadetb);
   const interval_number alifta(adx * adx);
   const interval_number aliftb(ady * ady);
   const interval_number aliftt(alifta + aliftb);
   const interval_number alift(aliftt * d2);
   const interval_number blifta(bdx * bdx);
   const interval_number bliftb(bdy * bdy);
   const interval_number blift(blifta + bliftb);
   const interval_number clifta(cdx * cdx);
   const interval_number cliftb(cdy * cdy);
   const interval_number cliftt(clifta + cliftb);
   const interval_number clift(cliftt * d2);
   const interval_number la(alift * bcdet);
   const interval_number lb(blift * cadet);
   const interval_number lc(clift * abdet);
   const interval_number lab(lc + lb);
   const interval_number lab2(lab * d1);
   const interval_number L(lab2 + la);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int incircle_indirect_IIEE_bigfloat(const genericPoint& p1, const genericPoint& p2, bigfloat pcx, bigfloat pcy, bigfloat pdx, bigfloat pdy)
{
   bigfloat l1x, l1y, d1, l2x, l2y, d2;
   p1.getBigfloatLambda(l1x, l1y, d1);
   p2.getBigfloatLambda(l2x, l2y, d2);
   const bigfloat pdx1(pdx * d1);
   const bigfloat pdy1(pdy * d1);
   const bigfloat adx(l1x - pdx1);
   const bigfloat ady(l1y - pdy1);
   const bigfloat pdx2(pdx * d2);
   const bigfloat pdy2(pdy * d2);
   const bigfloat bdx(l2x - pdx2);
   const bigfloat bdy(l2y - pdy2);
   const bigfloat cdx(pcx - pdx);
   const bigfloat cdy(pcy - pdy);
   const bigfloat abdeta(adx * bdy);
   const bigfloat abdetb(bdx * ady);
   const bigfloat abdet(abdeta - abdetb);
   const bigfloat bcdeta(bdx * cdy);
   const bigfloat bcdetb(cdx * bdy);
   const bigfloat bcdet(bcdeta - bcdetb);
   const bigfloat cadeta(cdx * ady);
   const bigfloat cadetb(adx * cdy);
   const bigfloat cadet(cadeta - cadetb);
   const bigfloat alifta(adx * adx);
   const bigfloat aliftb(ady * ady);
   const bigfloat aliftt(alifta + aliftb);
   const bigfloat alift(aliftt * d2);
   const bigfloat blifta(bdx * bdx);
   const bigfloat bliftb(bdy * bdy);
   const bigfloat blift(blifta + bliftb);
   const bigfloat clifta(cdx * cdx);
   const bigfloat cliftb(cdy * cdy);
   const bigfloat cliftt(clifta + cliftb);
   const bigfloat clift(cliftt * d2);
   const bigfloat la(alift * bcdet);
   const bigfloat lb(blift * cadet);
   const bigfloat lc(clift * abdet);
   const bigfloat lab(lc + lb);
   const bigfloat lab2(lab * d1);
   const bigfloat L(lab2 + la);
   return sgn(L);
}

inline int incircle_indirect_IIEE_exact(const genericPoint& p1, const genericPoint& p2, double pcx, double pcy, double pdx, double pdy)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[32], *l1x = l1x_p, l1y_p[32], *l1y = l1y_p, d1_p[32], *d1 = d1_p, l2x_p[32], *l2x = l2x_p, l2y_p[32], *l2y = l2y_p, d2_p[32], *d2 = d2_p;
 int l1x_len = 32, l1y_len = 32, d1_len = 32, l2x_len = 32, l2y_len = 32, d2_len = 32;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double pdx1_p[32], *pdx1 = pdx1_p;
   int pdx1_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdx, &pdx1, 32);
   double pdy1_p[32], *pdy1 = pdy1_p;
   int pdy1_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdy, &pdy1, 32);
   double adx_p[32], *adx = adx_p;
   int adx_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pdx1_len, pdx1, &adx, 32);
   double ady_p[32], *ady = ady_p;
   int ady_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, pdy1_len, pdy1, &ady, 32);
   double pdx2_p[32], *pdx2 = pdx2_p;
   int pdx2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pdx, &pdx2, 32);
   double pdy2_p[32], *pdy2 = pdy2_p;
   int pdy2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pdy, &pdy2, 32);
   double bdx_p[32], *bdx = bdx_p;
   int bdx_len = expansionObject::Gen_Diff_With_PreAlloc(l2x_len, l2x, pdx2_len, pdx2, &bdx, 32);
   double bdy_p[32], *bdy = bdy_p;
   int bdy_len = expansionObject::Gen_Diff_With_PreAlloc(l2y_len, l2y, pdy2_len, pdy2, &bdy, 32);
   double cdx[2];
   expansionObject::two_Diff(pcx, pdx, cdx);
   double cdy[2];
   expansionObject::two_Diff(pcy, pdy, cdy);
   double abdeta_p[32], *abdeta = abdeta_p;
   int abdeta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, bdy_len, bdy, &abdeta, 32);
   double abdetb_p[32], *abdetb = abdetb_p;
   int abdetb_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, ady_len, ady, &abdetb, 32);
   double abdet_p[32], *abdet = abdet_p;
   int abdet_len = expansionObject::Gen_Diff_With_PreAlloc(abdeta_len, abdeta, abdetb_len, abdetb, &abdet, 32);
   double bcdeta_p[32], *bcdeta = bcdeta_p;
   int bcdeta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, 2, cdy, &bcdeta, 32);
   double bcdetb_p[32], *bcdetb = bcdetb_p;
   int bcdetb_len = expansionObject::Gen_Product_With_PreAlloc(2, cdx, bdy_len, bdy, &bcdetb, 32);
   double bcdet_p[32], *bcdet = bcdet_p;
   int bcdet_len = expansionObject::Gen_Diff_With_PreAlloc(bcdeta_len, bcdeta, bcdetb_len, bcdetb, &bcdet, 32);
   double cadeta_p[32], *cadeta = cadeta_p;
   int cadeta_len = expansionObject::Gen_Product_With_PreAlloc(2, cdx, ady_len, ady, &cadeta, 32);
   double cadetb_p[32], *cadetb = cadetb_p;
   int cadetb_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, 2, cdy, &cadetb, 32);
   double cadet_p[32], *cadet = cadet_p;
   int cadet_len = expansionObject::Gen_Diff_With_PreAlloc(cadeta_len, cadeta, cadetb_len, cadetb, &cadet, 32);
   double alifta_p[32], *alifta = alifta_p;
   int alifta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, adx_len, adx, &alifta, 32);
   double aliftb_p[32], *aliftb = aliftb_p;
   int aliftb_len = expansionObject::Gen_Product_With_PreAlloc(ady_len, ady, ady_len, ady, &aliftb, 32);
   double aliftt_p[32], *aliftt = aliftt_p;
   int aliftt_len = expansionObject::Gen_Sum_With_PreAlloc(alifta_len, alifta, aliftb_len, aliftb, &aliftt, 32);
   double alift_p[32], *alift = alift_p;
   int alift_len = expansionObject::Gen_Product_With_PreAlloc(aliftt_len, aliftt, d2_len, d2, &alift, 32);
   double blifta_p[32], *blifta = blifta_p;
   int blifta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, bdx_len, bdx, &blifta, 32);
   double bliftb_p[32], *bliftb = bliftb_p;
   int bliftb_len = expansionObject::Gen_Product_With_PreAlloc(bdy_len, bdy, bdy_len, bdy, &bliftb, 32);
   double blift_p[32], *blift = blift_p;
   int blift_len = expansionObject::Gen_Sum_With_PreAlloc(blifta_len, blifta, bliftb_len, bliftb, &blift, 32);
   double clifta[8];
   int clifta_len = expansionObject::Gen_Product(2, cdx, 2, cdx, clifta);
   double cliftb[8];
   int cliftb_len = expansionObject::Gen_Product(2, cdy, 2, cdy, cliftb);
   double cliftt[16];
   int cliftt_len = expansionObject::Gen_Sum(clifta_len, clifta, cliftb_len, cliftb, cliftt);
   double clift_p[32], *clift = clift_p;
   int clift_len = expansionObject::Gen_Product_With_PreAlloc(cliftt_len, cliftt, d2_len, d2, &clift, 32);
   double la_p[32], *la = la_p;
   int la_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcdet_len, bcdet, &la, 32);
   double lb_p[32], *lb = lb_p;
   int lb_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cadet_len, cadet, &lb, 32);
   double lc_p[32], *lc = lc_p;
   int lc_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, abdet_len, abdet, &lc, 32);
   double lab_p[32], *lab = lab_p;
   int lab_len = expansionObject::Gen_Sum_With_PreAlloc(lc_len, lc, lb_len, lb, &lab, 32);
   double lab2_p[32], *lab2 = lab2_p;
   int lab2_len = expansionObject::Gen_Product_With_PreAlloc(lab_len, lab, d1_len, d1, &lab2, 32);
   double L_p[32], *L = L_p;
   int L_len = expansionObject::Gen_Sum_With_PreAlloc(lab2_len, lab2, la_len, la, &L, 32);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (lab2_p != lab2) FreeDoubles(lab2);
   if (lab_p != lab) FreeDoubles(lab);
   if (lc_p != lc) FreeDoubles(lc);
   if (lb_p != lb) FreeDoubles(lb);
   if (la_p != la) FreeDoubles(la);
   if (clift_p != clift) FreeDoubles(clift);
   if (blift_p != blift) FreeDoubles(blift);
   if (bliftb_p != bliftb) FreeDoubles(bliftb);
   if (blifta_p != blifta) FreeDoubles(blifta);
   if (alift_p != alift) FreeDoubles(alift);
   if (aliftt_p != aliftt) FreeDoubles(aliftt);
   if (aliftb_p != aliftb) FreeDoubles(aliftb);
   if (alifta_p != alifta) FreeDoubles(alifta);
   if (cadet_p != cadet) FreeDoubles(cadet);
   if (cadetb_p != cadetb) FreeDoubles(cadetb);
   if (cadeta_p != cadeta) FreeDoubles(cadeta);
   if (bcdet_p != bcdet) FreeDoubles(bcdet);
   if (bcdetb_p != bcdetb) FreeDoubles(bcdetb);
   if (bcdeta_p != bcdeta) FreeDoubles(bcdeta);
   if (abdet_p != abdet) FreeDoubles(abdet);
   if (abdetb_p != abdetb) FreeDoubles(abdetb);
   if (abdeta_p != abdeta) FreeDoubles(abdeta);
   if (bdy_p != bdy) FreeDoubles(bdy);
   if (bdx_p != bdx) FreeDoubles(bdx);
   if (pdy2_p != pdy2) FreeDoubles(pdy2);
   if (pdx2_p != pdx2) FreeDoubles(pdx2);
   if (ady_p != ady) FreeDoubles(ady);
   if (adx_p != adx) FreeDoubles(adx);
   if (pdy1_p != pdy1) FreeDoubles(pdy1);
   if (pdx1_p != pdx1) FreeDoubles(pdx1);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = incircle_indirect_IIEE_bigfloat(p1, p2, pcx, pcy, pdx, pdy);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (d2_p != d2) FreeDoubles(d2);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int incircle_indirect_IIEE(const genericPoint& p1, const genericPoint& p2, double pcx, double pcy, double pdx, double pdy)
{
   int ret;
   ret = incircle_indirect_IIEE_interval(p1, p2, pcx, pcy, pdx, pdy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return incircle_indirect_IIEE_exact(p1, p2, pcx, pcy, pdx, pdy);
}

inline int incircle_indirect_IIIE_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, interval_number pdx, interval_number pdy)
{
   interval_number l1x, l1y, d1, l2x, l2y, d2, l3x, l3y, d3;
   if (
   !p1.getIntervalLambda(l1x, l1y, d1)
   || !p2.getIntervalLambda(l2x, l2y, d2)
   || !p3.getIntervalLambda(l3x, l3y, d3)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pdx1(pdx * d1);
   const interval_number pdy1(pdy * d1);
   const interval_number adx(l1x - pdx1);
   const interval_number ady(l1y - pdy1);
   const interval_number pdx2(pdx * d2);
   const interval_number pdy2(pdy * d2);
   const interval_number bdx(l2x - pdx2);
   const interval_number bdy(l2y - pdy2);
   const interval_number pdx3(pdx * d3);
   const interval_number pdy3(pdy * d3);
   const interval_number cdx(l3x - pdx3);
   const interval_number cdy(l3y - pdy3);
   const interval_number abdeta(adx * bdy);
   const interval_number abdetb(bdx * ady);
   const interval_number abdet(abdeta - abdetb);
   const interval_number bcdeta(bdx * cdy);
   const interval_number bcdetb(cdx * bdy);
   const interval_number bcdet(bcdeta - bcdetb);
   const interval_number cadeta(cdx * ady);
   const interval_number cadetb(adx * cdy);
   const interval_number cadet(cadeta - cadetb);
   const interval_number alifta(adx * adx);
   const interval_number aliftb(ady * ady);
   const interval_number aliftt(alifta + aliftb);
   const interval_number alift2(aliftt * d2);
   const interval_number alift(alift2 * d3);
   const interval_number blifta(bdx * bdx);
   const interval_number bliftb(bdy * bdy);
   const interval_number bliftt(blifta + bliftb);
   const interval_number blift(bliftt * d3);
   const interval_number clifta(cdx * cdx);
   const interval_number cliftb(cdy * cdy);
   const interval_number cliftt(clifta + cliftb);
   const interval_number clift(cliftt * d2);
   const interval_number la(alift * bcdet);
   const interval_number lb(blift * cadet);
   const interval_number lc(clift * abdet);
   const interval_number lab2(lc + lb);
   const interval_number lab(lab2 * d1);
   const interval_number L(lab + la);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int incircle_indirect_IIIE_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, bigfloat pdx, bigfloat pdy)
{
   bigfloat l1x, l1y, d1, l2x, l2y, d2, l3x, l3y, d3;
   p1.getBigfloatLambda(l1x, l1y, d1);
   p2.getBigfloatLambda(l2x, l2y, d2);
   p3.getBigfloatLambda(l3x, l3y, d3);
   const bigfloat pdx1(pdx * d1);
   const bigfloat pdy1(pdy * d1);
   const bigfloat adx(l1x - pdx1);
   const bigfloat ady(l1y - pdy1);
   const bigfloat pdx2(pdx * d2);
   const bigfloat pdy2(pdy * d2);
   const bigfloat bdx(l2x - pdx2);
   const bigfloat bdy(l2y - pdy2);
   const bigfloat pdx3(pdx * d3);
   const bigfloat pdy3(pdy * d3);
   const bigfloat cdx(l3x - pdx3);
   const bigfloat cdy(l3y - pdy3);
   const bigfloat abdeta(adx * bdy);
   const bigfloat abdetb(bdx * ady);
   const bigfloat abdet(abdeta - abdetb);
   const bigfloat bcdeta(bdx * cdy);
   const bigfloat bcdetb(cdx * bdy);
   const bigfloat bcdet(bcdeta - bcdetb);
   const bigfloat cadeta(cdx * ady);
   const bigfloat cadetb(adx * cdy);
   const bigfloat cadet(cadeta - cadetb);
   const bigfloat alifta(adx * adx);
   const bigfloat aliftb(ady * ady);
   const bigfloat aliftt(alifta + aliftb);
   const bigfloat alift2(aliftt * d2);
   const bigfloat alift(alift2 * d3);
   const bigfloat blifta(bdx * bdx);
   const bigfloat bliftb(bdy * bdy);
   const bigfloat bliftt(blifta + bliftb);
   const bigfloat blift(bliftt * d3);
   const bigfloat clifta(cdx * cdx);
   const bigfloat cliftb(cdy * cdy);
   const bigfloat cliftt(clifta + cliftb);
   const bigfloat clift(cliftt * d2);
   const bigfloat la(alift * bcdet);
   const bigfloat lb(blift * cadet);
   const bigfloat lc(clift * abdet);
   const bigfloat lab2(lc + lb);
   const bigfloat lab(lab2 * d1);
   const bigfloat L(lab + la);
   return sgn(L);
}

inline int incircle_indirect_IIIE_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double pdx, double pdy)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[32], *l1x = l1x_p, l1y_p[32], *l1y = l1y_p, d1_p[32], *d1 = d1_p, l2x_p[32], *l2x = l2x_p, l2y_p[32], *l2y = l2y_p, d2_p[32], *d2 = d2_p, l3x_p[32], *l3x = l3x_p, l3y_p[32], *l3y = l3y_p, d3_p[32], *d3 = d3_p;
 int l1x_len = 32, l1y_len = 32, d1_len = 32, l2x_len = 32, l2y_len = 32, d2_len = 32, l3x_len = 32, l3y_len = 32, d3_len = 32;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &d3, d3_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0))
 {
   
   double pdx1_p[32], *pdx1 = pdx1_p;
   int pdx1_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdx, &pdx1, 32);
   double pdy1_p[32], *pdy1 = pdy1_p;
   int pdy1_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pdy, &pdy1, 32);
   double adx_p[32], *adx = adx_p;
   int adx_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pdx1_len, pdx1, &adx, 32);
   double ady_p[32], *ady = ady_p;
   int ady_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, pdy1_len, pdy1, &ady, 32);
   double pdx2_p[32], *pdx2 = pdx2_p;
   int pdx2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pdx, &pdx2, 32);
   double pdy2_p[32], *pdy2 = pdy2_p;
   int pdy2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pdy, &pdy2, 32);
   double bdx_p[32], *bdx = bdx_p;
   int bdx_len = expansionObject::Gen_Diff_With_PreAlloc(l2x_len, l2x, pdx2_len, pdx2, &bdx, 32);
   double bdy_p[32], *bdy = bdy_p;
   int bdy_len = expansionObject::Gen_Diff_With_PreAlloc(l2y_len, l2y, pdy2_len, pdy2, &bdy, 32);
   double pdx3_p[32], *pdx3 = pdx3_p;
   int pdx3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pdx, &pdx3, 32);
   double pdy3_p[32], *pdy3 = pdy3_p;
   int pdy3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pdy, &pdy3, 32);
   double cdx_p[32], *cdx = cdx_p;
   int cdx_len = expansionObject::Gen_Diff_With_PreAlloc(l3x_len, l3x, pdx3_len, pdx3, &cdx, 32);
   double cdy_p[32], *cdy = cdy_p;
   int cdy_len = expansionObject::Gen_Diff_With_PreAlloc(l3y_len, l3y, pdy3_len, pdy3, &cdy, 32);
   double abdeta_p[32], *abdeta = abdeta_p;
   int abdeta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, bdy_len, bdy, &abdeta, 32);
   double abdetb_p[32], *abdetb = abdetb_p;
   int abdetb_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, ady_len, ady, &abdetb, 32);
   double abdet_p[32], *abdet = abdet_p;
   int abdet_len = expansionObject::Gen_Diff_With_PreAlloc(abdeta_len, abdeta, abdetb_len, abdetb, &abdet, 32);
   double bcdeta_p[32], *bcdeta = bcdeta_p;
   int bcdeta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, cdy_len, cdy, &bcdeta, 32);
   double bcdetb_p[32], *bcdetb = bcdetb_p;
   int bcdetb_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, bdy_len, bdy, &bcdetb, 32);
   double bcdet_p[32], *bcdet = bcdet_p;
   int bcdet_len = expansionObject::Gen_Diff_With_PreAlloc(bcdeta_len, bcdeta, bcdetb_len, bcdetb, &bcdet, 32);
   double cadeta_p[32], *cadeta = cadeta_p;
   int cadeta_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, ady_len, ady, &cadeta, 32);
   double cadetb_p[32], *cadetb = cadetb_p;
   int cadetb_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, cdy_len, cdy, &cadetb, 32);
   double cadet_p[32], *cadet = cadet_p;
   int cadet_len = expansionObject::Gen_Diff_With_PreAlloc(cadeta_len, cadeta, cadetb_len, cadetb, &cadet, 32);
   double alifta_p[32], *alifta = alifta_p;
   int alifta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, adx_len, adx, &alifta, 32);
   double aliftb_p[32], *aliftb = aliftb_p;
   int aliftb_len = expansionObject::Gen_Product_With_PreAlloc(ady_len, ady, ady_len, ady, &aliftb, 32);
   double aliftt_p[32], *aliftt = aliftt_p;
   int aliftt_len = expansionObject::Gen_Sum_With_PreAlloc(alifta_len, alifta, aliftb_len, aliftb, &aliftt, 32);
   double alift2_p[32], *alift2 = alift2_p;
   int alift2_len = expansionObject::Gen_Product_With_PreAlloc(aliftt_len, aliftt, d2_len, d2, &alift2, 32);
   double alift_p[32], *alift = alift_p;
   int alift_len = expansionObject::Gen_Product_With_PreAlloc(alift2_len, alift2, d3_len, d3, &alift, 32);
   double blifta_p[32], *blifta = blifta_p;
   int blifta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, bdx_len, bdx, &blifta, 32);
   double bliftb_p[32], *bliftb = bliftb_p;
   int bliftb_len = expansionObject::Gen_Product_With_PreAlloc(bdy_len, bdy, bdy_len, bdy, &bliftb, 32);
   double bliftt_p[32], *bliftt = bliftt_p;
   int bliftt_len = expansionObject::Gen_Sum_With_PreAlloc(blifta_len, blifta, bliftb_len, bliftb, &bliftt, 32);
   double blift_p[32], *blift = blift_p;
   int blift_len = expansionObject::Gen_Product_With_PreAlloc(bliftt_len, bliftt, d3_len, d3, &blift, 32);
   double clifta_p[32], *clifta = clifta_p;
   int clifta_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, cdx_len, cdx, &clifta, 32);
   double cliftb_p[32], *cliftb = cliftb_p;
   int cliftb_len = expansionObject::Gen_Product_With_PreAlloc(cdy_len, cdy, cdy_len, cdy, &cliftb, 32);
   double cliftt_p[32], *cliftt = cliftt_p;
   int cliftt_len = expansionObject::Gen_Sum_With_PreAlloc(clifta_len, clifta, cliftb_len, cliftb, &cliftt, 32);
   double clift_p[32], *clift = clift_p;
   int clift_len = expansionObject::Gen_Product_With_PreAlloc(cliftt_len, cliftt, d2_len, d2, &clift, 32);
   double la_p[32], *la = la_p;
   int la_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcdet_len, bcdet, &la, 32);
   double lb_p[32], *lb = lb_p;
   int lb_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cadet_len, cadet, &lb, 32);
   double lc_p[32], *lc = lc_p;
   int lc_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, abdet_len, abdet, &lc, 32);
   double lab2_p[32], *lab2 = lab2_p;
   int lab2_len = expansionObject::Gen_Sum_With_PreAlloc(lc_len, lc, lb_len, lb, &lab2, 32);
   double lab_p[32], *lab = lab_p;
   int lab_len = expansionObject::Gen_Product_With_PreAlloc(lab2_len, lab2, d1_len, d1, &lab, 32);
   double L_p[32], *L = L_p;
   int L_len = expansionObject::Gen_Sum_With_PreAlloc(lab_len, lab, la_len, la, &L, 32);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (lab_p != lab) FreeDoubles(lab);
   if (lab2_p != lab2) FreeDoubles(lab2);
   if (lc_p != lc) FreeDoubles(lc);
   if (lb_p != lb) FreeDoubles(lb);
   if (la_p != la) FreeDoubles(la);
   if (clift_p != clift) FreeDoubles(clift);
   if (cliftt_p != cliftt) FreeDoubles(cliftt);
   if (cliftb_p != cliftb) FreeDoubles(cliftb);
   if (clifta_p != clifta) FreeDoubles(clifta);
   if (blift_p != blift) FreeDoubles(blift);
   if (bliftt_p != bliftt) FreeDoubles(bliftt);
   if (bliftb_p != bliftb) FreeDoubles(bliftb);
   if (blifta_p != blifta) FreeDoubles(blifta);
   if (alift_p != alift) FreeDoubles(alift);
   if (alift2_p != alift2) FreeDoubles(alift2);
   if (aliftt_p != aliftt) FreeDoubles(aliftt);
   if (aliftb_p != aliftb) FreeDoubles(aliftb);
   if (alifta_p != alifta) FreeDoubles(alifta);
   if (cadet_p != cadet) FreeDoubles(cadet);
   if (cadetb_p != cadetb) FreeDoubles(cadetb);
   if (cadeta_p != cadeta) FreeDoubles(cadeta);
   if (bcdet_p != bcdet) FreeDoubles(bcdet);
   if (bcdetb_p != bcdetb) FreeDoubles(bcdetb);
   if (bcdeta_p != bcdeta) FreeDoubles(bcdeta);
   if (abdet_p != abdet) FreeDoubles(abdet);
   if (abdetb_p != abdetb) FreeDoubles(abdetb);
   if (abdeta_p != abdeta) FreeDoubles(abdeta);
   if (cdy_p != cdy) FreeDoubles(cdy);
   if (cdx_p != cdx) FreeDoubles(cdx);
   if (pdy3_p != pdy3) FreeDoubles(pdy3);
   if (pdx3_p != pdx3) FreeDoubles(pdx3);
   if (bdy_p != bdy) FreeDoubles(bdy);
   if (bdx_p != bdx) FreeDoubles(bdx);
   if (pdy2_p != pdy2) FreeDoubles(pdy2);
   if (pdx2_p != pdx2) FreeDoubles(pdx2);
   if (ady_p != ady) FreeDoubles(ady);
   if (adx_p != adx) FreeDoubles(adx);
   if (pdy1_p != pdy1) FreeDoubles(pdy1);
   if (pdx1_p != pdx1) FreeDoubles(pdx1);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = incircle_indirect_IIIE_bigfloat(p1, p2, p3, pdx, pdy);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (d3_p != d3) FreeDoubles(d3);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int incircle_indirect_IIIE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double pdx, double pdy)
{
   int ret;
   ret = incircle_indirect_IIIE_interval(p1, p2, p3, pdx, pdy);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return incircle_indirect_IIIE_exact(p1, p2, p3, pdx, pdy);
}

inline int incircle_indirect_IIII_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
   interval_number l1x, l1y, d1, l2x, l2y, d2, l3x, l3y, d3, l4x, l4y, d4;
   if (
   !p1.getIntervalLambda(l1x, l1y, d1)
   || !p2.getIntervalLambda(l2x, l2y, d2)
   || !p3.getIntervalLambda(l3x, l3y, d3)
   || !p4.getIntervalLambda(l4x, l4y, d4)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number l1xt(l1x * d4);
   const interval_number l1yt(l1y * d4);
   const interval_number l2xt(l2x * d4);
   const interval_number l2yt(l2y * d4);
   const interval_number l3xt(l3x * d4);
   const interval_number l3yt(l3y * d4);
   const interval_number l4x1(l4x * d1);
   const interval_number l4y1(l4y * d1);
   const interval_number adx(l1xt - l4x1);
   const interval_number ady(l1yt - l4y1);
   const interval_number l4x2(l4x * d2);
   const interval_number l4y2(l4y * d2);
   const interval_number bdx(l2xt - l4x2);
   const interval_number bdy(l2yt - l4y2);
   const interval_number l4x3(l4x * d3);
   const interval_number l4y3(l4y * d3);
   const interval_number cdx(l3xt - l4x3);
   const interval_number cdy(l3yt - l4y3);
   const interval_number abdeta(adx * bdy);
   const interval_number abdetb(bdx * ady);
   const interval_number abdet(abdeta - abdetb);
   const interval_number bcdeta(bdx * cdy);
   const interval_number bcdetb(cdx * bdy);
   const interval_number bcdet(bcdeta - bcdetb);
   const interval_number cadeta(cdx * ady);
   const interval_number cadetb(adx * cdy);
   const interval_number cadet(cadeta - cadetb);
   const interval_number alifta(adx * adx);
   const interval_number aliftb(ady * ady);
   const interval_number aliftt(alifta + aliftb);
   const interval_number alift2(aliftt * d2);
   const interval_number alift(alift2 * d3);
   const interval_number blifta(bdx * bdx);
   const interval_number bliftb(bdy * bdy);
   const interval_number bliftt(blifta + bliftb);
   const interval_number blift(bliftt * d3);
   const interval_number clifta(cdx * cdx);
   const interval_number cliftb(cdy * cdy);
   const interval_number cliftt(clifta + cliftb);
   const interval_number clift(cliftt * d2);
   const interval_number la(alift * bcdet);
   const interval_number lb(blift * cadet);
   const interval_number lc(clift * abdet);
   const interval_number lab2(lc + lb);
   const interval_number lab(lab2 * d1);
   const interval_number L(lab + la);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int incircle_indirect_IIII_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
   bigfloat l1x, l1y, d1, l2x, l2y, d2, l3x, l3y, d3, l4x, l4y, d4;
   p1.getBigfloatLambda(l1x, l1y, d1);
   p2.getBigfloatLambda(l2x, l2y, d2);
   p3.getBigfloatLambda(l3x, l3y, d3);
   p4.getBigfloatLambda(l4x, l4y, d4);
   const bigfloat l1xt(l1x * d4);
   const bigfloat l1yt(l1y * d4);
   const bigfloat l2xt(l2x * d4);
   const bigfloat l2yt(l2y * d4);
   const bigfloat l3xt(l3x * d4);
   const bigfloat l3yt(l3y * d4);
   const bigfloat l4x1(l4x * d1);
   const bigfloat l4y1(l4y * d1);
   const bigfloat adx(l1xt - l4x1);
   const bigfloat ady(l1yt - l4y1);
   const bigfloat l4x2(l4x * d2);
   const bigfloat l4y2(l4y * d2);
   const bigfloat bdx(l2xt - l4x2);
   const bigfloat bdy(l2yt - l4y2);
   const bigfloat l4x3(l4x * d3);
   const bigfloat l4y3(l4y * d3);
   const bigfloat cdx(l3xt - l4x3);
   const bigfloat cdy(l3yt - l4y3);
   const bigfloat abdeta(adx * bdy);
   const bigfloat abdetb(bdx * ady);
   const bigfloat abdet(abdeta - abdetb);
   const bigfloat bcdeta(bdx * cdy);
   const bigfloat bcdetb(cdx * bdy);
   const bigfloat bcdet(bcdeta - bcdetb);
   const bigfloat cadeta(cdx * ady);
   const bigfloat cadetb(adx * cdy);
   const bigfloat cadet(cadeta - cadetb);
   const bigfloat alifta(adx * adx);
   const bigfloat aliftb(ady * ady);
   const bigfloat aliftt(alifta + aliftb);
   const bigfloat alift2(aliftt * d2);
   const bigfloat alift(alift2 * d3);
   const bigfloat blifta(bdx * bdx);
   const bigfloat bliftb(bdy * bdy);
   const bigfloat bliftt(blifta + bliftb);
   const bigfloat blift(bliftt * d3);
   const bigfloat clifta(cdx * cdx);
   const bigfloat cliftb(cdy * cdy);
   const bigfloat cliftt(clifta + cliftb);
   const bigfloat clift(cliftt * d2);
   const bigfloat la(alift * bcdet);
   const bigfloat lb(blift * cadet);
   const bigfloat lc(clift * abdet);
   const bigfloat lab2(lc + lb);
   const bigfloat lab(lab2 * d1);
   const bigfloat L(lab + la);
   return sgn(L);
}

inline int incircle_indirect_IIII_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[32], *l1x = l1x_p, l1y_p[32], *l1y = l1y_p, d1_p[32], *d1 = d1_p, l2x_p[32], *l2x = l2x_p, l2y_p[32], *l2y = l2y_p, d2_p[32], *d2 = d2_p, l3x_p[32], *l3x = l3x_p, l3y_p[32], *l3y = l3y_p, d3_p[32], *d3 = d3_p, l4x_p[32], *l4x = l4x_p, l4y_p[32], *l4y = l4y_p, d4_p[32], *d4 = d4_p;
 int l1x_len = 32, l1y_len = 32, d1_len = 32, l2x_len = 32, l2y_len = 32, d2_len = 32, l3x_len = 32, l3y_len = 32, d3_len = 32, l4x_len = 32, l4y_len = 32, d4_len = 32;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &d3, d3_len);
 p4.getExactLambda(&l4x, l4x_len, &l4y, l4y_len, &d4, d4_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0) && (d4[d4_len - 1] != 0))
 {
   
   double l1xt_p[32], *l1xt = l1xt_p;
   int l1xt_len = expansionObject::Gen_Product_With_PreAlloc(l1x_len, l1x, d4_len, d4, &l1xt, 32);
   double l1yt_p[32], *l1yt = l1yt_p;
   int l1yt_len = expansionObject::Gen_Product_With_PreAlloc(l1y_len, l1y, d4_len, d4, &l1yt, 32);
   double l2xt_p[32], *l2xt = l2xt_p;
   int l2xt_len = expansionObject::Gen_Product_With_PreAlloc(l2x_len, l2x, d4_len, d4, &l2xt, 32);
   double l2yt_p[32], *l2yt = l2yt_p;
   int l2yt_len = expansionObject::Gen_Product_With_PreAlloc(l2y_len, l2y, d4_len, d4, &l2yt, 32);
   double l3xt_p[32], *l3xt = l3xt_p;
   int l3xt_len = expansionObject::Gen_Product_With_PreAlloc(l3x_len, l3x, d4_len, d4, &l3xt, 32);
   double l3yt_p[32], *l3yt = l3yt_p;
   int l3yt_len = expansionObject::Gen_Product_With_PreAlloc(l3y_len, l3y, d4_len, d4, &l3yt, 32);
   double l4x1_p[32], *l4x1 = l4x1_p;
   int l4x1_len = expansionObject::Gen_Product_With_PreAlloc(l4x_len, l4x, d1_len, d1, &l4x1, 32);
   double l4y1_p[32], *l4y1 = l4y1_p;
   int l4y1_len = expansionObject::Gen_Product_With_PreAlloc(l4y_len, l4y, d1_len, d1, &l4y1, 32);
   double adx_p[32], *adx = adx_p;
   int adx_len = expansionObject::Gen_Diff_With_PreAlloc(l1xt_len, l1xt, l4x1_len, l4x1, &adx, 32);
   double ady_p[32], *ady = ady_p;
   int ady_len = expansionObject::Gen_Diff_With_PreAlloc(l1yt_len, l1yt, l4y1_len, l4y1, &ady, 32);
   double l4x2_p[32], *l4x2 = l4x2_p;
   int l4x2_len = expansionObject::Gen_Product_With_PreAlloc(l4x_len, l4x, d2_len, d2, &l4x2, 32);
   double l4y2_p[32], *l4y2 = l4y2_p;
   int l4y2_len = expansionObject::Gen_Product_With_PreAlloc(l4y_len, l4y, d2_len, d2, &l4y2, 32);
   double bdx_p[32], *bdx = bdx_p;
   int bdx_len = expansionObject::Gen_Diff_With_PreAlloc(l2xt_len, l2xt, l4x2_len, l4x2, &bdx, 32);
   double bdy_p[32], *bdy = bdy_p;
   int bdy_len = expansionObject::Gen_Diff_With_PreAlloc(l2yt_len, l2yt, l4y2_len, l4y2, &bdy, 32);
   double l4x3_p[32], *l4x3 = l4x3_p;
   int l4x3_len = expansionObject::Gen_Product_With_PreAlloc(l4x_len, l4x, d3_len, d3, &l4x3, 32);
   double l4y3_p[32], *l4y3 = l4y3_p;
   int l4y3_len = expansionObject::Gen_Product_With_PreAlloc(l4y_len, l4y, d3_len, d3, &l4y3, 32);
   double cdx_p[32], *cdx = cdx_p;
   int cdx_len = expansionObject::Gen_Diff_With_PreAlloc(l3xt_len, l3xt, l4x3_len, l4x3, &cdx, 32);
   double cdy_p[32], *cdy = cdy_p;
   int cdy_len = expansionObject::Gen_Diff_With_PreAlloc(l3yt_len, l3yt, l4y3_len, l4y3, &cdy, 32);
   double abdeta_p[32], *abdeta = abdeta_p;
   int abdeta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, bdy_len, bdy, &abdeta, 32);
   double abdetb_p[32], *abdetb = abdetb_p;
   int abdetb_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, ady_len, ady, &abdetb, 32);
   double abdet_p[32], *abdet = abdet_p;
   int abdet_len = expansionObject::Gen_Diff_With_PreAlloc(abdeta_len, abdeta, abdetb_len, abdetb, &abdet, 32);
   double bcdeta_p[32], *bcdeta = bcdeta_p;
   int bcdeta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, cdy_len, cdy, &bcdeta, 32);
   double bcdetb_p[32], *bcdetb = bcdetb_p;
   int bcdetb_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, bdy_len, bdy, &bcdetb, 32);
   double bcdet_p[32], *bcdet = bcdet_p;
   int bcdet_len = expansionObject::Gen_Diff_With_PreAlloc(bcdeta_len, bcdeta, bcdetb_len, bcdetb, &bcdet, 32);
   double cadeta_p[32], *cadeta = cadeta_p;
   int cadeta_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, ady_len, ady, &cadeta, 32);
   double cadetb_p[32], *cadetb = cadetb_p;
   int cadetb_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, cdy_len, cdy, &cadetb, 32);
   double cadet_p[32], *cadet = cadet_p;
   int cadet_len = expansionObject::Gen_Diff_With_PreAlloc(cadeta_len, cadeta, cadetb_len, cadetb, &cadet, 32);
   double alifta_p[32], *alifta = alifta_p;
   int alifta_len = expansionObject::Gen_Product_With_PreAlloc(adx_len, adx, adx_len, adx, &alifta, 32);
   double aliftb_p[32], *aliftb = aliftb_p;
   int aliftb_len = expansionObject::Gen_Product_With_PreAlloc(ady_len, ady, ady_len, ady, &aliftb, 32);
   double aliftt_p[32], *aliftt = aliftt_p;
   int aliftt_len = expansionObject::Gen_Sum_With_PreAlloc(alifta_len, alifta, aliftb_len, aliftb, &aliftt, 32);
   double alift2_p[32], *alift2 = alift2_p;
   int alift2_len = expansionObject::Gen_Product_With_PreAlloc(aliftt_len, aliftt, d2_len, d2, &alift2, 32);
   double alift_p[32], *alift = alift_p;
   int alift_len = expansionObject::Gen_Product_With_PreAlloc(alift2_len, alift2, d3_len, d3, &alift, 32);
   double blifta_p[32], *blifta = blifta_p;
   int blifta_len = expansionObject::Gen_Product_With_PreAlloc(bdx_len, bdx, bdx_len, bdx, &blifta, 32);
   double bliftb_p[32], *bliftb = bliftb_p;
   int bliftb_len = expansionObject::Gen_Product_With_PreAlloc(bdy_len, bdy, bdy_len, bdy, &bliftb, 32);
   double bliftt_p[32], *bliftt = bliftt_p;
   int bliftt_len = expansionObject::Gen_Sum_With_PreAlloc(blifta_len, blifta, bliftb_len, bliftb, &bliftt, 32);
   double blift_p[32], *blift = blift_p;
   int blift_len = expansionObject::Gen_Product_With_PreAlloc(bliftt_len, bliftt, d3_len, d3, &blift, 32);
   double clifta_p[32], *clifta = clifta_p;
   int clifta_len = expansionObject::Gen_Product_With_PreAlloc(cdx_len, cdx, cdx_len, cdx, &clifta, 32);
   double cliftb_p[32], *cliftb = cliftb_p;
   int cliftb_len = expansionObject::Gen_Product_With_PreAlloc(cdy_len, cdy, cdy_len, cdy, &cliftb, 32);
   double cliftt_p[32], *cliftt = cliftt_p;
   int cliftt_len = expansionObject::Gen_Sum_With_PreAlloc(clifta_len, clifta, cliftb_len, cliftb, &cliftt, 32);
   double clift_p[32], *clift = clift_p;
   int clift_len = expansionObject::Gen_Product_With_PreAlloc(cliftt_len, cliftt, d2_len, d2, &clift, 32);
   double la_p[32], *la = la_p;
   int la_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcdet_len, bcdet, &la, 32);
   double lb_p[32], *lb = lb_p;
   int lb_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cadet_len, cadet, &lb, 32);
   double lc_p[32], *lc = lc_p;
   int lc_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, abdet_len, abdet, &lc, 32);
   double lab2_p[32], *lab2 = lab2_p;
   int lab2_len = expansionObject::Gen_Sum_With_PreAlloc(lc_len, lc, lb_len, lb, &lab2, 32);
   double lab_p[32], *lab = lab_p;
   int lab_len = expansionObject::Gen_Product_With_PreAlloc(lab2_len, lab2, d1_len, d1, &lab, 32);
   double L_p[32], *L = L_p;
   int L_len = expansionObject::Gen_Sum_With_PreAlloc(lab_len, lab, la_len, la, &L, 32);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (lab_p != lab) FreeDoubles(lab);
   if (lab2_p != lab2) FreeDoubles(lab2);
   if (lc_p != lc) FreeDoubles(lc);
   if (lb_p != lb) FreeDoubles(lb);
   if (la_p != la) FreeDoubles(la);
   if (clift_p != clift) FreeDoubles(clift);
   if (cliftt_p != cliftt) FreeDoubles(cliftt);
   if (cliftb_p != cliftb) FreeDoubles(cliftb);
   if (clifta_p != clifta) FreeDoubles(clifta);
   if (blift_p != blift) FreeDoubles(blift);
   if (bliftt_p != bliftt) FreeDoubles(bliftt);
   if (bliftb_p != bliftb) FreeDoubles(bliftb);
   if (blifta_p != blifta) FreeDoubles(blifta);
   if (alift_p != alift) FreeDoubles(alift);
   if (alift2_p != alift2) FreeDoubles(alift2);
   if (aliftt_p != aliftt) FreeDoubles(aliftt);
   if (aliftb_p != aliftb) FreeDoubles(aliftb);
   if (alifta_p != alifta) FreeDoubles(alifta);
   if (cadet_p != cadet) FreeDoubles(cadet);
   if (cadetb_p != cadetb) FreeDoubles(cadetb);
   if (cadeta_p != cadeta) FreeDoubles(cadeta);
   if (bcdet_p != bcdet) FreeDoubles(bcdet);
   if (bcdetb_p != bcdetb) FreeDoubles(bcdetb);
   if (bcdeta_p != bcdeta) FreeDoubles(bcdeta);
   if (abdet_p != abdet) FreeDoubles(abdet);
   if (abdetb_p != abdetb) FreeDoubles(abdetb);
   if (abdeta_p != abdeta) FreeDoubles(abdeta);
   if (cdy_p != cdy) FreeDoubles(cdy);
   if (cdx_p != cdx) FreeDoubles(cdx);
   if (l4y3_p != l4y3) FreeDoubles(l4y3);
   if (l4x3_p != l4x3) FreeDoubles(l4x3);
   if (bdy_p != bdy) FreeDoubles(bdy);
   if (bdx_p != bdx) FreeDoubles(bdx);
   if (l4y2_p != l4y2) FreeDoubles(l4y2);
   if (l4x2_p != l4x2) FreeDoubles(l4x2);
   if (ady_p != ady) FreeDoubles(ady);
   if (adx_p != adx) FreeDoubles(adx);
   if (l4y1_p != l4y1) FreeDoubles(l4y1);
   if (l4x1_p != l4x1) FreeDoubles(l4x1);
   if (l3yt_p != l3yt) FreeDoubles(l3yt);
   if (l3xt_p != l3xt) FreeDoubles(l3xt);
   if (l2yt_p != l2yt) FreeDoubles(l2yt);
   if (l2xt_p != l2xt) FreeDoubles(l2xt);
   if (l1yt_p != l1yt) FreeDoubles(l1yt);
   if (l1xt_p != l1xt) FreeDoubles(l1xt);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = incircle_indirect_IIII_bigfloat(p1, p2, p3, p4);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (d3_p != d3) FreeDoubles(d3);
 if (l4x_p != l4x) FreeDoubles(l4x);
 if (l4y_p != l4y) FreeDoubles(l4y);
 if (d4_p != d4) FreeDoubles(d4);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int incircle_indirect_IIII(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
   int ret;
   ret = incircle_indirect_IIII_interval(p1, p2, p3, p4);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return incircle_indirect_IIII_exact(p1, p2, p3, p4);
}

inline int inSphere_IEEEE_interval(const genericPoint& p1, interval_number pbx, interval_number pby, interval_number pbz, interval_number pcx, interval_number pcy, interval_number pcz, interval_number pdx, interval_number pdy, interval_number pdz, interval_number pex, interval_number pey, interval_number pez)
{
   interval_number l1x, l1y, l1z, d1;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pexd(pex * d1);
   const interval_number peyd(pey * d1);
   const interval_number pezd(pez * d1);
   const interval_number aex(l1x - pexd);
   const interval_number aey(l1y - peyd);
   const interval_number aez(l1z - pezd);
   const interval_number bex(pbx - pex);
   const interval_number bey(pby - pey);
   const interval_number bez(pbz - pez);
   const interval_number cex(pcx - pex);
   const interval_number cey(pcy - pey);
   const interval_number cez(pcz - pez);
   const interval_number dex(pdx - pex);
   const interval_number dey(pdy - pey);
   const interval_number dez(pdz - pez);
   const interval_number aexbey(aex * bey);
   const interval_number bexaey(bex * aey);
   const interval_number ab(aexbey - bexaey);
   const interval_number bexcey(bex * cey);
   const interval_number cexbey(cex * bey);
   const interval_number bc(bexcey - cexbey);
   const interval_number cexdey(cex * dey);
   const interval_number dexcey(dex * cey);
   const interval_number cd(cexdey - dexcey);
   const interval_number dexaey(dex * aey);
   const interval_number aexdey(aex * dey);
   const interval_number da(dexaey - aexdey);
   const interval_number aexcey(aex * cey);
   const interval_number cexaey(cex * aey);
   const interval_number ac(aexcey - cexaey);
   const interval_number bexdey(bex * dey);
   const interval_number dexbey(dex * bey);
   const interval_number bd(bexdey - dexbey);
   const interval_number abc1(aez * bc);
   const interval_number abc2(bez * ac);
   const interval_number abc3(cez * ab);
   const interval_number abc4(abc1 + abc3);
   const interval_number abc(abc4 - abc2);
   const interval_number bcd1(bez * cd);
   const interval_number bcd2(cez * bd);
   const interval_number bcd3(dez * bc);
   const interval_number bcd4(bcd1 + bcd3);
   const interval_number bcd(bcd4 - bcd2);
   const interval_number cda1(cez * da);
   const interval_number cda2(dez * ac);
   const interval_number cda3(aez * cd);
   const interval_number cda4(cda1 + cda3);
   const interval_number cda(cda4 + cda2);
   const interval_number dab1(dez * ab);
   const interval_number dab2(aez * bd);
   const interval_number dab3(bez * da);
   const interval_number dab4(dab1 + dab3);
   const interval_number dab(dab4 + dab2);
   const interval_number al1(aex * aex);
   const interval_number al2(aey * aey);
   const interval_number al3(aez * aez);
   const interval_number al4(al1 + al2);
   const interval_number alift(al4 + al3);
   const interval_number bl1(bex * bex);
   const interval_number bl2(bey * bey);
   const interval_number bl3(bez * bez);
   const interval_number bl4(bl1 + bl2);
   const interval_number blift(bl4 + bl3);
   const interval_number cl1(cex * cex);
   const interval_number cl2(cey * cey);
   const interval_number cl3(cez * cez);
   const interval_number cl4(cl1 + cl2);
   const interval_number clift(cl4 + cl3);
   const interval_number dl1(dex * dex);
   const interval_number dl2(dey * dey);
   const interval_number dl3(dez * dez);
   const interval_number dl4(dl1 + dl2);
   const interval_number dlift(dl4 + dl3);
   const interval_number ds1(dlift * abc);
   const interval_number ds2(clift * dab);
   const interval_number dlp(ds2 - ds1);
   const interval_number dl(dlp * d1);
   const interval_number dr1p(blift * cda);
   const interval_number dr1(dr1p * d1);
   const interval_number dr2(alift * bcd);
   const interval_number dr(dr2 - dr1);
   const interval_number det(dl + dr);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int inSphere_IEEEE_bigfloat(const genericPoint& p1, bigfloat pbx, bigfloat pby, bigfloat pbz, bigfloat pcx, bigfloat pcy, bigfloat pcz, bigfloat pdx, bigfloat pdy, bigfloat pdz, bigfloat pex, bigfloat pey, bigfloat pez)
{
   bigfloat l1x, l1y, l1z, d1;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   const bigfloat pexd(pex * d1);
   const bigfloat peyd(pey * d1);
   const bigfloat pezd(pez * d1);
   const bigfloat aex(l1x - pexd);
   const bigfloat aey(l1y - peyd);
   const bigfloat aez(l1z - pezd);
   const bigfloat bex(pbx - pex);
   const bigfloat bey(pby - pey);
   const bigfloat bez(pbz - pez);
   const bigfloat cex(pcx - pex);
   const bigfloat cey(pcy - pey);
   const bigfloat cez(pcz - pez);
   const bigfloat dex(pdx - pex);
   const bigfloat dey(pdy - pey);
   const bigfloat dez(pdz - pez);
   const bigfloat aexbey(aex * bey);
   const bigfloat bexaey(bex * aey);
   const bigfloat ab(aexbey - bexaey);
   const bigfloat bexcey(bex * cey);
   const bigfloat cexbey(cex * bey);
   const bigfloat bc(bexcey - cexbey);
   const bigfloat cexdey(cex * dey);
   const bigfloat dexcey(dex * cey);
   const bigfloat cd(cexdey - dexcey);
   const bigfloat dexaey(dex * aey);
   const bigfloat aexdey(aex * dey);
   const bigfloat da(dexaey - aexdey);
   const bigfloat aexcey(aex * cey);
   const bigfloat cexaey(cex * aey);
   const bigfloat ac(aexcey - cexaey);
   const bigfloat bexdey(bex * dey);
   const bigfloat dexbey(dex * bey);
   const bigfloat bd(bexdey - dexbey);
   const bigfloat abc1(aez * bc);
   const bigfloat abc2(bez * ac);
   const bigfloat abc3(cez * ab);
   const bigfloat abc4(abc1 + abc3);
   const bigfloat abc(abc4 - abc2);
   const bigfloat bcd1(bez * cd);
   const bigfloat bcd2(cez * bd);
   const bigfloat bcd3(dez * bc);
   const bigfloat bcd4(bcd1 + bcd3);
   const bigfloat bcd(bcd4 - bcd2);
   const bigfloat cda1(cez * da);
   const bigfloat cda2(dez * ac);
   const bigfloat cda3(aez * cd);
   const bigfloat cda4(cda1 + cda3);
   const bigfloat cda(cda4 + cda2);
   const bigfloat dab1(dez * ab);
   const bigfloat dab2(aez * bd);
   const bigfloat dab3(bez * da);
   const bigfloat dab4(dab1 + dab3);
   const bigfloat dab(dab4 + dab2);
   const bigfloat al1(aex * aex);
   const bigfloat al2(aey * aey);
   const bigfloat al3(aez * aez);
   const bigfloat al4(al1 + al2);
   const bigfloat alift(al4 + al3);
   const bigfloat bl1(bex * bex);
   const bigfloat bl2(bey * bey);
   const bigfloat bl3(bez * bez);
   const bigfloat bl4(bl1 + bl2);
   const bigfloat blift(bl4 + bl3);
   const bigfloat cl1(cex * cex);
   const bigfloat cl2(cey * cey);
   const bigfloat cl3(cez * cez);
   const bigfloat cl4(cl1 + cl2);
   const bigfloat clift(cl4 + cl3);
   const bigfloat dl1(dex * dex);
   const bigfloat dl2(dey * dey);
   const bigfloat dl3(dez * dez);
   const bigfloat dl4(dl1 + dl2);
   const bigfloat dlift(dl4 + dl3);
   const bigfloat ds1(dlift * abc);
   const bigfloat ds2(clift * dab);
   const bigfloat dlp(ds2 - ds1);
   const bigfloat dl(dlp * d1);
   const bigfloat dr1p(blift * cda);
   const bigfloat dr1(dr1p * d1);
   const bigfloat dr2(alift * bcd);
   const bigfloat dr(dr2 - dr1);
   const bigfloat det(dl + dr);
   return sgn(det);
}

inline int inSphere_IEEEE_exact(const genericPoint& p1, double pbx, double pby, double pbz, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[16], *l1x = l1x_p, l1y_p[16], *l1y = l1y_p, l1z_p[16], *l1z = l1z_p, d1_p[16], *d1 = d1_p;
 int l1x_len = 16, l1y_len = 16, l1z_len = 16, d1_len = 16;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double pexd_p[16], *pexd = pexd_p;
   int pexd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pex, &pexd, 16);
   double peyd_p[16], *peyd = peyd_p;
   int peyd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pey, &peyd, 16);
   double pezd_p[16], *pezd = pezd_p;
   int pezd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pez, &pezd, 16);
   double aex_p[16], *aex = aex_p;
   int aex_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pexd_len, pexd, &aex, 16);
   double aey_p[16], *aey = aey_p;
   int aey_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, peyd_len, peyd, &aey, 16);
   double aez_p[16], *aez = aez_p;
   int aez_len = expansionObject::Gen_Diff_With_PreAlloc(l1z_len, l1z, pezd_len, pezd, &aez, 16);
   double bex[2];
   expansionObject::two_Diff(pbx, pex, bex);
   double bey[2];
   expansionObject::two_Diff(pby, pey, bey);
   double bez[2];
   expansionObject::two_Diff(pbz, pez, bez);
   double cex[2];
   expansionObject::two_Diff(pcx, pex, cex);
   double cey[2];
   expansionObject::two_Diff(pcy, pey, cey);
   double cez[2];
   expansionObject::two_Diff(pcz, pez, cez);
   double dex[2];
   expansionObject::two_Diff(pdx, pex, dex);
   double dey[2];
   expansionObject::two_Diff(pdy, pey, dey);
   double dez[2];
   expansionObject::two_Diff(pdz, pez, dez);
   double aexbey_p[16], *aexbey = aexbey_p;
   int aexbey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, 2, bey, &aexbey, 16);
   double bexaey_p[16], *bexaey = bexaey_p;
   int bexaey_len = expansionObject::Gen_Product_With_PreAlloc(2, bex, aey_len, aey, &bexaey, 16);
   double ab_p[16], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(aexbey_len, aexbey, bexaey_len, bexaey, &ab, 16);
   double bexcey[8];
   int bexcey_len = expansionObject::Gen_Product(2, bex, 2, cey, bexcey);
   double cexbey[8];
   int cexbey_len = expansionObject::Gen_Product(2, cex, 2, bey, cexbey);
   double bc[16];
   int bc_len = expansionObject::Gen_Diff(bexcey_len, bexcey, cexbey_len, cexbey, bc);
   double cexdey[8];
   int cexdey_len = expansionObject::Gen_Product(2, cex, 2, dey, cexdey);
   double dexcey[8];
   int dexcey_len = expansionObject::Gen_Product(2, dex, 2, cey, dexcey);
   double cd[16];
   int cd_len = expansionObject::Gen_Diff(cexdey_len, cexdey, dexcey_len, dexcey, cd);
   double dexaey_p[16], *dexaey = dexaey_p;
   int dexaey_len = expansionObject::Gen_Product_With_PreAlloc(2, dex, aey_len, aey, &dexaey, 16);
   double aexdey_p[16], *aexdey = aexdey_p;
   int aexdey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, 2, dey, &aexdey, 16);
   double da_p[16], *da = da_p;
   int da_len = expansionObject::Gen_Diff_With_PreAlloc(dexaey_len, dexaey, aexdey_len, aexdey, &da, 16);
   double aexcey_p[16], *aexcey = aexcey_p;
   int aexcey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, 2, cey, &aexcey, 16);
   double cexaey_p[16], *cexaey = cexaey_p;
   int cexaey_len = expansionObject::Gen_Product_With_PreAlloc(2, cex, aey_len, aey, &cexaey, 16);
   double ac_p[16], *ac = ac_p;
   int ac_len = expansionObject::Gen_Diff_With_PreAlloc(aexcey_len, aexcey, cexaey_len, cexaey, &ac, 16);
   double bexdey[8];
   int bexdey_len = expansionObject::Gen_Product(2, bex, 2, dey, bexdey);
   double dexbey[8];
   int dexbey_len = expansionObject::Gen_Product(2, dex, 2, bey, dexbey);
   double bd[16];
   int bd_len = expansionObject::Gen_Diff(bexdey_len, bexdey, dexbey_len, dexbey, bd);
   double abc1_p[16], *abc1 = abc1_p;
   int abc1_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bc_len, bc, &abc1, 16);
   double abc2_p[16], *abc2 = abc2_p;
   int abc2_len = expansionObject::Gen_Product_With_PreAlloc(2, bez, ac_len, ac, &abc2, 16);
   double abc3_p[16], *abc3 = abc3_p;
   int abc3_len = expansionObject::Gen_Product_With_PreAlloc(2, cez, ab_len, ab, &abc3, 16);
   double abc4_p[16], *abc4 = abc4_p;
   int abc4_len = expansionObject::Gen_Sum_With_PreAlloc(abc1_len, abc1, abc3_len, abc3, &abc4, 16);
   double abc_p[16], *abc = abc_p;
   int abc_len = expansionObject::Gen_Diff_With_PreAlloc(abc4_len, abc4, abc2_len, abc2, &abc, 16);
   double bcd1_p[16], *bcd1 = bcd1_p;
   int bcd1_len = expansionObject::Gen_Product_With_PreAlloc(2, bez, cd_len, cd, &bcd1, 16);
   double bcd2_p[16], *bcd2 = bcd2_p;
   int bcd2_len = expansionObject::Gen_Product_With_PreAlloc(2, cez, bd_len, bd, &bcd2, 16);
   double bcd3_p[16], *bcd3 = bcd3_p;
   int bcd3_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, bc_len, bc, &bcd3, 16);
   double bcd4_p[16], *bcd4 = bcd4_p;
   int bcd4_len = expansionObject::Gen_Sum_With_PreAlloc(bcd1_len, bcd1, bcd3_len, bcd3, &bcd4, 16);
   double bcd_p[16], *bcd = bcd_p;
   int bcd_len = expansionObject::Gen_Diff_With_PreAlloc(bcd4_len, bcd4, bcd2_len, bcd2, &bcd, 16);
   double cda1_p[16], *cda1 = cda1_p;
   int cda1_len = expansionObject::Gen_Product_With_PreAlloc(2, cez, da_len, da, &cda1, 16);
   double cda2_p[16], *cda2 = cda2_p;
   int cda2_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, ac_len, ac, &cda2, 16);
   double cda3_p[16], *cda3 = cda3_p;
   int cda3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, cd_len, cd, &cda3, 16);
   double cda4_p[16], *cda4 = cda4_p;
   int cda4_len = expansionObject::Gen_Sum_With_PreAlloc(cda1_len, cda1, cda3_len, cda3, &cda4, 16);
   double cda_p[16], *cda = cda_p;
   int cda_len = expansionObject::Gen_Sum_With_PreAlloc(cda4_len, cda4, cda2_len, cda2, &cda, 16);
   double dab1_p[16], *dab1 = dab1_p;
   int dab1_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, ab_len, ab, &dab1, 16);
   double dab2_p[16], *dab2 = dab2_p;
   int dab2_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bd_len, bd, &dab2, 16);
   double dab3_p[16], *dab3 = dab3_p;
   int dab3_len = expansionObject::Gen_Product_With_PreAlloc(2, bez, da_len, da, &dab3, 16);
   double dab4_p[16], *dab4 = dab4_p;
   int dab4_len = expansionObject::Gen_Sum_With_PreAlloc(dab1_len, dab1, dab3_len, dab3, &dab4, 16);
   double dab_p[16], *dab = dab_p;
   int dab_len = expansionObject::Gen_Sum_With_PreAlloc(dab4_len, dab4, dab2_len, dab2, &dab, 16);
   double al1_p[16], *al1 = al1_p;
   int al1_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, aex_len, aex, &al1, 16);
   double al2_p[16], *al2 = al2_p;
   int al2_len = expansionObject::Gen_Product_With_PreAlloc(aey_len, aey, aey_len, aey, &al2, 16);
   double al3_p[16], *al3 = al3_p;
   int al3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, aez_len, aez, &al3, 16);
   double al4_p[16], *al4 = al4_p;
   int al4_len = expansionObject::Gen_Sum_With_PreAlloc(al1_len, al1, al2_len, al2, &al4, 16);
   double alift_p[16], *alift = alift_p;
   int alift_len = expansionObject::Gen_Sum_With_PreAlloc(al4_len, al4, al3_len, al3, &alift, 16);
   double bl1[8];
   int bl1_len = expansionObject::Gen_Product(2, bex, 2, bex, bl1);
   double bl2[8];
   int bl2_len = expansionObject::Gen_Product(2, bey, 2, bey, bl2);
   double bl3[8];
   int bl3_len = expansionObject::Gen_Product(2, bez, 2, bez, bl3);
   double bl4[16];
   int bl4_len = expansionObject::Gen_Sum(bl1_len, bl1, bl2_len, bl2, bl4);
   double blift_p[16], *blift = blift_p;
   int blift_len = expansionObject::Gen_Sum_With_PreAlloc(bl4_len, bl4, bl3_len, bl3, &blift, 16);
   double cl1[8];
   int cl1_len = expansionObject::Gen_Product(2, cex, 2, cex, cl1);
   double cl2[8];
   int cl2_len = expansionObject::Gen_Product(2, cey, 2, cey, cl2);
   double cl3[8];
   int cl3_len = expansionObject::Gen_Product(2, cez, 2, cez, cl3);
   double cl4[16];
   int cl4_len = expansionObject::Gen_Sum(cl1_len, cl1, cl2_len, cl2, cl4);
   double clift_p[16], *clift = clift_p;
   int clift_len = expansionObject::Gen_Sum_With_PreAlloc(cl4_len, cl4, cl3_len, cl3, &clift, 16);
   double dl1[8];
   int dl1_len = expansionObject::Gen_Product(2, dex, 2, dex, dl1);
   double dl2[8];
   int dl2_len = expansionObject::Gen_Product(2, dey, 2, dey, dl2);
   double dl3[8];
   int dl3_len = expansionObject::Gen_Product(2, dez, 2, dez, dl3);
   double dl4[16];
   int dl4_len = expansionObject::Gen_Sum(dl1_len, dl1, dl2_len, dl2, dl4);
   double dlift_p[16], *dlift = dlift_p;
   int dlift_len = expansionObject::Gen_Sum_With_PreAlloc(dl4_len, dl4, dl3_len, dl3, &dlift, 16);
   double ds1_p[16], *ds1 = ds1_p;
   int ds1_len = expansionObject::Gen_Product_With_PreAlloc(dlift_len, dlift, abc_len, abc, &ds1, 16);
   double ds2_p[16], *ds2 = ds2_p;
   int ds2_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, dab_len, dab, &ds2, 16);
   double dlp_p[16], *dlp = dlp_p;
   int dlp_len = expansionObject::Gen_Diff_With_PreAlloc(ds2_len, ds2, ds1_len, ds1, &dlp, 16);
   double dl_p[16], *dl = dl_p;
   int dl_len = expansionObject::Gen_Product_With_PreAlloc(dlp_len, dlp, d1_len, d1, &dl, 16);
   double dr1p_p[16], *dr1p = dr1p_p;
   int dr1p_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cda_len, cda, &dr1p, 16);
   double dr1_p[16], *dr1 = dr1_p;
   int dr1_len = expansionObject::Gen_Product_With_PreAlloc(dr1p_len, dr1p, d1_len, d1, &dr1, 16);
   double dr2_p[16], *dr2 = dr2_p;
   int dr2_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcd_len, bcd, &dr2, 16);
   double dr_p[16], *dr = dr_p;
   int dr_len = expansionObject::Gen_Diff_With_PreAlloc(dr2_len, dr2, dr1_len, dr1, &dr, 16);
   double det_p[16], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dl_len, dl, dr_len, dr, &det, 16);

   return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (dr_p != dr) FreeDoubles(dr);
   if (dr2_p != dr2) FreeDoubles(dr2);
   if (dr1_p != dr1) FreeDoubles(dr1);
   if (dr1p_p != dr1p) FreeDoubles(dr1p);
   if (dl_p != dl) FreeDoubles(dl);
   if (dlp_p != dlp) FreeDoubles(dlp);
   if (ds2_p != ds2) FreeDoubles(ds2);
   if (ds1_p != ds1) FreeDoubles(ds1);
   if (dlift_p != dlift) FreeDoubles(dlift);
   if (clift_p != clift) FreeDoubles(clift);
   if (blift_p != blift) FreeDoubles(blift);
   if (alift_p != alift) FreeDoubles(alift);
   if (al4_p != al4) FreeDoubles(al4);
   if (al3_p != al3) FreeDoubles(al3);
   if (al2_p != al2) FreeDoubles(al2);
   if (al1_p != al1) FreeDoubles(al1);
   if (dab_p != dab) FreeDoubles(dab);
   if (dab4_p != dab4) FreeDoubles(dab4);
   if (dab3_p != dab3) FreeDoubles(dab3);
   if (dab2_p != dab2) FreeDoubles(dab2);
   if (dab1_p != dab1) FreeDoubles(dab1);
   if (cda_p != cda) FreeDoubles(cda);
   if (cda4_p != cda4) FreeDoubles(cda4);
   if (cda3_p != cda3) FreeDoubles(cda3);
   if (cda2_p != cda2) FreeDoubles(cda2);
   if (cda1_p != cda1) FreeDoubles(cda1);
   if (bcd_p != bcd) FreeDoubles(bcd);
   if (bcd4_p != bcd4) FreeDoubles(bcd4);
   if (bcd3_p != bcd3) FreeDoubles(bcd3);
   if (bcd2_p != bcd2) FreeDoubles(bcd2);
   if (bcd1_p != bcd1) FreeDoubles(bcd1);
   if (abc_p != abc) FreeDoubles(abc);
   if (abc4_p != abc4) FreeDoubles(abc4);
   if (abc3_p != abc3) FreeDoubles(abc3);
   if (abc2_p != abc2) FreeDoubles(abc2);
   if (abc1_p != abc1) FreeDoubles(abc1);
   if (ac_p != ac) FreeDoubles(ac);
   if (cexaey_p != cexaey) FreeDoubles(cexaey);
   if (aexcey_p != aexcey) FreeDoubles(aexcey);
   if (da_p != da) FreeDoubles(da);
   if (aexdey_p != aexdey) FreeDoubles(aexdey);
   if (dexaey_p != dexaey) FreeDoubles(dexaey);
   if (ab_p != ab) FreeDoubles(ab);
   if (bexaey_p != bexaey) FreeDoubles(bexaey);
   if (aexbey_p != aexbey) FreeDoubles(aexbey);
   if (aez_p != aez) FreeDoubles(aez);
   if (aey_p != aey) FreeDoubles(aey);
   if (aex_p != aex) FreeDoubles(aex);
   if (pezd_p != pezd) FreeDoubles(pezd);
   if (peyd_p != peyd) FreeDoubles(peyd);
   if (pexd_p != pexd) FreeDoubles(pexd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = inSphere_IEEEE_bigfloat(p1, pbx, pby, pbz, pcx, pcy, pcz, pdx, pdy, pdz, pex, pey, pez);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int inSphere_IEEEE(const genericPoint& p1, double pbx, double pby, double pbz, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez)
{
   int ret;
   ret = inSphere_IEEEE_interval(p1, pbx, pby, pbz, pcx, pcy, pcz, pdx, pdy, pdz, pex, pey, pez);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return inSphere_IEEEE_exact(p1, pbx, pby, pbz, pcx, pcy, pcz, pdx, pdy, pdz, pex, pey, pez);
}

inline int inSphere_IIEEE_interval(const genericPoint& p1, const genericPoint& p2, interval_number pcx, interval_number pcy, interval_number pcz, interval_number pdx, interval_number pdy, interval_number pdz, interval_number pex, interval_number pey, interval_number pez)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pexd(pex * d1);
   const interval_number peyd(pey * d1);
   const interval_number pezd(pez * d1);
   const interval_number aex(l1x - pexd);
   const interval_number aey(l1y - peyd);
   const interval_number aez(l1z - pezd);
   const interval_number pexd2(pex * d2);
   const interval_number peyd2(pey * d2);
   const interval_number pezd2(pez * d2);
   const interval_number bex(l2x - pexd2);
   const interval_number bey(l2y - peyd2);
   const interval_number bez(l2z - pezd2);
   const interval_number cex(pcx - pex);
   const interval_number cey(pcy - pey);
   const interval_number cez(pcz - pez);
   const interval_number dex(pdx - pex);
   const interval_number dey(pdy - pey);
   const interval_number dez(pdz - pez);
   const interval_number aexbey(aex * bey);
   const interval_number bexaey(bex * aey);
   const interval_number ab(aexbey - bexaey);
   const interval_number bexcey(bex * cey);
   const interval_number cexbey(cex * bey);
   const interval_number bc(bexcey - cexbey);
   const interval_number cexdey(cex * dey);
   const interval_number dexcey(dex * cey);
   const interval_number cd(cexdey - dexcey);
   const interval_number dexaey(dex * aey);
   const interval_number aexdey(aex * dey);
   const interval_number da(dexaey - aexdey);
   const interval_number aexcey(aex * cey);
   const interval_number cexaey(cex * aey);
   const interval_number ac(aexcey - cexaey);
   const interval_number bexdey(bex * dey);
   const interval_number dexbey(dex * bey);
   const interval_number bd(bexdey - dexbey);
   const interval_number abc1(aez * bc);
   const interval_number abc2(bez * ac);
   const interval_number abc3(cez * ab);
   const interval_number abc4(abc1 + abc3);
   const interval_number abc(abc4 - abc2);
   const interval_number bcd1(bez * cd);
   const interval_number bcd2(cez * bd);
   const interval_number bcd3(dez * bc);
   const interval_number bcd4(bcd1 + bcd3);
   const interval_number bcd(bcd4 - bcd2);
   const interval_number cda1(cez * da);
   const interval_number cda2(dez * ac);
   const interval_number cda3(aez * cd);
   const interval_number cda4(cda1 + cda3);
   const interval_number cda(cda4 + cda2);
   const interval_number dab1(dez * ab);
   const interval_number dab2(aez * bd);
   const interval_number dab3(bez * da);
   const interval_number dab4(dab1 + dab3);
   const interval_number dab(dab4 + dab2);
   const interval_number al1(aex * aex);
   const interval_number al2(aey * aey);
   const interval_number al3(aez * aez);
   const interval_number al4(al1 + al2);
   const interval_number alift(al4 + al3);
   const interval_number bl1(bex * bex);
   const interval_number bl2(bey * bey);
   const interval_number bl3(bez * bez);
   const interval_number bl4(bl1 + bl2);
   const interval_number blift(bl4 + bl3);
   const interval_number cl1(cex * cex);
   const interval_number cl2(cey * cey);
   const interval_number cl3(cez * cez);
   const interval_number cl4(cl1 + cl2);
   const interval_number clift(cl4 + cl3);
   const interval_number dl1(dex * dex);
   const interval_number dl2(dey * dey);
   const interval_number dl3(dez * dez);
   const interval_number dl4(dl1 + dl2);
   const interval_number dlift(dl4 + dl3);
   const interval_number ds1(dlift * abc);
   const interval_number ds2(clift * dab);
   const interval_number dl(ds2 - ds1);
   const interval_number dll(dl * d1);
   const interval_number dlll(dll * d2);
   const interval_number dr1(blift * cda);
   const interval_number dr12(dr1 * d1);
   const interval_number dr2(alift * bcd);
   const interval_number dr22(dr2 * d2);
   const interval_number dr(dr22 - dr12);
   const interval_number det(dlll + dr);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int inSphere_IIEEE_bigfloat(const genericPoint& p1, const genericPoint& p2, bigfloat pcx, bigfloat pcy, bigfloat pcz, bigfloat pdx, bigfloat pdy, bigfloat pdz, bigfloat pex, bigfloat pey, bigfloat pez)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   const bigfloat pexd(pex * d1);
   const bigfloat peyd(pey * d1);
   const bigfloat pezd(pez * d1);
   const bigfloat aex(l1x - pexd);
   const bigfloat aey(l1y - peyd);
   const bigfloat aez(l1z - pezd);
   const bigfloat pexd2(pex * d2);
   const bigfloat peyd2(pey * d2);
   const bigfloat pezd2(pez * d2);
   const bigfloat bex(l2x - pexd2);
   const bigfloat bey(l2y - peyd2);
   const bigfloat bez(l2z - pezd2);
   const bigfloat cex(pcx - pex);
   const bigfloat cey(pcy - pey);
   const bigfloat cez(pcz - pez);
   const bigfloat dex(pdx - pex);
   const bigfloat dey(pdy - pey);
   const bigfloat dez(pdz - pez);
   const bigfloat aexbey(aex * bey);
   const bigfloat bexaey(bex * aey);
   const bigfloat ab(aexbey - bexaey);
   const bigfloat bexcey(bex * cey);
   const bigfloat cexbey(cex * bey);
   const bigfloat bc(bexcey - cexbey);
   const bigfloat cexdey(cex * dey);
   const bigfloat dexcey(dex * cey);
   const bigfloat cd(cexdey - dexcey);
   const bigfloat dexaey(dex * aey);
   const bigfloat aexdey(aex * dey);
   const bigfloat da(dexaey - aexdey);
   const bigfloat aexcey(aex * cey);
   const bigfloat cexaey(cex * aey);
   const bigfloat ac(aexcey - cexaey);
   const bigfloat bexdey(bex * dey);
   const bigfloat dexbey(dex * bey);
   const bigfloat bd(bexdey - dexbey);
   const bigfloat abc1(aez * bc);
   const bigfloat abc2(bez * ac);
   const bigfloat abc3(cez * ab);
   const bigfloat abc4(abc1 + abc3);
   const bigfloat abc(abc4 - abc2);
   const bigfloat bcd1(bez * cd);
   const bigfloat bcd2(cez * bd);
   const bigfloat bcd3(dez * bc);
   const bigfloat bcd4(bcd1 + bcd3);
   const bigfloat bcd(bcd4 - bcd2);
   const bigfloat cda1(cez * da);
   const bigfloat cda2(dez * ac);
   const bigfloat cda3(aez * cd);
   const bigfloat cda4(cda1 + cda3);
   const bigfloat cda(cda4 + cda2);
   const bigfloat dab1(dez * ab);
   const bigfloat dab2(aez * bd);
   const bigfloat dab3(bez * da);
   const bigfloat dab4(dab1 + dab3);
   const bigfloat dab(dab4 + dab2);
   const bigfloat al1(aex * aex);
   const bigfloat al2(aey * aey);
   const bigfloat al3(aez * aez);
   const bigfloat al4(al1 + al2);
   const bigfloat alift(al4 + al3);
   const bigfloat bl1(bex * bex);
   const bigfloat bl2(bey * bey);
   const bigfloat bl3(bez * bez);
   const bigfloat bl4(bl1 + bl2);
   const bigfloat blift(bl4 + bl3);
   const bigfloat cl1(cex * cex);
   const bigfloat cl2(cey * cey);
   const bigfloat cl3(cez * cez);
   const bigfloat cl4(cl1 + cl2);
   const bigfloat clift(cl4 + cl3);
   const bigfloat dl1(dex * dex);
   const bigfloat dl2(dey * dey);
   const bigfloat dl3(dez * dez);
   const bigfloat dl4(dl1 + dl2);
   const bigfloat dlift(dl4 + dl3);
   const bigfloat ds1(dlift * abc);
   const bigfloat ds2(clift * dab);
   const bigfloat dl(ds2 - ds1);
   const bigfloat dll(dl * d1);
   const bigfloat dlll(dll * d2);
   const bigfloat dr1(blift * cda);
   const bigfloat dr12(dr1 * d1);
   const bigfloat dr2(alift * bcd);
   const bigfloat dr22(dr2 * d2);
   const bigfloat dr(dr22 - dr12);
   const bigfloat det(dlll + dr);
   return sgn(det);
}

inline int inSphere_IIEEE_exact(const genericPoint& p1, const genericPoint& p2, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[16], *l1x = l1x_p, l1y_p[16], *l1y = l1y_p, l1z_p[16], *l1z = l1z_p, d1_p[16], *d1 = d1_p, l2x_p[16], *l2x = l2x_p, l2y_p[16], *l2y = l2y_p, l2z_p[16], *l2z = l2z_p, d2_p[16], *d2 = d2_p;
 int l1x_len = 16, l1y_len = 16, l1z_len = 16, d1_len = 16, l2x_len = 16, l2y_len = 16, l2z_len = 16, d2_len = 16;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double pexd_p[16], *pexd = pexd_p;
   int pexd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pex, &pexd, 16);
   double peyd_p[16], *peyd = peyd_p;
   int peyd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pey, &peyd, 16);
   double pezd_p[16], *pezd = pezd_p;
   int pezd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pez, &pezd, 16);
   double aex_p[16], *aex = aex_p;
   int aex_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pexd_len, pexd, &aex, 16);
   double aey_p[16], *aey = aey_p;
   int aey_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, peyd_len, peyd, &aey, 16);
   double aez_p[16], *aez = aez_p;
   int aez_len = expansionObject::Gen_Diff_With_PreAlloc(l1z_len, l1z, pezd_len, pezd, &aez, 16);
   double pexd2_p[16], *pexd2 = pexd2_p;
   int pexd2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pex, &pexd2, 16);
   double peyd2_p[16], *peyd2 = peyd2_p;
   int peyd2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pey, &peyd2, 16);
   double pezd2_p[16], *pezd2 = pezd2_p;
   int pezd2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pez, &pezd2, 16);
   double bex_p[16], *bex = bex_p;
   int bex_len = expansionObject::Gen_Diff_With_PreAlloc(l2x_len, l2x, pexd2_len, pexd2, &bex, 16);
   double bey_p[16], *bey = bey_p;
   int bey_len = expansionObject::Gen_Diff_With_PreAlloc(l2y_len, l2y, peyd2_len, peyd2, &bey, 16);
   double bez_p[16], *bez = bez_p;
   int bez_len = expansionObject::Gen_Diff_With_PreAlloc(l2z_len, l2z, pezd2_len, pezd2, &bez, 16);
   double cex[2];
   expansionObject::two_Diff(pcx, pex, cex);
   double cey[2];
   expansionObject::two_Diff(pcy, pey, cey);
   double cez[2];
   expansionObject::two_Diff(pcz, pez, cez);
   double dex[2];
   expansionObject::two_Diff(pdx, pex, dex);
   double dey[2];
   expansionObject::two_Diff(pdy, pey, dey);
   double dez[2];
   expansionObject::two_Diff(pdz, pez, dez);
   double aexbey_p[16], *aexbey = aexbey_p;
   int aexbey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, bey_len, bey, &aexbey, 16);
   double bexaey_p[16], *bexaey = bexaey_p;
   int bexaey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, aey_len, aey, &bexaey, 16);
   double ab_p[16], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(aexbey_len, aexbey, bexaey_len, bexaey, &ab, 16);
   double bexcey_p[16], *bexcey = bexcey_p;
   int bexcey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, 2, cey, &bexcey, 16);
   double cexbey_p[16], *cexbey = cexbey_p;
   int cexbey_len = expansionObject::Gen_Product_With_PreAlloc(2, cex, bey_len, bey, &cexbey, 16);
   double bc_p[16], *bc = bc_p;
   int bc_len = expansionObject::Gen_Diff_With_PreAlloc(bexcey_len, bexcey, cexbey_len, cexbey, &bc, 16);
   double cexdey[8];
   int cexdey_len = expansionObject::Gen_Product(2, cex, 2, dey, cexdey);
   double dexcey[8];
   int dexcey_len = expansionObject::Gen_Product(2, dex, 2, cey, dexcey);
   double cd[16];
   int cd_len = expansionObject::Gen_Diff(cexdey_len, cexdey, dexcey_len, dexcey, cd);
   double dexaey_p[16], *dexaey = dexaey_p;
   int dexaey_len = expansionObject::Gen_Product_With_PreAlloc(2, dex, aey_len, aey, &dexaey, 16);
   double aexdey_p[16], *aexdey = aexdey_p;
   int aexdey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, 2, dey, &aexdey, 16);
   double da_p[16], *da = da_p;
   int da_len = expansionObject::Gen_Diff_With_PreAlloc(dexaey_len, dexaey, aexdey_len, aexdey, &da, 16);
   double aexcey_p[16], *aexcey = aexcey_p;
   int aexcey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, 2, cey, &aexcey, 16);
   double cexaey_p[16], *cexaey = cexaey_p;
   int cexaey_len = expansionObject::Gen_Product_With_PreAlloc(2, cex, aey_len, aey, &cexaey, 16);
   double ac_p[16], *ac = ac_p;
   int ac_len = expansionObject::Gen_Diff_With_PreAlloc(aexcey_len, aexcey, cexaey_len, cexaey, &ac, 16);
   double bexdey_p[16], *bexdey = bexdey_p;
   int bexdey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, 2, dey, &bexdey, 16);
   double dexbey_p[16], *dexbey = dexbey_p;
   int dexbey_len = expansionObject::Gen_Product_With_PreAlloc(2, dex, bey_len, bey, &dexbey, 16);
   double bd_p[16], *bd = bd_p;
   int bd_len = expansionObject::Gen_Diff_With_PreAlloc(bexdey_len, bexdey, dexbey_len, dexbey, &bd, 16);
   double abc1_p[16], *abc1 = abc1_p;
   int abc1_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bc_len, bc, &abc1, 16);
   double abc2_p[16], *abc2 = abc2_p;
   int abc2_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, ac_len, ac, &abc2, 16);
   double abc3_p[16], *abc3 = abc3_p;
   int abc3_len = expansionObject::Gen_Product_With_PreAlloc(2, cez, ab_len, ab, &abc3, 16);
   double abc4_p[16], *abc4 = abc4_p;
   int abc4_len = expansionObject::Gen_Sum_With_PreAlloc(abc1_len, abc1, abc3_len, abc3, &abc4, 16);
   double abc_p[16], *abc = abc_p;
   int abc_len = expansionObject::Gen_Diff_With_PreAlloc(abc4_len, abc4, abc2_len, abc2, &abc, 16);
   double bcd1_p[16], *bcd1 = bcd1_p;
   int bcd1_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, cd_len, cd, &bcd1, 16);
   double bcd2_p[16], *bcd2 = bcd2_p;
   int bcd2_len = expansionObject::Gen_Product_With_PreAlloc(2, cez, bd_len, bd, &bcd2, 16);
   double bcd3_p[16], *bcd3 = bcd3_p;
   int bcd3_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, bc_len, bc, &bcd3, 16);
   double bcd4_p[16], *bcd4 = bcd4_p;
   int bcd4_len = expansionObject::Gen_Sum_With_PreAlloc(bcd1_len, bcd1, bcd3_len, bcd3, &bcd4, 16);
   double bcd_p[16], *bcd = bcd_p;
   int bcd_len = expansionObject::Gen_Diff_With_PreAlloc(bcd4_len, bcd4, bcd2_len, bcd2, &bcd, 16);
   double cda1_p[16], *cda1 = cda1_p;
   int cda1_len = expansionObject::Gen_Product_With_PreAlloc(2, cez, da_len, da, &cda1, 16);
   double cda2_p[16], *cda2 = cda2_p;
   int cda2_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, ac_len, ac, &cda2, 16);
   double cda3_p[16], *cda3 = cda3_p;
   int cda3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, cd_len, cd, &cda3, 16);
   double cda4_p[16], *cda4 = cda4_p;
   int cda4_len = expansionObject::Gen_Sum_With_PreAlloc(cda1_len, cda1, cda3_len, cda3, &cda4, 16);
   double cda_p[16], *cda = cda_p;
   int cda_len = expansionObject::Gen_Sum_With_PreAlloc(cda4_len, cda4, cda2_len, cda2, &cda, 16);
   double dab1_p[16], *dab1 = dab1_p;
   int dab1_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, ab_len, ab, &dab1, 16);
   double dab2_p[16], *dab2 = dab2_p;
   int dab2_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bd_len, bd, &dab2, 16);
   double dab3_p[16], *dab3 = dab3_p;
   int dab3_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, da_len, da, &dab3, 16);
   double dab4_p[16], *dab4 = dab4_p;
   int dab4_len = expansionObject::Gen_Sum_With_PreAlloc(dab1_len, dab1, dab3_len, dab3, &dab4, 16);
   double dab_p[16], *dab = dab_p;
   int dab_len = expansionObject::Gen_Sum_With_PreAlloc(dab4_len, dab4, dab2_len, dab2, &dab, 16);
   double al1_p[16], *al1 = al1_p;
   int al1_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, aex_len, aex, &al1, 16);
   double al2_p[16], *al2 = al2_p;
   int al2_len = expansionObject::Gen_Product_With_PreAlloc(aey_len, aey, aey_len, aey, &al2, 16);
   double al3_p[16], *al3 = al3_p;
   int al3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, aez_len, aez, &al3, 16);
   double al4_p[16], *al4 = al4_p;
   int al4_len = expansionObject::Gen_Sum_With_PreAlloc(al1_len, al1, al2_len, al2, &al4, 16);
   double alift_p[16], *alift = alift_p;
   int alift_len = expansionObject::Gen_Sum_With_PreAlloc(al4_len, al4, al3_len, al3, &alift, 16);
   double bl1_p[16], *bl1 = bl1_p;
   int bl1_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, bex_len, bex, &bl1, 16);
   double bl2_p[16], *bl2 = bl2_p;
   int bl2_len = expansionObject::Gen_Product_With_PreAlloc(bey_len, bey, bey_len, bey, &bl2, 16);
   double bl3_p[16], *bl3 = bl3_p;
   int bl3_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, bez_len, bez, &bl3, 16);
   double bl4_p[16], *bl4 = bl4_p;
   int bl4_len = expansionObject::Gen_Sum_With_PreAlloc(bl1_len, bl1, bl2_len, bl2, &bl4, 16);
   double blift_p[16], *blift = blift_p;
   int blift_len = expansionObject::Gen_Sum_With_PreAlloc(bl4_len, bl4, bl3_len, bl3, &blift, 16);
   double cl1[8];
   int cl1_len = expansionObject::Gen_Product(2, cex, 2, cex, cl1);
   double cl2[8];
   int cl2_len = expansionObject::Gen_Product(2, cey, 2, cey, cl2);
   double cl3[8];
   int cl3_len = expansionObject::Gen_Product(2, cez, 2, cez, cl3);
   double cl4[16];
   int cl4_len = expansionObject::Gen_Sum(cl1_len, cl1, cl2_len, cl2, cl4);
   double clift_p[16], *clift = clift_p;
   int clift_len = expansionObject::Gen_Sum_With_PreAlloc(cl4_len, cl4, cl3_len, cl3, &clift, 16);
   double dl1[8];
   int dl1_len = expansionObject::Gen_Product(2, dex, 2, dex, dl1);
   double dl2[8];
   int dl2_len = expansionObject::Gen_Product(2, dey, 2, dey, dl2);
   double dl3[8];
   int dl3_len = expansionObject::Gen_Product(2, dez, 2, dez, dl3);
   double dl4[16];
   int dl4_len = expansionObject::Gen_Sum(dl1_len, dl1, dl2_len, dl2, dl4);
   double dlift_p[16], *dlift = dlift_p;
   int dlift_len = expansionObject::Gen_Sum_With_PreAlloc(dl4_len, dl4, dl3_len, dl3, &dlift, 16);
   double ds1_p[16], *ds1 = ds1_p;
   int ds1_len = expansionObject::Gen_Product_With_PreAlloc(dlift_len, dlift, abc_len, abc, &ds1, 16);
   double ds2_p[16], *ds2 = ds2_p;
   int ds2_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, dab_len, dab, &ds2, 16);
   double dl_p[16], *dl = dl_p;
   int dl_len = expansionObject::Gen_Diff_With_PreAlloc(ds2_len, ds2, ds1_len, ds1, &dl, 16);
   double dll_p[16], *dll = dll_p;
   int dll_len = expansionObject::Gen_Product_With_PreAlloc(dl_len, dl, d1_len, d1, &dll, 16);
   double dlll_p[16], *dlll = dlll_p;
   int dlll_len = expansionObject::Gen_Product_With_PreAlloc(dll_len, dll, d2_len, d2, &dlll, 16);
   double dr1_p[16], *dr1 = dr1_p;
   int dr1_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cda_len, cda, &dr1, 16);
   double dr12_p[16], *dr12 = dr12_p;
   int dr12_len = expansionObject::Gen_Product_With_PreAlloc(dr1_len, dr1, d1_len, d1, &dr12, 16);
   double dr2_p[16], *dr2 = dr2_p;
   int dr2_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcd_len, bcd, &dr2, 16);
   double dr22_p[16], *dr22 = dr22_p;
   int dr22_len = expansionObject::Gen_Product_With_PreAlloc(dr2_len, dr2, d2_len, d2, &dr22, 16);
   double dr_p[16], *dr = dr_p;
   int dr_len = expansionObject::Gen_Diff_With_PreAlloc(dr22_len, dr22, dr12_len, dr12, &dr, 16);
   double det_p[16], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dlll_len, dlll, dr_len, dr, &det, 16);

   return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (dr_p != dr) FreeDoubles(dr);
   if (dr22_p != dr22) FreeDoubles(dr22);
   if (dr2_p != dr2) FreeDoubles(dr2);
   if (dr12_p != dr12) FreeDoubles(dr12);
   if (dr1_p != dr1) FreeDoubles(dr1);
   if (dlll_p != dlll) FreeDoubles(dlll);
   if (dll_p != dll) FreeDoubles(dll);
   if (dl_p != dl) FreeDoubles(dl);
   if (ds2_p != ds2) FreeDoubles(ds2);
   if (ds1_p != ds1) FreeDoubles(ds1);
   if (dlift_p != dlift) FreeDoubles(dlift);
   if (clift_p != clift) FreeDoubles(clift);
   if (blift_p != blift) FreeDoubles(blift);
   if (bl4_p != bl4) FreeDoubles(bl4);
   if (bl3_p != bl3) FreeDoubles(bl3);
   if (bl2_p != bl2) FreeDoubles(bl2);
   if (bl1_p != bl1) FreeDoubles(bl1);
   if (alift_p != alift) FreeDoubles(alift);
   if (al4_p != al4) FreeDoubles(al4);
   if (al3_p != al3) FreeDoubles(al3);
   if (al2_p != al2) FreeDoubles(al2);
   if (al1_p != al1) FreeDoubles(al1);
   if (dab_p != dab) FreeDoubles(dab);
   if (dab4_p != dab4) FreeDoubles(dab4);
   if (dab3_p != dab3) FreeDoubles(dab3);
   if (dab2_p != dab2) FreeDoubles(dab2);
   if (dab1_p != dab1) FreeDoubles(dab1);
   if (cda_p != cda) FreeDoubles(cda);
   if (cda4_p != cda4) FreeDoubles(cda4);
   if (cda3_p != cda3) FreeDoubles(cda3);
   if (cda2_p != cda2) FreeDoubles(cda2);
   if (cda1_p != cda1) FreeDoubles(cda1);
   if (bcd_p != bcd) FreeDoubles(bcd);
   if (bcd4_p != bcd4) FreeDoubles(bcd4);
   if (bcd3_p != bcd3) FreeDoubles(bcd3);
   if (bcd2_p != bcd2) FreeDoubles(bcd2);
   if (bcd1_p != bcd1) FreeDoubles(bcd1);
   if (abc_p != abc) FreeDoubles(abc);
   if (abc4_p != abc4) FreeDoubles(abc4);
   if (abc3_p != abc3) FreeDoubles(abc3);
   if (abc2_p != abc2) FreeDoubles(abc2);
   if (abc1_p != abc1) FreeDoubles(abc1);
   if (bd_p != bd) FreeDoubles(bd);
   if (dexbey_p != dexbey) FreeDoubles(dexbey);
   if (bexdey_p != bexdey) FreeDoubles(bexdey);
   if (ac_p != ac) FreeDoubles(ac);
   if (cexaey_p != cexaey) FreeDoubles(cexaey);
   if (aexcey_p != aexcey) FreeDoubles(aexcey);
   if (da_p != da) FreeDoubles(da);
   if (aexdey_p != aexdey) FreeDoubles(aexdey);
   if (dexaey_p != dexaey) FreeDoubles(dexaey);
   if (bc_p != bc) FreeDoubles(bc);
   if (cexbey_p != cexbey) FreeDoubles(cexbey);
   if (bexcey_p != bexcey) FreeDoubles(bexcey);
   if (ab_p != ab) FreeDoubles(ab);
   if (bexaey_p != bexaey) FreeDoubles(bexaey);
   if (aexbey_p != aexbey) FreeDoubles(aexbey);
   if (bez_p != bez) FreeDoubles(bez);
   if (bey_p != bey) FreeDoubles(bey);
   if (bex_p != bex) FreeDoubles(bex);
   if (pezd2_p != pezd2) FreeDoubles(pezd2);
   if (peyd2_p != peyd2) FreeDoubles(peyd2);
   if (pexd2_p != pexd2) FreeDoubles(pexd2);
   if (aez_p != aez) FreeDoubles(aez);
   if (aey_p != aey) FreeDoubles(aey);
   if (aex_p != aex) FreeDoubles(aex);
   if (pezd_p != pezd) FreeDoubles(pezd);
   if (peyd_p != peyd) FreeDoubles(peyd);
   if (pexd_p != pexd) FreeDoubles(pexd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = inSphere_IIEEE_bigfloat(p1, p2, pcx, pcy, pcz, pdx, pdy, pdz, pex, pey, pez);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int inSphere_IIEEE(const genericPoint& p1, const genericPoint& p2, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez)
{
   int ret;
   ret = inSphere_IIEEE_interval(p1, p2, pcx, pcy, pcz, pdx, pdy, pdz, pex, pey, pez);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return inSphere_IIEEE_exact(p1, p2, pcx, pcy, pcz, pdx, pdy, pdz, pex, pey, pez);
}

inline int inSphere_IIIEE_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, interval_number pdx, interval_number pdy, interval_number pdz, interval_number pex, interval_number pey, interval_number pez)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   || !p3.getIntervalLambda(l3x, l3y, l3z, d3)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pexd(pex * d1);
   const interval_number peyd(pey * d1);
   const interval_number pezd(pez * d1);
   const interval_number aex(l1x - pexd);
   const interval_number aey(l1y - peyd);
   const interval_number aez(l1z - pezd);
   const interval_number pexd2(pex * d2);
   const interval_number peyd2(pey * d2);
   const interval_number pezd2(pez * d2);
   const interval_number bex(l2x - pexd2);
   const interval_number bey(l2y - peyd2);
   const interval_number bez(l2z - pezd2);
   const interval_number pexd3(pex * d3);
   const interval_number peyd3(pey * d3);
   const interval_number pezd3(pez * d3);
   const interval_number cex(l3x - pexd3);
   const interval_number cey(l3y - peyd3);
   const interval_number cez(l3z - pezd3);
   const interval_number dex(pdx - pex);
   const interval_number dey(pdy - pey);
   const interval_number dez(pdz - pez);
   const interval_number aexbey(aex * bey);
   const interval_number bexaey(bex * aey);
   const interval_number ab(aexbey - bexaey);
   const interval_number bexcey(bex * cey);
   const interval_number cexbey(cex * bey);
   const interval_number bc(bexcey - cexbey);
   const interval_number cexdey(cex * dey);
   const interval_number dexcey(dex * cey);
   const interval_number cd(cexdey - dexcey);
   const interval_number dexaey(dex * aey);
   const interval_number aexdey(aex * dey);
   const interval_number da(dexaey - aexdey);
   const interval_number aexcey(aex * cey);
   const interval_number cexaey(cex * aey);
   const interval_number ac(aexcey - cexaey);
   const interval_number bexdey(bex * dey);
   const interval_number dexbey(dex * bey);
   const interval_number bd(bexdey - dexbey);
   const interval_number abc1(aez * bc);
   const interval_number abc2(bez * ac);
   const interval_number abc3(cez * ab);
   const interval_number abc4(abc1 + abc3);
   const interval_number abc(abc4 - abc2);
   const interval_number bcd1(bez * cd);
   const interval_number bcd2(cez * bd);
   const interval_number bcd3(dez * bc);
   const interval_number bcd4(bcd1 + bcd3);
   const interval_number bcd(bcd4 - bcd2);
   const interval_number cda1(cez * da);
   const interval_number cda2(dez * ac);
   const interval_number cda3(aez * cd);
   const interval_number cda4(cda1 + cda3);
   const interval_number cda(cda4 + cda2);
   const interval_number dab1(dez * ab);
   const interval_number dab2(aez * bd);
   const interval_number dab3(bez * da);
   const interval_number dab4(dab1 + dab3);
   const interval_number dab(dab4 + dab2);
   const interval_number al1(aex * aex);
   const interval_number al2(aey * aey);
   const interval_number al3(aez * aez);
   const interval_number al4(al1 + al2);
   const interval_number alift(al4 + al3);
   const interval_number bl1(bex * bex);
   const interval_number bl2(bey * bey);
   const interval_number bl3(bez * bez);
   const interval_number bl4(bl1 + bl2);
   const interval_number blift(bl4 + bl3);
   const interval_number cl1(cex * cex);
   const interval_number cl2(cey * cey);
   const interval_number cl3(cez * cez);
   const interval_number cl4(cl1 + cl2);
   const interval_number clift(cl4 + cl3);
   const interval_number dl1(dex * dex);
   const interval_number dl2(dey * dey);
   const interval_number dl3(dez * dez);
   const interval_number dl4(dl1 + dl2);
   const interval_number dlift(dl4 + dl3);
   const interval_number ds1(dlift * abc);
   const interval_number ds1n(ds1 * d3);
   const interval_number ds2(clift * dab);
   const interval_number dl(ds2 - ds1n);
   const interval_number dlm(dl * d1);
   const interval_number dln(dlm * d2);
   const interval_number dr1(blift * cda);
   const interval_number dr1n(dr1 * d1);
   const interval_number dr2(alift * bcd);
   const interval_number dr2n(dr2 * d2);
   const interval_number dr(dr2n - dr1n);
   const interval_number drn(dr * d3);
   const interval_number det(dln + drn);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int inSphere_IIIEE_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, bigfloat pdx, bigfloat pdy, bigfloat pdz, bigfloat pex, bigfloat pey, bigfloat pez)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   p3.getBigfloatLambda(l3x, l3y, l3z, d3);
   const bigfloat pexd(pex * d1);
   const bigfloat peyd(pey * d1);
   const bigfloat pezd(pez * d1);
   const bigfloat aex(l1x - pexd);
   const bigfloat aey(l1y - peyd);
   const bigfloat aez(l1z - pezd);
   const bigfloat pexd2(pex * d2);
   const bigfloat peyd2(pey * d2);
   const bigfloat pezd2(pez * d2);
   const bigfloat bex(l2x - pexd2);
   const bigfloat bey(l2y - peyd2);
   const bigfloat bez(l2z - pezd2);
   const bigfloat pexd3(pex * d3);
   const bigfloat peyd3(pey * d3);
   const bigfloat pezd3(pez * d3);
   const bigfloat cex(l3x - pexd3);
   const bigfloat cey(l3y - peyd3);
   const bigfloat cez(l3z - pezd3);
   const bigfloat dex(pdx - pex);
   const bigfloat dey(pdy - pey);
   const bigfloat dez(pdz - pez);
   const bigfloat aexbey(aex * bey);
   const bigfloat bexaey(bex * aey);
   const bigfloat ab(aexbey - bexaey);
   const bigfloat bexcey(bex * cey);
   const bigfloat cexbey(cex * bey);
   const bigfloat bc(bexcey - cexbey);
   const bigfloat cexdey(cex * dey);
   const bigfloat dexcey(dex * cey);
   const bigfloat cd(cexdey - dexcey);
   const bigfloat dexaey(dex * aey);
   const bigfloat aexdey(aex * dey);
   const bigfloat da(dexaey - aexdey);
   const bigfloat aexcey(aex * cey);
   const bigfloat cexaey(cex * aey);
   const bigfloat ac(aexcey - cexaey);
   const bigfloat bexdey(bex * dey);
   const bigfloat dexbey(dex * bey);
   const bigfloat bd(bexdey - dexbey);
   const bigfloat abc1(aez * bc);
   const bigfloat abc2(bez * ac);
   const bigfloat abc3(cez * ab);
   const bigfloat abc4(abc1 + abc3);
   const bigfloat abc(abc4 - abc2);
   const bigfloat bcd1(bez * cd);
   const bigfloat bcd2(cez * bd);
   const bigfloat bcd3(dez * bc);
   const bigfloat bcd4(bcd1 + bcd3);
   const bigfloat bcd(bcd4 - bcd2);
   const bigfloat cda1(cez * da);
   const bigfloat cda2(dez * ac);
   const bigfloat cda3(aez * cd);
   const bigfloat cda4(cda1 + cda3);
   const bigfloat cda(cda4 + cda2);
   const bigfloat dab1(dez * ab);
   const bigfloat dab2(aez * bd);
   const bigfloat dab3(bez * da);
   const bigfloat dab4(dab1 + dab3);
   const bigfloat dab(dab4 + dab2);
   const bigfloat al1(aex * aex);
   const bigfloat al2(aey * aey);
   const bigfloat al3(aez * aez);
   const bigfloat al4(al1 + al2);
   const bigfloat alift(al4 + al3);
   const bigfloat bl1(bex * bex);
   const bigfloat bl2(bey * bey);
   const bigfloat bl3(bez * bez);
   const bigfloat bl4(bl1 + bl2);
   const bigfloat blift(bl4 + bl3);
   const bigfloat cl1(cex * cex);
   const bigfloat cl2(cey * cey);
   const bigfloat cl3(cez * cez);
   const bigfloat cl4(cl1 + cl2);
   const bigfloat clift(cl4 + cl3);
   const bigfloat dl1(dex * dex);
   const bigfloat dl2(dey * dey);
   const bigfloat dl3(dez * dez);
   const bigfloat dl4(dl1 + dl2);
   const bigfloat dlift(dl4 + dl3);
   const bigfloat ds1(dlift * abc);
   const bigfloat ds1n(ds1 * d3);
   const bigfloat ds2(clift * dab);
   const bigfloat dl(ds2 - ds1n);
   const bigfloat dlm(dl * d1);
   const bigfloat dln(dlm * d2);
   const bigfloat dr1(blift * cda);
   const bigfloat dr1n(dr1 * d1);
   const bigfloat dr2(alift * bcd);
   const bigfloat dr2n(dr2 * d2);
   const bigfloat dr(dr2n - dr1n);
   const bigfloat drn(dr * d3);
   const bigfloat det(dln + drn);
   return sgn(det);
}

inline int inSphere_IIIEE_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double pdx, double pdy, double pdz, double pex, double pey, double pez)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[16], *l1x = l1x_p, l1y_p[16], *l1y = l1y_p, l1z_p[16], *l1z = l1z_p, d1_p[16], *d1 = d1_p, l2x_p[16], *l2x = l2x_p, l2y_p[16], *l2y = l2y_p, l2z_p[16], *l2z = l2z_p, d2_p[16], *d2 = d2_p, l3x_p[16], *l3x = l3x_p, l3y_p[16], *l3y = l3y_p, l3z_p[16], *l3z = l3z_p, d3_p[16], *d3 = d3_p;
 int l1x_len = 16, l1y_len = 16, l1z_len = 16, d1_len = 16, l2x_len = 16, l2y_len = 16, l2z_len = 16, d2_len = 16, l3x_len = 16, l3y_len = 16, l3z_len = 16, d3_len = 16;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &l3z, l3z_len, &d3, d3_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0))
 {
   
   double pexd_p[16], *pexd = pexd_p;
   int pexd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pex, &pexd, 16);
   double peyd_p[16], *peyd = peyd_p;
   int peyd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pey, &peyd, 16);
   double pezd_p[16], *pezd = pezd_p;
   int pezd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pez, &pezd, 16);
   double aex_p[16], *aex = aex_p;
   int aex_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pexd_len, pexd, &aex, 16);
   double aey_p[16], *aey = aey_p;
   int aey_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, peyd_len, peyd, &aey, 16);
   double aez_p[16], *aez = aez_p;
   int aez_len = expansionObject::Gen_Diff_With_PreAlloc(l1z_len, l1z, pezd_len, pezd, &aez, 16);
   double pexd2_p[16], *pexd2 = pexd2_p;
   int pexd2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pex, &pexd2, 16);
   double peyd2_p[16], *peyd2 = peyd2_p;
   int peyd2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pey, &peyd2, 16);
   double pezd2_p[16], *pezd2 = pezd2_p;
   int pezd2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pez, &pezd2, 16);
   double bex_p[16], *bex = bex_p;
   int bex_len = expansionObject::Gen_Diff_With_PreAlloc(l2x_len, l2x, pexd2_len, pexd2, &bex, 16);
   double bey_p[16], *bey = bey_p;
   int bey_len = expansionObject::Gen_Diff_With_PreAlloc(l2y_len, l2y, peyd2_len, peyd2, &bey, 16);
   double bez_p[16], *bez = bez_p;
   int bez_len = expansionObject::Gen_Diff_With_PreAlloc(l2z_len, l2z, pezd2_len, pezd2, &bez, 16);
   double pexd3_p[16], *pexd3 = pexd3_p;
   int pexd3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pex, &pexd3, 16);
   double peyd3_p[16], *peyd3 = peyd3_p;
   int peyd3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pey, &peyd3, 16);
   double pezd3_p[16], *pezd3 = pezd3_p;
   int pezd3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pez, &pezd3, 16);
   double cex_p[16], *cex = cex_p;
   int cex_len = expansionObject::Gen_Diff_With_PreAlloc(l3x_len, l3x, pexd3_len, pexd3, &cex, 16);
   double cey_p[16], *cey = cey_p;
   int cey_len = expansionObject::Gen_Diff_With_PreAlloc(l3y_len, l3y, peyd3_len, peyd3, &cey, 16);
   double cez_p[16], *cez = cez_p;
   int cez_len = expansionObject::Gen_Diff_With_PreAlloc(l3z_len, l3z, pezd3_len, pezd3, &cez, 16);
   double dex[2];
   expansionObject::two_Diff(pdx, pex, dex);
   double dey[2];
   expansionObject::two_Diff(pdy, pey, dey);
   double dez[2];
   expansionObject::two_Diff(pdz, pez, dez);
   double aexbey_p[16], *aexbey = aexbey_p;
   int aexbey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, bey_len, bey, &aexbey, 16);
   double bexaey_p[16], *bexaey = bexaey_p;
   int bexaey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, aey_len, aey, &bexaey, 16);
   double ab_p[16], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(aexbey_len, aexbey, bexaey_len, bexaey, &ab, 16);
   double bexcey_p[16], *bexcey = bexcey_p;
   int bexcey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, cey_len, cey, &bexcey, 16);
   double cexbey_p[16], *cexbey = cexbey_p;
   int cexbey_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, bey_len, bey, &cexbey, 16);
   double bc_p[16], *bc = bc_p;
   int bc_len = expansionObject::Gen_Diff_With_PreAlloc(bexcey_len, bexcey, cexbey_len, cexbey, &bc, 16);
   double cexdey_p[16], *cexdey = cexdey_p;
   int cexdey_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, 2, dey, &cexdey, 16);
   double dexcey_p[16], *dexcey = dexcey_p;
   int dexcey_len = expansionObject::Gen_Product_With_PreAlloc(2, dex, cey_len, cey, &dexcey, 16);
   double cd_p[16], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(cexdey_len, cexdey, dexcey_len, dexcey, &cd, 16);
   double dexaey_p[16], *dexaey = dexaey_p;
   int dexaey_len = expansionObject::Gen_Product_With_PreAlloc(2, dex, aey_len, aey, &dexaey, 16);
   double aexdey_p[16], *aexdey = aexdey_p;
   int aexdey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, 2, dey, &aexdey, 16);
   double da_p[16], *da = da_p;
   int da_len = expansionObject::Gen_Diff_With_PreAlloc(dexaey_len, dexaey, aexdey_len, aexdey, &da, 16);
   double aexcey_p[16], *aexcey = aexcey_p;
   int aexcey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, cey_len, cey, &aexcey, 16);
   double cexaey_p[16], *cexaey = cexaey_p;
   int cexaey_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, aey_len, aey, &cexaey, 16);
   double ac_p[16], *ac = ac_p;
   int ac_len = expansionObject::Gen_Diff_With_PreAlloc(aexcey_len, aexcey, cexaey_len, cexaey, &ac, 16);
   double bexdey_p[16], *bexdey = bexdey_p;
   int bexdey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, 2, dey, &bexdey, 16);
   double dexbey_p[16], *dexbey = dexbey_p;
   int dexbey_len = expansionObject::Gen_Product_With_PreAlloc(2, dex, bey_len, bey, &dexbey, 16);
   double bd_p[16], *bd = bd_p;
   int bd_len = expansionObject::Gen_Diff_With_PreAlloc(bexdey_len, bexdey, dexbey_len, dexbey, &bd, 16);
   double abc1_p[16], *abc1 = abc1_p;
   int abc1_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bc_len, bc, &abc1, 16);
   double abc2_p[16], *abc2 = abc2_p;
   int abc2_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, ac_len, ac, &abc2, 16);
   double abc3_p[16], *abc3 = abc3_p;
   int abc3_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, ab_len, ab, &abc3, 16);
   double abc4_p[16], *abc4 = abc4_p;
   int abc4_len = expansionObject::Gen_Sum_With_PreAlloc(abc1_len, abc1, abc3_len, abc3, &abc4, 16);
   double abc_p[16], *abc = abc_p;
   int abc_len = expansionObject::Gen_Diff_With_PreAlloc(abc4_len, abc4, abc2_len, abc2, &abc, 16);
   double bcd1_p[16], *bcd1 = bcd1_p;
   int bcd1_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, cd_len, cd, &bcd1, 16);
   double bcd2_p[16], *bcd2 = bcd2_p;
   int bcd2_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, bd_len, bd, &bcd2, 16);
   double bcd3_p[16], *bcd3 = bcd3_p;
   int bcd3_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, bc_len, bc, &bcd3, 16);
   double bcd4_p[16], *bcd4 = bcd4_p;
   int bcd4_len = expansionObject::Gen_Sum_With_PreAlloc(bcd1_len, bcd1, bcd3_len, bcd3, &bcd4, 16);
   double bcd_p[16], *bcd = bcd_p;
   int bcd_len = expansionObject::Gen_Diff_With_PreAlloc(bcd4_len, bcd4, bcd2_len, bcd2, &bcd, 16);
   double cda1_p[16], *cda1 = cda1_p;
   int cda1_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, da_len, da, &cda1, 16);
   double cda2_p[16], *cda2 = cda2_p;
   int cda2_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, ac_len, ac, &cda2, 16);
   double cda3_p[16], *cda3 = cda3_p;
   int cda3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, cd_len, cd, &cda3, 16);
   double cda4_p[16], *cda4 = cda4_p;
   int cda4_len = expansionObject::Gen_Sum_With_PreAlloc(cda1_len, cda1, cda3_len, cda3, &cda4, 16);
   double cda_p[16], *cda = cda_p;
   int cda_len = expansionObject::Gen_Sum_With_PreAlloc(cda4_len, cda4, cda2_len, cda2, &cda, 16);
   double dab1_p[16], *dab1 = dab1_p;
   int dab1_len = expansionObject::Gen_Product_With_PreAlloc(2, dez, ab_len, ab, &dab1, 16);
   double dab2_p[16], *dab2 = dab2_p;
   int dab2_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bd_len, bd, &dab2, 16);
   double dab3_p[16], *dab3 = dab3_p;
   int dab3_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, da_len, da, &dab3, 16);
   double dab4_p[16], *dab4 = dab4_p;
   int dab4_len = expansionObject::Gen_Sum_With_PreAlloc(dab1_len, dab1, dab3_len, dab3, &dab4, 16);
   double dab_p[16], *dab = dab_p;
   int dab_len = expansionObject::Gen_Sum_With_PreAlloc(dab4_len, dab4, dab2_len, dab2, &dab, 16);
   double al1_p[16], *al1 = al1_p;
   int al1_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, aex_len, aex, &al1, 16);
   double al2_p[16], *al2 = al2_p;
   int al2_len = expansionObject::Gen_Product_With_PreAlloc(aey_len, aey, aey_len, aey, &al2, 16);
   double al3_p[16], *al3 = al3_p;
   int al3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, aez_len, aez, &al3, 16);
   double al4_p[16], *al4 = al4_p;
   int al4_len = expansionObject::Gen_Sum_With_PreAlloc(al1_len, al1, al2_len, al2, &al4, 16);
   double alift_p[16], *alift = alift_p;
   int alift_len = expansionObject::Gen_Sum_With_PreAlloc(al4_len, al4, al3_len, al3, &alift, 16);
   double bl1_p[16], *bl1 = bl1_p;
   int bl1_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, bex_len, bex, &bl1, 16);
   double bl2_p[16], *bl2 = bl2_p;
   int bl2_len = expansionObject::Gen_Product_With_PreAlloc(bey_len, bey, bey_len, bey, &bl2, 16);
   double bl3_p[16], *bl3 = bl3_p;
   int bl3_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, bez_len, bez, &bl3, 16);
   double bl4_p[16], *bl4 = bl4_p;
   int bl4_len = expansionObject::Gen_Sum_With_PreAlloc(bl1_len, bl1, bl2_len, bl2, &bl4, 16);
   double blift_p[16], *blift = blift_p;
   int blift_len = expansionObject::Gen_Sum_With_PreAlloc(bl4_len, bl4, bl3_len, bl3, &blift, 16);
   double cl1_p[16], *cl1 = cl1_p;
   int cl1_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, cex_len, cex, &cl1, 16);
   double cl2_p[16], *cl2 = cl2_p;
   int cl2_len = expansionObject::Gen_Product_With_PreAlloc(cey_len, cey, cey_len, cey, &cl2, 16);
   double cl3_p[16], *cl3 = cl3_p;
   int cl3_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, cez_len, cez, &cl3, 16);
   double cl4_p[16], *cl4 = cl4_p;
   int cl4_len = expansionObject::Gen_Sum_With_PreAlloc(cl1_len, cl1, cl2_len, cl2, &cl4, 16);
   double clift_p[16], *clift = clift_p;
   int clift_len = expansionObject::Gen_Sum_With_PreAlloc(cl4_len, cl4, cl3_len, cl3, &clift, 16);
   double dl1[8];
   int dl1_len = expansionObject::Gen_Product(2, dex, 2, dex, dl1);
   double dl2[8];
   int dl2_len = expansionObject::Gen_Product(2, dey, 2, dey, dl2);
   double dl3[8];
   int dl3_len = expansionObject::Gen_Product(2, dez, 2, dez, dl3);
   double dl4[16];
   int dl4_len = expansionObject::Gen_Sum(dl1_len, dl1, dl2_len, dl2, dl4);
   double dlift_p[16], *dlift = dlift_p;
   int dlift_len = expansionObject::Gen_Sum_With_PreAlloc(dl4_len, dl4, dl3_len, dl3, &dlift, 16);
   double ds1_p[16], *ds1 = ds1_p;
   int ds1_len = expansionObject::Gen_Product_With_PreAlloc(dlift_len, dlift, abc_len, abc, &ds1, 16);
   double ds1n_p[16], *ds1n = ds1n_p;
   int ds1n_len = expansionObject::Gen_Product_With_PreAlloc(ds1_len, ds1, d3_len, d3, &ds1n, 16);
   double ds2_p[16], *ds2 = ds2_p;
   int ds2_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, dab_len, dab, &ds2, 16);
   double dl_p[16], *dl = dl_p;
   int dl_len = expansionObject::Gen_Diff_With_PreAlloc(ds2_len, ds2, ds1n_len, ds1n, &dl, 16);
   double dlm_p[16], *dlm = dlm_p;
   int dlm_len = expansionObject::Gen_Product_With_PreAlloc(dl_len, dl, d1_len, d1, &dlm, 16);
   double dln_p[16], *dln = dln_p;
   int dln_len = expansionObject::Gen_Product_With_PreAlloc(dlm_len, dlm, d2_len, d2, &dln, 16);
   double dr1_p[16], *dr1 = dr1_p;
   int dr1_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cda_len, cda, &dr1, 16);
   double dr1n_p[16], *dr1n = dr1n_p;
   int dr1n_len = expansionObject::Gen_Product_With_PreAlloc(dr1_len, dr1, d1_len, d1, &dr1n, 16);
   double dr2_p[16], *dr2 = dr2_p;
   int dr2_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcd_len, bcd, &dr2, 16);
   double dr2n_p[16], *dr2n = dr2n_p;
   int dr2n_len = expansionObject::Gen_Product_With_PreAlloc(dr2_len, dr2, d2_len, d2, &dr2n, 16);
   double dr_p[16], *dr = dr_p;
   int dr_len = expansionObject::Gen_Diff_With_PreAlloc(dr2n_len, dr2n, dr1n_len, dr1n, &dr, 16);
   double drn_p[16], *drn = drn_p;
   int drn_len = expansionObject::Gen_Product_With_PreAlloc(dr_len, dr, d3_len, d3, &drn, 16);
   double det_p[16], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dln_len, dln, drn_len, drn, &det, 16);

   return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (drn_p != drn) FreeDoubles(drn);
   if (dr_p != dr) FreeDoubles(dr);
   if (dr2n_p != dr2n) FreeDoubles(dr2n);
   if (dr2_p != dr2) FreeDoubles(dr2);
   if (dr1n_p != dr1n) FreeDoubles(dr1n);
   if (dr1_p != dr1) FreeDoubles(dr1);
   if (dln_p != dln) FreeDoubles(dln);
   if (dlm_p != dlm) FreeDoubles(dlm);
   if (dl_p != dl) FreeDoubles(dl);
   if (ds2_p != ds2) FreeDoubles(ds2);
   if (ds1n_p != ds1n) FreeDoubles(ds1n);
   if (ds1_p != ds1) FreeDoubles(ds1);
   if (dlift_p != dlift) FreeDoubles(dlift);
   if (clift_p != clift) FreeDoubles(clift);
   if (cl4_p != cl4) FreeDoubles(cl4);
   if (cl3_p != cl3) FreeDoubles(cl3);
   if (cl2_p != cl2) FreeDoubles(cl2);
   if (cl1_p != cl1) FreeDoubles(cl1);
   if (blift_p != blift) FreeDoubles(blift);
   if (bl4_p != bl4) FreeDoubles(bl4);
   if (bl3_p != bl3) FreeDoubles(bl3);
   if (bl2_p != bl2) FreeDoubles(bl2);
   if (bl1_p != bl1) FreeDoubles(bl1);
   if (alift_p != alift) FreeDoubles(alift);
   if (al4_p != al4) FreeDoubles(al4);
   if (al3_p != al3) FreeDoubles(al3);
   if (al2_p != al2) FreeDoubles(al2);
   if (al1_p != al1) FreeDoubles(al1);
   if (dab_p != dab) FreeDoubles(dab);
   if (dab4_p != dab4) FreeDoubles(dab4);
   if (dab3_p != dab3) FreeDoubles(dab3);
   if (dab2_p != dab2) FreeDoubles(dab2);
   if (dab1_p != dab1) FreeDoubles(dab1);
   if (cda_p != cda) FreeDoubles(cda);
   if (cda4_p != cda4) FreeDoubles(cda4);
   if (cda3_p != cda3) FreeDoubles(cda3);
   if (cda2_p != cda2) FreeDoubles(cda2);
   if (cda1_p != cda1) FreeDoubles(cda1);
   if (bcd_p != bcd) FreeDoubles(bcd);
   if (bcd4_p != bcd4) FreeDoubles(bcd4);
   if (bcd3_p != bcd3) FreeDoubles(bcd3);
   if (bcd2_p != bcd2) FreeDoubles(bcd2);
   if (bcd1_p != bcd1) FreeDoubles(bcd1);
   if (abc_p != abc) FreeDoubles(abc);
   if (abc4_p != abc4) FreeDoubles(abc4);
   if (abc3_p != abc3) FreeDoubles(abc3);
   if (abc2_p != abc2) FreeDoubles(abc2);
   if (abc1_p != abc1) FreeDoubles(abc1);
   if (bd_p != bd) FreeDoubles(bd);
   if (dexbey_p != dexbey) FreeDoubles(dexbey);
   if (bexdey_p != bexdey) FreeDoubles(bexdey);
   if (ac_p != ac) FreeDoubles(ac);
   if (cexaey_p != cexaey) FreeDoubles(cexaey);
   if (aexcey_p != aexcey) FreeDoubles(aexcey);
   if (da_p != da) FreeDoubles(da);
   if (aexdey_p != aexdey) FreeDoubles(aexdey);
   if (dexaey_p != dexaey) FreeDoubles(dexaey);
   if (cd_p != cd) FreeDoubles(cd);
   if (dexcey_p != dexcey) FreeDoubles(dexcey);
   if (cexdey_p != cexdey) FreeDoubles(cexdey);
   if (bc_p != bc) FreeDoubles(bc);
   if (cexbey_p != cexbey) FreeDoubles(cexbey);
   if (bexcey_p != bexcey) FreeDoubles(bexcey);
   if (ab_p != ab) FreeDoubles(ab);
   if (bexaey_p != bexaey) FreeDoubles(bexaey);
   if (aexbey_p != aexbey) FreeDoubles(aexbey);
   if (cez_p != cez) FreeDoubles(cez);
   if (cey_p != cey) FreeDoubles(cey);
   if (cex_p != cex) FreeDoubles(cex);
   if (pezd3_p != pezd3) FreeDoubles(pezd3);
   if (peyd3_p != peyd3) FreeDoubles(peyd3);
   if (pexd3_p != pexd3) FreeDoubles(pexd3);
   if (bez_p != bez) FreeDoubles(bez);
   if (bey_p != bey) FreeDoubles(bey);
   if (bex_p != bex) FreeDoubles(bex);
   if (pezd2_p != pezd2) FreeDoubles(pezd2);
   if (peyd2_p != peyd2) FreeDoubles(peyd2);
   if (pexd2_p != pexd2) FreeDoubles(pexd2);
   if (aez_p != aez) FreeDoubles(aez);
   if (aey_p != aey) FreeDoubles(aey);
   if (aex_p != aex) FreeDoubles(aex);
   if (pezd_p != pezd) FreeDoubles(pezd);
   if (peyd_p != peyd) FreeDoubles(peyd);
   if (pexd_p != pexd) FreeDoubles(pexd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = inSphere_IIIEE_bigfloat(p1, p2, p3, pdx, pdy, pdz, pex, pey, pez);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (d3_p != d3) FreeDoubles(d3);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int inSphere_IIIEE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double pdx, double pdy, double pdz, double pex, double pey, double pez)
{
   int ret;
   ret = inSphere_IIIEE_interval(p1, p2, p3, pdx, pdy, pdz, pex, pey, pez);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return inSphere_IIIEE_exact(p1, p2, p3, pdx, pdy, pdz, pex, pey, pez);
}

inline int inSphere_IIIIE_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, interval_number pex, interval_number pey, interval_number pez)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3, l4x, l4y, l4z, d4;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   || !p3.getIntervalLambda(l3x, l3y, l3z, d3)
   || !p4.getIntervalLambda(l4x, l4y, l4z, d4)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pexd(pex * d1);
   const interval_number peyd(pey * d1);
   const interval_number pezd(pez * d1);
   const interval_number aex(l1x - pexd);
   const interval_number aey(l1y - peyd);
   const interval_number aez(l1z - pezd);
   const interval_number pexd2(pex * d2);
   const interval_number peyd2(pey * d2);
   const interval_number pezd2(pez * d2);
   const interval_number bex(l2x - pexd2);
   const interval_number bey(l2y - peyd2);
   const interval_number bez(l2z - pezd2);
   const interval_number pexd3(pex * d3);
   const interval_number peyd3(pey * d3);
   const interval_number pezd3(pez * d3);
   const interval_number cex(l3x - pexd3);
   const interval_number cey(l3y - peyd3);
   const interval_number cez(l3z - pezd3);
   const interval_number pexd4(pex * d4);
   const interval_number peyd4(pey * d4);
   const interval_number pezd4(pez * d4);
   const interval_number dex(l4x - pexd4);
   const interval_number dey(l4y - peyd4);
   const interval_number dez(l4z - pezd4);
   const interval_number aexbey(aex * bey);
   const interval_number bexaey(bex * aey);
   const interval_number ab(aexbey - bexaey);
   const interval_number bexcey(bex * cey);
   const interval_number cexbey(cex * bey);
   const interval_number bc(bexcey - cexbey);
   const interval_number cexdey(cex * dey);
   const interval_number dexcey(dex * cey);
   const interval_number cd(cexdey - dexcey);
   const interval_number dexaey(dex * aey);
   const interval_number aexdey(aex * dey);
   const interval_number da(dexaey - aexdey);
   const interval_number aexcey(aex * cey);
   const interval_number cexaey(cex * aey);
   const interval_number ac(aexcey - cexaey);
   const interval_number bexdey(bex * dey);
   const interval_number dexbey(dex * bey);
   const interval_number bd(bexdey - dexbey);
   const interval_number abc1(aez * bc);
   const interval_number abc2(bez * ac);
   const interval_number abc3(cez * ab);
   const interval_number abc4(abc1 + abc3);
   const interval_number abc(abc4 - abc2);
   const interval_number bcd1(bez * cd);
   const interval_number bcd2(cez * bd);
   const interval_number bcd3(dez * bc);
   const interval_number bcd4(bcd1 + bcd3);
   const interval_number bcd(bcd4 - bcd2);
   const interval_number cda1(cez * da);
   const interval_number cda2(dez * ac);
   const interval_number cda3(aez * cd);
   const interval_number cda4(cda1 + cda3);
   const interval_number cda(cda4 + cda2);
   const interval_number dab1(dez * ab);
   const interval_number dab2(aez * bd);
   const interval_number dab3(bez * da);
   const interval_number dab4(dab1 + dab3);
   const interval_number dab(dab4 + dab2);
   const interval_number al1(aex * aex);
   const interval_number al2(aey * aey);
   const interval_number al3(aez * aez);
   const interval_number al4(al1 + al2);
   const interval_number alift(al4 + al3);
   const interval_number bl1(bex * bex);
   const interval_number bl2(bey * bey);
   const interval_number bl3(bez * bez);
   const interval_number bl4(bl1 + bl2);
   const interval_number blift(bl4 + bl3);
   const interval_number cl1(cex * cex);
   const interval_number cl2(cey * cey);
   const interval_number cl3(cez * cez);
   const interval_number cl4(cl1 + cl2);
   const interval_number clift(cl4 + cl3);
   const interval_number dl1(dex * dex);
   const interval_number dl2(dey * dey);
   const interval_number dl3(dez * dez);
   const interval_number dl4(dl1 + dl2);
   const interval_number dlift(dl4 + dl3);
   const interval_number ds1(dlift * abc);
   const interval_number ds12(ds1 * d3);
   const interval_number ds2(clift * dab);
   const interval_number ds22(ds2 * d4);
   const interval_number dl(ds22 - ds12);
   const interval_number dlx1(dl * d1);
   const interval_number dlx2(dlx1 * d2);
   const interval_number dr1(blift * cda);
   const interval_number dr12(dr1 * d1);
   const interval_number dr2(alift * bcd);
   const interval_number dr22(dr2 * d2);
   const interval_number dr(dr22 - dr12);
   const interval_number drx1(dr * d3);
   const interval_number drx2(drx1 * d4);
   const interval_number det(dlx2 + drx2);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int inSphere_IIIIE_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, bigfloat pex, bigfloat pey, bigfloat pez)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3, l4x, l4y, l4z, d4;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   p3.getBigfloatLambda(l3x, l3y, l3z, d3);
   p4.getBigfloatLambda(l4x, l4y, l4z, d4);
   const bigfloat pexd(pex * d1);
   const bigfloat peyd(pey * d1);
   const bigfloat pezd(pez * d1);
   const bigfloat aex(l1x - pexd);
   const bigfloat aey(l1y - peyd);
   const bigfloat aez(l1z - pezd);
   const bigfloat pexd2(pex * d2);
   const bigfloat peyd2(pey * d2);
   const bigfloat pezd2(pez * d2);
   const bigfloat bex(l2x - pexd2);
   const bigfloat bey(l2y - peyd2);
   const bigfloat bez(l2z - pezd2);
   const bigfloat pexd3(pex * d3);
   const bigfloat peyd3(pey * d3);
   const bigfloat pezd3(pez * d3);
   const bigfloat cex(l3x - pexd3);
   const bigfloat cey(l3y - peyd3);
   const bigfloat cez(l3z - pezd3);
   const bigfloat pexd4(pex * d4);
   const bigfloat peyd4(pey * d4);
   const bigfloat pezd4(pez * d4);
   const bigfloat dex(l4x - pexd4);
   const bigfloat dey(l4y - peyd4);
   const bigfloat dez(l4z - pezd4);
   const bigfloat aexbey(aex * bey);
   const bigfloat bexaey(bex * aey);
   const bigfloat ab(aexbey - bexaey);
   const bigfloat bexcey(bex * cey);
   const bigfloat cexbey(cex * bey);
   const bigfloat bc(bexcey - cexbey);
   const bigfloat cexdey(cex * dey);
   const bigfloat dexcey(dex * cey);
   const bigfloat cd(cexdey - dexcey);
   const bigfloat dexaey(dex * aey);
   const bigfloat aexdey(aex * dey);
   const bigfloat da(dexaey - aexdey);
   const bigfloat aexcey(aex * cey);
   const bigfloat cexaey(cex * aey);
   const bigfloat ac(aexcey - cexaey);
   const bigfloat bexdey(bex * dey);
   const bigfloat dexbey(dex * bey);
   const bigfloat bd(bexdey - dexbey);
   const bigfloat abc1(aez * bc);
   const bigfloat abc2(bez * ac);
   const bigfloat abc3(cez * ab);
   const bigfloat abc4(abc1 + abc3);
   const bigfloat abc(abc4 - abc2);
   const bigfloat bcd1(bez * cd);
   const bigfloat bcd2(cez * bd);
   const bigfloat bcd3(dez * bc);
   const bigfloat bcd4(bcd1 + bcd3);
   const bigfloat bcd(bcd4 - bcd2);
   const bigfloat cda1(cez * da);
   const bigfloat cda2(dez * ac);
   const bigfloat cda3(aez * cd);
   const bigfloat cda4(cda1 + cda3);
   const bigfloat cda(cda4 + cda2);
   const bigfloat dab1(dez * ab);
   const bigfloat dab2(aez * bd);
   const bigfloat dab3(bez * da);
   const bigfloat dab4(dab1 + dab3);
   const bigfloat dab(dab4 + dab2);
   const bigfloat al1(aex * aex);
   const bigfloat al2(aey * aey);
   const bigfloat al3(aez * aez);
   const bigfloat al4(al1 + al2);
   const bigfloat alift(al4 + al3);
   const bigfloat bl1(bex * bex);
   const bigfloat bl2(bey * bey);
   const bigfloat bl3(bez * bez);
   const bigfloat bl4(bl1 + bl2);
   const bigfloat blift(bl4 + bl3);
   const bigfloat cl1(cex * cex);
   const bigfloat cl2(cey * cey);
   const bigfloat cl3(cez * cez);
   const bigfloat cl4(cl1 + cl2);
   const bigfloat clift(cl4 + cl3);
   const bigfloat dl1(dex * dex);
   const bigfloat dl2(dey * dey);
   const bigfloat dl3(dez * dez);
   const bigfloat dl4(dl1 + dl2);
   const bigfloat dlift(dl4 + dl3);
   const bigfloat ds1(dlift * abc);
   const bigfloat ds12(ds1 * d3);
   const bigfloat ds2(clift * dab);
   const bigfloat ds22(ds2 * d4);
   const bigfloat dl(ds22 - ds12);
   const bigfloat dlx1(dl * d1);
   const bigfloat dlx2(dlx1 * d2);
   const bigfloat dr1(blift * cda);
   const bigfloat dr12(dr1 * d1);
   const bigfloat dr2(alift * bcd);
   const bigfloat dr22(dr2 * d2);
   const bigfloat dr(dr22 - dr12);
   const bigfloat drx1(dr * d3);
   const bigfloat drx2(drx1 * d4);
   const bigfloat det(dlx2 + drx2);
   return sgn(det);
}

inline int inSphere_IIIIE_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, double pex, double pey, double pez)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[16], *l1x = l1x_p, l1y_p[16], *l1y = l1y_p, l1z_p[16], *l1z = l1z_p, d1_p[16], *d1 = d1_p, l2x_p[16], *l2x = l2x_p, l2y_p[16], *l2y = l2y_p, l2z_p[16], *l2z = l2z_p, d2_p[16], *d2 = d2_p, l3x_p[16], *l3x = l3x_p, l3y_p[16], *l3y = l3y_p, l3z_p[16], *l3z = l3z_p, d3_p[16], *d3 = d3_p, l4x_p[16], *l4x = l4x_p, l4y_p[16], *l4y = l4y_p, l4z_p[16], *l4z = l4z_p, d4_p[16], *d4 = d4_p;
 int l1x_len = 16, l1y_len = 16, l1z_len = 16, d1_len = 16, l2x_len = 16, l2y_len = 16, l2z_len = 16, d2_len = 16, l3x_len = 16, l3y_len = 16, l3z_len = 16, d3_len = 16, l4x_len = 16, l4y_len = 16, l4z_len = 16, d4_len = 16;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &l3z, l3z_len, &d3, d3_len);
 p4.getExactLambda(&l4x, l4x_len, &l4y, l4y_len, &l4z, l4z_len, &d4, d4_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0) && (d4[d4_len - 1] != 0))
 {
   
   double pexd_p[16], *pexd = pexd_p;
   int pexd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pex, &pexd, 16);
   double peyd_p[16], *peyd = peyd_p;
   int peyd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pey, &peyd, 16);
   double pezd_p[16], *pezd = pezd_p;
   int pezd_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, pez, &pezd, 16);
   double aex_p[16], *aex = aex_p;
   int aex_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, pexd_len, pexd, &aex, 16);
   double aey_p[16], *aey = aey_p;
   int aey_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, peyd_len, peyd, &aey, 16);
   double aez_p[16], *aez = aez_p;
   int aez_len = expansionObject::Gen_Diff_With_PreAlloc(l1z_len, l1z, pezd_len, pezd, &aez, 16);
   double pexd2_p[16], *pexd2 = pexd2_p;
   int pexd2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pex, &pexd2, 16);
   double peyd2_p[16], *peyd2 = peyd2_p;
   int peyd2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pey, &peyd2, 16);
   double pezd2_p[16], *pezd2 = pezd2_p;
   int pezd2_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, pez, &pezd2, 16);
   double bex_p[16], *bex = bex_p;
   int bex_len = expansionObject::Gen_Diff_With_PreAlloc(l2x_len, l2x, pexd2_len, pexd2, &bex, 16);
   double bey_p[16], *bey = bey_p;
   int bey_len = expansionObject::Gen_Diff_With_PreAlloc(l2y_len, l2y, peyd2_len, peyd2, &bey, 16);
   double bez_p[16], *bez = bez_p;
   int bez_len = expansionObject::Gen_Diff_With_PreAlloc(l2z_len, l2z, pezd2_len, pezd2, &bez, 16);
   double pexd3_p[16], *pexd3 = pexd3_p;
   int pexd3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pex, &pexd3, 16);
   double peyd3_p[16], *peyd3 = peyd3_p;
   int peyd3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pey, &peyd3, 16);
   double pezd3_p[16], *pezd3 = pezd3_p;
   int pezd3_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, pez, &pezd3, 16);
   double cex_p[16], *cex = cex_p;
   int cex_len = expansionObject::Gen_Diff_With_PreAlloc(l3x_len, l3x, pexd3_len, pexd3, &cex, 16);
   double cey_p[16], *cey = cey_p;
   int cey_len = expansionObject::Gen_Diff_With_PreAlloc(l3y_len, l3y, peyd3_len, peyd3, &cey, 16);
   double cez_p[16], *cez = cez_p;
   int cez_len = expansionObject::Gen_Diff_With_PreAlloc(l3z_len, l3z, pezd3_len, pezd3, &cez, 16);
   double pexd4_p[16], *pexd4 = pexd4_p;
   int pexd4_len = expansionObject::Gen_Scale_With_PreAlloc(d4_len, d4, pex, &pexd4, 16);
   double peyd4_p[16], *peyd4 = peyd4_p;
   int peyd4_len = expansionObject::Gen_Scale_With_PreAlloc(d4_len, d4, pey, &peyd4, 16);
   double pezd4_p[16], *pezd4 = pezd4_p;
   int pezd4_len = expansionObject::Gen_Scale_With_PreAlloc(d4_len, d4, pez, &pezd4, 16);
   double dex_p[16], *dex = dex_p;
   int dex_len = expansionObject::Gen_Diff_With_PreAlloc(l4x_len, l4x, pexd4_len, pexd4, &dex, 16);
   double dey_p[16], *dey = dey_p;
   int dey_len = expansionObject::Gen_Diff_With_PreAlloc(l4y_len, l4y, peyd4_len, peyd4, &dey, 16);
   double dez_p[16], *dez = dez_p;
   int dez_len = expansionObject::Gen_Diff_With_PreAlloc(l4z_len, l4z, pezd4_len, pezd4, &dez, 16);
   double aexbey_p[16], *aexbey = aexbey_p;
   int aexbey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, bey_len, bey, &aexbey, 16);
   double bexaey_p[16], *bexaey = bexaey_p;
   int bexaey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, aey_len, aey, &bexaey, 16);
   double ab_p[16], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(aexbey_len, aexbey, bexaey_len, bexaey, &ab, 16);
   double bexcey_p[16], *bexcey = bexcey_p;
   int bexcey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, cey_len, cey, &bexcey, 16);
   double cexbey_p[16], *cexbey = cexbey_p;
   int cexbey_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, bey_len, bey, &cexbey, 16);
   double bc_p[16], *bc = bc_p;
   int bc_len = expansionObject::Gen_Diff_With_PreAlloc(bexcey_len, bexcey, cexbey_len, cexbey, &bc, 16);
   double cexdey_p[16], *cexdey = cexdey_p;
   int cexdey_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, dey_len, dey, &cexdey, 16);
   double dexcey_p[16], *dexcey = dexcey_p;
   int dexcey_len = expansionObject::Gen_Product_With_PreAlloc(dex_len, dex, cey_len, cey, &dexcey, 16);
   double cd_p[16], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(cexdey_len, cexdey, dexcey_len, dexcey, &cd, 16);
   double dexaey_p[16], *dexaey = dexaey_p;
   int dexaey_len = expansionObject::Gen_Product_With_PreAlloc(dex_len, dex, aey_len, aey, &dexaey, 16);
   double aexdey_p[16], *aexdey = aexdey_p;
   int aexdey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, dey_len, dey, &aexdey, 16);
   double da_p[16], *da = da_p;
   int da_len = expansionObject::Gen_Diff_With_PreAlloc(dexaey_len, dexaey, aexdey_len, aexdey, &da, 16);
   double aexcey_p[16], *aexcey = aexcey_p;
   int aexcey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, cey_len, cey, &aexcey, 16);
   double cexaey_p[16], *cexaey = cexaey_p;
   int cexaey_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, aey_len, aey, &cexaey, 16);
   double ac_p[16], *ac = ac_p;
   int ac_len = expansionObject::Gen_Diff_With_PreAlloc(aexcey_len, aexcey, cexaey_len, cexaey, &ac, 16);
   double bexdey_p[16], *bexdey = bexdey_p;
   int bexdey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, dey_len, dey, &bexdey, 16);
   double dexbey_p[16], *dexbey = dexbey_p;
   int dexbey_len = expansionObject::Gen_Product_With_PreAlloc(dex_len, dex, bey_len, bey, &dexbey, 16);
   double bd_p[16], *bd = bd_p;
   int bd_len = expansionObject::Gen_Diff_With_PreAlloc(bexdey_len, bexdey, dexbey_len, dexbey, &bd, 16);
   double abc1_p[16], *abc1 = abc1_p;
   int abc1_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bc_len, bc, &abc1, 16);
   double abc2_p[16], *abc2 = abc2_p;
   int abc2_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, ac_len, ac, &abc2, 16);
   double abc3_p[16], *abc3 = abc3_p;
   int abc3_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, ab_len, ab, &abc3, 16);
   double abc4_p[16], *abc4 = abc4_p;
   int abc4_len = expansionObject::Gen_Sum_With_PreAlloc(abc1_len, abc1, abc3_len, abc3, &abc4, 16);
   double abc_p[16], *abc = abc_p;
   int abc_len = expansionObject::Gen_Diff_With_PreAlloc(abc4_len, abc4, abc2_len, abc2, &abc, 16);
   double bcd1_p[16], *bcd1 = bcd1_p;
   int bcd1_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, cd_len, cd, &bcd1, 16);
   double bcd2_p[16], *bcd2 = bcd2_p;
   int bcd2_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, bd_len, bd, &bcd2, 16);
   double bcd3_p[16], *bcd3 = bcd3_p;
   int bcd3_len = expansionObject::Gen_Product_With_PreAlloc(dez_len, dez, bc_len, bc, &bcd3, 16);
   double bcd4_p[16], *bcd4 = bcd4_p;
   int bcd4_len = expansionObject::Gen_Sum_With_PreAlloc(bcd1_len, bcd1, bcd3_len, bcd3, &bcd4, 16);
   double bcd_p[16], *bcd = bcd_p;
   int bcd_len = expansionObject::Gen_Diff_With_PreAlloc(bcd4_len, bcd4, bcd2_len, bcd2, &bcd, 16);
   double cda1_p[16], *cda1 = cda1_p;
   int cda1_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, da_len, da, &cda1, 16);
   double cda2_p[16], *cda2 = cda2_p;
   int cda2_len = expansionObject::Gen_Product_With_PreAlloc(dez_len, dez, ac_len, ac, &cda2, 16);
   double cda3_p[16], *cda3 = cda3_p;
   int cda3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, cd_len, cd, &cda3, 16);
   double cda4_p[16], *cda4 = cda4_p;
   int cda4_len = expansionObject::Gen_Sum_With_PreAlloc(cda1_len, cda1, cda3_len, cda3, &cda4, 16);
   double cda_p[16], *cda = cda_p;
   int cda_len = expansionObject::Gen_Sum_With_PreAlloc(cda4_len, cda4, cda2_len, cda2, &cda, 16);
   double dab1_p[16], *dab1 = dab1_p;
   int dab1_len = expansionObject::Gen_Product_With_PreAlloc(dez_len, dez, ab_len, ab, &dab1, 16);
   double dab2_p[16], *dab2 = dab2_p;
   int dab2_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bd_len, bd, &dab2, 16);
   double dab3_p[16], *dab3 = dab3_p;
   int dab3_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, da_len, da, &dab3, 16);
   double dab4_p[16], *dab4 = dab4_p;
   int dab4_len = expansionObject::Gen_Sum_With_PreAlloc(dab1_len, dab1, dab3_len, dab3, &dab4, 16);
   double dab_p[16], *dab = dab_p;
   int dab_len = expansionObject::Gen_Sum_With_PreAlloc(dab4_len, dab4, dab2_len, dab2, &dab, 16);
   double al1_p[16], *al1 = al1_p;
   int al1_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, aex_len, aex, &al1, 16);
   double al2_p[16], *al2 = al2_p;
   int al2_len = expansionObject::Gen_Product_With_PreAlloc(aey_len, aey, aey_len, aey, &al2, 16);
   double al3_p[16], *al3 = al3_p;
   int al3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, aez_len, aez, &al3, 16);
   double al4_p[16], *al4 = al4_p;
   int al4_len = expansionObject::Gen_Sum_With_PreAlloc(al1_len, al1, al2_len, al2, &al4, 16);
   double alift_p[16], *alift = alift_p;
   int alift_len = expansionObject::Gen_Sum_With_PreAlloc(al4_len, al4, al3_len, al3, &alift, 16);
   double bl1_p[16], *bl1 = bl1_p;
   int bl1_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, bex_len, bex, &bl1, 16);
   double bl2_p[16], *bl2 = bl2_p;
   int bl2_len = expansionObject::Gen_Product_With_PreAlloc(bey_len, bey, bey_len, bey, &bl2, 16);
   double bl3_p[16], *bl3 = bl3_p;
   int bl3_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, bez_len, bez, &bl3, 16);
   double bl4_p[16], *bl4 = bl4_p;
   int bl4_len = expansionObject::Gen_Sum_With_PreAlloc(bl1_len, bl1, bl2_len, bl2, &bl4, 16);
   double blift_p[16], *blift = blift_p;
   int blift_len = expansionObject::Gen_Sum_With_PreAlloc(bl4_len, bl4, bl3_len, bl3, &blift, 16);
   double cl1_p[16], *cl1 = cl1_p;
   int cl1_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, cex_len, cex, &cl1, 16);
   double cl2_p[16], *cl2 = cl2_p;
   int cl2_len = expansionObject::Gen_Product_With_PreAlloc(cey_len, cey, cey_len, cey, &cl2, 16);
   double cl3_p[16], *cl3 = cl3_p;
   int cl3_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, cez_len, cez, &cl3, 16);
   double cl4_p[16], *cl4 = cl4_p;
   int cl4_len = expansionObject::Gen_Sum_With_PreAlloc(cl1_len, cl1, cl2_len, cl2, &cl4, 16);
   double clift_p[16], *clift = clift_p;
   int clift_len = expansionObject::Gen_Sum_With_PreAlloc(cl4_len, cl4, cl3_len, cl3, &clift, 16);
   double dl1_p[16], *dl1 = dl1_p;
   int dl1_len = expansionObject::Gen_Product_With_PreAlloc(dex_len, dex, dex_len, dex, &dl1, 16);
   double dl2_p[16], *dl2 = dl2_p;
   int dl2_len = expansionObject::Gen_Product_With_PreAlloc(dey_len, dey, dey_len, dey, &dl2, 16);
   double dl3_p[16], *dl3 = dl3_p;
   int dl3_len = expansionObject::Gen_Product_With_PreAlloc(dez_len, dez, dez_len, dez, &dl3, 16);
   double dl4_p[16], *dl4 = dl4_p;
   int dl4_len = expansionObject::Gen_Sum_With_PreAlloc(dl1_len, dl1, dl2_len, dl2, &dl4, 16);
   double dlift_p[16], *dlift = dlift_p;
   int dlift_len = expansionObject::Gen_Sum_With_PreAlloc(dl4_len, dl4, dl3_len, dl3, &dlift, 16);
   double ds1_p[16], *ds1 = ds1_p;
   int ds1_len = expansionObject::Gen_Product_With_PreAlloc(dlift_len, dlift, abc_len, abc, &ds1, 16);
   double ds12_p[16], *ds12 = ds12_p;
   int ds12_len = expansionObject::Gen_Product_With_PreAlloc(ds1_len, ds1, d3_len, d3, &ds12, 16);
   double ds2_p[16], *ds2 = ds2_p;
   int ds2_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, dab_len, dab, &ds2, 16);
   double ds22_p[16], *ds22 = ds22_p;
   int ds22_len = expansionObject::Gen_Product_With_PreAlloc(ds2_len, ds2, d4_len, d4, &ds22, 16);
   double dl_p[16], *dl = dl_p;
   int dl_len = expansionObject::Gen_Diff_With_PreAlloc(ds22_len, ds22, ds12_len, ds12, &dl, 16);
   double dlx1_p[16], *dlx1 = dlx1_p;
   int dlx1_len = expansionObject::Gen_Product_With_PreAlloc(dl_len, dl, d1_len, d1, &dlx1, 16);
   double dlx2_p[16], *dlx2 = dlx2_p;
   int dlx2_len = expansionObject::Gen_Product_With_PreAlloc(dlx1_len, dlx1, d2_len, d2, &dlx2, 16);
   double dr1_p[16], *dr1 = dr1_p;
   int dr1_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cda_len, cda, &dr1, 16);
   double dr12_p[16], *dr12 = dr12_p;
   int dr12_len = expansionObject::Gen_Product_With_PreAlloc(dr1_len, dr1, d1_len, d1, &dr12, 16);
   double dr2_p[16], *dr2 = dr2_p;
   int dr2_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcd_len, bcd, &dr2, 16);
   double dr22_p[16], *dr22 = dr22_p;
   int dr22_len = expansionObject::Gen_Product_With_PreAlloc(dr2_len, dr2, d2_len, d2, &dr22, 16);
   double dr_p[16], *dr = dr_p;
   int dr_len = expansionObject::Gen_Diff_With_PreAlloc(dr22_len, dr22, dr12_len, dr12, &dr, 16);
   double drx1_p[16], *drx1 = drx1_p;
   int drx1_len = expansionObject::Gen_Product_With_PreAlloc(dr_len, dr, d3_len, d3, &drx1, 16);
   double drx2_p[16], *drx2 = drx2_p;
   int drx2_len = expansionObject::Gen_Product_With_PreAlloc(drx1_len, drx1, d4_len, d4, &drx2, 16);
   double det_p[16], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dlx2_len, dlx2, drx2_len, drx2, &det, 16);

   return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (drx2_p != drx2) FreeDoubles(drx2);
   if (drx1_p != drx1) FreeDoubles(drx1);
   if (dr_p != dr) FreeDoubles(dr);
   if (dr22_p != dr22) FreeDoubles(dr22);
   if (dr2_p != dr2) FreeDoubles(dr2);
   if (dr12_p != dr12) FreeDoubles(dr12);
   if (dr1_p != dr1) FreeDoubles(dr1);
   if (dlx2_p != dlx2) FreeDoubles(dlx2);
   if (dlx1_p != dlx1) FreeDoubles(dlx1);
   if (dl_p != dl) FreeDoubles(dl);
   if (ds22_p != ds22) FreeDoubles(ds22);
   if (ds2_p != ds2) FreeDoubles(ds2);
   if (ds12_p != ds12) FreeDoubles(ds12);
   if (ds1_p != ds1) FreeDoubles(ds1);
   if (dlift_p != dlift) FreeDoubles(dlift);
   if (dl4_p != dl4) FreeDoubles(dl4);
   if (dl3_p != dl3) FreeDoubles(dl3);
   if (dl2_p != dl2) FreeDoubles(dl2);
   if (dl1_p != dl1) FreeDoubles(dl1);
   if (clift_p != clift) FreeDoubles(clift);
   if (cl4_p != cl4) FreeDoubles(cl4);
   if (cl3_p != cl3) FreeDoubles(cl3);
   if (cl2_p != cl2) FreeDoubles(cl2);
   if (cl1_p != cl1) FreeDoubles(cl1);
   if (blift_p != blift) FreeDoubles(blift);
   if (bl4_p != bl4) FreeDoubles(bl4);
   if (bl3_p != bl3) FreeDoubles(bl3);
   if (bl2_p != bl2) FreeDoubles(bl2);
   if (bl1_p != bl1) FreeDoubles(bl1);
   if (alift_p != alift) FreeDoubles(alift);
   if (al4_p != al4) FreeDoubles(al4);
   if (al3_p != al3) FreeDoubles(al3);
   if (al2_p != al2) FreeDoubles(al2);
   if (al1_p != al1) FreeDoubles(al1);
   if (dab_p != dab) FreeDoubles(dab);
   if (dab4_p != dab4) FreeDoubles(dab4);
   if (dab3_p != dab3) FreeDoubles(dab3);
   if (dab2_p != dab2) FreeDoubles(dab2);
   if (dab1_p != dab1) FreeDoubles(dab1);
   if (cda_p != cda) FreeDoubles(cda);
   if (cda4_p != cda4) FreeDoubles(cda4);
   if (cda3_p != cda3) FreeDoubles(cda3);
   if (cda2_p != cda2) FreeDoubles(cda2);
   if (cda1_p != cda1) FreeDoubles(cda1);
   if (bcd_p != bcd) FreeDoubles(bcd);
   if (bcd4_p != bcd4) FreeDoubles(bcd4);
   if (bcd3_p != bcd3) FreeDoubles(bcd3);
   if (bcd2_p != bcd2) FreeDoubles(bcd2);
   if (bcd1_p != bcd1) FreeDoubles(bcd1);
   if (abc_p != abc) FreeDoubles(abc);
   if (abc4_p != abc4) FreeDoubles(abc4);
   if (abc3_p != abc3) FreeDoubles(abc3);
   if (abc2_p != abc2) FreeDoubles(abc2);
   if (abc1_p != abc1) FreeDoubles(abc1);
   if (bd_p != bd) FreeDoubles(bd);
   if (dexbey_p != dexbey) FreeDoubles(dexbey);
   if (bexdey_p != bexdey) FreeDoubles(bexdey);
   if (ac_p != ac) FreeDoubles(ac);
   if (cexaey_p != cexaey) FreeDoubles(cexaey);
   if (aexcey_p != aexcey) FreeDoubles(aexcey);
   if (da_p != da) FreeDoubles(da);
   if (aexdey_p != aexdey) FreeDoubles(aexdey);
   if (dexaey_p != dexaey) FreeDoubles(dexaey);
   if (cd_p != cd) FreeDoubles(cd);
   if (dexcey_p != dexcey) FreeDoubles(dexcey);
   if (cexdey_p != cexdey) FreeDoubles(cexdey);
   if (bc_p != bc) FreeDoubles(bc);
   if (cexbey_p != cexbey) FreeDoubles(cexbey);
   if (bexcey_p != bexcey) FreeDoubles(bexcey);
   if (ab_p != ab) FreeDoubles(ab);
   if (bexaey_p != bexaey) FreeDoubles(bexaey);
   if (aexbey_p != aexbey) FreeDoubles(aexbey);
   if (dez_p != dez) FreeDoubles(dez);
   if (dey_p != dey) FreeDoubles(dey);
   if (dex_p != dex) FreeDoubles(dex);
   if (pezd4_p != pezd4) FreeDoubles(pezd4);
   if (peyd4_p != peyd4) FreeDoubles(peyd4);
   if (pexd4_p != pexd4) FreeDoubles(pexd4);
   if (cez_p != cez) FreeDoubles(cez);
   if (cey_p != cey) FreeDoubles(cey);
   if (cex_p != cex) FreeDoubles(cex);
   if (pezd3_p != pezd3) FreeDoubles(pezd3);
   if (peyd3_p != peyd3) FreeDoubles(peyd3);
   if (pexd3_p != pexd3) FreeDoubles(pexd3);
   if (bez_p != bez) FreeDoubles(bez);
   if (bey_p != bey) FreeDoubles(bey);
   if (bex_p != bex) FreeDoubles(bex);
   if (pezd2_p != pezd2) FreeDoubles(pezd2);
   if (peyd2_p != peyd2) FreeDoubles(peyd2);
   if (pexd2_p != pexd2) FreeDoubles(pexd2);
   if (aez_p != aez) FreeDoubles(aez);
   if (aey_p != aey) FreeDoubles(aey);
   if (aex_p != aex) FreeDoubles(aex);
   if (pezd_p != pezd) FreeDoubles(pezd);
   if (peyd_p != peyd) FreeDoubles(peyd);
   if (pexd_p != pexd) FreeDoubles(pexd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = inSphere_IIIIE_bigfloat(p1, p2, p3, p4, pex, pey, pez);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (d3_p != d3) FreeDoubles(d3);
 if (l4x_p != l4x) FreeDoubles(l4x);
 if (l4y_p != l4y) FreeDoubles(l4y);
 if (l4z_p != l4z) FreeDoubles(l4z);
 if (d4_p != d4) FreeDoubles(d4);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int inSphere_IIIIE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, double pex, double pey, double pez)
{
   int ret;
   ret = inSphere_IIIIE_interval(p1, p2, p3, p4, pex, pey, pez);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return inSphere_IIIIE_exact(p1, p2, p3, p4, pex, pey, pez);
}

inline int inSphere_IIIII_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, const genericPoint& p5)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3, l4x, l4y, l4z, d4, l5x, l5y, l5z, d5;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   || !p3.getIntervalLambda(l3x, l3y, l3z, d3)
   || !p4.getIntervalLambda(l4x, l4y, l4z, d4)
   || !p5.getIntervalLambda(l5x, l5y, l5z, d5)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number pexd(l5x * d1);
   const interval_number peyd(l5y * d1);
   const interval_number pezd(l5z * d1);
   const interval_number ll1x(l1x * d5);
   const interval_number ll1y(l1y * d5);
   const interval_number ll1z(l1z * d5);
   const interval_number aex(ll1x - pexd);
   const interval_number aey(ll1y - peyd);
   const interval_number aez(ll1z - pezd);
   const interval_number pexd2(l5x * d2);
   const interval_number peyd2(l5y * d2);
   const interval_number pezd2(l5z * d2);
   const interval_number ll2x(l2x * d5);
   const interval_number ll2y(l2y * d5);
   const interval_number ll2z(l2z * d5);
   const interval_number bex(ll2x - pexd2);
   const interval_number bey(ll2y - peyd2);
   const interval_number bez(ll2z - pezd2);
   const interval_number pexd3(l5x * d3);
   const interval_number peyd3(l5y * d3);
   const interval_number pezd3(l5z * d3);
   const interval_number ll3x(l3x * d5);
   const interval_number ll3y(l3y * d5);
   const interval_number ll3z(l3z * d5);
   const interval_number cex(ll3x - pexd3);
   const interval_number cey(ll3y - peyd3);
   const interval_number cez(ll3z - pezd3);
   const interval_number pexd4(l5x * d4);
   const interval_number peyd4(l5y * d4);
   const interval_number pezd4(l5z * d4);
   const interval_number ll4x(l4x * d5);
   const interval_number ll4y(l4y * d5);
   const interval_number ll4z(l4z * d5);
   const interval_number dex(ll4x - pexd4);
   const interval_number dey(ll4y - peyd4);
   const interval_number dez(ll4z - pezd4);
   const interval_number aexbey(aex * bey);
   const interval_number bexaey(bex * aey);
   const interval_number ab(aexbey - bexaey);
   const interval_number bexcey(bex * cey);
   const interval_number cexbey(cex * bey);
   const interval_number bc(bexcey - cexbey);
   const interval_number cexdey(cex * dey);
   const interval_number dexcey(dex * cey);
   const interval_number cd(cexdey - dexcey);
   const interval_number dexaey(dex * aey);
   const interval_number aexdey(aex * dey);
   const interval_number da(dexaey - aexdey);
   const interval_number aexcey(aex * cey);
   const interval_number cexaey(cex * aey);
   const interval_number ac(aexcey - cexaey);
   const interval_number bexdey(bex * dey);
   const interval_number dexbey(dex * bey);
   const interval_number bd(bexdey - dexbey);
   const interval_number abc1(aez * bc);
   const interval_number abc2(bez * ac);
   const interval_number abc3(cez * ab);
   const interval_number abc4(abc1 + abc3);
   const interval_number abc(abc4 - abc2);
   const interval_number bcd1(bez * cd);
   const interval_number bcd2(cez * bd);
   const interval_number bcd3(dez * bc);
   const interval_number bcd4(bcd1 + bcd3);
   const interval_number bcd(bcd4 - bcd2);
   const interval_number cda1(cez * da);
   const interval_number cda2(dez * ac);
   const interval_number cda3(aez * cd);
   const interval_number cda4(cda1 + cda3);
   const interval_number cda(cda4 + cda2);
   const interval_number dab1(dez * ab);
   const interval_number dab2(aez * bd);
   const interval_number dab3(bez * da);
   const interval_number dab4(dab1 + dab3);
   const interval_number dab(dab4 + dab2);
   const interval_number al1(aex * aex);
   const interval_number al2(aey * aey);
   const interval_number al3(aez * aez);
   const interval_number al4(al1 + al2);
   const interval_number alift(al4 + al3);
   const interval_number bl1(bex * bex);
   const interval_number bl2(bey * bey);
   const interval_number bl3(bez * bez);
   const interval_number bl4(bl1 + bl2);
   const interval_number blift(bl4 + bl3);
   const interval_number cl1(cex * cex);
   const interval_number cl2(cey * cey);
   const interval_number cl3(cez * cez);
   const interval_number cl4(cl1 + cl2);
   const interval_number clift(cl4 + cl3);
   const interval_number dl1(dex * dex);
   const interval_number dl2(dey * dey);
   const interval_number dl3(dez * dez);
   const interval_number dl4(dl1 + dl2);
   const interval_number dlift(dl4 + dl3);
   const interval_number ds1(dlift * abc);
   const interval_number ds1n(ds1 * d3);
   const interval_number ds2(clift * dab);
   const interval_number ds2n(ds2 * d4);
   const interval_number dl(ds2n - ds1n);
   const interval_number dla(dl * d1);
   const interval_number dlb(dla * d2);
   const interval_number dr1(blift * cda);
   const interval_number dr1n(dr1 * d1);
   const interval_number dr2(alift * bcd);
   const interval_number dr2n(dr2 * d2);
   const interval_number dr(dr2n - dr1n);
   const interval_number dra(dr * d3);
   const interval_number drb(dra * d4);
   const interval_number det(dlb + drb);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int inSphere_IIIII_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, const genericPoint& p5)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3, l4x, l4y, l4z, d4, l5x, l5y, l5z, d5;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   p3.getBigfloatLambda(l3x, l3y, l3z, d3);
   p4.getBigfloatLambda(l4x, l4y, l4z, d4);
   p5.getBigfloatLambda(l5x, l5y, l5z, d5);
   const bigfloat pexd(l5x * d1);
   const bigfloat peyd(l5y * d1);
   const bigfloat pezd(l5z * d1);
   const bigfloat ll1x(l1x * d5);
   const bigfloat ll1y(l1y * d5);
   const bigfloat ll1z(l1z * d5);
   const bigfloat aex(ll1x - pexd);
   const bigfloat aey(ll1y - peyd);
   const bigfloat aez(ll1z - pezd);
   const bigfloat pexd2(l5x * d2);
   const bigfloat peyd2(l5y * d2);
   const bigfloat pezd2(l5z * d2);
   const bigfloat ll2x(l2x * d5);
   const bigfloat ll2y(l2y * d5);
   const bigfloat ll2z(l2z * d5);
   const bigfloat bex(ll2x - pexd2);
   const bigfloat bey(ll2y - peyd2);
   const bigfloat bez(ll2z - pezd2);
   const bigfloat pexd3(l5x * d3);
   const bigfloat peyd3(l5y * d3);
   const bigfloat pezd3(l5z * d3);
   const bigfloat ll3x(l3x * d5);
   const bigfloat ll3y(l3y * d5);
   const bigfloat ll3z(l3z * d5);
   const bigfloat cex(ll3x - pexd3);
   const bigfloat cey(ll3y - peyd3);
   const bigfloat cez(ll3z - pezd3);
   const bigfloat pexd4(l5x * d4);
   const bigfloat peyd4(l5y * d4);
   const bigfloat pezd4(l5z * d4);
   const bigfloat ll4x(l4x * d5);
   const bigfloat ll4y(l4y * d5);
   const bigfloat ll4z(l4z * d5);
   const bigfloat dex(ll4x - pexd4);
   const bigfloat dey(ll4y - peyd4);
   const bigfloat dez(ll4z - pezd4);
   const bigfloat aexbey(aex * bey);
   const bigfloat bexaey(bex * aey);
   const bigfloat ab(aexbey - bexaey);
   const bigfloat bexcey(bex * cey);
   const bigfloat cexbey(cex * bey);
   const bigfloat bc(bexcey - cexbey);
   const bigfloat cexdey(cex * dey);
   const bigfloat dexcey(dex * cey);
   const bigfloat cd(cexdey - dexcey);
   const bigfloat dexaey(dex * aey);
   const bigfloat aexdey(aex * dey);
   const bigfloat da(dexaey - aexdey);
   const bigfloat aexcey(aex * cey);
   const bigfloat cexaey(cex * aey);
   const bigfloat ac(aexcey - cexaey);
   const bigfloat bexdey(bex * dey);
   const bigfloat dexbey(dex * bey);
   const bigfloat bd(bexdey - dexbey);
   const bigfloat abc1(aez * bc);
   const bigfloat abc2(bez * ac);
   const bigfloat abc3(cez * ab);
   const bigfloat abc4(abc1 + abc3);
   const bigfloat abc(abc4 - abc2);
   const bigfloat bcd1(bez * cd);
   const bigfloat bcd2(cez * bd);
   const bigfloat bcd3(dez * bc);
   const bigfloat bcd4(bcd1 + bcd3);
   const bigfloat bcd(bcd4 - bcd2);
   const bigfloat cda1(cez * da);
   const bigfloat cda2(dez * ac);
   const bigfloat cda3(aez * cd);
   const bigfloat cda4(cda1 + cda3);
   const bigfloat cda(cda4 + cda2);
   const bigfloat dab1(dez * ab);
   const bigfloat dab2(aez * bd);
   const bigfloat dab3(bez * da);
   const bigfloat dab4(dab1 + dab3);
   const bigfloat dab(dab4 + dab2);
   const bigfloat al1(aex * aex);
   const bigfloat al2(aey * aey);
   const bigfloat al3(aez * aez);
   const bigfloat al4(al1 + al2);
   const bigfloat alift(al4 + al3);
   const bigfloat bl1(bex * bex);
   const bigfloat bl2(bey * bey);
   const bigfloat bl3(bez * bez);
   const bigfloat bl4(bl1 + bl2);
   const bigfloat blift(bl4 + bl3);
   const bigfloat cl1(cex * cex);
   const bigfloat cl2(cey * cey);
   const bigfloat cl3(cez * cez);
   const bigfloat cl4(cl1 + cl2);
   const bigfloat clift(cl4 + cl3);
   const bigfloat dl1(dex * dex);
   const bigfloat dl2(dey * dey);
   const bigfloat dl3(dez * dez);
   const bigfloat dl4(dl1 + dl2);
   const bigfloat dlift(dl4 + dl3);
   const bigfloat ds1(dlift * abc);
   const bigfloat ds1n(ds1 * d3);
   const bigfloat ds2(clift * dab);
   const bigfloat ds2n(ds2 * d4);
   const bigfloat dl(ds2n - ds1n);
   const bigfloat dla(dl * d1);
   const bigfloat dlb(dla * d2);
   const bigfloat dr1(blift * cda);
   const bigfloat dr1n(dr1 * d1);
   const bigfloat dr2(alift * bcd);
   const bigfloat dr2n(dr2 * d2);
   const bigfloat dr(dr2n - dr1n);
   const bigfloat dra(dr * d3);
   const bigfloat drb(dra * d4);
   const bigfloat det(dlb + drb);
   return sgn(det);
}

inline int inSphere_IIIII_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, const genericPoint& p5)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[8], *l1x = l1x_p, l1y_p[8], *l1y = l1y_p, l1z_p[8], *l1z = l1z_p, d1_p[8], *d1 = d1_p, l2x_p[8], *l2x = l2x_p, l2y_p[8], *l2y = l2y_p, l2z_p[8], *l2z = l2z_p, d2_p[8], *d2 = d2_p, l3x_p[8], *l3x = l3x_p, l3y_p[8], *l3y = l3y_p, l3z_p[8], *l3z = l3z_p, d3_p[8], *d3 = d3_p, l4x_p[8], *l4x = l4x_p, l4y_p[8], *l4y = l4y_p, l4z_p[8], *l4z = l4z_p, d4_p[8], *d4 = d4_p, l5x_p[8], *l5x = l5x_p, l5y_p[8], *l5y = l5y_p, l5z_p[8], *l5z = l5z_p, d5_p[8], *d5 = d5_p;
 int l1x_len = 8, l1y_len = 8, l1z_len = 8, d1_len = 8, l2x_len = 8, l2y_len = 8, l2z_len = 8, d2_len = 8, l3x_len = 8, l3y_len = 8, l3z_len = 8, d3_len = 8, l4x_len = 8, l4y_len = 8, l4z_len = 8, d4_len = 8, l5x_len = 8, l5y_len = 8, l5z_len = 8, d5_len = 8;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &l3z, l3z_len, &d3, d3_len);
 p4.getExactLambda(&l4x, l4x_len, &l4y, l4y_len, &l4z, l4z_len, &d4, d4_len);
 p5.getExactLambda(&l5x, l5x_len, &l5y, l5y_len, &l5z, l5z_len, &d5, d5_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0) && (d4[d4_len - 1] != 0) && (d5[d5_len - 1] != 0))
 {
   
   double pexd_p[8], *pexd = pexd_p;
   int pexd_len = expansionObject::Gen_Product_With_PreAlloc(l5x_len, l5x, d1_len, d1, &pexd, 8);
   double peyd_p[8], *peyd = peyd_p;
   int peyd_len = expansionObject::Gen_Product_With_PreAlloc(l5y_len, l5y, d1_len, d1, &peyd, 8);
   double pezd_p[8], *pezd = pezd_p;
   int pezd_len = expansionObject::Gen_Product_With_PreAlloc(l5z_len, l5z, d1_len, d1, &pezd, 8);
   double ll1x_p[8], *ll1x = ll1x_p;
   int ll1x_len = expansionObject::Gen_Product_With_PreAlloc(l1x_len, l1x, d5_len, d5, &ll1x, 8);
   double ll1y_p[8], *ll1y = ll1y_p;
   int ll1y_len = expansionObject::Gen_Product_With_PreAlloc(l1y_len, l1y, d5_len, d5, &ll1y, 8);
   double ll1z_p[8], *ll1z = ll1z_p;
   int ll1z_len = expansionObject::Gen_Product_With_PreAlloc(l1z_len, l1z, d5_len, d5, &ll1z, 8);
   double aex_p[8], *aex = aex_p;
   int aex_len = expansionObject::Gen_Diff_With_PreAlloc(ll1x_len, ll1x, pexd_len, pexd, &aex, 8);
   double aey_p[8], *aey = aey_p;
   int aey_len = expansionObject::Gen_Diff_With_PreAlloc(ll1y_len, ll1y, peyd_len, peyd, &aey, 8);
   double aez_p[8], *aez = aez_p;
   int aez_len = expansionObject::Gen_Diff_With_PreAlloc(ll1z_len, ll1z, pezd_len, pezd, &aez, 8);
   double pexd2_p[8], *pexd2 = pexd2_p;
   int pexd2_len = expansionObject::Gen_Product_With_PreAlloc(l5x_len, l5x, d2_len, d2, &pexd2, 8);
   double peyd2_p[8], *peyd2 = peyd2_p;
   int peyd2_len = expansionObject::Gen_Product_With_PreAlloc(l5y_len, l5y, d2_len, d2, &peyd2, 8);
   double pezd2_p[8], *pezd2 = pezd2_p;
   int pezd2_len = expansionObject::Gen_Product_With_PreAlloc(l5z_len, l5z, d2_len, d2, &pezd2, 8);
   double ll2x_p[8], *ll2x = ll2x_p;
   int ll2x_len = expansionObject::Gen_Product_With_PreAlloc(l2x_len, l2x, d5_len, d5, &ll2x, 8);
   double ll2y_p[8], *ll2y = ll2y_p;
   int ll2y_len = expansionObject::Gen_Product_With_PreAlloc(l2y_len, l2y, d5_len, d5, &ll2y, 8);
   double ll2z_p[8], *ll2z = ll2z_p;
   int ll2z_len = expansionObject::Gen_Product_With_PreAlloc(l2z_len, l2z, d5_len, d5, &ll2z, 8);
   double bex_p[8], *bex = bex_p;
   int bex_len = expansionObject::Gen_Diff_With_PreAlloc(ll2x_len, ll2x, pexd2_len, pexd2, &bex, 8);
   double bey_p[8], *bey = bey_p;
   int bey_len = expansionObject::Gen_Diff_With_PreAlloc(ll2y_len, ll2y, peyd2_len, peyd2, &bey, 8);
   double bez_p[8], *bez = bez_p;
   int bez_len = expansionObject::Gen_Diff_With_PreAlloc(ll2z_len, ll2z, pezd2_len, pezd2, &bez, 8);
   double pexd3_p[8], *pexd3 = pexd3_p;
   int pexd3_len = expansionObject::Gen_Product_With_PreAlloc(l5x_len, l5x, d3_len, d3, &pexd3, 8);
   double peyd3_p[8], *peyd3 = peyd3_p;
   int peyd3_len = expansionObject::Gen_Product_With_PreAlloc(l5y_len, l5y, d3_len, d3, &peyd3, 8);
   double pezd3_p[8], *pezd3 = pezd3_p;
   int pezd3_len = expansionObject::Gen_Product_With_PreAlloc(l5z_len, l5z, d3_len, d3, &pezd3, 8);
   double ll3x_p[8], *ll3x = ll3x_p;
   int ll3x_len = expansionObject::Gen_Product_With_PreAlloc(l3x_len, l3x, d5_len, d5, &ll3x, 8);
   double ll3y_p[8], *ll3y = ll3y_p;
   int ll3y_len = expansionObject::Gen_Product_With_PreAlloc(l3y_len, l3y, d5_len, d5, &ll3y, 8);
   double ll3z_p[8], *ll3z = ll3z_p;
   int ll3z_len = expansionObject::Gen_Product_With_PreAlloc(l3z_len, l3z, d5_len, d5, &ll3z, 8);
   double cex_p[8], *cex = cex_p;
   int cex_len = expansionObject::Gen_Diff_With_PreAlloc(ll3x_len, ll3x, pexd3_len, pexd3, &cex, 8);
   double cey_p[8], *cey = cey_p;
   int cey_len = expansionObject::Gen_Diff_With_PreAlloc(ll3y_len, ll3y, peyd3_len, peyd3, &cey, 8);
   double cez_p[8], *cez = cez_p;
   int cez_len = expansionObject::Gen_Diff_With_PreAlloc(ll3z_len, ll3z, pezd3_len, pezd3, &cez, 8);
   double pexd4_p[8], *pexd4 = pexd4_p;
   int pexd4_len = expansionObject::Gen_Product_With_PreAlloc(l5x_len, l5x, d4_len, d4, &pexd4, 8);
   double peyd4_p[8], *peyd4 = peyd4_p;
   int peyd4_len = expansionObject::Gen_Product_With_PreAlloc(l5y_len, l5y, d4_len, d4, &peyd4, 8);
   double pezd4_p[8], *pezd4 = pezd4_p;
   int pezd4_len = expansionObject::Gen_Product_With_PreAlloc(l5z_len, l5z, d4_len, d4, &pezd4, 8);
   double ll4x_p[8], *ll4x = ll4x_p;
   int ll4x_len = expansionObject::Gen_Product_With_PreAlloc(l4x_len, l4x, d5_len, d5, &ll4x, 8);
   double ll4y_p[8], *ll4y = ll4y_p;
   int ll4y_len = expansionObject::Gen_Product_With_PreAlloc(l4y_len, l4y, d5_len, d5, &ll4y, 8);
   double ll4z_p[8], *ll4z = ll4z_p;
   int ll4z_len = expansionObject::Gen_Product_With_PreAlloc(l4z_len, l4z, d5_len, d5, &ll4z, 8);
   double dex_p[8], *dex = dex_p;
   int dex_len = expansionObject::Gen_Diff_With_PreAlloc(ll4x_len, ll4x, pexd4_len, pexd4, &dex, 8);
   double dey_p[8], *dey = dey_p;
   int dey_len = expansionObject::Gen_Diff_With_PreAlloc(ll4y_len, ll4y, peyd4_len, peyd4, &dey, 8);
   double dez_p[8], *dez = dez_p;
   int dez_len = expansionObject::Gen_Diff_With_PreAlloc(ll4z_len, ll4z, pezd4_len, pezd4, &dez, 8);
   double aexbey_p[8], *aexbey = aexbey_p;
   int aexbey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, bey_len, bey, &aexbey, 8);
   double bexaey_p[8], *bexaey = bexaey_p;
   int bexaey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, aey_len, aey, &bexaey, 8);
   double ab_p[8], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(aexbey_len, aexbey, bexaey_len, bexaey, &ab, 8);
   double bexcey_p[8], *bexcey = bexcey_p;
   int bexcey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, cey_len, cey, &bexcey, 8);
   double cexbey_p[8], *cexbey = cexbey_p;
   int cexbey_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, bey_len, bey, &cexbey, 8);
   double bc_p[8], *bc = bc_p;
   int bc_len = expansionObject::Gen_Diff_With_PreAlloc(bexcey_len, bexcey, cexbey_len, cexbey, &bc, 8);
   double cexdey_p[8], *cexdey = cexdey_p;
   int cexdey_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, dey_len, dey, &cexdey, 8);
   double dexcey_p[8], *dexcey = dexcey_p;
   int dexcey_len = expansionObject::Gen_Product_With_PreAlloc(dex_len, dex, cey_len, cey, &dexcey, 8);
   double cd_p[8], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(cexdey_len, cexdey, dexcey_len, dexcey, &cd, 8);
   double dexaey_p[8], *dexaey = dexaey_p;
   int dexaey_len = expansionObject::Gen_Product_With_PreAlloc(dex_len, dex, aey_len, aey, &dexaey, 8);
   double aexdey_p[8], *aexdey = aexdey_p;
   int aexdey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, dey_len, dey, &aexdey, 8);
   double da_p[8], *da = da_p;
   int da_len = expansionObject::Gen_Diff_With_PreAlloc(dexaey_len, dexaey, aexdey_len, aexdey, &da, 8);
   double aexcey_p[8], *aexcey = aexcey_p;
   int aexcey_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, cey_len, cey, &aexcey, 8);
   double cexaey_p[8], *cexaey = cexaey_p;
   int cexaey_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, aey_len, aey, &cexaey, 8);
   double ac_p[8], *ac = ac_p;
   int ac_len = expansionObject::Gen_Diff_With_PreAlloc(aexcey_len, aexcey, cexaey_len, cexaey, &ac, 8);
   double bexdey_p[8], *bexdey = bexdey_p;
   int bexdey_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, dey_len, dey, &bexdey, 8);
   double dexbey_p[8], *dexbey = dexbey_p;
   int dexbey_len = expansionObject::Gen_Product_With_PreAlloc(dex_len, dex, bey_len, bey, &dexbey, 8);
   double bd_p[8], *bd = bd_p;
   int bd_len = expansionObject::Gen_Diff_With_PreAlloc(bexdey_len, bexdey, dexbey_len, dexbey, &bd, 8);
   double abc1_p[8], *abc1 = abc1_p;
   int abc1_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bc_len, bc, &abc1, 8);
   double abc2_p[8], *abc2 = abc2_p;
   int abc2_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, ac_len, ac, &abc2, 8);
   double abc3_p[8], *abc3 = abc3_p;
   int abc3_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, ab_len, ab, &abc3, 8);
   double abc4_p[8], *abc4 = abc4_p;
   int abc4_len = expansionObject::Gen_Sum_With_PreAlloc(abc1_len, abc1, abc3_len, abc3, &abc4, 8);
   double abc_p[8], *abc = abc_p;
   int abc_len = expansionObject::Gen_Diff_With_PreAlloc(abc4_len, abc4, abc2_len, abc2, &abc, 8);
   double bcd1_p[8], *bcd1 = bcd1_p;
   int bcd1_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, cd_len, cd, &bcd1, 8);
   double bcd2_p[8], *bcd2 = bcd2_p;
   int bcd2_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, bd_len, bd, &bcd2, 8);
   double bcd3_p[8], *bcd3 = bcd3_p;
   int bcd3_len = expansionObject::Gen_Product_With_PreAlloc(dez_len, dez, bc_len, bc, &bcd3, 8);
   double bcd4_p[8], *bcd4 = bcd4_p;
   int bcd4_len = expansionObject::Gen_Sum_With_PreAlloc(bcd1_len, bcd1, bcd3_len, bcd3, &bcd4, 8);
   double bcd_p[8], *bcd = bcd_p;
   int bcd_len = expansionObject::Gen_Diff_With_PreAlloc(bcd4_len, bcd4, bcd2_len, bcd2, &bcd, 8);
   double cda1_p[8], *cda1 = cda1_p;
   int cda1_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, da_len, da, &cda1, 8);
   double cda2_p[8], *cda2 = cda2_p;
   int cda2_len = expansionObject::Gen_Product_With_PreAlloc(dez_len, dez, ac_len, ac, &cda2, 8);
   double cda3_p[8], *cda3 = cda3_p;
   int cda3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, cd_len, cd, &cda3, 8);
   double cda4_p[8], *cda4 = cda4_p;
   int cda4_len = expansionObject::Gen_Sum_With_PreAlloc(cda1_len, cda1, cda3_len, cda3, &cda4, 8);
   double cda_p[8], *cda = cda_p;
   int cda_len = expansionObject::Gen_Sum_With_PreAlloc(cda4_len, cda4, cda2_len, cda2, &cda, 8);
   double dab1_p[8], *dab1 = dab1_p;
   int dab1_len = expansionObject::Gen_Product_With_PreAlloc(dez_len, dez, ab_len, ab, &dab1, 8);
   double dab2_p[8], *dab2 = dab2_p;
   int dab2_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, bd_len, bd, &dab2, 8);
   double dab3_p[8], *dab3 = dab3_p;
   int dab3_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, da_len, da, &dab3, 8);
   double dab4_p[8], *dab4 = dab4_p;
   int dab4_len = expansionObject::Gen_Sum_With_PreAlloc(dab1_len, dab1, dab3_len, dab3, &dab4, 8);
   double dab_p[8], *dab = dab_p;
   int dab_len = expansionObject::Gen_Sum_With_PreAlloc(dab4_len, dab4, dab2_len, dab2, &dab, 8);
   double al1_p[8], *al1 = al1_p;
   int al1_len = expansionObject::Gen_Product_With_PreAlloc(aex_len, aex, aex_len, aex, &al1, 8);
   double al2_p[8], *al2 = al2_p;
   int al2_len = expansionObject::Gen_Product_With_PreAlloc(aey_len, aey, aey_len, aey, &al2, 8);
   double al3_p[8], *al3 = al3_p;
   int al3_len = expansionObject::Gen_Product_With_PreAlloc(aez_len, aez, aez_len, aez, &al3, 8);
   double al4_p[8], *al4 = al4_p;
   int al4_len = expansionObject::Gen_Sum_With_PreAlloc(al1_len, al1, al2_len, al2, &al4, 8);
   double alift_p[8], *alift = alift_p;
   int alift_len = expansionObject::Gen_Sum_With_PreAlloc(al4_len, al4, al3_len, al3, &alift, 8);
   double bl1_p[8], *bl1 = bl1_p;
   int bl1_len = expansionObject::Gen_Product_With_PreAlloc(bex_len, bex, bex_len, bex, &bl1, 8);
   double bl2_p[8], *bl2 = bl2_p;
   int bl2_len = expansionObject::Gen_Product_With_PreAlloc(bey_len, bey, bey_len, bey, &bl2, 8);
   double bl3_p[8], *bl3 = bl3_p;
   int bl3_len = expansionObject::Gen_Product_With_PreAlloc(bez_len, bez, bez_len, bez, &bl3, 8);
   double bl4_p[8], *bl4 = bl4_p;
   int bl4_len = expansionObject::Gen_Sum_With_PreAlloc(bl1_len, bl1, bl2_len, bl2, &bl4, 8);
   double blift_p[8], *blift = blift_p;
   int blift_len = expansionObject::Gen_Sum_With_PreAlloc(bl4_len, bl4, bl3_len, bl3, &blift, 8);
   double cl1_p[8], *cl1 = cl1_p;
   int cl1_len = expansionObject::Gen_Product_With_PreAlloc(cex_len, cex, cex_len, cex, &cl1, 8);
   double cl2_p[8], *cl2 = cl2_p;
   int cl2_len = expansionObject::Gen_Product_With_PreAlloc(cey_len, cey, cey_len, cey, &cl2, 8);
   double cl3_p[8], *cl3 = cl3_p;
   int cl3_len = expansionObject::Gen_Product_With_PreAlloc(cez_len, cez, cez_len, cez, &cl3, 8);
   double cl4_p[8], *cl4 = cl4_p;
   int cl4_len = expansionObject::Gen_Sum_With_PreAlloc(cl1_len, cl1, cl2_len, cl2, &cl4, 8);
   double clift_p[8], *clift = clift_p;
   int clift_len = expansionObject::Gen_Sum_With_PreAlloc(cl4_len, cl4, cl3_len, cl3, &clift, 8);
   double dl1_p[8], *dl1 = dl1_p;
   int dl1_len = expansionObject::Gen_Product_With_PreAlloc(dex_len, dex, dex_len, dex, &dl1, 8);
   double dl2_p[8], *dl2 = dl2_p;
   int dl2_len = expansionObject::Gen_Product_With_PreAlloc(dey_len, dey, dey_len, dey, &dl2, 8);
   double dl3_p[8], *dl3 = dl3_p;
   int dl3_len = expansionObject::Gen_Product_With_PreAlloc(dez_len, dez, dez_len, dez, &dl3, 8);
   double dl4_p[8], *dl4 = dl4_p;
   int dl4_len = expansionObject::Gen_Sum_With_PreAlloc(dl1_len, dl1, dl2_len, dl2, &dl4, 8);
   double dlift_p[8], *dlift = dlift_p;
   int dlift_len = expansionObject::Gen_Sum_With_PreAlloc(dl4_len, dl4, dl3_len, dl3, &dlift, 8);
   double ds1_p[8], *ds1 = ds1_p;
   int ds1_len = expansionObject::Gen_Product_With_PreAlloc(dlift_len, dlift, abc_len, abc, &ds1, 8);
   double ds1n_p[8], *ds1n = ds1n_p;
   int ds1n_len = expansionObject::Gen_Product_With_PreAlloc(ds1_len, ds1, d3_len, d3, &ds1n, 8);
   double ds2_p[8], *ds2 = ds2_p;
   int ds2_len = expansionObject::Gen_Product_With_PreAlloc(clift_len, clift, dab_len, dab, &ds2, 8);
   double ds2n_p[8], *ds2n = ds2n_p;
   int ds2n_len = expansionObject::Gen_Product_With_PreAlloc(ds2_len, ds2, d4_len, d4, &ds2n, 8);
   double dl_p[8], *dl = dl_p;
   int dl_len = expansionObject::Gen_Diff_With_PreAlloc(ds2n_len, ds2n, ds1n_len, ds1n, &dl, 8);
   double dla_p[8], *dla = dla_p;
   int dla_len = expansionObject::Gen_Product_With_PreAlloc(dl_len, dl, d1_len, d1, &dla, 8);
   double dlb_p[8], *dlb = dlb_p;
   int dlb_len = expansionObject::Gen_Product_With_PreAlloc(dla_len, dla, d2_len, d2, &dlb, 8);
   double dr1_p[8], *dr1 = dr1_p;
   int dr1_len = expansionObject::Gen_Product_With_PreAlloc(blift_len, blift, cda_len, cda, &dr1, 8);
   double dr1n_p[8], *dr1n = dr1n_p;
   int dr1n_len = expansionObject::Gen_Product_With_PreAlloc(dr1_len, dr1, d1_len, d1, &dr1n, 8);
   double dr2_p[8], *dr2 = dr2_p;
   int dr2_len = expansionObject::Gen_Product_With_PreAlloc(alift_len, alift, bcd_len, bcd, &dr2, 8);
   double dr2n_p[8], *dr2n = dr2n_p;
   int dr2n_len = expansionObject::Gen_Product_With_PreAlloc(dr2_len, dr2, d2_len, d2, &dr2n, 8);
   double dr_p[8], *dr = dr_p;
   int dr_len = expansionObject::Gen_Diff_With_PreAlloc(dr2n_len, dr2n, dr1n_len, dr1n, &dr, 8);
   double dra_p[8], *dra = dra_p;
   int dra_len = expansionObject::Gen_Product_With_PreAlloc(dr_len, dr, d3_len, d3, &dra, 8);
   double drb_p[8], *drb = drb_p;
   int drb_len = expansionObject::Gen_Product_With_PreAlloc(dra_len, dra, d4_len, d4, &drb, 8);
   double det_p[8], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dlb_len, dlb, drb_len, drb, &det, 8);

   return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (drb_p != drb) FreeDoubles(drb);
   if (dra_p != dra) FreeDoubles(dra);
   if (dr_p != dr) FreeDoubles(dr);
   if (dr2n_p != dr2n) FreeDoubles(dr2n);
   if (dr2_p != dr2) FreeDoubles(dr2);
   if (dr1n_p != dr1n) FreeDoubles(dr1n);
   if (dr1_p != dr1) FreeDoubles(dr1);
   if (dlb_p != dlb) FreeDoubles(dlb);
   if (dla_p != dla) FreeDoubles(dla);
   if (dl_p != dl) FreeDoubles(dl);
   if (ds2n_p != ds2n) FreeDoubles(ds2n);
   if (ds2_p != ds2) FreeDoubles(ds2);
   if (ds1n_p != ds1n) FreeDoubles(ds1n);
   if (ds1_p != ds1) FreeDoubles(ds1);
   if (dlift_p != dlift) FreeDoubles(dlift);
   if (dl4_p != dl4) FreeDoubles(dl4);
   if (dl3_p != dl3) FreeDoubles(dl3);
   if (dl2_p != dl2) FreeDoubles(dl2);
   if (dl1_p != dl1) FreeDoubles(dl1);
   if (clift_p != clift) FreeDoubles(clift);
   if (cl4_p != cl4) FreeDoubles(cl4);
   if (cl3_p != cl3) FreeDoubles(cl3);
   if (cl2_p != cl2) FreeDoubles(cl2);
   if (cl1_p != cl1) FreeDoubles(cl1);
   if (blift_p != blift) FreeDoubles(blift);
   if (bl4_p != bl4) FreeDoubles(bl4);
   if (bl3_p != bl3) FreeDoubles(bl3);
   if (bl2_p != bl2) FreeDoubles(bl2);
   if (bl1_p != bl1) FreeDoubles(bl1);
   if (alift_p != alift) FreeDoubles(alift);
   if (al4_p != al4) FreeDoubles(al4);
   if (al3_p != al3) FreeDoubles(al3);
   if (al2_p != al2) FreeDoubles(al2);
   if (al1_p != al1) FreeDoubles(al1);
   if (dab_p != dab) FreeDoubles(dab);
   if (dab4_p != dab4) FreeDoubles(dab4);
   if (dab3_p != dab3) FreeDoubles(dab3);
   if (dab2_p != dab2) FreeDoubles(dab2);
   if (dab1_p != dab1) FreeDoubles(dab1);
   if (cda_p != cda) FreeDoubles(cda);
   if (cda4_p != cda4) FreeDoubles(cda4);
   if (cda3_p != cda3) FreeDoubles(cda3);
   if (cda2_p != cda2) FreeDoubles(cda2);
   if (cda1_p != cda1) FreeDoubles(cda1);
   if (bcd_p != bcd) FreeDoubles(bcd);
   if (bcd4_p != bcd4) FreeDoubles(bcd4);
   if (bcd3_p != bcd3) FreeDoubles(bcd3);
   if (bcd2_p != bcd2) FreeDoubles(bcd2);
   if (bcd1_p != bcd1) FreeDoubles(bcd1);
   if (abc_p != abc) FreeDoubles(abc);
   if (abc4_p != abc4) FreeDoubles(abc4);
   if (abc3_p != abc3) FreeDoubles(abc3);
   if (abc2_p != abc2) FreeDoubles(abc2);
   if (abc1_p != abc1) FreeDoubles(abc1);
   if (bd_p != bd) FreeDoubles(bd);
   if (dexbey_p != dexbey) FreeDoubles(dexbey);
   if (bexdey_p != bexdey) FreeDoubles(bexdey);
   if (ac_p != ac) FreeDoubles(ac);
   if (cexaey_p != cexaey) FreeDoubles(cexaey);
   if (aexcey_p != aexcey) FreeDoubles(aexcey);
   if (da_p != da) FreeDoubles(da);
   if (aexdey_p != aexdey) FreeDoubles(aexdey);
   if (dexaey_p != dexaey) FreeDoubles(dexaey);
   if (cd_p != cd) FreeDoubles(cd);
   if (dexcey_p != dexcey) FreeDoubles(dexcey);
   if (cexdey_p != cexdey) FreeDoubles(cexdey);
   if (bc_p != bc) FreeDoubles(bc);
   if (cexbey_p != cexbey) FreeDoubles(cexbey);
   if (bexcey_p != bexcey) FreeDoubles(bexcey);
   if (ab_p != ab) FreeDoubles(ab);
   if (bexaey_p != bexaey) FreeDoubles(bexaey);
   if (aexbey_p != aexbey) FreeDoubles(aexbey);
   if (dez_p != dez) FreeDoubles(dez);
   if (dey_p != dey) FreeDoubles(dey);
   if (dex_p != dex) FreeDoubles(dex);
   if (ll4z_p != ll4z) FreeDoubles(ll4z);
   if (ll4y_p != ll4y) FreeDoubles(ll4y);
   if (ll4x_p != ll4x) FreeDoubles(ll4x);
   if (pezd4_p != pezd4) FreeDoubles(pezd4);
   if (peyd4_p != peyd4) FreeDoubles(peyd4);
   if (pexd4_p != pexd4) FreeDoubles(pexd4);
   if (cez_p != cez) FreeDoubles(cez);
   if (cey_p != cey) FreeDoubles(cey);
   if (cex_p != cex) FreeDoubles(cex);
   if (ll3z_p != ll3z) FreeDoubles(ll3z);
   if (ll3y_p != ll3y) FreeDoubles(ll3y);
   if (ll3x_p != ll3x) FreeDoubles(ll3x);
   if (pezd3_p != pezd3) FreeDoubles(pezd3);
   if (peyd3_p != peyd3) FreeDoubles(peyd3);
   if (pexd3_p != pexd3) FreeDoubles(pexd3);
   if (bez_p != bez) FreeDoubles(bez);
   if (bey_p != bey) FreeDoubles(bey);
   if (bex_p != bex) FreeDoubles(bex);
   if (ll2z_p != ll2z) FreeDoubles(ll2z);
   if (ll2y_p != ll2y) FreeDoubles(ll2y);
   if (ll2x_p != ll2x) FreeDoubles(ll2x);
   if (pezd2_p != pezd2) FreeDoubles(pezd2);
   if (peyd2_p != peyd2) FreeDoubles(peyd2);
   if (pexd2_p != pexd2) FreeDoubles(pexd2);
   if (aez_p != aez) FreeDoubles(aez);
   if (aey_p != aey) FreeDoubles(aey);
   if (aex_p != aex) FreeDoubles(aex);
   if (ll1z_p != ll1z) FreeDoubles(ll1z);
   if (ll1y_p != ll1y) FreeDoubles(ll1y);
   if (ll1x_p != ll1x) FreeDoubles(ll1x);
   if (pezd_p != pezd) FreeDoubles(pezd);
   if (peyd_p != peyd) FreeDoubles(peyd);
   if (pexd_p != pexd) FreeDoubles(pexd);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = inSphere_IIIII_bigfloat(p1, p2, p3, p4, p5);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (d3_p != d3) FreeDoubles(d3);
 if (l4x_p != l4x) FreeDoubles(l4x);
 if (l4y_p != l4y) FreeDoubles(l4y);
 if (l4z_p != l4z) FreeDoubles(l4z);
 if (d4_p != d4) FreeDoubles(d4);
 if (l5x_p != l5x) FreeDoubles(l5x);
 if (l5y_p != l5y) FreeDoubles(l5y);
 if (l5z_p != l5z) FreeDoubles(l5z);
 if (d5_p != d5) FreeDoubles(d5);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int inSphere_IIIII(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, const genericPoint& p5)
{
   int ret;
   ret = inSphere_IIIII_interval(p1, p2, p3, p4, p5);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return inSphere_IIIII_exact(p1, p2, p3, p4, p5);
}

inline bool lambda2d_SSI_interval(interval_number ea1x, interval_number ea1y, interval_number ea2x, interval_number ea2y, interval_number eb1x, interval_number eb1y, interval_number eb2x, interval_number eb2y, interval_number& lambda_x, interval_number& lambda_y, interval_number& lambda_det)
{
   setFPUModeToRoundUP();
   const interval_number t1a(ea1x * ea2y);
   const interval_number t1b(ea2x * ea1y);
   const interval_number t1(t1a - t1b);
   const interval_number tx2(eb1x - eb2x);
   const interval_number t3a(eb1x * eb2y);
   const interval_number t3b(eb2x * eb1y);
   const interval_number t3(t3a - t3b);
   const interval_number tx4(ea1x - ea2x);
   const interval_number ty2(eb1y - eb2y);
   const interval_number ty4(ea1y - ea2y);
   const interval_number lxa(t1 * tx2);
   const interval_number lxb(t3 * tx4);
   lambda_x = lxa - lxb;
   const interval_number lya(t1 * ty2);
   const interval_number lyb(t3 * ty4);
   lambda_y = lya - lyb;
   const interval_number deta(tx4 * ty2);
   const interval_number detb(tx2 * ty4);
   lambda_det = deta - detb;
   setFPUModeToRoundNEAR();

   return lambda_det.signIsReliable();
}

inline void lambda2d_SSI_bigfloat(bigfloat ea1x, bigfloat ea1y, bigfloat ea2x, bigfloat ea2y, bigfloat eb1x, bigfloat eb1y, bigfloat eb2x, bigfloat eb2y, bigfloat& lambda_x, bigfloat& lambda_y, bigfloat& lambda_det)
{
   const bigfloat t1a(ea1x * ea2y);
   const bigfloat t1b(ea2x * ea1y);
   const bigfloat t1(t1a - t1b);
   const bigfloat tx2(eb1x - eb2x);
   const bigfloat t3a(eb1x * eb2y);
   const bigfloat t3b(eb2x * eb1y);
   const bigfloat t3(t3a - t3b);
   const bigfloat tx4(ea1x - ea2x);
   const bigfloat ty2(eb1y - eb2y);
   const bigfloat ty4(ea1y - ea2y);
   const bigfloat lxa(t1 * tx2);
   const bigfloat lxb(t3 * tx4);
   lambda_x = lxa - lxb;
   const bigfloat lya(t1 * ty2);
   const bigfloat lyb(t3 * ty4);
   lambda_y = lya - lyb;
   const bigfloat deta(tx4 * ty2);
   const bigfloat detb(tx2 * ty4);
   lambda_det = deta - detb;
}

inline void lambda2d_SSI_exact(double ea1x, double ea1y, double ea2x, double ea2y, double eb1x, double eb1y, double eb2x, double eb2y, double **lambda_x, int& lambda_x_len, double **lambda_y, int& lambda_y_len, double **lambda_det, int& lambda_det_len)
{
   
   double t1a[2];
   expansionObject::Two_Prod(ea1x, ea2y, t1a);
   double t1b[2];
   expansionObject::Two_Prod(ea2x, ea1y, t1b);
   double t1[4];
   expansionObject::Two_Two_Diff(t1a, t1b, t1);
   double tx2[2];
   expansionObject::two_Diff(eb1x, eb2x, tx2);
   double t3a[2];
   expansionObject::Two_Prod(eb1x, eb2y, t3a);
   double t3b[2];
   expansionObject::Two_Prod(eb2x, eb1y, t3b);
   double t3[4];
   expansionObject::Two_Two_Diff(t3a, t3b, t3);
   double tx4[2];
   expansionObject::two_Diff(ea1x, ea2x, tx4);
   double ty2[2];
   expansionObject::two_Diff(eb1y, eb2y, ty2);
   double ty4[2];
   expansionObject::two_Diff(ea1y, ea2y, ty4);
   double lxa[16];
   int lxa_len = expansionObject::Gen_Product(4, t1, 2, tx2, lxa);
   double lxb[16];
   int lxb_len = expansionObject::Gen_Product(4, t3, 2, tx4, lxb);
   lambda_x_len = expansionObject::Gen_Diff(lxa_len, lxa, lxb_len, lxb, *lambda_x);
   double lya[16];
   int lya_len = expansionObject::Gen_Product(4, t1, 2, ty2, lya);
   double lyb[16];
   int lyb_len = expansionObject::Gen_Product(4, t3, 2, ty4, lyb);
   lambda_y_len = expansionObject::Gen_Diff(lya_len, lya, lyb_len, lyb, *lambda_y);
   double deta[8];
   int deta_len = expansionObject::Gen_Product(2, tx4, 2, ty2, deta);
   double detb[8];
   int detb_len = expansionObject::Gen_Product(2, tx2, 2, ty4, detb);
   lambda_det_len = expansionObject::Gen_Diff(deta_len, deta, detb_len, detb, *lambda_det);

}

inline bool lambda3d_LNC_interval(interval_number px, interval_number py, interval_number pz, interval_number qx, interval_number qy, interval_number qz, interval_number t, interval_number& lambda_x, interval_number& lambda_y, interval_number& lambda_z, interval_number& lambda_d)
{
   setFPUModeToRoundUP();
   const interval_number vx(px - qx);
   const interval_number vy(py - qy);
   const interval_number vz(pz - qz);
   const interval_number vxt(vx * t);
   const interval_number vyt(vy * t);
   const interval_number vzt(vz * t);
   lambda_x = px - vxt;
   lambda_y = py - vyt;
   lambda_z = pz - vzt;
   lambda_d = 1;
   setFPUModeToRoundNEAR();

   return true;
}

inline void lambda3d_LNC_bigfloat(bigfloat px, bigfloat py, bigfloat pz, bigfloat qx, bigfloat qy, bigfloat qz, bigfloat t, bigfloat& lambda_x, bigfloat& lambda_y, bigfloat& lambda_z, bigfloat& lambda_d)
{
   const bigfloat vx(px - qx);
   const bigfloat vy(py - qy);
   const bigfloat vz(pz - qz);
   const bigfloat vxt(vx * t);
   const bigfloat vyt(vy * t);
   const bigfloat vzt(vz * t);
   lambda_x = px - vxt;
   lambda_y = py - vyt;
   lambda_z = pz - vzt;
   lambda_d = 1;
}

inline void lambda3d_LNC_exact(double px, double py, double pz, double qx, double qy, double qz, double t, double **lambda_x, int& lambda_x_len, double **lambda_y, int& lambda_y_len, double **lambda_z, int& lambda_z_len, double **lambda_d, int& lambda_d_len)
{
   
   double vx[2];
   expansionObject::two_Diff(px, qx, vx);
   double vy[2];
   expansionObject::two_Diff(py, qy, vy);
   double vz[2];
   expansionObject::two_Diff(pz, qz, vz);
   double vxt[4];
   expansionObject::Two_One_Prod(vx, t, vxt);
   double vyt[4];
   expansionObject::Two_One_Prod(vy, t, vyt);
   double vzt[4];
   expansionObject::Two_One_Prod(vz, t, vzt);
   lambda_x_len = expansionObject::Gen_Diff(1, &px, 4, vxt, *lambda_x);
   lambda_y_len = expansionObject::Gen_Diff(1, &py, 4, vyt, *lambda_y);
   lambda_z_len = expansionObject::Gen_Diff(1, &pz, 4, vzt, *lambda_z);
   (*lambda_d)[0] = 1;
   lambda_d_len = 1;

}

inline bool lambda3d_LPI_interval(interval_number px, interval_number py, interval_number pz, interval_number qx, interval_number qy, interval_number qz, interval_number rx, interval_number ry, interval_number rz, interval_number sx, interval_number sy, interval_number sz, interval_number tx, interval_number ty, interval_number tz, interval_number& lambda_x, interval_number& lambda_y, interval_number& lambda_z, interval_number& lambda_d)
{
   setFPUModeToRoundUP();
   const interval_number a11(px - qx);
   const interval_number a12(py - qy);
   const interval_number a13(pz - qz);
   const interval_number a21(sx - rx);
   const interval_number a22(sy - ry);
   const interval_number a23(sz - rz);
   const interval_number a31(tx - rx);
   const interval_number a32(ty - ry);
   const interval_number a33(tz - rz);
   const interval_number tv1(a22 * a33);
   const interval_number tv2(a23 * a32);
   const interval_number a2233(tv1 - tv2);
   const interval_number tv3(a21 * a33);
   const interval_number tv4(a23 * a31);
   const interval_number a2133(tv3 - tv4);
   const interval_number tv5(a21 * a32);
   const interval_number tv6(a22 * a31);
   const interval_number a2132(tv5 - tv6);
   const interval_number tv7(a11 * a2233);
   const interval_number tv8(a12 * a2133);
   const interval_number tv9(a13 * a2132);
   const interval_number tt1(tv7 - tv8);
   const interval_number ld(tt1 + tv9);
   const interval_number px_rx(px - rx);
   const interval_number py_ry(py - ry);
   const interval_number pz_rz(pz - rz);
   const interval_number tt2(py_ry * a2133);
   const interval_number tt3(px_rx * a2233);
   const interval_number tt4(pz_rz * a2132);
   const interval_number tt5(tt3 + tt4);
   const interval_number n(tt5 - tt2);
   const interval_number ax(a11 * n);
   const interval_number ay(a12 * n);
   const interval_number az(a13 * n);
   const interval_number dpx(ld * px);
   const interval_number dpy(ld * py);
   const interval_number dpz(ld * pz);
   lambda_x = dpx - ax;
   lambda_y = dpy - ay;
   lambda_z = dpz - az;
   lambda_d = tt1 + tv9;
   setFPUModeToRoundNEAR();

   return lambda_d.signIsReliable();
}

inline void lambda3d_LPI_bigfloat(bigfloat px, bigfloat py, bigfloat pz, bigfloat qx, bigfloat qy, bigfloat qz, bigfloat rx, bigfloat ry, bigfloat rz, bigfloat sx, bigfloat sy, bigfloat sz, bigfloat tx, bigfloat ty, bigfloat tz, bigfloat& lambda_x, bigfloat& lambda_y, bigfloat& lambda_z, bigfloat& lambda_d)
{
   const bigfloat a11(px - qx);
   const bigfloat a12(py - qy);
   const bigfloat a13(pz - qz);
   const bigfloat a21(sx - rx);
   const bigfloat a22(sy - ry);
   const bigfloat a23(sz - rz);
   const bigfloat a31(tx - rx);
   const bigfloat a32(ty - ry);
   const bigfloat a33(tz - rz);
   const bigfloat tv1(a22 * a33);
   const bigfloat tv2(a23 * a32);
   const bigfloat a2233(tv1 - tv2);
   const bigfloat tv3(a21 * a33);
   const bigfloat tv4(a23 * a31);
   const bigfloat a2133(tv3 - tv4);
   const bigfloat tv5(a21 * a32);
   const bigfloat tv6(a22 * a31);
   const bigfloat a2132(tv5 - tv6);
   const bigfloat tv7(a11 * a2233);
   const bigfloat tv8(a12 * a2133);
   const bigfloat tv9(a13 * a2132);
   const bigfloat tt1(tv7 - tv8);
   const bigfloat ld(tt1 + tv9);
   const bigfloat px_rx(px - rx);
   const bigfloat py_ry(py - ry);
   const bigfloat pz_rz(pz - rz);
   const bigfloat tt2(py_ry * a2133);
   const bigfloat tt3(px_rx * a2233);
   const bigfloat tt4(pz_rz * a2132);
   const bigfloat tt5(tt3 + tt4);
   const bigfloat n(tt5 - tt2);
   const bigfloat ax(a11 * n);
   const bigfloat ay(a12 * n);
   const bigfloat az(a13 * n);
   const bigfloat dpx(ld * px);
   const bigfloat dpy(ld * py);
   const bigfloat dpz(ld * pz);
   lambda_x = dpx - ax;
   lambda_y = dpy - ay;
   lambda_z = dpz - az;
   lambda_d = tt1 + tv9;
}

inline void lambda3d_LPI_exact(double px, double py, double pz, double qx, double qy, double qz, double rx, double ry, double rz, double sx, double sy, double sz, double tx, double ty, double tz, double **lambda_x, int& lambda_x_len, double **lambda_y, int& lambda_y_len, double **lambda_z, int& lambda_z_len, double **lambda_d, int& lambda_d_len)
{
   
   double a11[2];
   expansionObject::two_Diff(px, qx, a11);
   double a12[2];
   expansionObject::two_Diff(py, qy, a12);
   double a13[2];
   expansionObject::two_Diff(pz, qz, a13);
   double a21[2];
   expansionObject::two_Diff(sx, rx, a21);
   double a22[2];
   expansionObject::two_Diff(sy, ry, a22);
   double a23[2];
   expansionObject::two_Diff(sz, rz, a23);
   double a31[2];
   expansionObject::two_Diff(tx, rx, a31);
   double a32[2];
   expansionObject::two_Diff(ty, ry, a32);
   double a33[2];
   expansionObject::two_Diff(tz, rz, a33);
   double tv1[8];
   int tv1_len = expansionObject::Gen_Product(2, a22, 2, a33, tv1);
   double tv2[8];
   int tv2_len = expansionObject::Gen_Product(2, a23, 2, a32, tv2);
   double a2233[16];
   int a2233_len = expansionObject::Gen_Diff(tv1_len, tv1, tv2_len, tv2, a2233);
   double tv3[8];
   int tv3_len = expansionObject::Gen_Product(2, a21, 2, a33, tv3);
   double tv4[8];
   int tv4_len = expansionObject::Gen_Product(2, a23, 2, a31, tv4);
   double a2133[16];
   int a2133_len = expansionObject::Gen_Diff(tv3_len, tv3, tv4_len, tv4, a2133);
   double tv5[8];
   int tv5_len = expansionObject::Gen_Product(2, a21, 2, a32, tv5);
   double tv6[8];
   int tv6_len = expansionObject::Gen_Product(2, a22, 2, a31, tv6);
   double a2132[16];
   int a2132_len = expansionObject::Gen_Diff(tv5_len, tv5, tv6_len, tv6, a2132);
   double tv7[64];
   int tv7_len = expansionObject::Gen_Product(2, a11, a2233_len, a2233, tv7);
   double tv8[64];
   int tv8_len = expansionObject::Gen_Product(2, a12, a2133_len, a2133, tv8);
   double tv9[64];
   int tv9_len = expansionObject::Gen_Product(2, a13, a2132_len, a2132, tv9);
   double tt1[128];
   int tt1_len = expansionObject::Gen_Diff(tv7_len, tv7, tv8_len, tv8, tt1);
   double ld_p[128], *ld = ld_p;
   int ld_len = expansionObject::Gen_Sum_With_PreAlloc(tt1_len, tt1, tv9_len, tv9, &ld, 128);
   double px_rx[2];
   expansionObject::two_Diff(px, rx, px_rx);
   double py_ry[2];
   expansionObject::two_Diff(py, ry, py_ry);
   double pz_rz[2];
   expansionObject::two_Diff(pz, rz, pz_rz);
   double tt2[64];
   int tt2_len = expansionObject::Gen_Product(2, py_ry, a2133_len, a2133, tt2);
   double tt3[64];
   int tt3_len = expansionObject::Gen_Product(2, px_rx, a2233_len, a2233, tt3);
   double tt4[64];
   int tt4_len = expansionObject::Gen_Product(2, pz_rz, a2132_len, a2132, tt4);
   double tt5[128];
   int tt5_len = expansionObject::Gen_Sum(tt3_len, tt3, tt4_len, tt4, tt5);
   double n_p[128], *n = n_p;
   int n_len = expansionObject::Gen_Diff_With_PreAlloc(tt5_len, tt5, tt2_len, tt2, &n, 128);
   double ax_p[128], *ax = ax_p;
   int ax_len = expansionObject::Gen_Product_With_PreAlloc(2, a11, n_len, n, &ax, 128);
   double ay_p[128], *ay = ay_p;
   int ay_len = expansionObject::Gen_Product_With_PreAlloc(2, a12, n_len, n, &ay, 128);
   double az_p[128], *az = az_p;
   int az_len = expansionObject::Gen_Product_With_PreAlloc(2, a13, n_len, n, &az, 128);
   double dpx_p[128], *dpx = dpx_p;
   int dpx_len = expansionObject::Gen_Scale_With_PreAlloc(ld_len, ld, px, &dpx, 128);
   double dpy_p[128], *dpy = dpy_p;
   int dpy_len = expansionObject::Gen_Scale_With_PreAlloc(ld_len, ld, py, &dpy, 128);
   double dpz_p[128], *dpz = dpz_p;
   int dpz_len = expansionObject::Gen_Scale_With_PreAlloc(ld_len, ld, pz, &dpz, 128);
   lambda_x_len = expansionObject::Gen_Diff_With_PreAlloc(dpx_len, dpx, ax_len, ax, lambda_x, lambda_x_len);
   lambda_y_len = expansionObject::Gen_Diff_With_PreAlloc(dpy_len, dpy, ay_len, ay, lambda_y, lambda_y_len);
   lambda_z_len = expansionObject::Gen_Diff_With_PreAlloc(dpz_len, dpz, az_len, az, lambda_z, lambda_z_len);
   lambda_d_len = expansionObject::Gen_Sum_With_PreAlloc(tt1_len, tt1, tv9_len, tv9, lambda_d, lambda_d_len);

   if (dpz_p != dpz) FreeDoubles(dpz);
   if (dpy_p != dpy) FreeDoubles(dpy);
   if (dpx_p != dpx) FreeDoubles(dpx);
   if (az_p != az) FreeDoubles(az);
   if (ay_p != ay) FreeDoubles(ay);
   if (ax_p != ax) FreeDoubles(ax);
   if (n_p != n) FreeDoubles(n);
   if (ld_p != ld) FreeDoubles(ld);
}

inline bool lambda3d_TPI_interval(interval_number ov1x, interval_number ov1y, interval_number ov1z, interval_number ov2x, interval_number ov2y, interval_number ov2z, interval_number ov3x, interval_number ov3y, interval_number ov3z, interval_number ow1x, interval_number ow1y, interval_number ow1z, interval_number ow2x, interval_number ow2y, interval_number ow2z, interval_number ow3x, interval_number ow3y, interval_number ow3z, interval_number ou1x, interval_number ou1y, interval_number ou1z, interval_number ou2x, interval_number ou2y, interval_number ou2z, interval_number ou3x, interval_number ou3y, interval_number ou3z, interval_number& lambda_x, interval_number& lambda_y, interval_number& lambda_z, interval_number& lambda_d)
{
   setFPUModeToRoundUP();
   const interval_number v3x(ov3x - ov2x);
   const interval_number v3y(ov3y - ov2y);
   const interval_number v3z(ov3z - ov2z);
   const interval_number v2x(ov2x - ov1x);
   const interval_number v2y(ov2y - ov1y);
   const interval_number v2z(ov2z - ov1z);
   const interval_number w3x(ow3x - ow2x);
   const interval_number w3y(ow3y - ow2y);
   const interval_number w3z(ow3z - ow2z);
   const interval_number w2x(ow2x - ow1x);
   const interval_number w2y(ow2y - ow1y);
   const interval_number w2z(ow2z - ow1z);
   const interval_number u3x(ou3x - ou2x);
   const interval_number u3y(ou3y - ou2y);
   const interval_number u3z(ou3z - ou2z);
   const interval_number u2x(ou2x - ou1x);
   const interval_number u2y(ou2y - ou1y);
   const interval_number u2z(ou2z - ou1z);
   const interval_number nvx1(v2y * v3z);
   const interval_number nvx2(v2z * v3y);
   const interval_number nvx(nvx1 - nvx2);
   const interval_number nvy1(v3x * v2z);
   const interval_number nvy2(v3z * v2x);
   const interval_number nvy(nvy1 - nvy2);
   const interval_number nvz1(v2x * v3y);
   const interval_number nvz2(v2y * v3x);
   const interval_number nvz(nvz1 - nvz2);
   const interval_number nwx1(w2y * w3z);
   const interval_number nwx2(w2z * w3y);
   const interval_number nwx(nwx1 - nwx2);
   const interval_number nwy1(w3x * w2z);
   const interval_number nwy2(w3z * w2x);
   const interval_number nwy(nwy1 - nwy2);
   const interval_number nwz1(w2x * w3y);
   const interval_number nwz2(w2y * w3x);
   const interval_number nwz(nwz1 - nwz2);
   const interval_number nux1(u2y * u3z);
   const interval_number nux2(u2z * u3y);
   const interval_number nux(nux1 - nux2);
   const interval_number nuy1(u3x * u2z);
   const interval_number nuy2(u3z * u2x);
   const interval_number nuy(nuy1 - nuy2);
   const interval_number nuz1(u2x * u3y);
   const interval_number nuz2(u2y * u3x);
   const interval_number nuz(nuz1 - nuz2);
   const interval_number nwyuz1(nwy * nuz);
   const interval_number nwyuz2(nwz * nuy);
   const interval_number nwyuz(nwyuz1 - nwyuz2);
   const interval_number nwxuz1(nwx * nuz);
   const interval_number nwxuz2(nwz * nux);
   const interval_number nwxuz(nwxuz1 - nwxuz2);
   const interval_number nwxuy1(nwx * nuy);
   const interval_number nwxuy2(nwy * nux);
   const interval_number nwxuy(nwxuy1 - nwxuy2);
   const interval_number nvyuz1(nvy * nuz);
   const interval_number nvyuz2(nvz * nuy);
   const interval_number nvyuz(nvyuz1 - nvyuz2);
   const interval_number nvxuz1(nvx * nuz);
   const interval_number nvxuz2(nvz * nux);
   const interval_number nvxuz(nvxuz1 - nvxuz2);
   const interval_number nvxuy1(nvx * nuy);
   const interval_number nvxuy2(nvy * nux);
   const interval_number nvxuy(nvxuy1 - nvxuy2);
   const interval_number nvywz1(nvy * nwz);
   const interval_number nvywz2(nvz * nwy);
   const interval_number nvywz(nvywz1 - nvywz2);
   const interval_number nvxwz1(nvx * nwz);
   const interval_number nvxwz2(nvz * nwx);
   const interval_number nvxwz(nvxwz1 - nvxwz2);
   const interval_number nvxwy1(nvx * nwy);
   const interval_number nvxwy2(nvy * nwx);
   const interval_number nvxwy(nvxwy1 - nvxwy2);
   const interval_number p1a(nvx * ov1x);
   const interval_number p1b(nvy * ov1y);
   const interval_number p1c(nvz * ov1z);
   const interval_number p1ab(p1a + p1b);
   const interval_number p1(p1ab + p1c);
   const interval_number p2a(nwx * ow1x);
   const interval_number p2b(nwy * ow1y);
   const interval_number p2c(nwz * ow1z);
   const interval_number p2ab(p2a + p2b);
   const interval_number p2(p2ab + p2c);
   const interval_number p3a(nux * ou1x);
   const interval_number p3b(nuy * ou1y);
   const interval_number p3c(nuz * ou1z);
   const interval_number p3ab(p3a + p3b);
   const interval_number p3(p3ab + p3c);
   const interval_number lxa(p1 * nwyuz);
   const interval_number lxb(p3 * nvywz);
   const interval_number lxc(p2 * nvyuz);
   const interval_number lxab(lxa + lxb);
   lambda_x = lxab - lxc;
   const interval_number lya(p2 * nvxuz);
   const interval_number lyb(p3 * nvxwz);
   const interval_number lyc(p1 * nwxuz);
   const interval_number lybc(lyc + lyb);
   lambda_y = lya - lybc;
   const interval_number lza(p3 * nvxwy);
   const interval_number lzb(p1 * nwxuy);
   const interval_number lzc(p2 * nvxuy);
   const interval_number lzab(lza + lzb);
   lambda_z = lzab - lzc;
   const interval_number da(nvx * nwyuz);
   const interval_number db(nvz * nwxuy);
   const interval_number dc(nvy * nwxuz);
   const interval_number dab(da + db);
   lambda_d = dab - dc;
   setFPUModeToRoundNEAR();

   return lambda_d.signIsReliable();
}

inline void lambda3d_TPI_bigfloat(bigfloat ov1x, bigfloat ov1y, bigfloat ov1z, bigfloat ov2x, bigfloat ov2y, bigfloat ov2z, bigfloat ov3x, bigfloat ov3y, bigfloat ov3z, bigfloat ow1x, bigfloat ow1y, bigfloat ow1z, bigfloat ow2x, bigfloat ow2y, bigfloat ow2z, bigfloat ow3x, bigfloat ow3y, bigfloat ow3z, bigfloat ou1x, bigfloat ou1y, bigfloat ou1z, bigfloat ou2x, bigfloat ou2y, bigfloat ou2z, bigfloat ou3x, bigfloat ou3y, bigfloat ou3z, bigfloat& lambda_x, bigfloat& lambda_y, bigfloat& lambda_z, bigfloat& lambda_d)
{
   const bigfloat v3x(ov3x - ov2x);
   const bigfloat v3y(ov3y - ov2y);
   const bigfloat v3z(ov3z - ov2z);
   const bigfloat v2x(ov2x - ov1x);
   const bigfloat v2y(ov2y - ov1y);
   const bigfloat v2z(ov2z - ov1z);
   const bigfloat w3x(ow3x - ow2x);
   const bigfloat w3y(ow3y - ow2y);
   const bigfloat w3z(ow3z - ow2z);
   const bigfloat w2x(ow2x - ow1x);
   const bigfloat w2y(ow2y - ow1y);
   const bigfloat w2z(ow2z - ow1z);
   const bigfloat u3x(ou3x - ou2x);
   const bigfloat u3y(ou3y - ou2y);
   const bigfloat u3z(ou3z - ou2z);
   const bigfloat u2x(ou2x - ou1x);
   const bigfloat u2y(ou2y - ou1y);
   const bigfloat u2z(ou2z - ou1z);
   const bigfloat nvx1(v2y * v3z);
   const bigfloat nvx2(v2z * v3y);
   const bigfloat nvx(nvx1 - nvx2);
   const bigfloat nvy1(v3x * v2z);
   const bigfloat nvy2(v3z * v2x);
   const bigfloat nvy(nvy1 - nvy2);
   const bigfloat nvz1(v2x * v3y);
   const bigfloat nvz2(v2y * v3x);
   const bigfloat nvz(nvz1 - nvz2);
   const bigfloat nwx1(w2y * w3z);
   const bigfloat nwx2(w2z * w3y);
   const bigfloat nwx(nwx1 - nwx2);
   const bigfloat nwy1(w3x * w2z);
   const bigfloat nwy2(w3z * w2x);
   const bigfloat nwy(nwy1 - nwy2);
   const bigfloat nwz1(w2x * w3y);
   const bigfloat nwz2(w2y * w3x);
   const bigfloat nwz(nwz1 - nwz2);
   const bigfloat nux1(u2y * u3z);
   const bigfloat nux2(u2z * u3y);
   const bigfloat nux(nux1 - nux2);
   const bigfloat nuy1(u3x * u2z);
   const bigfloat nuy2(u3z * u2x);
   const bigfloat nuy(nuy1 - nuy2);
   const bigfloat nuz1(u2x * u3y);
   const bigfloat nuz2(u2y * u3x);
   const bigfloat nuz(nuz1 - nuz2);
   const bigfloat nwyuz1(nwy * nuz);
   const bigfloat nwyuz2(nwz * nuy);
   const bigfloat nwyuz(nwyuz1 - nwyuz2);
   const bigfloat nwxuz1(nwx * nuz);
   const bigfloat nwxuz2(nwz * nux);
   const bigfloat nwxuz(nwxuz1 - nwxuz2);
   const bigfloat nwxuy1(nwx * nuy);
   const bigfloat nwxuy2(nwy * nux);
   const bigfloat nwxuy(nwxuy1 - nwxuy2);
   const bigfloat nvyuz1(nvy * nuz);
   const bigfloat nvyuz2(nvz * nuy);
   const bigfloat nvyuz(nvyuz1 - nvyuz2);
   const bigfloat nvxuz1(nvx * nuz);
   const bigfloat nvxuz2(nvz * nux);
   const bigfloat nvxuz(nvxuz1 - nvxuz2);
   const bigfloat nvxuy1(nvx * nuy);
   const bigfloat nvxuy2(nvy * nux);
   const bigfloat nvxuy(nvxuy1 - nvxuy2);
   const bigfloat nvywz1(nvy * nwz);
   const bigfloat nvywz2(nvz * nwy);
   const bigfloat nvywz(nvywz1 - nvywz2);
   const bigfloat nvxwz1(nvx * nwz);
   const bigfloat nvxwz2(nvz * nwx);
   const bigfloat nvxwz(nvxwz1 - nvxwz2);
   const bigfloat nvxwy1(nvx * nwy);
   const bigfloat nvxwy2(nvy * nwx);
   const bigfloat nvxwy(nvxwy1 - nvxwy2);
   const bigfloat p1a(nvx * ov1x);
   const bigfloat p1b(nvy * ov1y);
   const bigfloat p1c(nvz * ov1z);
   const bigfloat p1ab(p1a + p1b);
   const bigfloat p1(p1ab + p1c);
   const bigfloat p2a(nwx * ow1x);
   const bigfloat p2b(nwy * ow1y);
   const bigfloat p2c(nwz * ow1z);
   const bigfloat p2ab(p2a + p2b);
   const bigfloat p2(p2ab + p2c);
   const bigfloat p3a(nux * ou1x);
   const bigfloat p3b(nuy * ou1y);
   const bigfloat p3c(nuz * ou1z);
   const bigfloat p3ab(p3a + p3b);
   const bigfloat p3(p3ab + p3c);
   const bigfloat lxa(p1 * nwyuz);
   const bigfloat lxb(p3 * nvywz);
   const bigfloat lxc(p2 * nvyuz);
   const bigfloat lxab(lxa + lxb);
   lambda_x = lxab - lxc;
   const bigfloat lya(p2 * nvxuz);
   const bigfloat lyb(p3 * nvxwz);
   const bigfloat lyc(p1 * nwxuz);
   const bigfloat lybc(lyc + lyb);
   lambda_y = lya - lybc;
   const bigfloat lza(p3 * nvxwy);
   const bigfloat lzb(p1 * nwxuy);
   const bigfloat lzc(p2 * nvxuy);
   const bigfloat lzab(lza + lzb);
   lambda_z = lzab - lzc;
   const bigfloat da(nvx * nwyuz);
   const bigfloat db(nvz * nwxuy);
   const bigfloat dc(nvy * nwxuz);
   const bigfloat dab(da + db);
   lambda_d = dab - dc;
}

inline void lambda3d_TPI_exact(double ov1x, double ov1y, double ov1z, double ov2x, double ov2y, double ov2z, double ov3x, double ov3y, double ov3z, double ow1x, double ow1y, double ow1z, double ow2x, double ow2y, double ow2z, double ow3x, double ow3y, double ow3z, double ou1x, double ou1y, double ou1z, double ou2x, double ou2y, double ou2z, double ou3x, double ou3y, double ou3z, double **lambda_x, int& lambda_x_len, double **lambda_y, int& lambda_y_len, double **lambda_z, int& lambda_z_len, double **lambda_d, int& lambda_d_len)
{
   
   double v3x[2];
   expansionObject::two_Diff(ov3x, ov2x, v3x);
   double v3y[2];
   expansionObject::two_Diff(ov3y, ov2y, v3y);
   double v3z[2];
   expansionObject::two_Diff(ov3z, ov2z, v3z);
   double v2x[2];
   expansionObject::two_Diff(ov2x, ov1x, v2x);
   double v2y[2];
   expansionObject::two_Diff(ov2y, ov1y, v2y);
   double v2z[2];
   expansionObject::two_Diff(ov2z, ov1z, v2z);
   double w3x[2];
   expansionObject::two_Diff(ow3x, ow2x, w3x);
   double w3y[2];
   expansionObject::two_Diff(ow3y, ow2y, w3y);
   double w3z[2];
   expansionObject::two_Diff(ow3z, ow2z, w3z);
   double w2x[2];
   expansionObject::two_Diff(ow2x, ow1x, w2x);
   double w2y[2];
   expansionObject::two_Diff(ow2y, ow1y, w2y);
   double w2z[2];
   expansionObject::two_Diff(ow2z, ow1z, w2z);
   double u3x[2];
   expansionObject::two_Diff(ou3x, ou2x, u3x);
   double u3y[2];
   expansionObject::two_Diff(ou3y, ou2y, u3y);
   double u3z[2];
   expansionObject::two_Diff(ou3z, ou2z, u3z);
   double u2x[2];
   expansionObject::two_Diff(ou2x, ou1x, u2x);
   double u2y[2];
   expansionObject::two_Diff(ou2y, ou1y, u2y);
   double u2z[2];
   expansionObject::two_Diff(ou2z, ou1z, u2z);
   double nvx1[8];
   int nvx1_len = expansionObject::Gen_Product(2, v2y, 2, v3z, nvx1);
   double nvx2[8];
   int nvx2_len = expansionObject::Gen_Product(2, v2z, 2, v3y, nvx2);
   double nvx[16];
   int nvx_len = expansionObject::Gen_Diff(nvx1_len, nvx1, nvx2_len, nvx2, nvx);
   double nvy1[8];
   int nvy1_len = expansionObject::Gen_Product(2, v3x, 2, v2z, nvy1);
   double nvy2[8];
   int nvy2_len = expansionObject::Gen_Product(2, v3z, 2, v2x, nvy2);
   double nvy[16];
   int nvy_len = expansionObject::Gen_Diff(nvy1_len, nvy1, nvy2_len, nvy2, nvy);
   double nvz1[8];
   int nvz1_len = expansionObject::Gen_Product(2, v2x, 2, v3y, nvz1);
   double nvz2[8];
   int nvz2_len = expansionObject::Gen_Product(2, v2y, 2, v3x, nvz2);
   double nvz[16];
   int nvz_len = expansionObject::Gen_Diff(nvz1_len, nvz1, nvz2_len, nvz2, nvz);
   double nwx1[8];
   int nwx1_len = expansionObject::Gen_Product(2, w2y, 2, w3z, nwx1);
   double nwx2[8];
   int nwx2_len = expansionObject::Gen_Product(2, w2z, 2, w3y, nwx2);
   double nwx[16];
   int nwx_len = expansionObject::Gen_Diff(nwx1_len, nwx1, nwx2_len, nwx2, nwx);
   double nwy1[8];
   int nwy1_len = expansionObject::Gen_Product(2, w3x, 2, w2z, nwy1);
   double nwy2[8];
   int nwy2_len = expansionObject::Gen_Product(2, w3z, 2, w2x, nwy2);
   double nwy[16];
   int nwy_len = expansionObject::Gen_Diff(nwy1_len, nwy1, nwy2_len, nwy2, nwy);
   double nwz1[8];
   int nwz1_len = expansionObject::Gen_Product(2, w2x, 2, w3y, nwz1);
   double nwz2[8];
   int nwz2_len = expansionObject::Gen_Product(2, w2y, 2, w3x, nwz2);
   double nwz[16];
   int nwz_len = expansionObject::Gen_Diff(nwz1_len, nwz1, nwz2_len, nwz2, nwz);
   double nux1[8];
   int nux1_len = expansionObject::Gen_Product(2, u2y, 2, u3z, nux1);
   double nux2[8];
   int nux2_len = expansionObject::Gen_Product(2, u2z, 2, u3y, nux2);
   double nux[16];
   int nux_len = expansionObject::Gen_Diff(nux1_len, nux1, nux2_len, nux2, nux);
   double nuy1[8];
   int nuy1_len = expansionObject::Gen_Product(2, u3x, 2, u2z, nuy1);
   double nuy2[8];
   int nuy2_len = expansionObject::Gen_Product(2, u3z, 2, u2x, nuy2);
   double nuy[16];
   int nuy_len = expansionObject::Gen_Diff(nuy1_len, nuy1, nuy2_len, nuy2, nuy);
   double nuz1[8];
   int nuz1_len = expansionObject::Gen_Product(2, u2x, 2, u3y, nuz1);
   double nuz2[8];
   int nuz2_len = expansionObject::Gen_Product(2, u2y, 2, u3x, nuz2);
   double nuz[16];
   int nuz_len = expansionObject::Gen_Diff(nuz1_len, nuz1, nuz2_len, nuz2, nuz);
   double nwyuz1_p[16], *nwyuz1 = nwyuz1_p;
   int nwyuz1_len = expansionObject::Gen_Product_With_PreAlloc(nwy_len, nwy, nuz_len, nuz, &nwyuz1, 16);
   double nwyuz2_p[16], *nwyuz2 = nwyuz2_p;
   int nwyuz2_len = expansionObject::Gen_Product_With_PreAlloc(nwz_len, nwz, nuy_len, nuy, &nwyuz2, 16);
   double nwyuz_p[16], *nwyuz = nwyuz_p;
   int nwyuz_len = expansionObject::Gen_Diff_With_PreAlloc(nwyuz1_len, nwyuz1, nwyuz2_len, nwyuz2, &nwyuz, 16);
   double nwxuz1_p[16], *nwxuz1 = nwxuz1_p;
   int nwxuz1_len = expansionObject::Gen_Product_With_PreAlloc(nwx_len, nwx, nuz_len, nuz, &nwxuz1, 16);
   double nwxuz2_p[16], *nwxuz2 = nwxuz2_p;
   int nwxuz2_len = expansionObject::Gen_Product_With_PreAlloc(nwz_len, nwz, nux_len, nux, &nwxuz2, 16);
   double nwxuz_p[16], *nwxuz = nwxuz_p;
   int nwxuz_len = expansionObject::Gen_Diff_With_PreAlloc(nwxuz1_len, nwxuz1, nwxuz2_len, nwxuz2, &nwxuz, 16);
   double nwxuy1_p[16], *nwxuy1 = nwxuy1_p;
   int nwxuy1_len = expansionObject::Gen_Product_With_PreAlloc(nwx_len, nwx, nuy_len, nuy, &nwxuy1, 16);
   double nwxuy2_p[16], *nwxuy2 = nwxuy2_p;
   int nwxuy2_len = expansionObject::Gen_Product_With_PreAlloc(nwy_len, nwy, nux_len, nux, &nwxuy2, 16);
   double nwxuy_p[16], *nwxuy = nwxuy_p;
   int nwxuy_len = expansionObject::Gen_Diff_With_PreAlloc(nwxuy1_len, nwxuy1, nwxuy2_len, nwxuy2, &nwxuy, 16);
   double nvyuz1_p[16], *nvyuz1 = nvyuz1_p;
   int nvyuz1_len = expansionObject::Gen_Product_With_PreAlloc(nvy_len, nvy, nuz_len, nuz, &nvyuz1, 16);
   double nvyuz2_p[16], *nvyuz2 = nvyuz2_p;
   int nvyuz2_len = expansionObject::Gen_Product_With_PreAlloc(nvz_len, nvz, nuy_len, nuy, &nvyuz2, 16);
   double nvyuz_p[16], *nvyuz = nvyuz_p;
   int nvyuz_len = expansionObject::Gen_Diff_With_PreAlloc(nvyuz1_len, nvyuz1, nvyuz2_len, nvyuz2, &nvyuz, 16);
   double nvxuz1_p[16], *nvxuz1 = nvxuz1_p;
   int nvxuz1_len = expansionObject::Gen_Product_With_PreAlloc(nvx_len, nvx, nuz_len, nuz, &nvxuz1, 16);
   double nvxuz2_p[16], *nvxuz2 = nvxuz2_p;
   int nvxuz2_len = expansionObject::Gen_Product_With_PreAlloc(nvz_len, nvz, nux_len, nux, &nvxuz2, 16);
   double nvxuz_p[16], *nvxuz = nvxuz_p;
   int nvxuz_len = expansionObject::Gen_Diff_With_PreAlloc(nvxuz1_len, nvxuz1, nvxuz2_len, nvxuz2, &nvxuz, 16);
   double nvxuy1_p[16], *nvxuy1 = nvxuy1_p;
   int nvxuy1_len = expansionObject::Gen_Product_With_PreAlloc(nvx_len, nvx, nuy_len, nuy, &nvxuy1, 16);
   double nvxuy2_p[16], *nvxuy2 = nvxuy2_p;
   int nvxuy2_len = expansionObject::Gen_Product_With_PreAlloc(nvy_len, nvy, nux_len, nux, &nvxuy2, 16);
   double nvxuy_p[16], *nvxuy = nvxuy_p;
   int nvxuy_len = expansionObject::Gen_Diff_With_PreAlloc(nvxuy1_len, nvxuy1, nvxuy2_len, nvxuy2, &nvxuy, 16);
   double nvywz1_p[16], *nvywz1 = nvywz1_p;
   int nvywz1_len = expansionObject::Gen_Product_With_PreAlloc(nvy_len, nvy, nwz_len, nwz, &nvywz1, 16);
   double nvywz2_p[16], *nvywz2 = nvywz2_p;
   int nvywz2_len = expansionObject::Gen_Product_With_PreAlloc(nvz_len, nvz, nwy_len, nwy, &nvywz2, 16);
   double nvywz_p[16], *nvywz = nvywz_p;
   int nvywz_len = expansionObject::Gen_Diff_With_PreAlloc(nvywz1_len, nvywz1, nvywz2_len, nvywz2, &nvywz, 16);
   double nvxwz1_p[16], *nvxwz1 = nvxwz1_p;
   int nvxwz1_len = expansionObject::Gen_Product_With_PreAlloc(nvx_len, nvx, nwz_len, nwz, &nvxwz1, 16);
   double nvxwz2_p[16], *nvxwz2 = nvxwz2_p;
   int nvxwz2_len = expansionObject::Gen_Product_With_PreAlloc(nvz_len, nvz, nwx_len, nwx, &nvxwz2, 16);
   double nvxwz_p[16], *nvxwz = nvxwz_p;
   int nvxwz_len = expansionObject::Gen_Diff_With_PreAlloc(nvxwz1_len, nvxwz1, nvxwz2_len, nvxwz2, &nvxwz, 16);
   double nvxwy1_p[16], *nvxwy1 = nvxwy1_p;
   int nvxwy1_len = expansionObject::Gen_Product_With_PreAlloc(nvx_len, nvx, nwy_len, nwy, &nvxwy1, 16);
   double nvxwy2_p[16], *nvxwy2 = nvxwy2_p;
   int nvxwy2_len = expansionObject::Gen_Product_With_PreAlloc(nvy_len, nvy, nwx_len, nwx, &nvxwy2, 16);
   double nvxwy_p[16], *nvxwy = nvxwy_p;
   int nvxwy_len = expansionObject::Gen_Diff_With_PreAlloc(nvxwy1_len, nvxwy1, nvxwy2_len, nvxwy2, &nvxwy, 16);
   double p1a_p[16], *p1a = p1a_p;
   int p1a_len = expansionObject::Gen_Scale_With_PreAlloc(nvx_len, nvx, ov1x, &p1a, 16);
   double p1b_p[16], *p1b = p1b_p;
   int p1b_len = expansionObject::Gen_Scale_With_PreAlloc(nvy_len, nvy, ov1y, &p1b, 16);
   double p1c_p[16], *p1c = p1c_p;
   int p1c_len = expansionObject::Gen_Scale_With_PreAlloc(nvz_len, nvz, ov1z, &p1c, 16);
   double p1ab_p[16], *p1ab = p1ab_p;
   int p1ab_len = expansionObject::Gen_Sum_With_PreAlloc(p1a_len, p1a, p1b_len, p1b, &p1ab, 16);
   double p1_p[16], *p1 = p1_p;
   int p1_len = expansionObject::Gen_Sum_With_PreAlloc(p1ab_len, p1ab, p1c_len, p1c, &p1, 16);
   double p2a_p[16], *p2a = p2a_p;
   int p2a_len = expansionObject::Gen_Scale_With_PreAlloc(nwx_len, nwx, ow1x, &p2a, 16);
   double p2b_p[16], *p2b = p2b_p;
   int p2b_len = expansionObject::Gen_Scale_With_PreAlloc(nwy_len, nwy, ow1y, &p2b, 16);
   double p2c_p[16], *p2c = p2c_p;
   int p2c_len = expansionObject::Gen_Scale_With_PreAlloc(nwz_len, nwz, ow1z, &p2c, 16);
   double p2ab_p[16], *p2ab = p2ab_p;
   int p2ab_len = expansionObject::Gen_Sum_With_PreAlloc(p2a_len, p2a, p2b_len, p2b, &p2ab, 16);
   double p2_p[16], *p2 = p2_p;
   int p2_len = expansionObject::Gen_Sum_With_PreAlloc(p2ab_len, p2ab, p2c_len, p2c, &p2, 16);
   double p3a_p[16], *p3a = p3a_p;
   int p3a_len = expansionObject::Gen_Scale_With_PreAlloc(nux_len, nux, ou1x, &p3a, 16);
   double p3b_p[16], *p3b = p3b_p;
   int p3b_len = expansionObject::Gen_Scale_With_PreAlloc(nuy_len, nuy, ou1y, &p3b, 16);
   double p3c_p[16], *p3c = p3c_p;
   int p3c_len = expansionObject::Gen_Scale_With_PreAlloc(nuz_len, nuz, ou1z, &p3c, 16);
   double p3ab_p[16], *p3ab = p3ab_p;
   int p3ab_len = expansionObject::Gen_Sum_With_PreAlloc(p3a_len, p3a, p3b_len, p3b, &p3ab, 16);
   double p3_p[16], *p3 = p3_p;
   int p3_len = expansionObject::Gen_Sum_With_PreAlloc(p3ab_len, p3ab, p3c_len, p3c, &p3, 16);
   double lxa_p[16], *lxa = lxa_p;
   int lxa_len = expansionObject::Gen_Product_With_PreAlloc(p1_len, p1, nwyuz_len, nwyuz, &lxa, 16);
   double lxb_p[16], *lxb = lxb_p;
   int lxb_len = expansionObject::Gen_Product_With_PreAlloc(p3_len, p3, nvywz_len, nvywz, &lxb, 16);
   double lxc_p[16], *lxc = lxc_p;
   int lxc_len = expansionObject::Gen_Product_With_PreAlloc(p2_len, p2, nvyuz_len, nvyuz, &lxc, 16);
   double lxab_p[16], *lxab = lxab_p;
   int lxab_len = expansionObject::Gen_Sum_With_PreAlloc(lxa_len, lxa, lxb_len, lxb, &lxab, 16);
   lambda_x_len = expansionObject::Gen_Diff_With_PreAlloc(lxab_len, lxab, lxc_len, lxc, lambda_x, lambda_x_len);
   double lya_p[16], *lya = lya_p;
   int lya_len = expansionObject::Gen_Product_With_PreAlloc(p2_len, p2, nvxuz_len, nvxuz, &lya, 16);
   double lyb_p[16], *lyb = lyb_p;
   int lyb_len = expansionObject::Gen_Product_With_PreAlloc(p3_len, p3, nvxwz_len, nvxwz, &lyb, 16);
   double lyc_p[16], *lyc = lyc_p;
   int lyc_len = expansionObject::Gen_Product_With_PreAlloc(p1_len, p1, nwxuz_len, nwxuz, &lyc, 16);
   double lybc_p[16], *lybc = lybc_p;
   int lybc_len = expansionObject::Gen_Sum_With_PreAlloc(lyc_len, lyc, lyb_len, lyb, &lybc, 16);
   lambda_y_len = expansionObject::Gen_Diff_With_PreAlloc(lya_len, lya, lybc_len, lybc, lambda_y, lambda_y_len);
   double lza_p[16], *lza = lza_p;
   int lza_len = expansionObject::Gen_Product_With_PreAlloc(p3_len, p3, nvxwy_len, nvxwy, &lza, 16);
   double lzb_p[16], *lzb = lzb_p;
   int lzb_len = expansionObject::Gen_Product_With_PreAlloc(p1_len, p1, nwxuy_len, nwxuy, &lzb, 16);
   double lzc_p[16], *lzc = lzc_p;
   int lzc_len = expansionObject::Gen_Product_With_PreAlloc(p2_len, p2, nvxuy_len, nvxuy, &lzc, 16);
   double lzab_p[16], *lzab = lzab_p;
   int lzab_len = expansionObject::Gen_Sum_With_PreAlloc(lza_len, lza, lzb_len, lzb, &lzab, 16);
   lambda_z_len = expansionObject::Gen_Diff_With_PreAlloc(lzab_len, lzab, lzc_len, lzc, lambda_z, lambda_z_len);
   double da_p[16], *da = da_p;
   int da_len = expansionObject::Gen_Product_With_PreAlloc(nvx_len, nvx, nwyuz_len, nwyuz, &da, 16);
   double db_p[16], *db = db_p;
   int db_len = expansionObject::Gen_Product_With_PreAlloc(nvz_len, nvz, nwxuy_len, nwxuy, &db, 16);
   double dc_p[16], *dc = dc_p;
   int dc_len = expansionObject::Gen_Product_With_PreAlloc(nvy_len, nvy, nwxuz_len, nwxuz, &dc, 16);
   double dab_p[16], *dab = dab_p;
   int dab_len = expansionObject::Gen_Sum_With_PreAlloc(da_len, da, db_len, db, &dab, 16);
   lambda_d_len = expansionObject::Gen_Diff_With_PreAlloc(dab_len, dab, dc_len, dc, lambda_d, lambda_d_len);

   if (dab_p != dab) FreeDoubles(dab);
   if (dc_p != dc) FreeDoubles(dc);
   if (db_p != db) FreeDoubles(db);
   if (da_p != da) FreeDoubles(da);
   if (lzab_p != lzab) FreeDoubles(lzab);
   if (lzc_p != lzc) FreeDoubles(lzc);
   if (lzb_p != lzb) FreeDoubles(lzb);
   if (lza_p != lza) FreeDoubles(lza);
   if (lybc_p != lybc) FreeDoubles(lybc);
   if (lyc_p != lyc) FreeDoubles(lyc);
   if (lyb_p != lyb) FreeDoubles(lyb);
   if (lya_p != lya) FreeDoubles(lya);
   if (lxab_p != lxab) FreeDoubles(lxab);
   if (lxc_p != lxc) FreeDoubles(lxc);
   if (lxb_p != lxb) FreeDoubles(lxb);
   if (lxa_p != lxa) FreeDoubles(lxa);
   if (p3_p != p3) FreeDoubles(p3);
   if (p3ab_p != p3ab) FreeDoubles(p3ab);
   if (p3c_p != p3c) FreeDoubles(p3c);
   if (p3b_p != p3b) FreeDoubles(p3b);
   if (p3a_p != p3a) FreeDoubles(p3a);
   if (p2_p != p2) FreeDoubles(p2);
   if (p2ab_p != p2ab) FreeDoubles(p2ab);
   if (p2c_p != p2c) FreeDoubles(p2c);
   if (p2b_p != p2b) FreeDoubles(p2b);
   if (p2a_p != p2a) FreeDoubles(p2a);
   if (p1_p != p1) FreeDoubles(p1);
   if (p1ab_p != p1ab) FreeDoubles(p1ab);
   if (p1c_p != p1c) FreeDoubles(p1c);
   if (p1b_p != p1b) FreeDoubles(p1b);
   if (p1a_p != p1a) FreeDoubles(p1a);
   if (nvxwy_p != nvxwy) FreeDoubles(nvxwy);
   if (nvxwy2_p != nvxwy2) FreeDoubles(nvxwy2);
   if (nvxwy1_p != nvxwy1) FreeDoubles(nvxwy1);
   if (nvxwz_p != nvxwz) FreeDoubles(nvxwz);
   if (nvxwz2_p != nvxwz2) FreeDoubles(nvxwz2);
   if (nvxwz1_p != nvxwz1) FreeDoubles(nvxwz1);
   if (nvywz_p != nvywz) FreeDoubles(nvywz);
   if (nvywz2_p != nvywz2) FreeDoubles(nvywz2);
   if (nvywz1_p != nvywz1) FreeDoubles(nvywz1);
   if (nvxuy_p != nvxuy) FreeDoubles(nvxuy);
   if (nvxuy2_p != nvxuy2) FreeDoubles(nvxuy2);
   if (nvxuy1_p != nvxuy1) FreeDoubles(nvxuy1);
   if (nvxuz_p != nvxuz) FreeDoubles(nvxuz);
   if (nvxuz2_p != nvxuz2) FreeDoubles(nvxuz2);
   if (nvxuz1_p != nvxuz1) FreeDoubles(nvxuz1);
   if (nvyuz_p != nvyuz) FreeDoubles(nvyuz);
   if (nvyuz2_p != nvyuz2) FreeDoubles(nvyuz2);
   if (nvyuz1_p != nvyuz1) FreeDoubles(nvyuz1);
   if (nwxuy_p != nwxuy) FreeDoubles(nwxuy);
   if (nwxuy2_p != nwxuy2) FreeDoubles(nwxuy2);
   if (nwxuy1_p != nwxuy1) FreeDoubles(nwxuy1);
   if (nwxuz_p != nwxuz) FreeDoubles(nwxuz);
   if (nwxuz2_p != nwxuz2) FreeDoubles(nwxuz2);
   if (nwxuz1_p != nwxuz1) FreeDoubles(nwxuz1);
   if (nwyuz_p != nwyuz) FreeDoubles(nwyuz);
   if (nwyuz2_p != nwyuz2) FreeDoubles(nwyuz2);
   if (nwyuz1_p != nwyuz1) FreeDoubles(nwyuz1);
}

inline int lessThanOnX_IE_interval(const genericPoint& p1, interval_number bx)
{
   interval_number l1x, l1y, l1z, d1;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number dbx(bx * d1);
   const interval_number kx(l1x - dbx);
   setFPUModeToRoundNEAR();

   if (!kx.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return kx.sign();
}

inline int lessThanOnX_IE_bigfloat(const genericPoint& p1, bigfloat bx)
{
   bigfloat l1x, l1y, l1z, d1;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   const bigfloat dbx(bx * d1);
   const bigfloat kx(l1x - dbx);
   return sgn(kx);
}

inline int lessThanOnX_IE_exact(const genericPoint& p1, double bx)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[128], *l1x = l1x_p, l1y_p[128], *l1y = l1y_p, l1z_p[128], *l1z = l1z_p, d1_p[128], *d1 = d1_p;
 int l1x_len = 128, l1y_len = 128, l1z_len = 128, d1_len = 128;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double dbx_p[128], *dbx = dbx_p;
   int dbx_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, bx, &dbx, 128);
   double kx_p[128], *kx = kx_p;
   int kx_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, dbx_len, dbx, &kx, 128);

   return_value = kx[kx_len - 1];
   if (kx_p != kx) FreeDoubles(kx);
   if (dbx_p != dbx) FreeDoubles(dbx);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = lessThanOnX_IE_bigfloat(p1, bx);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int lessThanOnX_IE(const genericPoint& p1, double bx)
{
   int ret;
   ret = lessThanOnX_IE_interval(p1, bx);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return lessThanOnX_IE_exact(p1, bx);
}

inline int lessThanOnX_II_interval(const genericPoint& p1, const genericPoint& p2)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number k1(d2 * l1x);
   const interval_number k2(d1 * l2x);
   const interval_number kx(k1 - k2);
   setFPUModeToRoundNEAR();

   if (!kx.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return kx.sign();
}

inline int lessThanOnX_II_bigfloat(const genericPoint& p1, const genericPoint& p2)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   const bigfloat k1(d2 * l1x);
   const bigfloat k2(d1 * l2x);
   const bigfloat kx(k1 - k2);
   return sgn(kx);
}

inline int lessThanOnX_II_exact(const genericPoint& p1, const genericPoint& p2)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[128], *l1x = l1x_p, l1y_p[128], *l1y = l1y_p, l1z_p[128], *l1z = l1z_p, d1_p[128], *d1 = d1_p, l2x_p[128], *l2x = l2x_p, l2y_p[128], *l2y = l2y_p, l2z_p[128], *l2z = l2z_p, d2_p[128], *d2 = d2_p;
 int l1x_len = 128, l1y_len = 128, l1z_len = 128, d1_len = 128, l2x_len = 128, l2y_len = 128, l2z_len = 128, d2_len = 128;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double k1_p[128], *k1 = k1_p;
   int k1_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1x_len, l1x, &k1, 128);
   double k2_p[128], *k2 = k2_p;
   int k2_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2x_len, l2x, &k2, 128);
   double kx_p[128], *kx = kx_p;
   int kx_len = expansionObject::Gen_Diff_With_PreAlloc(k1_len, k1, k2_len, k2, &kx, 128);

   return_value = kx[kx_len - 1];
   if (kx_p != kx) FreeDoubles(kx);
   if (k2_p != k2) FreeDoubles(k2);
   if (k1_p != k1) FreeDoubles(k1);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = lessThanOnX_II_bigfloat(p1, p2);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int lessThanOnX_II(const genericPoint& p1, const genericPoint& p2)
{
   int ret;
   ret = lessThanOnX_II_interval(p1, p2);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return lessThanOnX_II_exact(p1, p2);
}

inline int lessThanOnY_IE_interval(const genericPoint& p1, interval_number by)
{
   interval_number l1x, l1y, l1z, d1;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number dby(by * d1);
   const interval_number ky(l1y - dby);
   setFPUModeToRoundNEAR();

   if (!ky.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return ky.sign();
}

inline int lessThanOnY_IE_bigfloat(const genericPoint& p1, bigfloat by)
{
   bigfloat l1x, l1y, l1z, d1;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   const bigfloat dby(by * d1);
   const bigfloat ky(l1y - dby);
   return sgn(ky);
}

inline int lessThanOnY_IE_exact(const genericPoint& p1, double by)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[128], *l1x = l1x_p, l1y_p[128], *l1y = l1y_p, l1z_p[128], *l1z = l1z_p, d1_p[128], *d1 = d1_p;
 int l1x_len = 128, l1y_len = 128, l1z_len = 128, d1_len = 128;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double dby_p[128], *dby = dby_p;
   int dby_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, by, &dby, 128);
   double ky_p[128], *ky = ky_p;
   int ky_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, dby_len, dby, &ky, 128);

   return_value = ky[ky_len - 1];
   if (ky_p != ky) FreeDoubles(ky);
   if (dby_p != dby) FreeDoubles(dby);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = lessThanOnY_IE_bigfloat(p1, by);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int lessThanOnY_IE(const genericPoint& p1, double by)
{
   int ret;
   ret = lessThanOnY_IE_interval(p1, by);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return lessThanOnY_IE_exact(p1, by);
}

inline int lessThanOnY_II_interval(const genericPoint& p1, const genericPoint& p2)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number k1(d2 * l1y);
   const interval_number k2(d1 * l2y);
   const interval_number ky(k1 - k2);
   setFPUModeToRoundNEAR();

   if (!ky.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return ky.sign();
}

inline int lessThanOnY_II_bigfloat(const genericPoint& p1, const genericPoint& p2)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   const bigfloat k1(d2 * l1y);
   const bigfloat k2(d1 * l2y);
   const bigfloat ky(k1 - k2);
   return sgn(ky);
}

inline int lessThanOnY_II_exact(const genericPoint& p1, const genericPoint& p2)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[128], *l1x = l1x_p, l1y_p[128], *l1y = l1y_p, l1z_p[128], *l1z = l1z_p, d1_p[128], *d1 = d1_p, l2x_p[128], *l2x = l2x_p, l2y_p[128], *l2y = l2y_p, l2z_p[128], *l2z = l2z_p, d2_p[128], *d2 = d2_p;
 int l1x_len = 128, l1y_len = 128, l1z_len = 128, d1_len = 128, l2x_len = 128, l2y_len = 128, l2z_len = 128, d2_len = 128;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double k1_p[128], *k1 = k1_p;
   int k1_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1y_len, l1y, &k1, 128);
   double k2_p[128], *k2 = k2_p;
   int k2_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2y_len, l2y, &k2, 128);
   double ky_p[128], *ky = ky_p;
   int ky_len = expansionObject::Gen_Diff_With_PreAlloc(k1_len, k1, k2_len, k2, &ky, 128);

   return_value = ky[ky_len - 1];
   if (ky_p != ky) FreeDoubles(ky);
   if (k2_p != k2) FreeDoubles(k2);
   if (k1_p != k1) FreeDoubles(k1);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = lessThanOnY_II_bigfloat(p1, p2);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int lessThanOnY_II(const genericPoint& p1, const genericPoint& p2)
{
   int ret;
   ret = lessThanOnY_II_interval(p1, p2);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return lessThanOnY_II_exact(p1, p2);
}

inline int lessThanOnZ_IE_interval(const genericPoint& p1, interval_number bz)
{
   interval_number l1x, l1y, l1z, d1;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number dbz(bz * d1);
   const interval_number kz(l1z - dbz);
   setFPUModeToRoundNEAR();

   if (!kz.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return kz.sign();
}

inline int lessThanOnZ_IE_bigfloat(const genericPoint& p1, bigfloat bz)
{
   bigfloat l1x, l1y, l1z, d1;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   const bigfloat dbz(bz * d1);
   const bigfloat kz(l1z - dbz);
   return sgn(kz);
}

inline int lessThanOnZ_IE_exact(const genericPoint& p1, double bz)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[128], *l1x = l1x_p, l1y_p[128], *l1y = l1y_p, l1z_p[128], *l1z = l1z_p, d1_p[128], *d1 = d1_p;
 int l1x_len = 128, l1y_len = 128, l1z_len = 128, d1_len = 128;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double dbz_p[128], *dbz = dbz_p;
   int dbz_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, bz, &dbz, 128);
   double kz_p[128], *kz = kz_p;
   int kz_len = expansionObject::Gen_Diff_With_PreAlloc(l1z_len, l1z, dbz_len, dbz, &kz, 128);

   return_value = kz[kz_len - 1];
   if (kz_p != kz) FreeDoubles(kz);
   if (dbz_p != dbz) FreeDoubles(dbz);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = lessThanOnZ_IE_bigfloat(p1, bz);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int lessThanOnZ_IE(const genericPoint& p1, double bz)
{
   int ret;
   ret = lessThanOnZ_IE_interval(p1, bz);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return lessThanOnZ_IE_exact(p1, bz);
}

inline int lessThanOnZ_II_interval(const genericPoint& p1, const genericPoint& p2)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number k1(d2 * l1z);
   const interval_number k2(d1 * l2z);
   const interval_number kz(k1 - k2);
   setFPUModeToRoundNEAR();

   if (!kz.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return kz.sign();
}

inline int lessThanOnZ_II_bigfloat(const genericPoint& p1, const genericPoint& p2)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   const bigfloat k1(d2 * l1z);
   const bigfloat k2(d1 * l2z);
   const bigfloat kz(k1 - k2);
   return sgn(kz);
}

inline int lessThanOnZ_II_exact(const genericPoint& p1, const genericPoint& p2)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[128], *l1x = l1x_p, l1y_p[128], *l1y = l1y_p, l1z_p[128], *l1z = l1z_p, d1_p[128], *d1 = d1_p, l2x_p[128], *l2x = l2x_p, l2y_p[128], *l2y = l2y_p, l2z_p[128], *l2z = l2z_p, d2_p[128], *d2 = d2_p;
 int l1x_len = 128, l1y_len = 128, l1z_len = 128, d1_len = 128, l2x_len = 128, l2y_len = 128, l2z_len = 128, d2_len = 128;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double k1_p[128], *k1 = k1_p;
   int k1_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1z_len, l1z, &k1, 128);
   double k2_p[128], *k2 = k2_p;
   int k2_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2z_len, l2z, &k2, 128);
   double kz_p[128], *kz = kz_p;
   int kz_len = expansionObject::Gen_Diff_With_PreAlloc(k1_len, k1, k2_len, k2, &kz, 128);

   return_value = kz[kz_len - 1];
   if (kz_p != kz) FreeDoubles(kz);
   if (k2_p != k2) FreeDoubles(k2);
   if (k1_p != k1) FreeDoubles(k1);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = lessThanOnZ_II_bigfloat(p1, p2);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int lessThanOnZ_II(const genericPoint& p1, const genericPoint& p2)
{
   int ret;
   ret = lessThanOnZ_II_interval(p1, p2);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return lessThanOnZ_II_exact(p1, p2);
}

inline int orient2dxy_indirect_IEE_interval(const genericPoint& p1, interval_number p2x, interval_number p2y, interval_number p3x, interval_number p3y)
{
   interval_number l1x, l1y, l1z, d1;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number t1x(p2y - p3y);
   const interval_number t1y(p3x - p2x);
   const interval_number e2(l1x * t1x);
   const interval_number e3(l1y * t1y);
   const interval_number e(e2 + e3);
   const interval_number pr1(p2x * p3y);
   const interval_number pr2(p2y * p3x);
   const interval_number pr(pr1 - pr2);
   const interval_number dpr(d1 * pr);
   const interval_number det(dpr + e);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int orient2dxy_indirect_IEE_bigfloat(const genericPoint& p1, bigfloat p2x, bigfloat p2y, bigfloat p3x, bigfloat p3y)
{
   bigfloat l1x, l1y, l1z, d1;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   const bigfloat t1x(p2y - p3y);
   const bigfloat t1y(p3x - p2x);
   const bigfloat e2(l1x * t1x);
   const bigfloat e3(l1y * t1y);
   const bigfloat e(e2 + e3);
   const bigfloat pr1(p2x * p3y);
   const bigfloat pr2(p2y * p3x);
   const bigfloat pr(pr1 - pr2);
   const bigfloat dpr(d1 * pr);
   const bigfloat det(dpr + e);
   return sgn(det);
}

inline int orient2dxy_indirect_IEE_exact(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[128], *l1x = l1x_p, l1y_p[128], *l1y = l1y_p, l1z_p[128], *l1z = l1z_p, d1_p[128], *d1 = d1_p;
 int l1x_len = 128, l1y_len = 128, l1z_len = 128, d1_len = 128;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double t1x[2];
   expansionObject::two_Diff(p2y, p3y, t1x);
   double t1y[2];
   expansionObject::two_Diff(p3x, p2x, t1y);
   double e2_p[128], *e2 = e2_p;
   int e2_len = expansionObject::Gen_Product_With_PreAlloc(l1x_len, l1x, 2, t1x, &e2, 128);
   double e3_p[128], *e3 = e3_p;
   int e3_len = expansionObject::Gen_Product_With_PreAlloc(l1y_len, l1y, 2, t1y, &e3, 128);
   double e_p[128], *e = e_p;
   int e_len = expansionObject::Gen_Sum_With_PreAlloc(e2_len, e2, e3_len, e3, &e, 128);
   double pr1[2];
   expansionObject::Two_Prod(p2x, p3y, pr1);
   double pr2[2];
   expansionObject::Two_Prod(p2y, p3x, pr2);
   double pr[4];
   expansionObject::Two_Two_Diff(pr1, pr2, pr);
   double dpr_p[128], *dpr = dpr_p;
   int dpr_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, 4, pr, &dpr, 128);
   double det_p[128], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dpr_len, dpr, e_len, e, &det, 128);

   return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (dpr_p != dpr) FreeDoubles(dpr);
   if (e_p != e) FreeDoubles(e);
   if (e3_p != e3) FreeDoubles(e3);
   if (e2_p != e2) FreeDoubles(e2);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2dxy_indirect_IEE_bigfloat(p1, p2x, p2y, p3x, p3y);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2dxy_indirect_IEE(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y)
{
   int ret;
   ret = orient2dxy_indirect_IEE_interval(p1, p2x, p2y, p3x, p3y);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2dxy_indirect_IEE_exact(p1, p2x, p2y, p3x, p3y);
}

inline int orient2dxy_indirect_IIE_interval(const genericPoint& p1, const genericPoint& p2, interval_number op3x, interval_number op3y)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number a(d1 * l2x);
   const interval_number b(d2 * l1x);
   const interval_number c(d1 * op3y);
   const interval_number e(d1 * l2y);
   const interval_number f(d2 * l1y);
   const interval_number g(d1 * op3x);
   const interval_number ab(a - b);
   const interval_number cd(c - l1y);
   const interval_number ef(e - f);
   const interval_number gh(g - l1x);
   const interval_number abcd(ab * cd);
   const interval_number efgh(ef * gh);
   const interval_number L(abcd - efgh);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int orient2dxy_indirect_IIE_bigfloat(const genericPoint& p1, const genericPoint& p2, bigfloat op3x, bigfloat op3y)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   const bigfloat a(d1 * l2x);
   const bigfloat b(d2 * l1x);
   const bigfloat c(d1 * op3y);
   const bigfloat e(d1 * l2y);
   const bigfloat f(d2 * l1y);
   const bigfloat g(d1 * op3x);
   const bigfloat ab(a - b);
   const bigfloat cd(c - l1y);
   const bigfloat ef(e - f);
   const bigfloat gh(g - l1x);
   const bigfloat abcd(ab * cd);
   const bigfloat efgh(ef * gh);
   const bigfloat L(abcd - efgh);
   return sgn(L);
}

inline int orient2dxy_indirect_IIE_exact(const genericPoint& p1, const genericPoint& p2, double op3x, double op3y)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[64], *l1x = l1x_p, l1y_p[64], *l1y = l1y_p, l1z_p[64], *l1z = l1z_p, d1_p[64], *d1 = d1_p, l2x_p[64], *l2x = l2x_p, l2y_p[64], *l2y = l2y_p, l2z_p[64], *l2z = l2z_p, d2_p[64], *d2 = d2_p;
 int l1x_len = 64, l1y_len = 64, l1z_len = 64, d1_len = 64, l2x_len = 64, l2y_len = 64, l2z_len = 64, d2_len = 64;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double a_p[64], *a = a_p;
   int a_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2x_len, l2x, &a, 64);
   double b_p[64], *b = b_p;
   int b_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1x_len, l1x, &b, 64);
   double c_p[64], *c = c_p;
   int c_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, op3y, &c, 64);
   double e_p[64], *e = e_p;
   int e_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2y_len, l2y, &e, 64);
   double f_p[64], *f = f_p;
   int f_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1y_len, l1y, &f, 64);
   double g_p[64], *g = g_p;
   int g_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, op3x, &g, 64);
   double ab_p[64], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(a_len, a, b_len, b, &ab, 64);
   double cd_p[64], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(c_len, c, l1y_len, l1y, &cd, 64);
   double ef_p[64], *ef = ef_p;
   int ef_len = expansionObject::Gen_Diff_With_PreAlloc(e_len, e, f_len, f, &ef, 64);
   double gh_p[64], *gh = gh_p;
   int gh_len = expansionObject::Gen_Diff_With_PreAlloc(g_len, g, l1x_len, l1x, &gh, 64);
   double abcd_p[64], *abcd = abcd_p;
   int abcd_len = expansionObject::Gen_Product_With_PreAlloc(ab_len, ab, cd_len, cd, &abcd, 64);
   double efgh_p[64], *efgh = efgh_p;
   int efgh_len = expansionObject::Gen_Product_With_PreAlloc(ef_len, ef, gh_len, gh, &efgh, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Diff_With_PreAlloc(abcd_len, abcd, efgh_len, efgh, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (efgh_p != efgh) FreeDoubles(efgh);
   if (abcd_p != abcd) FreeDoubles(abcd);
   if (gh_p != gh) FreeDoubles(gh);
   if (ef_p != ef) FreeDoubles(ef);
   if (cd_p != cd) FreeDoubles(cd);
   if (ab_p != ab) FreeDoubles(ab);
   if (g_p != g) FreeDoubles(g);
   if (f_p != f) FreeDoubles(f);
   if (e_p != e) FreeDoubles(e);
   if (c_p != c) FreeDoubles(c);
   if (b_p != b) FreeDoubles(b);
   if (a_p != a) FreeDoubles(a);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2dxy_indirect_IIE_bigfloat(p1, p2, op3x, op3y);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2dxy_indirect_IIE(const genericPoint& p1, const genericPoint& p2, double op3x, double op3y)
{
   int ret;
   ret = orient2dxy_indirect_IIE_interval(p1, p2, op3x, op3y);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2dxy_indirect_IIE_exact(p1, p2, op3x, op3y);
}

inline int orient2dxy_indirect_III_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   || !p3.getIntervalLambda(l3x, l3y, l3z, d3)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number a(d1 * l2x);
   const interval_number b(d2 * l1x);
   const interval_number c(d1 * l3y);
   const interval_number d(d3 * l1y);
   const interval_number e(d1 * l2y);
   const interval_number f(d2 * l1y);
   const interval_number g(d1 * l3x);
   const interval_number h(d3 * l1x);
   const interval_number ab(a - b);
   const interval_number cd(c - d);
   const interval_number ef(e - f);
   const interval_number gh(g - h);
   const interval_number abcd(ab * cd);
   const interval_number efgh(ef * gh);
   const interval_number L(abcd - efgh);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int orient2dxy_indirect_III_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   p3.getBigfloatLambda(l3x, l3y, l3z, d3);
   const bigfloat a(d1 * l2x);
   const bigfloat b(d2 * l1x);
   const bigfloat c(d1 * l3y);
   const bigfloat d(d3 * l1y);
   const bigfloat e(d1 * l2y);
   const bigfloat f(d2 * l1y);
   const bigfloat g(d1 * l3x);
   const bigfloat h(d3 * l1x);
   const bigfloat ab(a - b);
   const bigfloat cd(c - d);
   const bigfloat ef(e - f);
   const bigfloat gh(g - h);
   const bigfloat abcd(ab * cd);
   const bigfloat efgh(ef * gh);
   const bigfloat L(abcd - efgh);
   return sgn(L);
}

inline int orient2dxy_indirect_III_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[64], *l1x = l1x_p, l1y_p[64], *l1y = l1y_p, l1z_p[64], *l1z = l1z_p, d1_p[64], *d1 = d1_p, l2x_p[64], *l2x = l2x_p, l2y_p[64], *l2y = l2y_p, l2z_p[64], *l2z = l2z_p, d2_p[64], *d2 = d2_p, l3x_p[64], *l3x = l3x_p, l3y_p[64], *l3y = l3y_p, l3z_p[64], *l3z = l3z_p, d3_p[64], *d3 = d3_p;
 int l1x_len = 64, l1y_len = 64, l1z_len = 64, d1_len = 64, l2x_len = 64, l2y_len = 64, l2z_len = 64, d2_len = 64, l3x_len = 64, l3y_len = 64, l3z_len = 64, d3_len = 64;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &l3z, l3z_len, &d3, d3_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0))
 {
   
   double a_p[64], *a = a_p;
   int a_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2x_len, l2x, &a, 64);
   double b_p[64], *b = b_p;
   int b_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1x_len, l1x, &b, 64);
   double c_p[64], *c = c_p;
   int c_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l3y_len, l3y, &c, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l1y_len, l1y, &d, 64);
   double e_p[64], *e = e_p;
   int e_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2y_len, l2y, &e, 64);
   double f_p[64], *f = f_p;
   int f_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1y_len, l1y, &f, 64);
   double g_p[64], *g = g_p;
   int g_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l3x_len, l3x, &g, 64);
   double h_p[64], *h = h_p;
   int h_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l1x_len, l1x, &h, 64);
   double ab_p[64], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(a_len, a, b_len, b, &ab, 64);
   double cd_p[64], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(c_len, c, d_len, d, &cd, 64);
   double ef_p[64], *ef = ef_p;
   int ef_len = expansionObject::Gen_Diff_With_PreAlloc(e_len, e, f_len, f, &ef, 64);
   double gh_p[64], *gh = gh_p;
   int gh_len = expansionObject::Gen_Diff_With_PreAlloc(g_len, g, h_len, h, &gh, 64);
   double abcd_p[64], *abcd = abcd_p;
   int abcd_len = expansionObject::Gen_Product_With_PreAlloc(ab_len, ab, cd_len, cd, &abcd, 64);
   double efgh_p[64], *efgh = efgh_p;
   int efgh_len = expansionObject::Gen_Product_With_PreAlloc(ef_len, ef, gh_len, gh, &efgh, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Diff_With_PreAlloc(abcd_len, abcd, efgh_len, efgh, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (efgh_p != efgh) FreeDoubles(efgh);
   if (abcd_p != abcd) FreeDoubles(abcd);
   if (gh_p != gh) FreeDoubles(gh);
   if (ef_p != ef) FreeDoubles(ef);
   if (cd_p != cd) FreeDoubles(cd);
   if (ab_p != ab) FreeDoubles(ab);
   if (h_p != h) FreeDoubles(h);
   if (g_p != g) FreeDoubles(g);
   if (f_p != f) FreeDoubles(f);
   if (e_p != e) FreeDoubles(e);
   if (d_p != d) FreeDoubles(d);
   if (c_p != c) FreeDoubles(c);
   if (b_p != b) FreeDoubles(b);
   if (a_p != a) FreeDoubles(a);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2dxy_indirect_III_bigfloat(p1, p2, p3);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (d3_p != d3) FreeDoubles(d3);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2dxy_indirect_III(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   int ret;
   ret = orient2dxy_indirect_III_interval(p1, p2, p3);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2dxy_indirect_III_exact(p1, p2, p3);
}

inline int orient2dyz_indirect_IEE_interval(const genericPoint& p1, interval_number p2x, interval_number p2y, interval_number p3x, interval_number p3y)
{
   interval_number l1z, l1x, l1y, d1;
   if (
   !p1.getIntervalLambda(l1z, l1x, l1y, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number t1x(p2y - p3y);
   const interval_number t1y(p3x - p2x);
   const interval_number e2(l1x * t1x);
   const interval_number e3(l1y * t1y);
   const interval_number e(e2 + e3);
   const interval_number pr1(p2x * p3y);
   const interval_number pr2(p2y * p3x);
   const interval_number pr(pr1 - pr2);
   const interval_number dpr(d1 * pr);
   const interval_number det(dpr + e);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int orient2dyz_indirect_IEE_bigfloat(const genericPoint& p1, bigfloat p2x, bigfloat p2y, bigfloat p3x, bigfloat p3y)
{
   bigfloat l1z, l1x, l1y, d1;
   p1.getBigfloatLambda(l1z, l1x, l1y, d1);
   const bigfloat t1x(p2y - p3y);
   const bigfloat t1y(p3x - p2x);
   const bigfloat e2(l1x * t1x);
   const bigfloat e3(l1y * t1y);
   const bigfloat e(e2 + e3);
   const bigfloat pr1(p2x * p3y);
   const bigfloat pr2(p2y * p3x);
   const bigfloat pr(pr1 - pr2);
   const bigfloat dpr(d1 * pr);
   const bigfloat det(dpr + e);
   return sgn(det);
}

inline int orient2dyz_indirect_IEE_exact(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1z_p[128], *l1z = l1z_p, l1x_p[128], *l1x = l1x_p, l1y_p[128], *l1y = l1y_p, d1_p[128], *d1 = d1_p;
 int l1z_len = 128, l1x_len = 128, l1y_len = 128, d1_len = 128;
 p1.getExactLambda(&l1z, l1z_len, &l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double t1x[2];
   expansionObject::two_Diff(p2y, p3y, t1x);
   double t1y[2];
   expansionObject::two_Diff(p3x, p2x, t1y);
   double e2_p[128], *e2 = e2_p;
   int e2_len = expansionObject::Gen_Product_With_PreAlloc(l1x_len, l1x, 2, t1x, &e2, 128);
   double e3_p[128], *e3 = e3_p;
   int e3_len = expansionObject::Gen_Product_With_PreAlloc(l1y_len, l1y, 2, t1y, &e3, 128);
   double e_p[128], *e = e_p;
   int e_len = expansionObject::Gen_Sum_With_PreAlloc(e2_len, e2, e3_len, e3, &e, 128);
   double pr1[2];
   expansionObject::Two_Prod(p2x, p3y, pr1);
   double pr2[2];
   expansionObject::Two_Prod(p2y, p3x, pr2);
   double pr[4];
   expansionObject::Two_Two_Diff(pr1, pr2, pr);
   double dpr_p[128], *dpr = dpr_p;
   int dpr_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, 4, pr, &dpr, 128);
   double det_p[128], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dpr_len, dpr, e_len, e, &det, 128);

   return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (dpr_p != dpr) FreeDoubles(dpr);
   if (e_p != e) FreeDoubles(e);
   if (e3_p != e3) FreeDoubles(e3);
   if (e2_p != e2) FreeDoubles(e2);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2dyz_indirect_IEE_bigfloat(p1, p2x, p2y, p3x, p3y);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2dyz_indirect_IEE(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y)
{
   int ret;
   ret = orient2dyz_indirect_IEE_interval(p1, p2x, p2y, p3x, p3y);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2dyz_indirect_IEE_exact(p1, p2x, p2y, p3x, p3y);
}

inline int orient2dyz_indirect_IIE_interval(const genericPoint& p1, const genericPoint& p2, interval_number op3x, interval_number op3y)
{
   interval_number l1z, l1x, l1y, d1, l2z, l2x, l2y, d2;
   if (
   !p1.getIntervalLambda(l1z, l1x, l1y, d1)
   || !p2.getIntervalLambda(l2z, l2x, l2y, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number a(d1 * l2x);
   const interval_number b(d2 * l1x);
   const interval_number c(d1 * op3y);
   const interval_number e(d1 * l2y);
   const interval_number f(d2 * l1y);
   const interval_number g(d1 * op3x);
   const interval_number ab(a - b);
   const interval_number cd(c - l1y);
   const interval_number ef(e - f);
   const interval_number gh(g - l1x);
   const interval_number abcd(ab * cd);
   const interval_number efgh(ef * gh);
   const interval_number L(abcd - efgh);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int orient2dyz_indirect_IIE_bigfloat(const genericPoint& p1, const genericPoint& p2, bigfloat op3x, bigfloat op3y)
{
   bigfloat l1z, l1x, l1y, d1, l2z, l2x, l2y, d2;
   p1.getBigfloatLambda(l1z, l1x, l1y, d1);
   p2.getBigfloatLambda(l2z, l2x, l2y, d2);
   const bigfloat a(d1 * l2x);
   const bigfloat b(d2 * l1x);
   const bigfloat c(d1 * op3y);
   const bigfloat e(d1 * l2y);
   const bigfloat f(d2 * l1y);
   const bigfloat g(d1 * op3x);
   const bigfloat ab(a - b);
   const bigfloat cd(c - l1y);
   const bigfloat ef(e - f);
   const bigfloat gh(g - l1x);
   const bigfloat abcd(ab * cd);
   const bigfloat efgh(ef * gh);
   const bigfloat L(abcd - efgh);
   return sgn(L);
}

inline int orient2dyz_indirect_IIE_exact(const genericPoint& p1, const genericPoint& p2, double op3x, double op3y)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1z_p[64], *l1z = l1z_p, l1x_p[64], *l1x = l1x_p, l1y_p[64], *l1y = l1y_p, d1_p[64], *d1 = d1_p, l2z_p[64], *l2z = l2z_p, l2x_p[64], *l2x = l2x_p, l2y_p[64], *l2y = l2y_p, d2_p[64], *d2 = d2_p;
 int l1z_len = 64, l1x_len = 64, l1y_len = 64, d1_len = 64, l2z_len = 64, l2x_len = 64, l2y_len = 64, d2_len = 64;
 p1.getExactLambda(&l1z, l1z_len, &l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 p2.getExactLambda(&l2z, l2z_len, &l2x, l2x_len, &l2y, l2y_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double a_p[64], *a = a_p;
   int a_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2x_len, l2x, &a, 64);
   double b_p[64], *b = b_p;
   int b_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1x_len, l1x, &b, 64);
   double c_p[64], *c = c_p;
   int c_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, op3y, &c, 64);
   double e_p[64], *e = e_p;
   int e_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2y_len, l2y, &e, 64);
   double f_p[64], *f = f_p;
   int f_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1y_len, l1y, &f, 64);
   double g_p[64], *g = g_p;
   int g_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, op3x, &g, 64);
   double ab_p[64], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(a_len, a, b_len, b, &ab, 64);
   double cd_p[64], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(c_len, c, l1y_len, l1y, &cd, 64);
   double ef_p[64], *ef = ef_p;
   int ef_len = expansionObject::Gen_Diff_With_PreAlloc(e_len, e, f_len, f, &ef, 64);
   double gh_p[64], *gh = gh_p;
   int gh_len = expansionObject::Gen_Diff_With_PreAlloc(g_len, g, l1x_len, l1x, &gh, 64);
   double abcd_p[64], *abcd = abcd_p;
   int abcd_len = expansionObject::Gen_Product_With_PreAlloc(ab_len, ab, cd_len, cd, &abcd, 64);
   double efgh_p[64], *efgh = efgh_p;
   int efgh_len = expansionObject::Gen_Product_With_PreAlloc(ef_len, ef, gh_len, gh, &efgh, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Diff_With_PreAlloc(abcd_len, abcd, efgh_len, efgh, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (efgh_p != efgh) FreeDoubles(efgh);
   if (abcd_p != abcd) FreeDoubles(abcd);
   if (gh_p != gh) FreeDoubles(gh);
   if (ef_p != ef) FreeDoubles(ef);
   if (cd_p != cd) FreeDoubles(cd);
   if (ab_p != ab) FreeDoubles(ab);
   if (g_p != g) FreeDoubles(g);
   if (f_p != f) FreeDoubles(f);
   if (e_p != e) FreeDoubles(e);
   if (c_p != c) FreeDoubles(c);
   if (b_p != b) FreeDoubles(b);
   if (a_p != a) FreeDoubles(a);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2dyz_indirect_IIE_bigfloat(p1, p2, op3x, op3y);
#endif

 if (l1z_p != l1z) FreeDoubles(l1z);
 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (d2_p != d2) FreeDoubles(d2);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2dyz_indirect_IIE(const genericPoint& p1, const genericPoint& p2, double op3x, double op3y)
{
   int ret;
   ret = orient2dyz_indirect_IIE_interval(p1, p2, op3x, op3y);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2dyz_indirect_IIE_exact(p1, p2, op3x, op3y);
}

inline int orient2dyz_indirect_III_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   interval_number l1z, l1x, l1y, d1, l2z, l2x, l2y, d2, l3z, l3x, l3y, d3;
   if (
   !p1.getIntervalLambda(l1z, l1x, l1y, d1)
   || !p2.getIntervalLambda(l2z, l2x, l2y, d2)
   || !p3.getIntervalLambda(l3z, l3x, l3y, d3)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number a(d1 * l2x);
   const interval_number b(d2 * l1x);
   const interval_number c(d1 * l3y);
   const interval_number d(d3 * l1y);
   const interval_number e(d1 * l2y);
   const interval_number f(d2 * l1y);
   const interval_number g(d1 * l3x);
   const interval_number h(d3 * l1x);
   const interval_number ab(a - b);
   const interval_number cd(c - d);
   const interval_number ef(e - f);
   const interval_number gh(g - h);
   const interval_number abcd(ab * cd);
   const interval_number efgh(ef * gh);
   const interval_number L(abcd - efgh);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int orient2dyz_indirect_III_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   bigfloat l1z, l1x, l1y, d1, l2z, l2x, l2y, d2, l3z, l3x, l3y, d3;
   p1.getBigfloatLambda(l1z, l1x, l1y, d1);
   p2.getBigfloatLambda(l2z, l2x, l2y, d2);
   p3.getBigfloatLambda(l3z, l3x, l3y, d3);
   const bigfloat a(d1 * l2x);
   const bigfloat b(d2 * l1x);
   const bigfloat c(d1 * l3y);
   const bigfloat d(d3 * l1y);
   const bigfloat e(d1 * l2y);
   const bigfloat f(d2 * l1y);
   const bigfloat g(d1 * l3x);
   const bigfloat h(d3 * l1x);
   const bigfloat ab(a - b);
   const bigfloat cd(c - d);
   const bigfloat ef(e - f);
   const bigfloat gh(g - h);
   const bigfloat abcd(ab * cd);
   const bigfloat efgh(ef * gh);
   const bigfloat L(abcd - efgh);
   return sgn(L);
}

inline int orient2dyz_indirect_III_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1z_p[64], *l1z = l1z_p, l1x_p[64], *l1x = l1x_p, l1y_p[64], *l1y = l1y_p, d1_p[64], *d1 = d1_p, l2z_p[64], *l2z = l2z_p, l2x_p[64], *l2x = l2x_p, l2y_p[64], *l2y = l2y_p, d2_p[64], *d2 = d2_p, l3z_p[64], *l3z = l3z_p, l3x_p[64], *l3x = l3x_p, l3y_p[64], *l3y = l3y_p, d3_p[64], *d3 = d3_p;
 int l1z_len = 64, l1x_len = 64, l1y_len = 64, d1_len = 64, l2z_len = 64, l2x_len = 64, l2y_len = 64, d2_len = 64, l3z_len = 64, l3x_len = 64, l3y_len = 64, d3_len = 64;
 p1.getExactLambda(&l1z, l1z_len, &l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 p2.getExactLambda(&l2z, l2z_len, &l2x, l2x_len, &l2y, l2y_len, &d2, d2_len);
 p3.getExactLambda(&l3z, l3z_len, &l3x, l3x_len, &l3y, l3y_len, &d3, d3_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0))
 {
   
   double a_p[64], *a = a_p;
   int a_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2x_len, l2x, &a, 64);
   double b_p[64], *b = b_p;
   int b_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1x_len, l1x, &b, 64);
   double c_p[64], *c = c_p;
   int c_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l3y_len, l3y, &c, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l1y_len, l1y, &d, 64);
   double e_p[64], *e = e_p;
   int e_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2y_len, l2y, &e, 64);
   double f_p[64], *f = f_p;
   int f_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1y_len, l1y, &f, 64);
   double g_p[64], *g = g_p;
   int g_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l3x_len, l3x, &g, 64);
   double h_p[64], *h = h_p;
   int h_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l1x_len, l1x, &h, 64);
   double ab_p[64], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(a_len, a, b_len, b, &ab, 64);
   double cd_p[64], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(c_len, c, d_len, d, &cd, 64);
   double ef_p[64], *ef = ef_p;
   int ef_len = expansionObject::Gen_Diff_With_PreAlloc(e_len, e, f_len, f, &ef, 64);
   double gh_p[64], *gh = gh_p;
   int gh_len = expansionObject::Gen_Diff_With_PreAlloc(g_len, g, h_len, h, &gh, 64);
   double abcd_p[64], *abcd = abcd_p;
   int abcd_len = expansionObject::Gen_Product_With_PreAlloc(ab_len, ab, cd_len, cd, &abcd, 64);
   double efgh_p[64], *efgh = efgh_p;
   int efgh_len = expansionObject::Gen_Product_With_PreAlloc(ef_len, ef, gh_len, gh, &efgh, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Diff_With_PreAlloc(abcd_len, abcd, efgh_len, efgh, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (efgh_p != efgh) FreeDoubles(efgh);
   if (abcd_p != abcd) FreeDoubles(abcd);
   if (gh_p != gh) FreeDoubles(gh);
   if (ef_p != ef) FreeDoubles(ef);
   if (cd_p != cd) FreeDoubles(cd);
   if (ab_p != ab) FreeDoubles(ab);
   if (h_p != h) FreeDoubles(h);
   if (g_p != g) FreeDoubles(g);
   if (f_p != f) FreeDoubles(f);
   if (e_p != e) FreeDoubles(e);
   if (d_p != d) FreeDoubles(d);
   if (c_p != c) FreeDoubles(c);
   if (b_p != b) FreeDoubles(b);
   if (a_p != a) FreeDoubles(a);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2dyz_indirect_III_bigfloat(p1, p2, p3);
#endif

 if (l1z_p != l1z) FreeDoubles(l1z);
 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (d3_p != d3) FreeDoubles(d3);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2dyz_indirect_III(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   int ret;
   ret = orient2dyz_indirect_III_interval(p1, p2, p3);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2dyz_indirect_III_exact(p1, p2, p3);
}

inline int orient2dzx_indirect_IEE_interval(const genericPoint& p1, interval_number p2x, interval_number p2y, interval_number p3x, interval_number p3y)
{
   interval_number l1y, l1z, l1x, d1;
   if (
   !p1.getIntervalLambda(l1y, l1z, l1x, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number t1x(p2y - p3y);
   const interval_number t1y(p3x - p2x);
   const interval_number e2(l1x * t1x);
   const interval_number e3(l1y * t1y);
   const interval_number e(e2 + e3);
   const interval_number pr1(p2x * p3y);
   const interval_number pr2(p2y * p3x);
   const interval_number pr(pr1 - pr2);
   const interval_number dpr(d1 * pr);
   const interval_number det(dpr + e);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int orient2dzx_indirect_IEE_bigfloat(const genericPoint& p1, bigfloat p2x, bigfloat p2y, bigfloat p3x, bigfloat p3y)
{
   bigfloat l1y, l1z, l1x, d1;
   p1.getBigfloatLambda(l1y, l1z, l1x, d1);
   const bigfloat t1x(p2y - p3y);
   const bigfloat t1y(p3x - p2x);
   const bigfloat e2(l1x * t1x);
   const bigfloat e3(l1y * t1y);
   const bigfloat e(e2 + e3);
   const bigfloat pr1(p2x * p3y);
   const bigfloat pr2(p2y * p3x);
   const bigfloat pr(pr1 - pr2);
   const bigfloat dpr(d1 * pr);
   const bigfloat det(dpr + e);
   return sgn(det);
}

inline int orient2dzx_indirect_IEE_exact(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1y_p[128], *l1y = l1y_p, l1z_p[128], *l1z = l1z_p, l1x_p[128], *l1x = l1x_p, d1_p[128], *d1 = d1_p;
 int l1y_len = 128, l1z_len = 128, l1x_len = 128, d1_len = 128;
 p1.getExactLambda(&l1y, l1y_len, &l1z, l1z_len, &l1x, l1x_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double t1x[2];
   expansionObject::two_Diff(p2y, p3y, t1x);
   double t1y[2];
   expansionObject::two_Diff(p3x, p2x, t1y);
   double e2_p[128], *e2 = e2_p;
   int e2_len = expansionObject::Gen_Product_With_PreAlloc(l1x_len, l1x, 2, t1x, &e2, 128);
   double e3_p[128], *e3 = e3_p;
   int e3_len = expansionObject::Gen_Product_With_PreAlloc(l1y_len, l1y, 2, t1y, &e3, 128);
   double e_p[128], *e = e_p;
   int e_len = expansionObject::Gen_Sum_With_PreAlloc(e2_len, e2, e3_len, e3, &e, 128);
   double pr1[2];
   expansionObject::Two_Prod(p2x, p3y, pr1);
   double pr2[2];
   expansionObject::Two_Prod(p2y, p3x, pr2);
   double pr[4];
   expansionObject::Two_Two_Diff(pr1, pr2, pr);
   double dpr_p[128], *dpr = dpr_p;
   int dpr_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, 4, pr, &dpr, 128);
   double det_p[128], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dpr_len, dpr, e_len, e, &det, 128);

   return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (dpr_p != dpr) FreeDoubles(dpr);
   if (e_p != e) FreeDoubles(e);
   if (e3_p != e3) FreeDoubles(e3);
   if (e2_p != e2) FreeDoubles(e2);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2dzx_indirect_IEE_bigfloat(p1, p2x, p2y, p3x, p3y);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2dzx_indirect_IEE(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y)
{
   int ret;
   ret = orient2dzx_indirect_IEE_interval(p1, p2x, p2y, p3x, p3y);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2dzx_indirect_IEE_exact(p1, p2x, p2y, p3x, p3y);
}

inline int orient2dzx_indirect_IIE_interval(const genericPoint& p1, const genericPoint& p2, interval_number op3x, interval_number op3y)
{
   interval_number l1y, l1z, l1x, d1, l2y, l2z, l2x, d2;
   if (
   !p1.getIntervalLambda(l1y, l1z, l1x, d1)
   || !p2.getIntervalLambda(l2y, l2z, l2x, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number a(d1 * l2x);
   const interval_number b(d2 * l1x);
   const interval_number c(d1 * op3y);
   const interval_number e(d1 * l2y);
   const interval_number f(d2 * l1y);
   const interval_number g(d1 * op3x);
   const interval_number ab(a - b);
   const interval_number cd(c - l1y);
   const interval_number ef(e - f);
   const interval_number gh(g - l1x);
   const interval_number abcd(ab * cd);
   const interval_number efgh(ef * gh);
   const interval_number L(abcd - efgh);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int orient2dzx_indirect_IIE_bigfloat(const genericPoint& p1, const genericPoint& p2, bigfloat op3x, bigfloat op3y)
{
   bigfloat l1y, l1z, l1x, d1, l2y, l2z, l2x, d2;
   p1.getBigfloatLambda(l1y, l1z, l1x, d1);
   p2.getBigfloatLambda(l2y, l2z, l2x, d2);
   const bigfloat a(d1 * l2x);
   const bigfloat b(d2 * l1x);
   const bigfloat c(d1 * op3y);
   const bigfloat e(d1 * l2y);
   const bigfloat f(d2 * l1y);
   const bigfloat g(d1 * op3x);
   const bigfloat ab(a - b);
   const bigfloat cd(c - l1y);
   const bigfloat ef(e - f);
   const bigfloat gh(g - l1x);
   const bigfloat abcd(ab * cd);
   const bigfloat efgh(ef * gh);
   const bigfloat L(abcd - efgh);
   return sgn(L);
}

inline int orient2dzx_indirect_IIE_exact(const genericPoint& p1, const genericPoint& p2, double op3x, double op3y)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1y_p[64], *l1y = l1y_p, l1z_p[64], *l1z = l1z_p, l1x_p[64], *l1x = l1x_p, d1_p[64], *d1 = d1_p, l2y_p[64], *l2y = l2y_p, l2z_p[64], *l2z = l2z_p, l2x_p[64], *l2x = l2x_p, d2_p[64], *d2 = d2_p;
 int l1y_len = 64, l1z_len = 64, l1x_len = 64, d1_len = 64, l2y_len = 64, l2z_len = 64, l2x_len = 64, d2_len = 64;
 p1.getExactLambda(&l1y, l1y_len, &l1z, l1z_len, &l1x, l1x_len, &d1, d1_len);
 p2.getExactLambda(&l2y, l2y_len, &l2z, l2z_len, &l2x, l2x_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double a_p[64], *a = a_p;
   int a_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2x_len, l2x, &a, 64);
   double b_p[64], *b = b_p;
   int b_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1x_len, l1x, &b, 64);
   double c_p[64], *c = c_p;
   int c_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, op3y, &c, 64);
   double e_p[64], *e = e_p;
   int e_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2y_len, l2y, &e, 64);
   double f_p[64], *f = f_p;
   int f_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1y_len, l1y, &f, 64);
   double g_p[64], *g = g_p;
   int g_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, op3x, &g, 64);
   double ab_p[64], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(a_len, a, b_len, b, &ab, 64);
   double cd_p[64], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(c_len, c, l1y_len, l1y, &cd, 64);
   double ef_p[64], *ef = ef_p;
   int ef_len = expansionObject::Gen_Diff_With_PreAlloc(e_len, e, f_len, f, &ef, 64);
   double gh_p[64], *gh = gh_p;
   int gh_len = expansionObject::Gen_Diff_With_PreAlloc(g_len, g, l1x_len, l1x, &gh, 64);
   double abcd_p[64], *abcd = abcd_p;
   int abcd_len = expansionObject::Gen_Product_With_PreAlloc(ab_len, ab, cd_len, cd, &abcd, 64);
   double efgh_p[64], *efgh = efgh_p;
   int efgh_len = expansionObject::Gen_Product_With_PreAlloc(ef_len, ef, gh_len, gh, &efgh, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Diff_With_PreAlloc(abcd_len, abcd, efgh_len, efgh, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (efgh_p != efgh) FreeDoubles(efgh);
   if (abcd_p != abcd) FreeDoubles(abcd);
   if (gh_p != gh) FreeDoubles(gh);
   if (ef_p != ef) FreeDoubles(ef);
   if (cd_p != cd) FreeDoubles(cd);
   if (ab_p != ab) FreeDoubles(ab);
   if (g_p != g) FreeDoubles(g);
   if (f_p != f) FreeDoubles(f);
   if (e_p != e) FreeDoubles(e);
   if (c_p != c) FreeDoubles(c);
   if (b_p != b) FreeDoubles(b);
   if (a_p != a) FreeDoubles(a);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2dzx_indirect_IIE_bigfloat(p1, p2, op3x, op3y);
#endif

 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (l1x_p != l1x) FreeDoubles(l1x);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (d2_p != d2) FreeDoubles(d2);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2dzx_indirect_IIE(const genericPoint& p1, const genericPoint& p2, double op3x, double op3y)
{
   int ret;
   ret = orient2dzx_indirect_IIE_interval(p1, p2, op3x, op3y);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2dzx_indirect_IIE_exact(p1, p2, op3x, op3y);
}

inline int orient2dzx_indirect_III_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   interval_number l1y, l1z, l1x, d1, l2y, l2z, l2x, d2, l3y, l3z, l3x, d3;
   if (
   !p1.getIntervalLambda(l1y, l1z, l1x, d1)
   || !p2.getIntervalLambda(l2y, l2z, l2x, d2)
   || !p3.getIntervalLambda(l3y, l3z, l3x, d3)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number a(d1 * l2x);
   const interval_number b(d2 * l1x);
   const interval_number c(d1 * l3y);
   const interval_number d(d3 * l1y);
   const interval_number e(d1 * l2y);
   const interval_number f(d2 * l1y);
   const interval_number g(d1 * l3x);
   const interval_number h(d3 * l1x);
   const interval_number ab(a - b);
   const interval_number cd(c - d);
   const interval_number ef(e - f);
   const interval_number gh(g - h);
   const interval_number abcd(ab * cd);
   const interval_number efgh(ef * gh);
   const interval_number L(abcd - efgh);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int orient2dzx_indirect_III_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   bigfloat l1y, l1z, l1x, d1, l2y, l2z, l2x, d2, l3y, l3z, l3x, d3;
   p1.getBigfloatLambda(l1y, l1z, l1x, d1);
   p2.getBigfloatLambda(l2y, l2z, l2x, d2);
   p3.getBigfloatLambda(l3y, l3z, l3x, d3);
   const bigfloat a(d1 * l2x);
   const bigfloat b(d2 * l1x);
   const bigfloat c(d1 * l3y);
   const bigfloat d(d3 * l1y);
   const bigfloat e(d1 * l2y);
   const bigfloat f(d2 * l1y);
   const bigfloat g(d1 * l3x);
   const bigfloat h(d3 * l1x);
   const bigfloat ab(a - b);
   const bigfloat cd(c - d);
   const bigfloat ef(e - f);
   const bigfloat gh(g - h);
   const bigfloat abcd(ab * cd);
   const bigfloat efgh(ef * gh);
   const bigfloat L(abcd - efgh);
   return sgn(L);
}

inline int orient2dzx_indirect_III_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1y_p[64], *l1y = l1y_p, l1z_p[64], *l1z = l1z_p, l1x_p[64], *l1x = l1x_p, d1_p[64], *d1 = d1_p, l2y_p[64], *l2y = l2y_p, l2z_p[64], *l2z = l2z_p, l2x_p[64], *l2x = l2x_p, d2_p[64], *d2 = d2_p, l3y_p[64], *l3y = l3y_p, l3z_p[64], *l3z = l3z_p, l3x_p[64], *l3x = l3x_p, d3_p[64], *d3 = d3_p;
 int l1y_len = 64, l1z_len = 64, l1x_len = 64, d1_len = 64, l2y_len = 64, l2z_len = 64, l2x_len = 64, d2_len = 64, l3y_len = 64, l3z_len = 64, l3x_len = 64, d3_len = 64;
 p1.getExactLambda(&l1y, l1y_len, &l1z, l1z_len, &l1x, l1x_len, &d1, d1_len);
 p2.getExactLambda(&l2y, l2y_len, &l2z, l2z_len, &l2x, l2x_len, &d2, d2_len);
 p3.getExactLambda(&l3y, l3y_len, &l3z, l3z_len, &l3x, l3x_len, &d3, d3_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0))
 {
   
   double a_p[64], *a = a_p;
   int a_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2x_len, l2x, &a, 64);
   double b_p[64], *b = b_p;
   int b_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1x_len, l1x, &b, 64);
   double c_p[64], *c = c_p;
   int c_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l3y_len, l3y, &c, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l1y_len, l1y, &d, 64);
   double e_p[64], *e = e_p;
   int e_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2y_len, l2y, &e, 64);
   double f_p[64], *f = f_p;
   int f_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1y_len, l1y, &f, 64);
   double g_p[64], *g = g_p;
   int g_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l3x_len, l3x, &g, 64);
   double h_p[64], *h = h_p;
   int h_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l1x_len, l1x, &h, 64);
   double ab_p[64], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(a_len, a, b_len, b, &ab, 64);
   double cd_p[64], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(c_len, c, d_len, d, &cd, 64);
   double ef_p[64], *ef = ef_p;
   int ef_len = expansionObject::Gen_Diff_With_PreAlloc(e_len, e, f_len, f, &ef, 64);
   double gh_p[64], *gh = gh_p;
   int gh_len = expansionObject::Gen_Diff_With_PreAlloc(g_len, g, h_len, h, &gh, 64);
   double abcd_p[64], *abcd = abcd_p;
   int abcd_len = expansionObject::Gen_Product_With_PreAlloc(ab_len, ab, cd_len, cd, &abcd, 64);
   double efgh_p[64], *efgh = efgh_p;
   int efgh_len = expansionObject::Gen_Product_With_PreAlloc(ef_len, ef, gh_len, gh, &efgh, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Diff_With_PreAlloc(abcd_len, abcd, efgh_len, efgh, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (efgh_p != efgh) FreeDoubles(efgh);
   if (abcd_p != abcd) FreeDoubles(abcd);
   if (gh_p != gh) FreeDoubles(gh);
   if (ef_p != ef) FreeDoubles(ef);
   if (cd_p != cd) FreeDoubles(cd);
   if (ab_p != ab) FreeDoubles(ab);
   if (h_p != h) FreeDoubles(h);
   if (g_p != g) FreeDoubles(g);
   if (f_p != f) FreeDoubles(f);
   if (e_p != e) FreeDoubles(e);
   if (d_p != d) FreeDoubles(d);
   if (c_p != c) FreeDoubles(c);
   if (b_p != b) FreeDoubles(b);
   if (a_p != a) FreeDoubles(a);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2dzx_indirect_III_bigfloat(p1, p2, p3);
#endif

 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (l1x_p != l1x) FreeDoubles(l1x);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (d3_p != d3) FreeDoubles(d3);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2dzx_indirect_III(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   int ret;
   ret = orient2dzx_indirect_III_interval(p1, p2, p3);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2dzx_indirect_III_exact(p1, p2, p3);
}

inline int orient2d_indirect_IEE_interval(const genericPoint& p1, interval_number p2x, interval_number p2y, interval_number p3x, interval_number p3y)
{
   interval_number l1x, l1y, d1;
   if (
   !p1.getIntervalLambda(l1x, l1y, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number t1x(p2y - p3y);
   const interval_number t1y(p3x - p2x);
   const interval_number e2(l1x * t1x);
   const interval_number e3(l1y * t1y);
   const interval_number e(e2 + e3);
   const interval_number pr1(p2x * p3y);
   const interval_number pr2(p2y * p3x);
   const interval_number pr(pr1 - pr2);
   const interval_number dpr(d1 * pr);
   const interval_number det(dpr + e);
   setFPUModeToRoundNEAR();

   if (!det.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return det.sign();
}

inline int orient2d_indirect_IEE_bigfloat(const genericPoint& p1, bigfloat p2x, bigfloat p2y, bigfloat p3x, bigfloat p3y)
{
   bigfloat l1x, l1y, d1;
   p1.getBigfloatLambda(l1x, l1y, d1);
   const bigfloat t1x(p2y - p3y);
   const bigfloat t1y(p3x - p2x);
   const bigfloat e2(l1x * t1x);
   const bigfloat e3(l1y * t1y);
   const bigfloat e(e2 + e3);
   const bigfloat pr1(p2x * p3y);
   const bigfloat pr2(p2y * p3x);
   const bigfloat pr(pr1 - pr2);
   const bigfloat dpr(d1 * pr);
   const bigfloat det(dpr + e);
   return sgn(det);
}

inline int orient2d_indirect_IEE_exact(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[128], *l1x = l1x_p, l1y_p[128], *l1y = l1y_p, d1_p[128], *d1 = d1_p;
 int l1x_len = 128, l1y_len = 128, d1_len = 128;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double t1x[2];
   expansionObject::two_Diff(p2y, p3y, t1x);
   double t1y[2];
   expansionObject::two_Diff(p3x, p2x, t1y);
   double e2_p[128], *e2 = e2_p;
   int e2_len = expansionObject::Gen_Product_With_PreAlloc(l1x_len, l1x, 2, t1x, &e2, 128);
   double e3_p[128], *e3 = e3_p;
   int e3_len = expansionObject::Gen_Product_With_PreAlloc(l1y_len, l1y, 2, t1y, &e3, 128);
   double e_p[128], *e = e_p;
   int e_len = expansionObject::Gen_Sum_With_PreAlloc(e2_len, e2, e3_len, e3, &e, 128);
   double pr1[2];
   expansionObject::Two_Prod(p2x, p3y, pr1);
   double pr2[2];
   expansionObject::Two_Prod(p2y, p3x, pr2);
   double pr[4];
   expansionObject::Two_Two_Diff(pr1, pr2, pr);
   double dpr_p[128], *dpr = dpr_p;
   int dpr_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, 4, pr, &dpr, 128);
   double det_p[128], *det = det_p;
   int det_len = expansionObject::Gen_Sum_With_PreAlloc(dpr_len, dpr, e_len, e, &det, 128);

   return_value = det[det_len - 1];
   if (det_p != det) FreeDoubles(det);
   if (dpr_p != dpr) FreeDoubles(dpr);
   if (e_p != e) FreeDoubles(e);
   if (e3_p != e3) FreeDoubles(e3);
   if (e2_p != e2) FreeDoubles(e2);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2d_indirect_IEE_bigfloat(p1, p2x, p2y, p3x, p3y);
#endif


 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2d_indirect_IEE(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y)
{
   int ret;
   ret = orient2d_indirect_IEE_interval(p1, p2x, p2y, p3x, p3y);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2d_indirect_IEE_exact(p1, p2x, p2y, p3x, p3y);
}

inline int orient2d_indirect_IIE_interval(const genericPoint& p1, const genericPoint& p2, interval_number p3x, interval_number p3y)
{
   interval_number l1x, l1y, d1, l2x, l2y, d2;
   if (
   !p1.getIntervalLambda(l1x, l1y, d1)
   || !p2.getIntervalLambda(l2x, l2y, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number a(d1 * l2x);
   const interval_number b(d2 * l1x);
   const interval_number c(d1 * p3y);
   const interval_number e(d1 * l2y);
   const interval_number f(d2 * l1y);
   const interval_number g(d1 * p3x);
   const interval_number ab(a - b);
   const interval_number cd(c - l1y);
   const interval_number ef(e - f);
   const interval_number gh(g - l1x);
   const interval_number abcd(ab * cd);
   const interval_number efgh(ef * gh);
   const interval_number L(abcd - efgh);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int orient2d_indirect_IIE_bigfloat(const genericPoint& p1, const genericPoint& p2, bigfloat p3x, bigfloat p3y)
{
   bigfloat l1x, l1y, d1, l2x, l2y, d2;
   p1.getBigfloatLambda(l1x, l1y, d1);
   p2.getBigfloatLambda(l2x, l2y, d2);
   const bigfloat a(d1 * l2x);
   const bigfloat b(d2 * l1x);
   const bigfloat c(d1 * p3y);
   const bigfloat e(d1 * l2y);
   const bigfloat f(d2 * l1y);
   const bigfloat g(d1 * p3x);
   const bigfloat ab(a - b);
   const bigfloat cd(c - l1y);
   const bigfloat ef(e - f);
   const bigfloat gh(g - l1x);
   const bigfloat abcd(ab * cd);
   const bigfloat efgh(ef * gh);
   const bigfloat L(abcd - efgh);
   return sgn(L);
}

inline int orient2d_indirect_IIE_exact(const genericPoint& p1, const genericPoint& p2, double p3x, double p3y)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[64], *l1x = l1x_p, l1y_p[64], *l1y = l1y_p, d1_p[64], *d1 = d1_p, l2x_p[64], *l2x = l2x_p, l2y_p[64], *l2y = l2y_p, d2_p[64], *d2 = d2_p;
 int l1x_len = 64, l1y_len = 64, d1_len = 64, l2x_len = 64, l2y_len = 64, d2_len = 64;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double a_p[64], *a = a_p;
   int a_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2x_len, l2x, &a, 64);
   double b_p[64], *b = b_p;
   int b_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1x_len, l1x, &b, 64);
   double c_p[64], *c = c_p;
   int c_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, p3y, &c, 64);
   double e_p[64], *e = e_p;
   int e_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2y_len, l2y, &e, 64);
   double f_p[64], *f = f_p;
   int f_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1y_len, l1y, &f, 64);
   double g_p[64], *g = g_p;
   int g_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, p3x, &g, 64);
   double ab_p[64], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(a_len, a, b_len, b, &ab, 64);
   double cd_p[64], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(c_len, c, l1y_len, l1y, &cd, 64);
   double ef_p[64], *ef = ef_p;
   int ef_len = expansionObject::Gen_Diff_With_PreAlloc(e_len, e, f_len, f, &ef, 64);
   double gh_p[64], *gh = gh_p;
   int gh_len = expansionObject::Gen_Diff_With_PreAlloc(g_len, g, l1x_len, l1x, &gh, 64);
   double abcd_p[64], *abcd = abcd_p;
   int abcd_len = expansionObject::Gen_Product_With_PreAlloc(ab_len, ab, cd_len, cd, &abcd, 64);
   double efgh_p[64], *efgh = efgh_p;
   int efgh_len = expansionObject::Gen_Product_With_PreAlloc(ef_len, ef, gh_len, gh, &efgh, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Diff_With_PreAlloc(abcd_len, abcd, efgh_len, efgh, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (efgh_p != efgh) FreeDoubles(efgh);
   if (abcd_p != abcd) FreeDoubles(abcd);
   if (gh_p != gh) FreeDoubles(gh);
   if (ef_p != ef) FreeDoubles(ef);
   if (cd_p != cd) FreeDoubles(cd);
   if (ab_p != ab) FreeDoubles(ab);
   if (g_p != g) FreeDoubles(g);
   if (f_p != f) FreeDoubles(f);
   if (e_p != e) FreeDoubles(e);
   if (c_p != c) FreeDoubles(c);
   if (b_p != b) FreeDoubles(b);
   if (a_p != a) FreeDoubles(a);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2d_indirect_IIE_bigfloat(p1, p2, p3x, p3y);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (d2_p != d2) FreeDoubles(d2);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2d_indirect_IIE(const genericPoint& p1, const genericPoint& p2, double p3x, double p3y)
{
   int ret;
   ret = orient2d_indirect_IIE_interval(p1, p2, p3x, p3y);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2d_indirect_IIE_exact(p1, p2, p3x, p3y);
}

inline int orient2d_indirect_III_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   interval_number l1x, l1y, d1, l2x, l2y, d2, l3x, l3y, d3;
   if (
   !p1.getIntervalLambda(l1x, l1y, d1)
   || !p2.getIntervalLambda(l2x, l2y, d2)
   || !p3.getIntervalLambda(l3x, l3y, d3)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number a(d1 * l2x);
   const interval_number b(d2 * l1x);
   const interval_number c(d1 * l3y);
   const interval_number d(d3 * l1y);
   const interval_number e(d1 * l2y);
   const interval_number f(d2 * l1y);
   const interval_number g(d1 * l3x);
   const interval_number h(d3 * l1x);
   const interval_number ab(a - b);
   const interval_number cd(c - d);
   const interval_number ef(e - f);
   const interval_number gh(g - h);
   const interval_number abcd(ab * cd);
   const interval_number efgh(ef * gh);
   const interval_number L(abcd - efgh);
   setFPUModeToRoundNEAR();

   if (!L.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return L.sign();
}

inline int orient2d_indirect_III_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   bigfloat l1x, l1y, d1, l2x, l2y, d2, l3x, l3y, d3;
   p1.getBigfloatLambda(l1x, l1y, d1);
   p2.getBigfloatLambda(l2x, l2y, d2);
   p3.getBigfloatLambda(l3x, l3y, d3);
   const bigfloat a(d1 * l2x);
   const bigfloat b(d2 * l1x);
   const bigfloat c(d1 * l3y);
   const bigfloat d(d3 * l1y);
   const bigfloat e(d1 * l2y);
   const bigfloat f(d2 * l1y);
   const bigfloat g(d1 * l3x);
   const bigfloat h(d3 * l1x);
   const bigfloat ab(a - b);
   const bigfloat cd(c - d);
   const bigfloat ef(e - f);
   const bigfloat gh(g - h);
   const bigfloat abcd(ab * cd);
   const bigfloat efgh(ef * gh);
   const bigfloat L(abcd - efgh);
   return sgn(L);
}

inline int orient2d_indirect_III_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[64], *l1x = l1x_p, l1y_p[64], *l1y = l1y_p, d1_p[64], *d1 = d1_p, l2x_p[64], *l2x = l2x_p, l2y_p[64], *l2y = l2y_p, d2_p[64], *d2 = d2_p, l3x_p[64], *l3x = l3x_p, l3y_p[64], *l3y = l3y_p, d3_p[64], *d3 = d3_p;
 int l1x_len = 64, l1y_len = 64, d1_len = 64, l2x_len = 64, l2y_len = 64, d2_len = 64, l3x_len = 64, l3y_len = 64, d3_len = 64;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &d3, d3_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0))
 {
   
   double a_p[64], *a = a_p;
   int a_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2x_len, l2x, &a, 64);
   double b_p[64], *b = b_p;
   int b_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1x_len, l1x, &b, 64);
   double c_p[64], *c = c_p;
   int c_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l3y_len, l3y, &c, 64);
   double d_p[64], *d = d_p;
   int d_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l1y_len, l1y, &d, 64);
   double e_p[64], *e = e_p;
   int e_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l2y_len, l2y, &e, 64);
   double f_p[64], *f = f_p;
   int f_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l1y_len, l1y, &f, 64);
   double g_p[64], *g = g_p;
   int g_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l3x_len, l3x, &g, 64);
   double h_p[64], *h = h_p;
   int h_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l1x_len, l1x, &h, 64);
   double ab_p[64], *ab = ab_p;
   int ab_len = expansionObject::Gen_Diff_With_PreAlloc(a_len, a, b_len, b, &ab, 64);
   double cd_p[64], *cd = cd_p;
   int cd_len = expansionObject::Gen_Diff_With_PreAlloc(c_len, c, d_len, d, &cd, 64);
   double ef_p[64], *ef = ef_p;
   int ef_len = expansionObject::Gen_Diff_With_PreAlloc(e_len, e, f_len, f, &ef, 64);
   double gh_p[64], *gh = gh_p;
   int gh_len = expansionObject::Gen_Diff_With_PreAlloc(g_len, g, h_len, h, &gh, 64);
   double abcd_p[64], *abcd = abcd_p;
   int abcd_len = expansionObject::Gen_Product_With_PreAlloc(ab_len, ab, cd_len, cd, &abcd, 64);
   double efgh_p[64], *efgh = efgh_p;
   int efgh_len = expansionObject::Gen_Product_With_PreAlloc(ef_len, ef, gh_len, gh, &efgh, 64);
   double L_p[64], *L = L_p;
   int L_len = expansionObject::Gen_Diff_With_PreAlloc(abcd_len, abcd, efgh_len, efgh, &L, 64);

   return_value = L[L_len - 1];
   if (L_p != L) FreeDoubles(L);
   if (efgh_p != efgh) FreeDoubles(efgh);
   if (abcd_p != abcd) FreeDoubles(abcd);
   if (gh_p != gh) FreeDoubles(gh);
   if (ef_p != ef) FreeDoubles(ef);
   if (cd_p != cd) FreeDoubles(cd);
   if (ab_p != ab) FreeDoubles(ab);
   if (h_p != h) FreeDoubles(h);
   if (g_p != g) FreeDoubles(g);
   if (f_p != f) FreeDoubles(f);
   if (e_p != e) FreeDoubles(e);
   if (d_p != d) FreeDoubles(d);
   if (c_p != c) FreeDoubles(c);
   if (b_p != b) FreeDoubles(b);
   if (a_p != a) FreeDoubles(a);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient2d_indirect_III_bigfloat(p1, p2, p3);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (d3_p != d3) FreeDoubles(d3);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient2d_indirect_III(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3)
{
   int ret;
   ret = orient2d_indirect_III_interval(p1, p2, p3);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2d_indirect_III_exact(p1, p2, p3);
}

inline int orient3d_indirect_IEEE_interval(const genericPoint& p1, interval_number ax, interval_number ay, interval_number az, interval_number bx, interval_number by, interval_number bz, interval_number cx, interval_number cy, interval_number cz)
{
   interval_number l1x, l1y, l1z, d1;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number dcx(d1 * cx);
   const interval_number dcy(d1 * cy);
   const interval_number dcz(d1 * cz);
   const interval_number ix_cx(l1x - dcx);
   const interval_number iy_cy(l1y - dcy);
   const interval_number ax_cx(ax - cx);
   const interval_number ay_cy(ay - cy);
   const interval_number az_cz(az - cz);
   const interval_number iz_cz(l1z - dcz);
   const interval_number bx_cx(bx - cx);
   const interval_number by_cy(by - cy);
   const interval_number bz_cz(bz - cz);
   const interval_number tmc_a(ix_cx * ay_cy);
   const interval_number tmc_b(iy_cy * ax_cx);
   const interval_number m01(tmc_a - tmc_b);
   const interval_number tmi_a(ix_cx * az_cz);
   const interval_number tmi_b(iz_cz * ax_cx);
   const interval_number m02(tmi_a - tmi_b);
   const interval_number tma_a(iy_cy * az_cz);
   const interval_number tma_b(iz_cz * ay_cy);
   const interval_number m12(tma_a - tma_b);
   const interval_number mt1(m01 * bz_cz);
   const interval_number mt2(m02 * by_cy);
   const interval_number mt3(m12 * bx_cx);
   const interval_number mtt(mt2 - mt1);
   const interval_number m012(mtt - mt3);
   setFPUModeToRoundNEAR();

   if (!m012.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return m012.sign();
}

inline int orient3d_indirect_IEEE_bigfloat(const genericPoint& p1, bigfloat ax, bigfloat ay, bigfloat az, bigfloat bx, bigfloat by, bigfloat bz, bigfloat cx, bigfloat cy, bigfloat cz)
{
   bigfloat l1x, l1y, l1z, d1;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   const bigfloat dcx(d1 * cx);
   const bigfloat dcy(d1 * cy);
   const bigfloat dcz(d1 * cz);
   const bigfloat ix_cx(l1x - dcx);
   const bigfloat iy_cy(l1y - dcy);
   const bigfloat ax_cx(ax - cx);
   const bigfloat ay_cy(ay - cy);
   const bigfloat az_cz(az - cz);
   const bigfloat iz_cz(l1z - dcz);
   const bigfloat bx_cx(bx - cx);
   const bigfloat by_cy(by - cy);
   const bigfloat bz_cz(bz - cz);
   const bigfloat tmc_a(ix_cx * ay_cy);
   const bigfloat tmc_b(iy_cy * ax_cx);
   const bigfloat m01(tmc_a - tmc_b);
   const bigfloat tmi_a(ix_cx * az_cz);
   const bigfloat tmi_b(iz_cz * ax_cx);
   const bigfloat m02(tmi_a - tmi_b);
   const bigfloat tma_a(iy_cy * az_cz);
   const bigfloat tma_b(iz_cz * ay_cy);
   const bigfloat m12(tma_a - tma_b);
   const bigfloat mt1(m01 * bz_cz);
   const bigfloat mt2(m02 * by_cy);
   const bigfloat mt3(m12 * bx_cx);
   const bigfloat mtt(mt2 - mt1);
   const bigfloat m012(mtt - mt3);
   return sgn(m012);
}

inline int orient3d_indirect_IEEE_exact(const genericPoint& p1, double ax, double ay, double az, double bx, double by, double bz, double cx, double cy, double cz)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[64], *l1x = l1x_p, l1y_p[64], *l1y = l1y_p, l1z_p[64], *l1z = l1z_p, d1_p[64], *d1 = d1_p;
 int l1x_len = 64, l1y_len = 64, l1z_len = 64, d1_len = 64;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 if ((d1[d1_len - 1] != 0))
 {
   
   double dcx_p[64], *dcx = dcx_p;
   int dcx_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, cx, &dcx, 64);
   double dcy_p[64], *dcy = dcy_p;
   int dcy_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, cy, &dcy, 64);
   double dcz_p[64], *dcz = dcz_p;
   int dcz_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, cz, &dcz, 64);
   double ix_cx_p[64], *ix_cx = ix_cx_p;
   int ix_cx_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, dcx_len, dcx, &ix_cx, 64);
   double iy_cy_p[64], *iy_cy = iy_cy_p;
   int iy_cy_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, dcy_len, dcy, &iy_cy, 64);
   double ax_cx[2];
   expansionObject::two_Diff(ax, cx, ax_cx);
   double ay_cy[2];
   expansionObject::two_Diff(ay, cy, ay_cy);
   double az_cz[2];
   expansionObject::two_Diff(az, cz, az_cz);
   double iz_cz_p[64], *iz_cz = iz_cz_p;
   int iz_cz_len = expansionObject::Gen_Diff_With_PreAlloc(l1z_len, l1z, dcz_len, dcz, &iz_cz, 64);
   double bx_cx[2];
   expansionObject::two_Diff(bx, cx, bx_cx);
   double by_cy[2];
   expansionObject::two_Diff(by, cy, by_cy);
   double bz_cz[2];
   expansionObject::two_Diff(bz, cz, bz_cz);
   double tmc_a_p[64], *tmc_a = tmc_a_p;
   int tmc_a_len = expansionObject::Gen_Product_With_PreAlloc(ix_cx_len, ix_cx, 2, ay_cy, &tmc_a, 64);
   double tmc_b_p[64], *tmc_b = tmc_b_p;
   int tmc_b_len = expansionObject::Gen_Product_With_PreAlloc(iy_cy_len, iy_cy, 2, ax_cx, &tmc_b, 64);
   double m01_p[64], *m01 = m01_p;
   int m01_len = expansionObject::Gen_Diff_With_PreAlloc(tmc_a_len, tmc_a, tmc_b_len, tmc_b, &m01, 64);
   double tmi_a_p[64], *tmi_a = tmi_a_p;
   int tmi_a_len = expansionObject::Gen_Product_With_PreAlloc(ix_cx_len, ix_cx, 2, az_cz, &tmi_a, 64);
   double tmi_b_p[64], *tmi_b = tmi_b_p;
   int tmi_b_len = expansionObject::Gen_Product_With_PreAlloc(iz_cz_len, iz_cz, 2, ax_cx, &tmi_b, 64);
   double m02_p[64], *m02 = m02_p;
   int m02_len = expansionObject::Gen_Diff_With_PreAlloc(tmi_a_len, tmi_a, tmi_b_len, tmi_b, &m02, 64);
   double tma_a_p[64], *tma_a = tma_a_p;
   int tma_a_len = expansionObject::Gen_Product_With_PreAlloc(iy_cy_len, iy_cy, 2, az_cz, &tma_a, 64);
   double tma_b_p[64], *tma_b = tma_b_p;
   int tma_b_len = expansionObject::Gen_Product_With_PreAlloc(iz_cz_len, iz_cz, 2, ay_cy, &tma_b, 64);
   double m12_p[64], *m12 = m12_p;
   int m12_len = expansionObject::Gen_Diff_With_PreAlloc(tma_a_len, tma_a, tma_b_len, tma_b, &m12, 64);
   double mt1_p[64], *mt1 = mt1_p;
   int mt1_len = expansionObject::Gen_Product_With_PreAlloc(m01_len, m01, 2, bz_cz, &mt1, 64);
   double mt2_p[64], *mt2 = mt2_p;
   int mt2_len = expansionObject::Gen_Product_With_PreAlloc(m02_len, m02, 2, by_cy, &mt2, 64);
   double mt3_p[64], *mt3 = mt3_p;
   int mt3_len = expansionObject::Gen_Product_With_PreAlloc(m12_len, m12, 2, bx_cx, &mt3, 64);
   double mtt_p[64], *mtt = mtt_p;
   int mtt_len = expansionObject::Gen_Diff_With_PreAlloc(mt2_len, mt2, mt1_len, mt1, &mtt, 64);
   double m012_p[64], *m012 = m012_p;
   int m012_len = expansionObject::Gen_Diff_With_PreAlloc(mtt_len, mtt, mt3_len, mt3, &m012, 64);

   return_value = m012[m012_len - 1];
   if (m012_p != m012) FreeDoubles(m012);
   if (mtt_p != mtt) FreeDoubles(mtt);
   if (mt3_p != mt3) FreeDoubles(mt3);
   if (mt2_p != mt2) FreeDoubles(mt2);
   if (mt1_p != mt1) FreeDoubles(mt1);
   if (m12_p != m12) FreeDoubles(m12);
   if (tma_b_p != tma_b) FreeDoubles(tma_b);
   if (tma_a_p != tma_a) FreeDoubles(tma_a);
   if (m02_p != m02) FreeDoubles(m02);
   if (tmi_b_p != tmi_b) FreeDoubles(tmi_b);
   if (tmi_a_p != tmi_a) FreeDoubles(tmi_a);
   if (m01_p != m01) FreeDoubles(m01);
   if (tmc_b_p != tmc_b) FreeDoubles(tmc_b);
   if (tmc_a_p != tmc_a) FreeDoubles(tmc_a);
   if (iz_cz_p != iz_cz) FreeDoubles(iz_cz);
   if (iy_cy_p != iy_cy) FreeDoubles(iy_cy);
   if (ix_cx_p != ix_cx) FreeDoubles(ix_cx);
   if (dcz_p != dcz) FreeDoubles(dcz);
   if (dcy_p != dcy) FreeDoubles(dcy);
   if (dcx_p != dcx) FreeDoubles(dcx);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient3d_indirect_IEEE_bigfloat(p1, ax, ay, az, bx, by, bz, cx, cy, cz);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient3d_indirect_IEEE(const genericPoint& p1, double ax, double ay, double az, double bx, double by, double bz, double cx, double cy, double cz)
{
   int ret;
   ret = orient3d_indirect_IEEE_interval(p1, ax, ay, az, bx, by, bz, cx, cy, cz);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient3d_indirect_IEEE_exact(p1, ax, ay, az, bx, by, bz, cx, cy, cz);
}

inline int orient3d_indirect_IIEE_interval(const genericPoint& p1, const genericPoint& p2, interval_number p3x, interval_number p3y, interval_number p3z, interval_number p4x, interval_number p4y, interval_number p4z)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number d1p4x(d1 * p4x);
   const interval_number d1p4y(d1 * p4y);
   const interval_number d1p4z(d1 * p4z);
   const interval_number d2p4x(d2 * p4x);
   const interval_number d2p4y(d2 * p4y);
   const interval_number d2p4z(d2 * p4z);
   const interval_number p1p4x(l1x - d1p4x);
   const interval_number p1p4y(l1y - d1p4y);
   const interval_number p1p4z(l1z - d1p4z);
   const interval_number p2p4x(l2x - d2p4x);
   const interval_number p2p4y(l2y - d2p4y);
   const interval_number p2p4z(l2z - d2p4z);
   const interval_number p3p4x(p3x - p4x);
   const interval_number p3p4y(p3y - p4y);
   const interval_number p3p4z(p3z - p4z);
   const interval_number tmc_a(p1p4x * p2p4y);
   const interval_number tmc_b(p1p4y * p2p4x);
   const interval_number m01(tmc_a - tmc_b);
   const interval_number tmi_a(p1p4x * p2p4z);
   const interval_number tmi_b(p1p4z * p2p4x);
   const interval_number m02(tmi_a - tmi_b);
   const interval_number tma_a(p1p4y * p2p4z);
   const interval_number tma_b(p1p4z * p2p4y);
   const interval_number m12(tma_a - tma_b);
   const interval_number mt1(m01 * p3p4z);
   const interval_number mt2(m02 * p3p4y);
   const interval_number mt3(m12 * p3p4x);
   const interval_number mtt(mt2 - mt1);
   const interval_number m012(mtt - mt3);
   setFPUModeToRoundNEAR();

   if (!m012.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return m012.sign();
}

inline int orient3d_indirect_IIEE_bigfloat(const genericPoint& p1, const genericPoint& p2, bigfloat p3x, bigfloat p3y, bigfloat p3z, bigfloat p4x, bigfloat p4y, bigfloat p4z)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   const bigfloat d1p4x(d1 * p4x);
   const bigfloat d1p4y(d1 * p4y);
   const bigfloat d1p4z(d1 * p4z);
   const bigfloat d2p4x(d2 * p4x);
   const bigfloat d2p4y(d2 * p4y);
   const bigfloat d2p4z(d2 * p4z);
   const bigfloat p1p4x(l1x - d1p4x);
   const bigfloat p1p4y(l1y - d1p4y);
   const bigfloat p1p4z(l1z - d1p4z);
   const bigfloat p2p4x(l2x - d2p4x);
   const bigfloat p2p4y(l2y - d2p4y);
   const bigfloat p2p4z(l2z - d2p4z);
   const bigfloat p3p4x(p3x - p4x);
   const bigfloat p3p4y(p3y - p4y);
   const bigfloat p3p4z(p3z - p4z);
   const bigfloat tmc_a(p1p4x * p2p4y);
   const bigfloat tmc_b(p1p4y * p2p4x);
   const bigfloat m01(tmc_a - tmc_b);
   const bigfloat tmi_a(p1p4x * p2p4z);
   const bigfloat tmi_b(p1p4z * p2p4x);
   const bigfloat m02(tmi_a - tmi_b);
   const bigfloat tma_a(p1p4y * p2p4z);
   const bigfloat tma_b(p1p4z * p2p4y);
   const bigfloat m12(tma_a - tma_b);
   const bigfloat mt1(m01 * p3p4z);
   const bigfloat mt2(m02 * p3p4y);
   const bigfloat mt3(m12 * p3p4x);
   const bigfloat mtt(mt2 - mt1);
   const bigfloat m012(mtt - mt3);
   return sgn(m012);
}

inline int orient3d_indirect_IIEE_exact(const genericPoint& p1, const genericPoint& p2, double p3x, double p3y, double p3z, double p4x, double p4y, double p4z)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[32], *l1x = l1x_p, l1y_p[32], *l1y = l1y_p, l1z_p[32], *l1z = l1z_p, d1_p[32], *d1 = d1_p, l2x_p[32], *l2x = l2x_p, l2y_p[32], *l2y = l2y_p, l2z_p[32], *l2z = l2z_p, d2_p[32], *d2 = d2_p;
 int l1x_len = 32, l1y_len = 32, l1z_len = 32, d1_len = 32, l2x_len = 32, l2y_len = 32, l2z_len = 32, d2_len = 32;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0))
 {
   
   double d1p4x_p[32], *d1p4x = d1p4x_p;
   int d1p4x_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, p4x, &d1p4x, 32);
   double d1p4y_p[32], *d1p4y = d1p4y_p;
   int d1p4y_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, p4y, &d1p4y, 32);
   double d1p4z_p[32], *d1p4z = d1p4z_p;
   int d1p4z_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, p4z, &d1p4z, 32);
   double d2p4x_p[32], *d2p4x = d2p4x_p;
   int d2p4x_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, p4x, &d2p4x, 32);
   double d2p4y_p[32], *d2p4y = d2p4y_p;
   int d2p4y_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, p4y, &d2p4y, 32);
   double d2p4z_p[32], *d2p4z = d2p4z_p;
   int d2p4z_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, p4z, &d2p4z, 32);
   double p1p4x_p[32], *p1p4x = p1p4x_p;
   int p1p4x_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, d1p4x_len, d1p4x, &p1p4x, 32);
   double p1p4y_p[32], *p1p4y = p1p4y_p;
   int p1p4y_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, d1p4y_len, d1p4y, &p1p4y, 32);
   double p1p4z_p[32], *p1p4z = p1p4z_p;
   int p1p4z_len = expansionObject::Gen_Diff_With_PreAlloc(l1z_len, l1z, d1p4z_len, d1p4z, &p1p4z, 32);
   double p2p4x_p[32], *p2p4x = p2p4x_p;
   int p2p4x_len = expansionObject::Gen_Diff_With_PreAlloc(l2x_len, l2x, d2p4x_len, d2p4x, &p2p4x, 32);
   double p2p4y_p[32], *p2p4y = p2p4y_p;
   int p2p4y_len = expansionObject::Gen_Diff_With_PreAlloc(l2y_len, l2y, d2p4y_len, d2p4y, &p2p4y, 32);
   double p2p4z_p[32], *p2p4z = p2p4z_p;
   int p2p4z_len = expansionObject::Gen_Diff_With_PreAlloc(l2z_len, l2z, d2p4z_len, d2p4z, &p2p4z, 32);
   double p3p4x[2];
   expansionObject::two_Diff(p3x, p4x, p3p4x);
   double p3p4y[2];
   expansionObject::two_Diff(p3y, p4y, p3p4y);
   double p3p4z[2];
   expansionObject::two_Diff(p3z, p4z, p3p4z);
   double tmc_a_p[32], *tmc_a = tmc_a_p;
   int tmc_a_len = expansionObject::Gen_Product_With_PreAlloc(p1p4x_len, p1p4x, p2p4y_len, p2p4y, &tmc_a, 32);
   double tmc_b_p[32], *tmc_b = tmc_b_p;
   int tmc_b_len = expansionObject::Gen_Product_With_PreAlloc(p1p4y_len, p1p4y, p2p4x_len, p2p4x, &tmc_b, 32);
   double m01_p[32], *m01 = m01_p;
   int m01_len = expansionObject::Gen_Diff_With_PreAlloc(tmc_a_len, tmc_a, tmc_b_len, tmc_b, &m01, 32);
   double tmi_a_p[32], *tmi_a = tmi_a_p;
   int tmi_a_len = expansionObject::Gen_Product_With_PreAlloc(p1p4x_len, p1p4x, p2p4z_len, p2p4z, &tmi_a, 32);
   double tmi_b_p[32], *tmi_b = tmi_b_p;
   int tmi_b_len = expansionObject::Gen_Product_With_PreAlloc(p1p4z_len, p1p4z, p2p4x_len, p2p4x, &tmi_b, 32);
   double m02_p[32], *m02 = m02_p;
   int m02_len = expansionObject::Gen_Diff_With_PreAlloc(tmi_a_len, tmi_a, tmi_b_len, tmi_b, &m02, 32);
   double tma_a_p[32], *tma_a = tma_a_p;
   int tma_a_len = expansionObject::Gen_Product_With_PreAlloc(p1p4y_len, p1p4y, p2p4z_len, p2p4z, &tma_a, 32);
   double tma_b_p[32], *tma_b = tma_b_p;
   int tma_b_len = expansionObject::Gen_Product_With_PreAlloc(p1p4z_len, p1p4z, p2p4y_len, p2p4y, &tma_b, 32);
   double m12_p[32], *m12 = m12_p;
   int m12_len = expansionObject::Gen_Diff_With_PreAlloc(tma_a_len, tma_a, tma_b_len, tma_b, &m12, 32);
   double mt1_p[32], *mt1 = mt1_p;
   int mt1_len = expansionObject::Gen_Product_With_PreAlloc(m01_len, m01, 2, p3p4z, &mt1, 32);
   double mt2_p[32], *mt2 = mt2_p;
   int mt2_len = expansionObject::Gen_Product_With_PreAlloc(m02_len, m02, 2, p3p4y, &mt2, 32);
   double mt3_p[32], *mt3 = mt3_p;
   int mt3_len = expansionObject::Gen_Product_With_PreAlloc(m12_len, m12, 2, p3p4x, &mt3, 32);
   double mtt_p[32], *mtt = mtt_p;
   int mtt_len = expansionObject::Gen_Diff_With_PreAlloc(mt2_len, mt2, mt1_len, mt1, &mtt, 32);
   double m012_p[32], *m012 = m012_p;
   int m012_len = expansionObject::Gen_Diff_With_PreAlloc(mtt_len, mtt, mt3_len, mt3, &m012, 32);

   return_value = m012[m012_len - 1];
   if (m012_p != m012) FreeDoubles(m012);
   if (mtt_p != mtt) FreeDoubles(mtt);
   if (mt3_p != mt3) FreeDoubles(mt3);
   if (mt2_p != mt2) FreeDoubles(mt2);
   if (mt1_p != mt1) FreeDoubles(mt1);
   if (m12_p != m12) FreeDoubles(m12);
   if (tma_b_p != tma_b) FreeDoubles(tma_b);
   if (tma_a_p != tma_a) FreeDoubles(tma_a);
   if (m02_p != m02) FreeDoubles(m02);
   if (tmi_b_p != tmi_b) FreeDoubles(tmi_b);
   if (tmi_a_p != tmi_a) FreeDoubles(tmi_a);
   if (m01_p != m01) FreeDoubles(m01);
   if (tmc_b_p != tmc_b) FreeDoubles(tmc_b);
   if (tmc_a_p != tmc_a) FreeDoubles(tmc_a);
   if (p2p4z_p != p2p4z) FreeDoubles(p2p4z);
   if (p2p4y_p != p2p4y) FreeDoubles(p2p4y);
   if (p2p4x_p != p2p4x) FreeDoubles(p2p4x);
   if (p1p4z_p != p1p4z) FreeDoubles(p1p4z);
   if (p1p4y_p != p1p4y) FreeDoubles(p1p4y);
   if (p1p4x_p != p1p4x) FreeDoubles(p1p4x);
   if (d2p4z_p != d2p4z) FreeDoubles(d2p4z);
   if (d2p4y_p != d2p4y) FreeDoubles(d2p4y);
   if (d2p4x_p != d2p4x) FreeDoubles(d2p4x);
   if (d1p4z_p != d1p4z) FreeDoubles(d1p4z);
   if (d1p4y_p != d1p4y) FreeDoubles(d1p4y);
   if (d1p4x_p != d1p4x) FreeDoubles(d1p4x);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient3d_indirect_IIEE_bigfloat(p1, p2, p3x, p3y, p3z, p4x, p4y, p4z);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient3d_indirect_IIEE(const genericPoint& p1, const genericPoint& p2, double p3x, double p3y, double p3z, double p4x, double p4y, double p4z)
{
   int ret;
   ret = orient3d_indirect_IIEE_interval(p1, p2, p3x, p3y, p3z, p4x, p4y, p4z);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient3d_indirect_IIEE_exact(p1, p2, p3x, p3y, p3z, p4x, p4y, p4z);
}

inline int orient3d_indirect_IIIE_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, interval_number p4x, interval_number p4y, interval_number p4z)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   || !p3.getIntervalLambda(l3x, l3y, l3z, d3)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number d1p4x(d1 * p4x);
   const interval_number d1p4y(d1 * p4y);
   const interval_number d1p4z(d1 * p4z);
   const interval_number d2p4x(d2 * p4x);
   const interval_number d2p4y(d2 * p4y);
   const interval_number d2p4z(d2 * p4z);
   const interval_number d3p4x(d3 * p4x);
   const interval_number d3p4y(d3 * p4y);
   const interval_number d3p4z(d3 * p4z);
   const interval_number p1p4x(l1x - d1p4x);
   const interval_number p1p4y(l1y - d1p4y);
   const interval_number p1p4z(l1z - d1p4z);
   const interval_number p2p4x(l2x - d2p4x);
   const interval_number p2p4y(l2y - d2p4y);
   const interval_number p2p4z(l2z - d2p4z);
   const interval_number p3p4x(l3x - d3p4x);
   const interval_number p3p4y(l3y - d3p4y);
   const interval_number p3p4z(l3z - d3p4z);
   const interval_number tmc_a(p1p4x * p2p4y);
   const interval_number tmc_b(p1p4y * p2p4x);
   const interval_number m01(tmc_a - tmc_b);
   const interval_number tmi_a(p1p4x * p2p4z);
   const interval_number tmi_b(p1p4z * p2p4x);
   const interval_number m02(tmi_a - tmi_b);
   const interval_number tma_a(p1p4y * p2p4z);
   const interval_number tma_b(p1p4z * p2p4y);
   const interval_number m12(tma_a - tma_b);
   const interval_number mt1(m01 * p3p4z);
   const interval_number mt2(m02 * p3p4y);
   const interval_number mt3(m12 * p3p4x);
   const interval_number mtt(mt2 - mt1);
   const interval_number m012(mtt - mt3);
   setFPUModeToRoundNEAR();

   if (!m012.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return m012.sign();
}

inline int orient3d_indirect_IIIE_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, bigfloat p4x, bigfloat p4y, bigfloat p4z)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   p3.getBigfloatLambda(l3x, l3y, l3z, d3);
   const bigfloat d1p4x(d1 * p4x);
   const bigfloat d1p4y(d1 * p4y);
   const bigfloat d1p4z(d1 * p4z);
   const bigfloat d2p4x(d2 * p4x);
   const bigfloat d2p4y(d2 * p4y);
   const bigfloat d2p4z(d2 * p4z);
   const bigfloat d3p4x(d3 * p4x);
   const bigfloat d3p4y(d3 * p4y);
   const bigfloat d3p4z(d3 * p4z);
   const bigfloat p1p4x(l1x - d1p4x);
   const bigfloat p1p4y(l1y - d1p4y);
   const bigfloat p1p4z(l1z - d1p4z);
   const bigfloat p2p4x(l2x - d2p4x);
   const bigfloat p2p4y(l2y - d2p4y);
   const bigfloat p2p4z(l2z - d2p4z);
   const bigfloat p3p4x(l3x - d3p4x);
   const bigfloat p3p4y(l3y - d3p4y);
   const bigfloat p3p4z(l3z - d3p4z);
   const bigfloat tmc_a(p1p4x * p2p4y);
   const bigfloat tmc_b(p1p4y * p2p4x);
   const bigfloat m01(tmc_a - tmc_b);
   const bigfloat tmi_a(p1p4x * p2p4z);
   const bigfloat tmi_b(p1p4z * p2p4x);
   const bigfloat m02(tmi_a - tmi_b);
   const bigfloat tma_a(p1p4y * p2p4z);
   const bigfloat tma_b(p1p4z * p2p4y);
   const bigfloat m12(tma_a - tma_b);
   const bigfloat mt1(m01 * p3p4z);
   const bigfloat mt2(m02 * p3p4y);
   const bigfloat mt3(m12 * p3p4x);
   const bigfloat mtt(mt2 - mt1);
   const bigfloat m012(mtt - mt3);
   return sgn(m012);
}

inline int orient3d_indirect_IIIE_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double p4x, double p4y, double p4z)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[32], *l1x = l1x_p, l1y_p[32], *l1y = l1y_p, l1z_p[32], *l1z = l1z_p, d1_p[32], *d1 = d1_p, l2x_p[32], *l2x = l2x_p, l2y_p[32], *l2y = l2y_p, l2z_p[32], *l2z = l2z_p, d2_p[32], *d2 = d2_p, l3x_p[32], *l3x = l3x_p, l3y_p[32], *l3y = l3y_p, l3z_p[32], *l3z = l3z_p, d3_p[32], *d3 = d3_p;
 int l1x_len = 32, l1y_len = 32, l1z_len = 32, d1_len = 32, l2x_len = 32, l2y_len = 32, l2z_len = 32, d2_len = 32, l3x_len = 32, l3y_len = 32, l3z_len = 32, d3_len = 32;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &l3z, l3z_len, &d3, d3_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0))
 {
   
   double d1p4x_p[32], *d1p4x = d1p4x_p;
   int d1p4x_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, p4x, &d1p4x, 32);
   double d1p4y_p[32], *d1p4y = d1p4y_p;
   int d1p4y_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, p4y, &d1p4y, 32);
   double d1p4z_p[32], *d1p4z = d1p4z_p;
   int d1p4z_len = expansionObject::Gen_Scale_With_PreAlloc(d1_len, d1, p4z, &d1p4z, 32);
   double d2p4x_p[32], *d2p4x = d2p4x_p;
   int d2p4x_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, p4x, &d2p4x, 32);
   double d2p4y_p[32], *d2p4y = d2p4y_p;
   int d2p4y_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, p4y, &d2p4y, 32);
   double d2p4z_p[32], *d2p4z = d2p4z_p;
   int d2p4z_len = expansionObject::Gen_Scale_With_PreAlloc(d2_len, d2, p4z, &d2p4z, 32);
   double d3p4x_p[32], *d3p4x = d3p4x_p;
   int d3p4x_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, p4x, &d3p4x, 32);
   double d3p4y_p[32], *d3p4y = d3p4y_p;
   int d3p4y_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, p4y, &d3p4y, 32);
   double d3p4z_p[32], *d3p4z = d3p4z_p;
   int d3p4z_len = expansionObject::Gen_Scale_With_PreAlloc(d3_len, d3, p4z, &d3p4z, 32);
   double p1p4x_p[32], *p1p4x = p1p4x_p;
   int p1p4x_len = expansionObject::Gen_Diff_With_PreAlloc(l1x_len, l1x, d1p4x_len, d1p4x, &p1p4x, 32);
   double p1p4y_p[32], *p1p4y = p1p4y_p;
   int p1p4y_len = expansionObject::Gen_Diff_With_PreAlloc(l1y_len, l1y, d1p4y_len, d1p4y, &p1p4y, 32);
   double p1p4z_p[32], *p1p4z = p1p4z_p;
   int p1p4z_len = expansionObject::Gen_Diff_With_PreAlloc(l1z_len, l1z, d1p4z_len, d1p4z, &p1p4z, 32);
   double p2p4x_p[32], *p2p4x = p2p4x_p;
   int p2p4x_len = expansionObject::Gen_Diff_With_PreAlloc(l2x_len, l2x, d2p4x_len, d2p4x, &p2p4x, 32);
   double p2p4y_p[32], *p2p4y = p2p4y_p;
   int p2p4y_len = expansionObject::Gen_Diff_With_PreAlloc(l2y_len, l2y, d2p4y_len, d2p4y, &p2p4y, 32);
   double p2p4z_p[32], *p2p4z = p2p4z_p;
   int p2p4z_len = expansionObject::Gen_Diff_With_PreAlloc(l2z_len, l2z, d2p4z_len, d2p4z, &p2p4z, 32);
   double p3p4x_p[32], *p3p4x = p3p4x_p;
   int p3p4x_len = expansionObject::Gen_Diff_With_PreAlloc(l3x_len, l3x, d3p4x_len, d3p4x, &p3p4x, 32);
   double p3p4y_p[32], *p3p4y = p3p4y_p;
   int p3p4y_len = expansionObject::Gen_Diff_With_PreAlloc(l3y_len, l3y, d3p4y_len, d3p4y, &p3p4y, 32);
   double p3p4z_p[32], *p3p4z = p3p4z_p;
   int p3p4z_len = expansionObject::Gen_Diff_With_PreAlloc(l3z_len, l3z, d3p4z_len, d3p4z, &p3p4z, 32);
   double tmc_a_p[32], *tmc_a = tmc_a_p;
   int tmc_a_len = expansionObject::Gen_Product_With_PreAlloc(p1p4x_len, p1p4x, p2p4y_len, p2p4y, &tmc_a, 32);
   double tmc_b_p[32], *tmc_b = tmc_b_p;
   int tmc_b_len = expansionObject::Gen_Product_With_PreAlloc(p1p4y_len, p1p4y, p2p4x_len, p2p4x, &tmc_b, 32);
   double m01_p[32], *m01 = m01_p;
   int m01_len = expansionObject::Gen_Diff_With_PreAlloc(tmc_a_len, tmc_a, tmc_b_len, tmc_b, &m01, 32);
   double tmi_a_p[32], *tmi_a = tmi_a_p;
   int tmi_a_len = expansionObject::Gen_Product_With_PreAlloc(p1p4x_len, p1p4x, p2p4z_len, p2p4z, &tmi_a, 32);
   double tmi_b_p[32], *tmi_b = tmi_b_p;
   int tmi_b_len = expansionObject::Gen_Product_With_PreAlloc(p1p4z_len, p1p4z, p2p4x_len, p2p4x, &tmi_b, 32);
   double m02_p[32], *m02 = m02_p;
   int m02_len = expansionObject::Gen_Diff_With_PreAlloc(tmi_a_len, tmi_a, tmi_b_len, tmi_b, &m02, 32);
   double tma_a_p[32], *tma_a = tma_a_p;
   int tma_a_len = expansionObject::Gen_Product_With_PreAlloc(p1p4y_len, p1p4y, p2p4z_len, p2p4z, &tma_a, 32);
   double tma_b_p[32], *tma_b = tma_b_p;
   int tma_b_len = expansionObject::Gen_Product_With_PreAlloc(p1p4z_len, p1p4z, p2p4y_len, p2p4y, &tma_b, 32);
   double m12_p[32], *m12 = m12_p;
   int m12_len = expansionObject::Gen_Diff_With_PreAlloc(tma_a_len, tma_a, tma_b_len, tma_b, &m12, 32);
   double mt1_p[32], *mt1 = mt1_p;
   int mt1_len = expansionObject::Gen_Product_With_PreAlloc(m01_len, m01, p3p4z_len, p3p4z, &mt1, 32);
   double mt2_p[32], *mt2 = mt2_p;
   int mt2_len = expansionObject::Gen_Product_With_PreAlloc(m02_len, m02, p3p4y_len, p3p4y, &mt2, 32);
   double mt3_p[32], *mt3 = mt3_p;
   int mt3_len = expansionObject::Gen_Product_With_PreAlloc(m12_len, m12, p3p4x_len, p3p4x, &mt3, 32);
   double mtt_p[32], *mtt = mtt_p;
   int mtt_len = expansionObject::Gen_Diff_With_PreAlloc(mt2_len, mt2, mt1_len, mt1, &mtt, 32);
   double m012_p[32], *m012 = m012_p;
   int m012_len = expansionObject::Gen_Diff_With_PreAlloc(mtt_len, mtt, mt3_len, mt3, &m012, 32);

   return_value = m012[m012_len - 1];
   if (m012_p != m012) FreeDoubles(m012);
   if (mtt_p != mtt) FreeDoubles(mtt);
   if (mt3_p != mt3) FreeDoubles(mt3);
   if (mt2_p != mt2) FreeDoubles(mt2);
   if (mt1_p != mt1) FreeDoubles(mt1);
   if (m12_p != m12) FreeDoubles(m12);
   if (tma_b_p != tma_b) FreeDoubles(tma_b);
   if (tma_a_p != tma_a) FreeDoubles(tma_a);
   if (m02_p != m02) FreeDoubles(m02);
   if (tmi_b_p != tmi_b) FreeDoubles(tmi_b);
   if (tmi_a_p != tmi_a) FreeDoubles(tmi_a);
   if (m01_p != m01) FreeDoubles(m01);
   if (tmc_b_p != tmc_b) FreeDoubles(tmc_b);
   if (tmc_a_p != tmc_a) FreeDoubles(tmc_a);
   if (p3p4z_p != p3p4z) FreeDoubles(p3p4z);
   if (p3p4y_p != p3p4y) FreeDoubles(p3p4y);
   if (p3p4x_p != p3p4x) FreeDoubles(p3p4x);
   if (p2p4z_p != p2p4z) FreeDoubles(p2p4z);
   if (p2p4y_p != p2p4y) FreeDoubles(p2p4y);
   if (p2p4x_p != p2p4x) FreeDoubles(p2p4x);
   if (p1p4z_p != p1p4z) FreeDoubles(p1p4z);
   if (p1p4y_p != p1p4y) FreeDoubles(p1p4y);
   if (p1p4x_p != p1p4x) FreeDoubles(p1p4x);
   if (d3p4z_p != d3p4z) FreeDoubles(d3p4z);
   if (d3p4y_p != d3p4y) FreeDoubles(d3p4y);
   if (d3p4x_p != d3p4x) FreeDoubles(d3p4x);
   if (d2p4z_p != d2p4z) FreeDoubles(d2p4z);
   if (d2p4y_p != d2p4y) FreeDoubles(d2p4y);
   if (d2p4x_p != d2p4x) FreeDoubles(d2p4x);
   if (d1p4z_p != d1p4z) FreeDoubles(d1p4z);
   if (d1p4y_p != d1p4y) FreeDoubles(d1p4y);
   if (d1p4x_p != d1p4x) FreeDoubles(d1p4x);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient3d_indirect_IIIE_bigfloat(p1, p2, p3, p4x, p4y, p4z);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (d3_p != d3) FreeDoubles(d3);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient3d_indirect_IIIE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double p4x, double p4y, double p4z)
{
   int ret;
   ret = orient3d_indirect_IIIE_interval(p1, p2, p3, p4x, p4y, p4z);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient3d_indirect_IIIE_exact(p1, p2, p3, p4x, p4y, p4z);
}

inline int orient3d_indirect_IIII_interval(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
   interval_number l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3, l4x, l4y, l4z, d4;
   if (
   !p1.getIntervalLambda(l1x, l1y, l1z, d1)
   || !p2.getIntervalLambda(l2x, l2y, l2z, d2)
   || !p3.getIntervalLambda(l3x, l3y, l3z, d3)
   || !p4.getIntervalLambda(l4x, l4y, l4z, d4)
   ) return Filtered_Sign::UNCERTAIN;

   setFPUModeToRoundUP();
   const interval_number d1p4x(d1 * l4x);
   const interval_number d1p4y(d1 * l4y);
   const interval_number d1p4z(d1 * l4z);
   const interval_number d2p4x(d2 * l4x);
   const interval_number d2p4y(d2 * l4y);
   const interval_number d2p4z(d2 * l4z);
   const interval_number d3p4x(d3 * l4x);
   const interval_number d3p4y(d3 * l4y);
   const interval_number d3p4z(d3 * l4z);
   const interval_number d4l1x(d4 * l1x);
   const interval_number d4l1y(d4 * l1y);
   const interval_number d4l1z(d4 * l1z);
   const interval_number d4l2x(d4 * l2x);
   const interval_number d4l2y(d4 * l2y);
   const interval_number d4l2z(d4 * l2z);
   const interval_number d4l3x(d4 * l3x);
   const interval_number d4l3y(d4 * l3y);
   const interval_number d4l3z(d4 * l3z);
   const interval_number p1p4x(d4l1x - d1p4x);
   const interval_number p1p4y(d4l1y - d1p4y);
   const interval_number p1p4z(d4l1z - d1p4z);
   const interval_number p2p4x(d4l2x - d2p4x);
   const interval_number p2p4y(d4l2y - d2p4y);
   const interval_number p2p4z(d4l2z - d2p4z);
   const interval_number p3p4x(d4l3x - d3p4x);
   const interval_number p3p4y(d4l3y - d3p4y);
   const interval_number p3p4z(d4l3z - d3p4z);
   const interval_number tmc_a(p1p4x * p2p4y);
   const interval_number tmc_b(p1p4y * p2p4x);
   const interval_number m01(tmc_a - tmc_b);
   const interval_number tmi_a(p1p4x * p2p4z);
   const interval_number tmi_b(p1p4z * p2p4x);
   const interval_number m02(tmi_a - tmi_b);
   const interval_number tma_a(p1p4y * p2p4z);
   const interval_number tma_b(p1p4z * p2p4y);
   const interval_number m12(tma_a - tma_b);
   const interval_number mt1(m01 * p3p4z);
   const interval_number mt2(m02 * p3p4y);
   const interval_number mt3(m12 * p3p4x);
   const interval_number mtt(mt2 - mt1);
   const interval_number m012(mtt - mt3);
   setFPUModeToRoundNEAR();

   if (!m012.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return m012.sign();
}

inline int orient3d_indirect_IIII_bigfloat(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
   bigfloat l1x, l1y, l1z, d1, l2x, l2y, l2z, d2, l3x, l3y, l3z, d3, l4x, l4y, l4z, d4;
   p1.getBigfloatLambda(l1x, l1y, l1z, d1);
   p2.getBigfloatLambda(l2x, l2y, l2z, d2);
   p3.getBigfloatLambda(l3x, l3y, l3z, d3);
   p4.getBigfloatLambda(l4x, l4y, l4z, d4);
   const bigfloat d1p4x(d1 * l4x);
   const bigfloat d1p4y(d1 * l4y);
   const bigfloat d1p4z(d1 * l4z);
   const bigfloat d2p4x(d2 * l4x);
   const bigfloat d2p4y(d2 * l4y);
   const bigfloat d2p4z(d2 * l4z);
   const bigfloat d3p4x(d3 * l4x);
   const bigfloat d3p4y(d3 * l4y);
   const bigfloat d3p4z(d3 * l4z);
   const bigfloat d4l1x(d4 * l1x);
   const bigfloat d4l1y(d4 * l1y);
   const bigfloat d4l1z(d4 * l1z);
   const bigfloat d4l2x(d4 * l2x);
   const bigfloat d4l2y(d4 * l2y);
   const bigfloat d4l2z(d4 * l2z);
   const bigfloat d4l3x(d4 * l3x);
   const bigfloat d4l3y(d4 * l3y);
   const bigfloat d4l3z(d4 * l3z);
   const bigfloat p1p4x(d4l1x - d1p4x);
   const bigfloat p1p4y(d4l1y - d1p4y);
   const bigfloat p1p4z(d4l1z - d1p4z);
   const bigfloat p2p4x(d4l2x - d2p4x);
   const bigfloat p2p4y(d4l2y - d2p4y);
   const bigfloat p2p4z(d4l2z - d2p4z);
   const bigfloat p3p4x(d4l3x - d3p4x);
   const bigfloat p3p4y(d4l3y - d3p4y);
   const bigfloat p3p4z(d4l3z - d3p4z);
   const bigfloat tmc_a(p1p4x * p2p4y);
   const bigfloat tmc_b(p1p4y * p2p4x);
   const bigfloat m01(tmc_a - tmc_b);
   const bigfloat tmi_a(p1p4x * p2p4z);
   const bigfloat tmi_b(p1p4z * p2p4x);
   const bigfloat m02(tmi_a - tmi_b);
   const bigfloat tma_a(p1p4y * p2p4z);
   const bigfloat tma_b(p1p4z * p2p4y);
   const bigfloat m12(tma_a - tma_b);
   const bigfloat mt1(m01 * p3p4z);
   const bigfloat mt2(m02 * p3p4y);
   const bigfloat mt3(m12 * p3p4x);
   const bigfloat mtt(mt2 - mt1);
   const bigfloat m012(mtt - mt3);
   return sgn(m012);
}

inline int orient3d_indirect_IIII_exact(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
 double return_value = NAN;
#ifdef CHECK_FOR_XYZERFLOWS
   feclearexcept(FE_ALL_EXCEPT);
#endif
 double l1x_p[32], *l1x = l1x_p, l1y_p[32], *l1y = l1y_p, l1z_p[32], *l1z = l1z_p, d1_p[32], *d1 = d1_p, l2x_p[32], *l2x = l2x_p, l2y_p[32], *l2y = l2y_p, l2z_p[32], *l2z = l2z_p, d2_p[32], *d2 = d2_p, l3x_p[32], *l3x = l3x_p, l3y_p[32], *l3y = l3y_p, l3z_p[32], *l3z = l3z_p, d3_p[32], *d3 = d3_p, l4x_p[32], *l4x = l4x_p, l4y_p[32], *l4y = l4y_p, l4z_p[32], *l4z = l4z_p, d4_p[32], *d4 = d4_p;
 int l1x_len = 32, l1y_len = 32, l1z_len = 32, d1_len = 32, l2x_len = 32, l2y_len = 32, l2z_len = 32, d2_len = 32, l3x_len = 32, l3y_len = 32, l3z_len = 32, d3_len = 32, l4x_len = 32, l4y_len = 32, l4z_len = 32, d4_len = 32;
 p1.getExactLambda(&l1x, l1x_len, &l1y, l1y_len, &l1z, l1z_len, &d1, d1_len);
 p2.getExactLambda(&l2x, l2x_len, &l2y, l2y_len, &l2z, l2z_len, &d2, d2_len);
 p3.getExactLambda(&l3x, l3x_len, &l3y, l3y_len, &l3z, l3z_len, &d3, d3_len);
 p4.getExactLambda(&l4x, l4x_len, &l4y, l4y_len, &l4z, l4z_len, &d4, d4_len);
 if ((d1[d1_len - 1] != 0) && (d2[d2_len - 1] != 0) && (d3[d3_len - 1] != 0) && (d4[d4_len - 1] != 0))
 {
   
   double d1p4x_p[32], *d1p4x = d1p4x_p;
   int d1p4x_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l4x_len, l4x, &d1p4x, 32);
   double d1p4y_p[32], *d1p4y = d1p4y_p;
   int d1p4y_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l4y_len, l4y, &d1p4y, 32);
   double d1p4z_p[32], *d1p4z = d1p4z_p;
   int d1p4z_len = expansionObject::Gen_Product_With_PreAlloc(d1_len, d1, l4z_len, l4z, &d1p4z, 32);
   double d2p4x_p[32], *d2p4x = d2p4x_p;
   int d2p4x_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l4x_len, l4x, &d2p4x, 32);
   double d2p4y_p[32], *d2p4y = d2p4y_p;
   int d2p4y_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l4y_len, l4y, &d2p4y, 32);
   double d2p4z_p[32], *d2p4z = d2p4z_p;
   int d2p4z_len = expansionObject::Gen_Product_With_PreAlloc(d2_len, d2, l4z_len, l4z, &d2p4z, 32);
   double d3p4x_p[32], *d3p4x = d3p4x_p;
   int d3p4x_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l4x_len, l4x, &d3p4x, 32);
   double d3p4y_p[32], *d3p4y = d3p4y_p;
   int d3p4y_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l4y_len, l4y, &d3p4y, 32);
   double d3p4z_p[32], *d3p4z = d3p4z_p;
   int d3p4z_len = expansionObject::Gen_Product_With_PreAlloc(d3_len, d3, l4z_len, l4z, &d3p4z, 32);
   double d4l1x_p[32], *d4l1x = d4l1x_p;
   int d4l1x_len = expansionObject::Gen_Product_With_PreAlloc(d4_len, d4, l1x_len, l1x, &d4l1x, 32);
   double d4l1y_p[32], *d4l1y = d4l1y_p;
   int d4l1y_len = expansionObject::Gen_Product_With_PreAlloc(d4_len, d4, l1y_len, l1y, &d4l1y, 32);
   double d4l1z_p[32], *d4l1z = d4l1z_p;
   int d4l1z_len = expansionObject::Gen_Product_With_PreAlloc(d4_len, d4, l1z_len, l1z, &d4l1z, 32);
   double d4l2x_p[32], *d4l2x = d4l2x_p;
   int d4l2x_len = expansionObject::Gen_Product_With_PreAlloc(d4_len, d4, l2x_len, l2x, &d4l2x, 32);
   double d4l2y_p[32], *d4l2y = d4l2y_p;
   int d4l2y_len = expansionObject::Gen_Product_With_PreAlloc(d4_len, d4, l2y_len, l2y, &d4l2y, 32);
   double d4l2z_p[32], *d4l2z = d4l2z_p;
   int d4l2z_len = expansionObject::Gen_Product_With_PreAlloc(d4_len, d4, l2z_len, l2z, &d4l2z, 32);
   double d4l3x_p[32], *d4l3x = d4l3x_p;
   int d4l3x_len = expansionObject::Gen_Product_With_PreAlloc(d4_len, d4, l3x_len, l3x, &d4l3x, 32);
   double d4l3y_p[32], *d4l3y = d4l3y_p;
   int d4l3y_len = expansionObject::Gen_Product_With_PreAlloc(d4_len, d4, l3y_len, l3y, &d4l3y, 32);
   double d4l3z_p[32], *d4l3z = d4l3z_p;
   int d4l3z_len = expansionObject::Gen_Product_With_PreAlloc(d4_len, d4, l3z_len, l3z, &d4l3z, 32);
   double p1p4x_p[32], *p1p4x = p1p4x_p;
   int p1p4x_len = expansionObject::Gen_Diff_With_PreAlloc(d4l1x_len, d4l1x, d1p4x_len, d1p4x, &p1p4x, 32);
   double p1p4y_p[32], *p1p4y = p1p4y_p;
   int p1p4y_len = expansionObject::Gen_Diff_With_PreAlloc(d4l1y_len, d4l1y, d1p4y_len, d1p4y, &p1p4y, 32);
   double p1p4z_p[32], *p1p4z = p1p4z_p;
   int p1p4z_len = expansionObject::Gen_Diff_With_PreAlloc(d4l1z_len, d4l1z, d1p4z_len, d1p4z, &p1p4z, 32);
   double p2p4x_p[32], *p2p4x = p2p4x_p;
   int p2p4x_len = expansionObject::Gen_Diff_With_PreAlloc(d4l2x_len, d4l2x, d2p4x_len, d2p4x, &p2p4x, 32);
   double p2p4y_p[32], *p2p4y = p2p4y_p;
   int p2p4y_len = expansionObject::Gen_Diff_With_PreAlloc(d4l2y_len, d4l2y, d2p4y_len, d2p4y, &p2p4y, 32);
   double p2p4z_p[32], *p2p4z = p2p4z_p;
   int p2p4z_len = expansionObject::Gen_Diff_With_PreAlloc(d4l2z_len, d4l2z, d2p4z_len, d2p4z, &p2p4z, 32);
   double p3p4x_p[32], *p3p4x = p3p4x_p;
   int p3p4x_len = expansionObject::Gen_Diff_With_PreAlloc(d4l3x_len, d4l3x, d3p4x_len, d3p4x, &p3p4x, 32);
   double p3p4y_p[32], *p3p4y = p3p4y_p;
   int p3p4y_len = expansionObject::Gen_Diff_With_PreAlloc(d4l3y_len, d4l3y, d3p4y_len, d3p4y, &p3p4y, 32);
   double p3p4z_p[32], *p3p4z = p3p4z_p;
   int p3p4z_len = expansionObject::Gen_Diff_With_PreAlloc(d4l3z_len, d4l3z, d3p4z_len, d3p4z, &p3p4z, 32);
   double tmc_a_p[32], *tmc_a = tmc_a_p;
   int tmc_a_len = expansionObject::Gen_Product_With_PreAlloc(p1p4x_len, p1p4x, p2p4y_len, p2p4y, &tmc_a, 32);
   double tmc_b_p[32], *tmc_b = tmc_b_p;
   int tmc_b_len = expansionObject::Gen_Product_With_PreAlloc(p1p4y_len, p1p4y, p2p4x_len, p2p4x, &tmc_b, 32);
   double m01_p[32], *m01 = m01_p;
   int m01_len = expansionObject::Gen_Diff_With_PreAlloc(tmc_a_len, tmc_a, tmc_b_len, tmc_b, &m01, 32);
   double tmi_a_p[32], *tmi_a = tmi_a_p;
   int tmi_a_len = expansionObject::Gen_Product_With_PreAlloc(p1p4x_len, p1p4x, p2p4z_len, p2p4z, &tmi_a, 32);
   double tmi_b_p[32], *tmi_b = tmi_b_p;
   int tmi_b_len = expansionObject::Gen_Product_With_PreAlloc(p1p4z_len, p1p4z, p2p4x_len, p2p4x, &tmi_b, 32);
   double m02_p[32], *m02 = m02_p;
   int m02_len = expansionObject::Gen_Diff_With_PreAlloc(tmi_a_len, tmi_a, tmi_b_len, tmi_b, &m02, 32);
   double tma_a_p[32], *tma_a = tma_a_p;
   int tma_a_len = expansionObject::Gen_Product_With_PreAlloc(p1p4y_len, p1p4y, p2p4z_len, p2p4z, &tma_a, 32);
   double tma_b_p[32], *tma_b = tma_b_p;
   int tma_b_len = expansionObject::Gen_Product_With_PreAlloc(p1p4z_len, p1p4z, p2p4y_len, p2p4y, &tma_b, 32);
   double m12_p[32], *m12 = m12_p;
   int m12_len = expansionObject::Gen_Diff_With_PreAlloc(tma_a_len, tma_a, tma_b_len, tma_b, &m12, 32);
   double mt1_p[32], *mt1 = mt1_p;
   int mt1_len = expansionObject::Gen_Product_With_PreAlloc(m01_len, m01, p3p4z_len, p3p4z, &mt1, 32);
   double mt2_p[32], *mt2 = mt2_p;
   int mt2_len = expansionObject::Gen_Product_With_PreAlloc(m02_len, m02, p3p4y_len, p3p4y, &mt2, 32);
   double mt3_p[32], *mt3 = mt3_p;
   int mt3_len = expansionObject::Gen_Product_With_PreAlloc(m12_len, m12, p3p4x_len, p3p4x, &mt3, 32);
   double mtt_p[32], *mtt = mtt_p;
   int mtt_len = expansionObject::Gen_Diff_With_PreAlloc(mt2_len, mt2, mt1_len, mt1, &mtt, 32);
   double m012_p[32], *m012 = m012_p;
   int m012_len = expansionObject::Gen_Diff_With_PreAlloc(mtt_len, mtt, mt3_len, mt3, &m012, 32);

   return_value = m012[m012_len - 1];
   if (m012_p != m012) FreeDoubles(m012);
   if (mtt_p != mtt) FreeDoubles(mtt);
   if (mt3_p != mt3) FreeDoubles(mt3);
   if (mt2_p != mt2) FreeDoubles(mt2);
   if (mt1_p != mt1) FreeDoubles(mt1);
   if (m12_p != m12) FreeDoubles(m12);
   if (tma_b_p != tma_b) FreeDoubles(tma_b);
   if (tma_a_p != tma_a) FreeDoubles(tma_a);
   if (m02_p != m02) FreeDoubles(m02);
   if (tmi_b_p != tmi_b) FreeDoubles(tmi_b);
   if (tmi_a_p != tmi_a) FreeDoubles(tmi_a);
   if (m01_p != m01) FreeDoubles(m01);
   if (tmc_b_p != tmc_b) FreeDoubles(tmc_b);
   if (tmc_a_p != tmc_a) FreeDoubles(tmc_a);
   if (p3p4z_p != p3p4z) FreeDoubles(p3p4z);
   if (p3p4y_p != p3p4y) FreeDoubles(p3p4y);
   if (p3p4x_p != p3p4x) FreeDoubles(p3p4x);
   if (p2p4z_p != p2p4z) FreeDoubles(p2p4z);
   if (p2p4y_p != p2p4y) FreeDoubles(p2p4y);
   if (p2p4x_p != p2p4x) FreeDoubles(p2p4x);
   if (p1p4z_p != p1p4z) FreeDoubles(p1p4z);
   if (p1p4y_p != p1p4y) FreeDoubles(p1p4y);
   if (p1p4x_p != p1p4x) FreeDoubles(p1p4x);
   if (d4l3z_p != d4l3z) FreeDoubles(d4l3z);
   if (d4l3y_p != d4l3y) FreeDoubles(d4l3y);
   if (d4l3x_p != d4l3x) FreeDoubles(d4l3x);
   if (d4l2z_p != d4l2z) FreeDoubles(d4l2z);
   if (d4l2y_p != d4l2y) FreeDoubles(d4l2y);
   if (d4l2x_p != d4l2x) FreeDoubles(d4l2x);
   if (d4l1z_p != d4l1z) FreeDoubles(d4l1z);
   if (d4l1y_p != d4l1y) FreeDoubles(d4l1y);
   if (d4l1x_p != d4l1x) FreeDoubles(d4l1x);
   if (d3p4z_p != d3p4z) FreeDoubles(d3p4z);
   if (d3p4y_p != d3p4y) FreeDoubles(d3p4y);
   if (d3p4x_p != d3p4x) FreeDoubles(d3p4x);
   if (d2p4z_p != d2p4z) FreeDoubles(d2p4z);
   if (d2p4y_p != d2p4y) FreeDoubles(d2p4y);
   if (d2p4x_p != d2p4x) FreeDoubles(d2p4x);
   if (d1p4z_p != d1p4z) FreeDoubles(d1p4z);
   if (d1p4y_p != d1p4y) FreeDoubles(d1p4y);
   if (d1p4x_p != d1p4x) FreeDoubles(d1p4x);
 }

#ifdef CHECK_FOR_XYZERFLOWS
   if (fetestexcept(FE_UNDERFLOW | FE_OVERFLOW)) return_value = orient3d_indirect_IIII_bigfloat(p1, p2, p3, p4);
#endif

 if (l1x_p != l1x) FreeDoubles(l1x);
 if (l1y_p != l1y) FreeDoubles(l1y);
 if (l1z_p != l1z) FreeDoubles(l1z);
 if (d1_p != d1) FreeDoubles(d1);
 if (l2x_p != l2x) FreeDoubles(l2x);
 if (l2y_p != l2y) FreeDoubles(l2y);
 if (l2z_p != l2z) FreeDoubles(l2z);
 if (d2_p != d2) FreeDoubles(d2);
 if (l3x_p != l3x) FreeDoubles(l3x);
 if (l3y_p != l3y) FreeDoubles(l3y);
 if (l3z_p != l3z) FreeDoubles(l3z);
 if (d3_p != d3) FreeDoubles(d3);
 if (l4x_p != l4x) FreeDoubles(l4x);
 if (l4y_p != l4y) FreeDoubles(l4y);
 if (l4z_p != l4z) FreeDoubles(l4z);
 if (d4_p != d4) FreeDoubles(d4);

 if (return_value > 0) return IP_Sign::POSITIVE;
 if (return_value < 0) return IP_Sign::NEGATIVE;
 if (return_value == 0) return IP_Sign::ZERO;
 return IP_Sign::UNDEFINED;
}

inline int orient3d_indirect_IIII(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4)
{
   int ret;
   ret = orient3d_indirect_IIII_interval(p1, p2, p3, p4);
   if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient3d_indirect_IIII_exact(p1, p2, p3, p4);
}

