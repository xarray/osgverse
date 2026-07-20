#pragma once
#include <set>
#include <vector>
#include <limits>
#include <stdexcept>

class cluster
{
private:
	int _id;
	double _birthLevel;
	double _deathLevel;
	int _numPoints;
	double _propagatedStability;
	int _numConstraintsSatisfied;
	int _propagatedNumConstraintsSatisfied;
	std::set<int> _virtualChildCluster;
	static int counter;

public:
	std::vector<cluster*> PropagatedDescendants;
	double PropagatedLowestChildDeathLevel;
	cluster* Parent;
	double Stability;
	bool HasChildren;
	int Label;
	int HierarchyPosition;   //First level where points with this cluster's label appear

	cluster();

	cluster(int label, cluster *parent, double birthLevel, int numPoints);
	bool operator==(const cluster& other) const;
	void detachPoints(int numPoints, double level);
	void propagate();
	void addPointsToVirtualChildCluster(std::set<int> points);
	
	bool virtualChildClusterConstraintsPoint(int point);

	void addVirtualChildConstraintsSatisfied(int numConstraints);
	

	void addConstraintsSatisfied(int numConstraints);


	void releaseVirtualChildCluster();

	int getClusterId();

};
