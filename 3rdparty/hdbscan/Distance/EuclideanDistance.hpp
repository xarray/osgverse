#pragma once
#include"IDistanceCalculator.hpp"
/// <summary>
/// Computes the euclidean distance between two points, d = sqrt((x1-y1)^2 + (x2-y2)^2 + ... + (xn-yn)^2).
/// </summary>
class EuclideanDistance : IDistanceCalculator
{
public:
	double computeDistance(std::vector<double> attributesOne, std::vector<double> attributesTwo);

};

