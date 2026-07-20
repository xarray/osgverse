#pragma once
#include<vector>
/// <summary>
/// An interface for classes which compute the distance between two points (where points are
/// represented as arrays of doubles).
/// </summary>
class IDistanceCalculator
{
	/// <summary>
	/// Computes the distance between two points.
	/// Note that larger values indicate that the two points are farther apart.
	/// </summary>
	/// <param name="attributesOne">The attributes of the first point</param>
	/// <param name="attributesTwo">The attributes of the second point</param>
	/// <returns>A double for the distance between the two points</returns>
public:
	virtual double computeDistance(std::vector<double> attributesOne, std::vector<double> attributesTwo)=0;
};

