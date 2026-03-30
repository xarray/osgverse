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
* This program is free software; you can redistribute it and/or modify      * 
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

/* Should include incircle and insphere toexpansionObject:: */


#include "implicit_point.h"

#pragma intrinsic(fabs)


inline int orient2d_filtered(double p1x, double p1y, double p2x, double p2y, double p3x, double p3y)
{
	double dl = (p2x - p1x) * (p3y - p1y);
	double dr = (p2y - p1y) * (p3x - p1x);
	double det = dl - dr;
	double eb = 3.3306690738754706e-016 * (fabs(dl) + fabs(dr));
	return ((det >= eb) - (-det >= eb));
}

inline int orient2d_interval(interval_number p1x, interval_number p1y, interval_number p2x, interval_number p2y, interval_number p3x, interval_number p3y)
{
   setFPUModeToRoundUP();
   interval_number a11(p2x - p1x);
   interval_number a12(p2y - p1y);
   interval_number a21(p3x - p1x);
   interval_number a22(p3y - p1y);
   interval_number d1(a11 * a22);
   interval_number d2(a12 * a21);
   interval_number d(d1 - d2);
   setFPUModeToRoundNEAR();

   if (!d.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return d.sign();
}

inline int orient2d_exact(double p1x, double p1y, double p2x, double p2y, double p3x, double p3y)
{
    double acx[2], acy[2], bcx[2], bcy[2], dtl[2], dtr[2], B[4];
    double s[2], t[2], u[4], C1[8], C2[12], D[16];
    int C1l, C2l, Dl;
    

    acx[1] = (p1x - p3x);
    bcx[1] = (p2x - p3x);
    acy[1] = (p1y - p3y);
    bcy[1] = (p2y - p3y);

    expansionObject::Two_Prod(acx[1], bcy[1], dtl);
    expansionObject::Two_Prod(acy[1], bcx[1], dtr);
    expansionObject::Two_Two_Diff(dtl, dtr, B);

    double dsm = (fabs(dtl[1]) + fabs(dtr[1]));
    double det = expansionObject::To_Double(4, B);
    double eb = 2.2204460492503146e-16 * dsm;
    Dl = ((det >= eb) - (-det >= eb));
    if (Dl) return Dl;

    expansionObject::Two_Diff_Back(p1x, p3x, acx);
    expansionObject::Two_Diff_Back(p2x, p3x, bcx);
    expansionObject::Two_Diff_Back(p1y, p3y, acy);
    expansionObject::Two_Diff_Back(p2y, p3y, bcy);

    if ((acx[0] == 0.0) && (acy[0] == 0.0) && (bcx[0] == 0.0) && (bcy[0] == 0.0)) return ((det > 0) - (det < 0));

    eb = 1.1093356479670487e-31 * dsm + 3.3306690738754706e-16 * fabs(det);
    det += (acx[1] * bcy[0] + bcy[1] * acx[0]) - (acy[1] * bcx[0] + bcx[1] * acy[0]);
    Dl = ((det >= eb) - (-det >= eb));
    if (Dl) return Dl;

    expansionObject::Two_Prod(acx[0], bcy[1], s);
    expansionObject::Two_Prod(acy[0], bcx[1], t);
    expansionObject::Two_Two_Diff(s, t, u);
    C1l = expansionObject::Gen_Sum(4, B, 4, u, C1);

    expansionObject::Two_Prod(acx[1], bcy[0], s);
    expansionObject::Two_Prod(acy[1], bcx[0], t);
    expansionObject::Two_Two_Diff(s, t, u);
    C2l = expansionObject::Gen_Sum(C1l, C1, 4, u, C2);

    expansionObject::Two_Prod(acx[0], bcy[0], s);
    expansionObject::Two_Prod(acy[0], bcx[0], t);
    expansionObject::Two_Two_Diff(s, t, u);
    Dl = expansionObject::Gen_Sum(C2l, C2, 4, u, D);

    det = D[Dl - 1];
    return ((det >= eb) - (-det >= eb));
}

inline int orient2d(double p1x, double p1y, double p2x, double p2y, double p3x, double p3y)
{
   int ret = orient2d_filtered(p1x, p1y, p2x, p2y, p3x, p3y);
   if (ret) return ret;
   //ret = orient2d_interval(p1x, p1y, p2x, p2y, p3x, p3y);
   //if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient2d_exact(p1x, p1y, p2x, p2y, p3x, p3y);
}

inline int orient3d_filtered(double px, double py, double pz, double qx, double qy, double qz, double rx, double ry, double rz, double sx, double sy, double sz)
{
	double fadx, fbdx, fcdx, fady, fbdy, fcdy, fadz, fbdz, fcdz, eb;
	double fbdxcdy, fcdxbdy, fcdxady, fadxcdy, fadxbdy, fbdxady, det;

	fadx = qx - px; fbdx = rx - px; fcdx = sx - px;
	fady = qy - py; fbdy = ry - py; fcdy = sy - py;
	fadz = qz - pz; fbdz = rz - pz; fcdz = sz - pz;

	fbdxcdy = fbdx * fcdy * fadz; fcdxbdy = fcdx * fbdy * fadz;
	fcdxady = fcdx * fady * fbdz; fadxcdy = fadx * fcdy * fbdz;
	fadxbdy = fadx * fbdy * fcdz; fbdxady = fbdx * fady * fcdz;

	det = (fbdxcdy - fcdxbdy) + (fcdxady - fadxcdy) + (fadxbdy - fbdxady);
	eb = 7.7715611723761027e-016 * (fabs(fbdxcdy) + fabs(fcdxbdy) + fabs(fcdxady) + fabs(fadxcdy) + fabs(fadxbdy) + fabs(fbdxady));
	return ((det >= eb) - (-det >= eb));
}

inline int orient3d_interval(interval_number px, interval_number py, interval_number pz, interval_number qx, interval_number qy, interval_number qz, interval_number rx, interval_number ry, interval_number rz, interval_number sx, interval_number sy, interval_number sz)
{
   setFPUModeToRoundUP();
   interval_number qx_px(qx - px);
   interval_number qy_py(qy - py);
   interval_number rx_px(rx - px);
   interval_number ry_py(ry - py);
   interval_number rz_pz(rz - pz);
   interval_number qz_pz(qz - pz);
   interval_number sx_px(sx - px);
   interval_number sy_py(sy - py);
   interval_number sz_pz(sz - pz);
   interval_number tmp_a(qx_px * ry_py);
   interval_number tmp_b(qy_py * rx_px);
   interval_number m01(tmp_a - tmp_b);
   interval_number tmq_a(qx_px * rz_pz);
   interval_number tmq_b(qz_pz * rx_px);
   interval_number m02(tmq_a - tmq_b);
   interval_number tmr_a(qy_py * rz_pz);
   interval_number tmr_b(qz_pz * ry_py);
   interval_number m12(tmr_a - tmr_b);
   interval_number mt1(m01 * sz_pz);
   interval_number mt2(m02 * sy_py);
   interval_number mt3(m12 * sx_px);
   interval_number mtt(mt1 - mt2);
   interval_number m012(mtt + mt3);
   setFPUModeToRoundNEAR();

   if (!m012.signIsReliable()) return Filtered_Sign::UNCERTAIN;
   return m012.sign();
}


inline void supo3d1(
	double* c1, double* c2, double* c3, double* c4, double* c5, double* c6,
	double* a1, double* a2, double& i, double* k1, double* k2, double* k3,
	double* k4, int& l1, int& l2)
{
	if (c1[0] == 0.0) {
		if (c2[0] == 0.0) {
			a1[0] = a2[0] = 0.0;
			l1 = l2 = 1;
		}
		else {
			i = -c2[0];
			expansionObject::Two_Prod(i, c3[1], a1);
			expansionObject::Two_Prod(c2[0], c4[1], a2);
			l1 = l2 = 2;
		}
	}
	else {
		if (c2[0] == 0.0) {
			i = -c1[0];
			expansionObject::Two_Prod(c1[0], c5[1], a1);
			expansionObject::Two_Prod(i, c6[1], a2);
			l1 = l2 = 2;
		}
		else {
			expansionObject::Two_Prod(c1[0], c5[1], k1);
			expansionObject::Two_Prod(c2[0], c3[1], k2);
			expansionObject::Two_Two_Diff(k1, k2, a1);
			expansionObject::Two_Prod(c2[0], c4[1], k3);
			expansionObject::Two_Prod(c1[0], c6[1], k4);
			expansionObject::Two_Two_Diff(k3, k4, a2);
			l1 = l2 = 4;
		}
	}
}

inline void supo3d2(
	double* c1, double* c2, double* c3, double* c4, double* u,
	int& fl, double fin[2][192], int& wh,
	double* c5, double& i, double* c6, double* c7)

{
	if (c1[0] != 0.0) {
		if (c2[0] != 0.0) {
			expansionObject::Two_Prod(c1[0], c2[0], c3);
			expansionObject::Two_One_Prod(c3, c4[1], u);
			fl = expansionObject::Gen_Sum(fl, fin[wh], 4, u, fin[!wh]);
			wh = !wh;
			if (c4[0] != 0.0) {
				expansionObject::Two_One_Prod(c3, c4[0], u);
				fl = expansionObject::Gen_Sum(fl, fin[wh], 4, u, fin[!wh]);
				wh = !wh;
			}
		}
		if (c5[0] != 0.0) {
			i = -c1[0];
			expansionObject::Two_Prod(i, c5[0], c6);
			expansionObject::Two_One_Prod(c6, c7[1], u);
			fl = expansionObject::Gen_Sum(fl, fin[wh], 4, u, fin[!wh]);
			wh = !wh;
			if (c7[0] != 0.0) {
				expansionObject::Two_One_Prod(c6, c7[0], u);
				fl = expansionObject::Gen_Sum(fl, fin[wh], 4, u, fin[!wh]);
				wh = !wh;
			}
		}
	}
}

inline int orient3d_exact(double pdx, double pdy, double pdz, double pax, double pay, double paz, double pbx, double pby, double pbz, double pcx, double pcy, double pcz)
{
	double eb, det;
	double adx[2], bdx[2], cdx[2], ady[2], bdy[2], cdy[2], adz[2], bdz[2], cdz[2];
	double bdxcdy[2], cdxbdy[2], cdxady[2], adxcdy[2], adxbdy[2], bdxady[2];
	double bc[4], ca[4], ab[4];
	double bdxt_cdy[2], cdxt_bdy[2], cdxt_ady[2];
	double adxt_cdy[2], adxt_bdy[2], bdxt_ady[2];
	double bdyt_cdx[2], cdyt_bdx[2], cdyt_adx[2];
	double adyt_cdx[2], adyt_bdx[2], bdyt_adx[2];
	double bdxt_cdyt[2], cdxt_bdyt[2], cdxt_adyt[2];
	double adxt_cdyt[2], adxt_bdyt[2], bdxt_adyt[2];
	double u[4], v[12], w[16];
	double adet[8], bdet[8], cdet[8], abdet[16];
	double fin[2][192];
	int wh = 0;
	double at_b[4], at_c[4], bt_c[4], bt_a[4], ct_a[4], ct_b[4];
	double bct[8], cat[8], abt[8];
	int alen, blen, clen, finlen, vlen, wlen;
	int at_blen, at_clen, bt_clen, bt_alen, ct_alen, ct_blen;
	int bctlen, catlen, abtlen;
	int ablen;
	double inv;
	int ri;

	

	adx[1] = pax - pdx;
	bdx[1] = pbx - pdx;
	cdx[1] = pcx - pdx;
	ady[1] = pay - pdy;
	bdy[1] = pby - pdy;
	cdy[1] = pcy - pdy;
	adz[1] = paz - pdz;
	bdz[1] = pbz - pdz;
	cdz[1] = pcz - pdz;

	expansionObject::Two_Prod(bdx[1], cdy[1], bdxcdy);
	expansionObject::Two_Prod(cdx[1], bdy[1], cdxbdy);
	expansionObject::Two_Two_Diff(bdxcdy, cdxbdy, bc);
	alen = expansionObject::Gen_Scale(4, bc, adz[1], adet);

	expansionObject::Two_Prod(cdx[1], ady[1], cdxady);
	expansionObject::Two_Prod(adx[1], cdy[1], adxcdy);
	expansionObject::Two_Two_Diff(cdxady, adxcdy, ca);
	blen = expansionObject::Gen_Scale(4, ca, bdz[1], bdet);

	expansionObject::Two_Prod(adx[1], bdy[1], adxbdy);
	expansionObject::Two_Prod(bdx[1], ady[1], bdxady);
	expansionObject::Two_Two_Diff(adxbdy, bdxady, ab);
	clen = expansionObject::Gen_Scale(4, ab, cdz[1], cdet);

	ablen = expansionObject::Gen_Sum(alen, adet, blen, bdet, abdet);
	finlen = expansionObject::Gen_Sum(ablen, abdet, clen, cdet, fin[wh]);

	double xx1 = bdxcdy[1] * adz[1]; double xx2 = cdxbdy[1] * adz[1];
	double yy1 = cdxady[1] * bdz[1]; double yy2 = adxcdy[1] * bdz[1];
	double zz1 = adxbdy[1] * cdz[1]; double zz2 = bdxady[1] * cdz[1];
	double pm = fabs(xx1) + fabs(xx2) + fabs(yy1) + fabs(yy2) + fabs(zz1) + fabs(zz2);

	det = expansionObject::To_Double(finlen, fin[wh]);
	eb = 3.3306690738754731e-016 * pm;
	ri = (det >= eb) - (-det >= eb);
	if (ri) return ri;

	expansionObject::Two_Diff_Back(pax, pdx, adx);
	expansionObject::Two_Diff_Back(pbx, pdx, bdx);
	expansionObject::Two_Diff_Back(pcx, pdx, cdx);
	expansionObject::Two_Diff_Back(pay, pdy, ady);
	expansionObject::Two_Diff_Back(pby, pdy, bdy);
	expansionObject::Two_Diff_Back(pcy, pdy, cdy);
	expansionObject::Two_Diff_Back(paz, pdz, adz);
	expansionObject::Two_Diff_Back(pbz, pdz, bdz);
	expansionObject::Two_Diff_Back(pcz, pdz, cdz);

	if ((adx[0] == 0.0) && (bdx[0] == 0.0) && (cdx[0] == 0.0) &&
		(ady[0] == 0.0) && (bdy[0] == 0.0) && (cdy[0] == 0.0) &&
		(adz[0] == 0.0) && (bdz[0] == 0.0) && (cdz[0] == 0.0)) return (det > 0) - (det < 0);

	eb = 3.2047474274603644e-031 * pm + 1.1102230246251565e-016 * fabs(det);
	det += (adz[1] * ((bdx[1] * cdy[0] + cdy[1] * bdx[0])
		- (bdy[1] * cdx[0] + cdx[1] * bdy[0]))
		+ adz[0] * (bdx[1] * cdy[1] - bdy[1] * cdx[1]))
		+ (bdz[1] * ((cdx[1] * ady[0] + ady[1] * cdx[0])
			- (cdy[1] * adx[0] + adx[1] * cdy[0]))
			+ bdz[0] * (cdx[1] * ady[1] - cdy[1] * adx[1]))
		+ (cdz[1] * ((adx[1] * bdy[0] + bdy[1] * adx[0])
			- (ady[1] * bdx[0] + bdx[1] * ady[0]))
			+ cdz[0] * (adx[1] * bdy[1] - ady[1] * bdx[1]));
	ri = (det >= eb) - (-det >= eb);
	if (ri) return ri;

	// Filters did not work. Compute exactly...
	supo3d1(adx, ady, bdx, cdx, bdy, cdy, at_b, at_c, inv,
		adxt_bdy, adyt_bdx, adyt_cdx, adxt_cdy, at_blen, at_clen);

	supo3d1(bdx, bdy, cdx, adx, cdy, ady, bt_c, bt_a, inv,
		bdxt_cdy, bdyt_cdx, bdyt_adx, bdxt_ady, bt_alen, bt_clen);

	supo3d1(cdx, cdy, adx, bdx, ady, bdy, ct_a, ct_b, inv,
		cdxt_ady, cdyt_adx, cdyt_bdx, cdxt_bdy, ct_alen, ct_blen);

	bctlen = expansionObject::Gen_Sum(bt_clen, bt_c, ct_blen, ct_b, bct);
	wlen = expansionObject::Gen_Scale(bctlen, bct, adz[1], w);
	finlen = expansionObject::Gen_Sum(finlen, fin[wh], wlen, w, fin[(!wh)]); wh = !wh;

	catlen = expansionObject::Gen_Sum(ct_alen, ct_a, at_clen, at_c, cat);
	wlen = expansionObject::Gen_Scale(catlen, cat, bdz[1], w);
	finlen = expansionObject::Gen_Sum(finlen, fin[wh], wlen, w, fin[(!wh)]); wh = !wh;

	abtlen = expansionObject::Gen_Sum(at_blen, at_b, bt_alen, bt_a, abt);
	wlen = expansionObject::Gen_Scale(abtlen, abt, cdz[1], w);
	finlen = expansionObject::Gen_Sum(finlen, fin[wh], wlen, w, fin[(!wh)]); wh = !wh;

	if (adz[0] != 0.0) {
		vlen = expansionObject::Gen_Scale(4, bc, adz[0], v);
		finlen = expansionObject::Gen_Sum(finlen, fin[wh], vlen, v, fin[(!wh)]); wh = !wh;
	}
	if (bdz[0] != 0.0) {
		vlen = expansionObject::Gen_Scale(4, ca, bdz[0], v);
		finlen = expansionObject::Gen_Sum(finlen, fin[wh], vlen, v, fin[(!wh)]); wh = !wh;
	}
	if (cdz[0] != 0.0) {
		vlen = expansionObject::Gen_Scale(4, ab, cdz[0], v);
		finlen = expansionObject::Gen_Sum(finlen, fin[wh], vlen, v, fin[(!wh)]); wh = !wh;
	}

	supo3d2(adx, bdy, adxt_bdyt, cdz, u, finlen, fin, wh, cdy, inv, adxt_cdyt, bdz);
	supo3d2(bdx, cdy, bdxt_cdyt, adz, u, finlen, fin, wh, ady, inv, bdxt_adyt, cdz);
	supo3d2(cdx, ady, cdxt_adyt, bdz, u, finlen, fin, wh, bdy, inv, cdxt_bdyt, adz);

	if (adz[0] != 0.0) {
		wlen = expansionObject::Gen_Scale(bctlen, bct, adz[0], w);
		finlen = expansionObject::Gen_Sum(finlen, fin[wh], wlen, w, fin[!wh]); wh = !wh;
	}
	if (bdz[0] != 0.0) {
		wlen = expansionObject::Gen_Scale(catlen, cat, bdz[0], w);
		finlen = expansionObject::Gen_Sum(finlen, fin[wh], wlen, w, fin[!wh]); wh = !wh;
	}
	if (cdz[0] != 0.0) {
		wlen = expansionObject::Gen_Scale(abtlen, abt, cdz[0], w);
		finlen = expansionObject::Gen_Sum(finlen, fin[wh], wlen, w, fin[!wh]);	wh = !wh;
	}

	det = fin[wh][finlen - 1];
	return (det > 0) - (det < 0);
}

inline int orient3d(double px, double py, double pz, double qx, double qy, double qz, double rx, double ry, double rz, double sx, double sy, double sz)
{
   int ret;
   ret = orient3d_filtered(px, py, pz, qx, qy, qz, rx, ry, rz, sx, sy, sz);
   if (ret) return ret;
   //ret = orient3d_interval(px, py, pz, qx, qy, qz, rx, ry, rz, sx, sy, sz);
   //if (ret != Filtered_Sign::UNCERTAIN) return ret;
   return orient3d_exact(px, py, pz, qx, qy, qz, rx, ry, rz, sx, sy, sz);
}
