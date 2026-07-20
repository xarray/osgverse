#include "outlierScore.hpp"
#include <tuple> 

outlierScore::outlierScore() {
	;
}

outlierScore::outlierScore(double score, double coreDistance, int id) {
	outlierScore::score = score;
	outlierScore::coreDistance = coreDistance;
	outlierScore::id = id;
}

bool outlierScore::operator<(const outlierScore& other) const {
	/*
	if (score < other.score)
		return score < other.score;
	else if (coreDistance < other.coreDistance)
		return coreDistance < other.coreDistance;
	else if (id < other.id)
		return id < other.id;
	else
		return false;*/
	return std::tie(score, coreDistance, id) < std::tie(other.score, other.coreDistance, other.id);
}