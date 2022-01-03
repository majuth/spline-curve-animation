#pragma once

#define M_PI 3.14159265358979323846

// The profile curve is a subdivision curve. Use the points on this curve to construct surface of revolution
#define MAXCIRCLEPOINTS 50
#define MAXCONTROLPOINTS 16
#define MINCONTROLPOINTS 3
#define MAXSTEPS 10

typedef struct SubdivisionCurve
{
	int numControlPoints;
	Vector2D controlPoints[MAXCONTROLPOINTS];
	int subdivisionSteps;
	int numCurvePoints;
	Vector2D* curvePoints;
} SubivisionCurve;

// Used to draw control points of profile curve
typedef struct Circle
{
	Vector2D circleCenter;
	Vector2D circlePoints[MAXCIRCLEPOINTS];
} Circle;

Vector2D getPoint(Vector2D* currentPoints, int numCurrentPoints, int j)
{
	int n = numCurrentPoints;
	if (j >= 0 && j < n) return currentPoints[j];

	Vector2D p0, p1;
	// boundary cases
	if (j == -1)
	{
		p0 = currentPoints[0];
		p1 = currentPoints[1];
	}
	if (j == n)
	{
		p1 = currentPoints[n - 2];
		p0 = currentPoints[n - 1];
	}

	Vector2D linearCombo;
	linearCombo.x = 2 * p0.x + -1 * p1.x;
	linearCombo.y = 2 * p0.y + -1 * p1.y;
	return linearCombo;
}

Vector2D subdivide(Vector2D v1, Vector2D v2, Vector2D v3, Vector2D v4)
{
	Vector2D result;
	result.x = (9.0 * (v2.x + v3.x) - v1.x - v4.x) / 16.0;
	result.y = (9.0 * (v2.y + v3.y) - v1.y - v4.y) / 16.0;
	return result;
}

void computeSubdivisionCurve(SubdivisionCurve* subcurvePointer)
{
	// make an array of curvepoints, with MAXSTEPS slots
	Vector2D* curvePoints[MAXSTEPS];
	// we're only going to look at the control points when subdividing
	int numCurrentPoints = subcurvePointer->numControlPoints;
	int n;
	// get a convenient pointer to the control points
	Vector2D* cur = subcurvePointer->controlPoints;
	// this will hold a subdivision
	Vector2D* sub;

	if (subcurvePointer->subdivisionSteps > MAXSTEPS) return;

	// reset numCurvePoints to the lower bound
	subcurvePointer->numCurvePoints = subcurvePointer->numControlPoints;

	// 
	curvePoints[0] = subcurvePointer->controlPoints;

	// find out the actual number of curve points we're going to make, and allocate the appropriate amount of memory
	for (int i = 1; i <= subcurvePointer->subdivisionSteps; i++)
	{
		subcurvePointer->numCurvePoints += subcurvePointer->numCurvePoints - 1;
		curvePoints[i] = (Vector2D*)malloc(subcurvePointer->numCurvePoints * sizeof(Vector2D));
	}

	// get going!
	sub = curvePoints[1];
	for (int i = 0; i < subcurvePointer->subdivisionSteps; i++)
	{
		n = numCurrentPoints - 1;
		for (int j = 0; j < n; j++)
		{
			sub[2 * j] = cur[j];
			sub[2 * j + 1] = subdivide(getPoint(cur, numCurrentPoints, j - 1),
				getPoint(cur, numCurrentPoints, j),
				getPoint(cur, numCurrentPoints, j + 1),
				getPoint(cur, numCurrentPoints, j + 2));
		}
		sub[2 * n] = cur[n];

		cur = sub;
		sub = curvePoints[i + 2];
		numCurrentPoints += n;
	}
	free(subcurvePointer->curvePoints);
	subcurvePointer->curvePoints = curvePoints[subcurvePointer->subdivisionSteps];

}

void computeRunningDistances(SubdivisionCurve* subcurvePointer) {
	// run through and give each u a temp value based on linear distance from its (left)
	GLdouble runningDistance = 0;
	subcurvePointer->curvePoints[0].u = 0;
	int numPoints = subcurvePointer->numCurvePoints;
	int i;
	for (i = 1; i < numPoints; i++) {
		GLdouble distToX = subcurvePointer->curvePoints[i].x - subcurvePointer->curvePoints[i - 1].x;
		GLdouble distToY = subcurvePointer->curvePoints[i].y - subcurvePointer->curvePoints[i - 1].y;
		GLdouble euclideanDist = sqrt(distToX * distToX + distToY * distToY);
		runningDistance += euclideanDist;
		subcurvePointer->curvePoints[i].u = runningDistance;
		//printf("example u = %.2f\n", subcurvePointer->curvePoints[i].u);
	}
	// run through again and normalize
	for (i = 1; i < numPoints; i++) {
		subcurvePointer->curvePoints[i].u = subcurvePointer->curvePoints[i].u / runningDistance;
		//printf("running dist for point %d is %.2f\n", i, subcurvePointer->curvePoints[i].u);
	}
}

void computeNormalVectors(SubdivisionCurve* subcurvePointer) {
	int numPoints = subcurvePointer->numCurvePoints - 1 * (subcurvePointer->numControlPoints);
	//printf("numPoints = %d\n", numPoints);
	int i;
	GLdouble dx;
	GLdouble dy;
	// we can approximate the tangent vector at point (x_i,y_i) on the curve as (x_(i+1), y_(i+1))-(x_i,y_i)
	for (i = 0; i < numPoints; i++) {
		dx = subcurvePointer->curvePoints[i + 1].x - subcurvePointer->curvePoints[i].x;
		dy = subcurvePointer->curvePoints[i + 1].y - subcurvePointer->curvePoints[i].y;
		// to get n, we just need to flip around dx and dy
		GLdouble magnitude = sqrt(dx * dx + dy * dy);
		subcurvePointer->curvePoints[i].nx = -dy / magnitude;
		subcurvePointer->curvePoints[i].ny = dx / magnitude;
	}
}

void constructCircle(double radius, int numPoints, Vector2D* circlePts)
{
	double deltaTheta = 2 * M_PI / numPoints;
	double theta = 0.0;
	for (int i = 0; i < numPoints; i++)
	{
		theta += deltaTheta;
		double x = radius * cos(theta);
		double y = radius * sin(theta);
		circlePts[i].x = x;
		circlePts[i].y = y;
	}
}
