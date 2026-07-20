#pragma once
enum hdbscanConstraintType{mustLink, cannotLink};
/// <summary>
/// A clustering constraint (either a must-link or cannot-link constraint between two points).
/// </summary>
class hdbscanConstraint
{
private : 
	hdbscanConstraintType _constraintType;
	int _pointA;
	int _pointB;
/// <summary>
/// Creates a new constraint.
/// </summary>
/// <param name="pointA">The first point involved in the constraint</param>
/// <param name="pointB">The second point involved in the constraint</param>
/// <param name="type">The constraint type</param>
public:
	hdbscanConstraint(int pointA, int pointB, hdbscanConstraintType type);

	int getPointA();

	int getPointB();

	hdbscanConstraintType getConstraintType();

};

