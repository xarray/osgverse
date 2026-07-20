#include "hdbscanConstraint.hpp"

hdbscanConstraint::hdbscanConstraint(int pointA, int pointB, hdbscanConstraintType type) {
	_pointA = pointA;
	_pointB = pointB;
	_constraintType = type;
}

int hdbscanConstraint::getPointA() {
	return _pointA;
}

int hdbscanConstraint::getPointB() {
	return _pointB;
}

hdbscanConstraintType hdbscanConstraint::getConstraintType() {
	return _constraintType;
}