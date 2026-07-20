#include "hdbscanResult.hpp"

hdbscanResult::hdbscanResult() {
	;
}
hdbscanResult::hdbscanResult(vector<int> pLables, vector<outlierScore> pOutlierScores, vector<double> pmembershipProbabilities, bool pHsInfiniteStability) {
	labels = pLables;
	outliersScores = pOutlierScores;
	membershipProbabilities = pmembershipProbabilities;
	hasInfiniteStability = pHsInfiniteStability;
}