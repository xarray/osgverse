#pragma once

extern "C" {

	void prepareDeform(
		int* cells, const int nCells,

		double* positions, const int nPositions,

		int* roiIndices, const int nRoi,

		const int unconstrainedBegin,

		bool RSI);

	void doDeform(double* handlePositions, int nHandlePositions, double* outPositions);

	void freeDeform();
}