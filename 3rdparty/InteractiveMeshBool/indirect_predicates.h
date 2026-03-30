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

#pragma once

/* This code was generated automatically. Do not edit unless you exactly   */
/* know what you are doing!                                                */

#include "implicit_point.h"

int dotProductSign2D(double px, double py, double rx, double ry, double qx, double qy);
int dotProductSign3D(double px, double py, double pz, double rx, double ry, double rz, double qx, double qy, double qz);
int incircle(double pax, double pay, double pbx, double pby, double pcx, double pcy, double pdx, double pdy);
int inSphere(double pax, double pay, double paz, double pbx, double pby, double pbz, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez);
int dotProductSign2D_EEI(const genericPoint& q, double px, double py, double rx, double ry);
int dotProductSign2D_IEE(const genericPoint& p, double rx, double ry, double qx, double qy);
int dotProductSign2D_IEI(const genericPoint& p, const genericPoint& q, double rx, double ry);
int dotProductSign2D_IIE(const genericPoint& p, const genericPoint& r, double qx, double qy);
int dotProductSign2D_III(const genericPoint& p, const genericPoint& r, const genericPoint& q);
int dotProductSign3D_EEI(const genericPoint& q, double px, double py, double pz, double rx, double ry, double rz);
int dotProductSign3D_IEE(const genericPoint& p, double rx, double ry, double rz, double qx, double qy, double qz);
int dotProductSign3D_IEI(const genericPoint& p, const genericPoint& q, double rx, double ry, double rz);
int dotProductSign3D_IIE(const genericPoint& p, const genericPoint& r, double qx, double qy, double qz);
int dotProductSign3D_III(const genericPoint& p, const genericPoint& r, const genericPoint& q);
int incirclexy_indirect_IEEE(const genericPoint& p1, double pbx, double pby, double pcx, double pcy, double pdx, double pdy);
int incirclexy_indirect_IIEE(const genericPoint& p1, const genericPoint& p2, double pcx, double pcy, double pdx, double pdy);
int incirclexy_indirect_IIIE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double pdx, double pdy);
int incirclexy_indirect_IIII(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4);
int incircle_indirect_IEEE(const genericPoint& p1, double pbx, double pby, double pcx, double pcy, double pdx, double pdy);
int incircle_indirect_IIEE(const genericPoint& p1, const genericPoint& p2, double pcx, double pcy, double pdx, double pdy);
int incircle_indirect_IIIE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double pdx, double pdy);
int incircle_indirect_IIII(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4);
int inSphere_IEEEE(const genericPoint& p1, double pbx, double pby, double pbz, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez);
int inSphere_IIEEE(const genericPoint& p1, const genericPoint& p2, double pcx, double pcy, double pcz, double pdx, double pdy, double pdz, double pex, double pey, double pez);
int inSphere_IIIEE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double pdx, double pdy, double pdz, double pex, double pey, double pez);
int inSphere_IIIIE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, double pex, double pey, double pez);
int inSphere_IIIII(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4, const genericPoint& p5);
bool lambda2d_SSI_interval(interval_number ea1x, interval_number ea1y, interval_number ea2x, interval_number ea2y, interval_number eb1x, interval_number eb1y, interval_number eb2x, interval_number eb2y, interval_number& lambda_x, interval_number& lambda_y, interval_number& lambda_det);
void lambda2d_SSI_exact(double ea1x, double ea1y, double ea2x, double ea2y, double eb1x, double eb1y, double eb2x, double eb2y, double **lambda_x, int& lambda_x_len, double **lambda_y, int& lambda_y_len, double **lambda_det, int& lambda_det_len);
void lambda2d_SSI_bigfloat(bigfloat ea1x, bigfloat ea1y, bigfloat ea2x, bigfloat ea2y, bigfloat eb1x, bigfloat eb1y, bigfloat eb2x, bigfloat eb2y, bigfloat& lambda_x, bigfloat& lambda_y, bigfloat& lambda_det);
bool lambda3d_LNC_interval(interval_number px, interval_number py, interval_number pz, interval_number qx, interval_number qy, interval_number qz, interval_number t, interval_number& lambda_x, interval_number& lambda_y, interval_number& lambda_z, interval_number& lambda_d);
void lambda3d_LNC_exact(double px, double py, double pz, double qx, double qy, double qz, double t, double **lambda_x, int& lambda_x_len, double **lambda_y, int& lambda_y_len, double **lambda_z, int& lambda_z_len, double **lambda_d, int& lambda_d_len);
void lambda3d_LNC_bigfloat(bigfloat px, bigfloat py, bigfloat pz, bigfloat qx, bigfloat qy, bigfloat qz, bigfloat t, bigfloat& lambda_x, bigfloat& lambda_y, bigfloat& lambda_z, bigfloat& lambda_d);
bool lambda3d_LPI_interval(interval_number px, interval_number py, interval_number pz, interval_number qx, interval_number qy, interval_number qz, interval_number rx, interval_number ry, interval_number rz, interval_number sx, interval_number sy, interval_number sz, interval_number tx, interval_number ty, interval_number tz, interval_number& lambda_x, interval_number& lambda_y, interval_number& lambda_z, interval_number& lambda_d);
void lambda3d_LPI_exact(double px, double py, double pz, double qx, double qy, double qz, double rx, double ry, double rz, double sx, double sy, double sz, double tx, double ty, double tz, double **lambda_x, int& lambda_x_len, double **lambda_y, int& lambda_y_len, double **lambda_z, int& lambda_z_len, double **lambda_d, int& lambda_d_len);
void lambda3d_LPI_bigfloat(bigfloat px, bigfloat py, bigfloat pz, bigfloat qx, bigfloat qy, bigfloat qz, bigfloat rx, bigfloat ry, bigfloat rz, bigfloat sx, bigfloat sy, bigfloat sz, bigfloat tx, bigfloat ty, bigfloat tz, bigfloat& lambda_x, bigfloat& lambda_y, bigfloat& lambda_z, bigfloat& lambda_d);
bool lambda3d_TPI_interval(interval_number ov1x, interval_number ov1y, interval_number ov1z, interval_number ov2x, interval_number ov2y, interval_number ov2z, interval_number ov3x, interval_number ov3y, interval_number ov3z, interval_number ow1x, interval_number ow1y, interval_number ow1z, interval_number ow2x, interval_number ow2y, interval_number ow2z, interval_number ow3x, interval_number ow3y, interval_number ow3z, interval_number ou1x, interval_number ou1y, interval_number ou1z, interval_number ou2x, interval_number ou2y, interval_number ou2z, interval_number ou3x, interval_number ou3y, interval_number ou3z, interval_number& lambda_x, interval_number& lambda_y, interval_number& lambda_z, interval_number& lambda_d);
void lambda3d_TPI_exact(double ov1x, double ov1y, double ov1z, double ov2x, double ov2y, double ov2z, double ov3x, double ov3y, double ov3z, double ow1x, double ow1y, double ow1z, double ow2x, double ow2y, double ow2z, double ow3x, double ow3y, double ow3z, double ou1x, double ou1y, double ou1z, double ou2x, double ou2y, double ou2z, double ou3x, double ou3y, double ou3z, double **lambda_x, int& lambda_x_len, double **lambda_y, int& lambda_y_len, double **lambda_z, int& lambda_z_len, double **lambda_d, int& lambda_d_len);
void lambda3d_TPI_bigfloat(bigfloat ov1x, bigfloat ov1y, bigfloat ov1z, bigfloat ov2x, bigfloat ov2y, bigfloat ov2z, bigfloat ov3x, bigfloat ov3y, bigfloat ov3z, bigfloat ow1x, bigfloat ow1y, bigfloat ow1z, bigfloat ow2x, bigfloat ow2y, bigfloat ow2z, bigfloat ow3x, bigfloat ow3y, bigfloat ow3z, bigfloat ou1x, bigfloat ou1y, bigfloat ou1z, bigfloat ou2x, bigfloat ou2y, bigfloat ou2z, bigfloat ou3x, bigfloat ou3y, bigfloat ou3z, bigfloat& lambda_x, bigfloat& lambda_y, bigfloat& lambda_z, bigfloat& lambda_d);
int lessThanOnX_IE(const genericPoint& p1, double bx);
int lessThanOnX_II(const genericPoint& p1, const genericPoint& p2);
int lessThanOnY_IE(const genericPoint& p1, double by);
int lessThanOnY_II(const genericPoint& p1, const genericPoint& p2);
int lessThanOnZ_IE(const genericPoint& p1, double bz);
int lessThanOnZ_II(const genericPoint& p1, const genericPoint& p2);
int orient2dxy_indirect_IEE(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y);
int orient2dxy_indirect_IIE(const genericPoint& p1, const genericPoint& p2, double op3x, double op3y);
int orient2dxy_indirect_III(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3);
int orient2dyz_indirect_IEE(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y);
int orient2dyz_indirect_IIE(const genericPoint& p1, const genericPoint& p2, double op3x, double op3y);
int orient2dyz_indirect_III(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3);
int orient2dzx_indirect_IEE(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y);
int orient2dzx_indirect_IIE(const genericPoint& p1, const genericPoint& p2, double op3x, double op3y);
int orient2dzx_indirect_III(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3);
int orient2d_indirect_IEE(const genericPoint& p1, double p2x, double p2y, double p3x, double p3y);
int orient2d_indirect_IIE(const genericPoint& p1, const genericPoint& p2, double p3x, double p3y);
int orient2d_indirect_III(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3);
int orient3d_indirect_IEEE(const genericPoint& p1, double ax, double ay, double az, double bx, double by, double bz, double cx, double cy, double cz);
int orient3d_indirect_IIEE(const genericPoint& p1, const genericPoint& p2, double p3x, double p3y, double p3z, double p4x, double p4y, double p4z);
int orient3d_indirect_IIIE(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, double p4x, double p4y, double p4z);
int orient3d_indirect_IIII(const genericPoint& p1, const genericPoint& p2, const genericPoint& p3, const genericPoint& p4);

#include "indirect_predicates.hpp" 
