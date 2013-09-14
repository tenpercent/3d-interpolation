#include "threads.hpp"

void synchronize(Args *a) {
	pthread_mutex_lock(a->mutex);
	++(*(a->donethreads));
	if (*(a->donethreads) == a->threads) {
		pthread_cond_signal(a->condvar_tomf);
	}
	pthread_cond_wait(a->condvar_totf, a->mutex);
	pthread_mutex_unlock(a->mutex);
}
void taskGetNzCount(Args *a) {
	quint32 row, column;
	const quint32 side_points = a->segments + 1;
	const quint32 total_points = side_points * side_points;
	
	for (quint32 i = a->thread; i < total_points; i += a->threads) {
		row = i / side_points;
		//column = i - side_points * row;
		column = i % side_points;
		if (row == 0) {
			if (column == 0) {
				a->locnzc[i] = 2;
			}
			else if (column + 1 == side_points) {
				a->locnzc[i] = 3;
			}
			else {
				a->locnzc[i] = 4;
			}
		}
		else if (row == a->segments) {
			if (column == 0) {
				a->locnzc[i] = 3;
			}
			else if (column + 1 == side_points) {
				a->locnzc[i] = 2;
			}
			else {
				a->locnzc[i] = 4;
			}
		}
		else {
			if (column == 0) {
				a->locnzc[i] = 4;
			}
			else if (column + 1 == side_points) {
				a->locnzc[i] = 4;
			}
			else {
				a->locnzc[i] = 6;
			}
		}
	}
	return;
}
void taskFillMatrix(Args *a) {
	const quint32 msize = (a->segments + 1) * (a->segments + 1);
	//const quint32 msize2 = a->matrix->size;
	quint32 ind = msize + 1;
	qreal partialScalProd;
	TriangleList triangleList;
	Polynom polynom, interpolationPolynom;
	Vertex vertex1, vertex2;
	QPointF point1, point2;
	QPointF intPoints[3];
	for (quint32 i = 0; i < msize; ++i) {
		if (i % a->threads != a->thread) {
			ind += a->locnzc[i];
			continue;
		}
		a->matrix->indices[i] = ind;
		vertex1 = getVertexByIndex(a->segments, i);
		point1 = getPointFromVertex(a->points, a->segments, vertex1);
		for (quint32 j = 0; j < msize; ++j) {
			partialScalProd = 0.;
			vertex2 = getVertexByIndex(a->segments, j);
			point2 = getPointFromVertex(a->points, a->segments, vertex2);
			if (i == j) {
				//a->matrix->indices[i] = ind;
				triangleList = getSurroundingTriangles(a->points, a->segments, vertex1);
				// fill right part while we have that list
				a->matrix->rightCol[i] = 0.;
				for (int k = 0; k < triangleList.size(); ++k) {
					Triangle t = triangleList.at(k);
					intPoints[0] = t[0];
					intPoints[1] = t[1];
					intPoints[2] = t[2];
					interpolationPolynom = getLinearInterpolation(intPoints);
					polynom = polynomMultiply(
						phiFunctionPolynom(t, point1),
						interpolationPolynom);
					a->matrix->rightCol[i] += getIntegral(intPoints, polynom);
				}
				/*
				if (triangleList.isEmpty()) {}
				*/
				for (int k = 0; k < triangleList.size(); ++k) {
					polynom = polynomMultiply(
						phiFunctionPolynom(triangleList.at(k), point1),
						phiFunctionPolynom(triangleList.at(k), point2));
					partialScalProd += getIntegral(triangleList.at(k), polynom);
				}
				a->matrix->elements[i] = partialScalProd;
			} else { 
				triangleList = getCommonTriangles(a->points, a->segments, vertex1, vertex2);
				if (triangleList.isEmpty()) {
					continue;
				}
				for (int k = 0; k < triangleList.size(); ++k) {
					polynom = polynomMultiply(
						phiFunctionPolynom(triangleList.at(k), point1),
						phiFunctionPolynom(triangleList.at(k), point2));
					partialScalProd += getIntegral(triangleList.at(k), polynom);
				}
				a->matrix->elements[ind] = partialScalProd;
				a->matrix->indices[ind] = j;
				++ind;
			}
		}
	}
	a->matrix->indices[msize] = ind;
}

void taskFillValues(Args *a) {
	const quint32 meshDrawSize = (a->drawSegments + 1) * (a->drawSegments + 1);
	const quint32 meshCalcSize = (a->segments + 1) * (a->segments + 1);
	quint32 i, j;
	double phi;
	QPointF point;
	for (j = a->thread; j < meshDrawSize; j += a->threads) {
		a->values[j] = 0;
		point = getPointFromVertex(a->points, a->drawSegments, getVertexByIndex(a->segments, j));
		for (i = 0; i < meshCalcSize; ++i) {
			phi = phiFunctionGetByVertexIndex(a->points, a->segments, point, i);
			a->values[j] += a->alphas[i] * phi;
		}
	}
}

void *tf(void *args) {
	Args *a = (Args *)args;
	synchronize(a);
	while (a->task) {
		if (a->task == GetNzCount)
			taskGetNzCount(a);
		if (a->task == FillAndSolveMatrix)
			taskFillMatrix(a);
		if (a->task == FillValues)
			taskFillValues(a);
		synchronize(a);
	}
	return NULL;
}
