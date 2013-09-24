#include "threads.hpp"
#include <QTextStream>
#include <QDebug>

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
	const quint32 side_points = a->calcSegments + 1;
	const quint32 total_points = side_points * side_points;
	
	for (quint32 i = a->thread; i < total_points; i += a->threads) {
		row = i / side_points;
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
		else if (row == a->calcSegments) {
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
	const quint32 msize = (a->calcSegments + 1) * (a->calcSegments + 1);
	quint32 ind = msize + 1;
	TriangleList triangleList;
	Polynom polynom, interpolationPolynom;
	Vertex vertex1, vertex2, vertex;
	QPointF point1, point2;
	QPointF intPoints[3];
	QPointF smallerIntPoints[3];
	qreal interpolationCoef[3];
	quint32 neighborCount(0);
	const qreal vectorProduct = ((a->points)[2].y() - (a->points)[0].y()) * ((a->points)[1].x() - (a->points)[0].x()) - 
						((a->points)[2].x() - (a->points)[0].x()) * ((a->points)[1].y() - (a->points)[0].y());
	const qreal smallTriangleArea = qAbs(vectorProduct) / (2 * a->calcSegments * a->calcSegments);
	
	for (quint32 i = 0; i < msize; ++i) {
		if (i % a->threads != a->thread) {
			ind += a->locnzc[i];
			continue;
		}
		a->matrix->indices[i] = ind;
		a->matrix->rightCol[i] = 0.;
		for (quint32 j = 0; j < msize; ++j) {
			if (i == j) {
				vertex = getVertexByIndex (a->calcSegments, j);
				neighborCount = getSurroundingNeighborCount (a->calcSegments, i);
				triangleList = getSurroundingTriangles (a->points, a->calcSegments, vertex);
				a->matrix->elements[i] = neighborCount * smallTriangleArea * (1./6);
				//not forgetting to fill right part
				for (qint8 k = 0; k < triangleList.size(); ++k){
					Triangle t = triangleList.at(k);
					intPoints[0] = t[0];
					intPoints[1] = t[1];
					intPoints[2] = t[2];

					//	use points of triangulation for the right part linear interpolation
					//	as a result, the accuracy of the method is the same as of the one using linear interpolation
					// 	some people might get angry and shout, however, it works

					//getLinearInterpolationPlane (intPoints, interpolationCoef);
					//a->matrix->rightCol[i] += (1./12) * smallTriangleArea * (interpolationCoef[0] + interpolationCoef[1] + 4 * interpolationCoef[2]);

					//	Honestly, I tried to make it work with a smaller triangulation but failed
					//	Thus, I leave it to the future generations
			
					smallerIntPoints[0] = t[0];
					smallerIntPoints[1] = .5 * (t[0] + t[1]);
					smallerIntPoints[2] = .5 * (t[0] + t[2]);
					getLinearInterpolationPlane_1 (smallerIntPoints, interpolationCoef);
					a->matrix->rightCol[i] += (1./192) * smallTriangleArea * (interpolationCoef[0] * 5 + interpolationCoef[1] * 5 + interpolationCoef[2] * 32);

					smallerIntPoints[0] = .5 * (t[0] + t[1]);
					smallerIntPoints[1] = t[1];
					smallerIntPoints[2] = .5 * (t[1] + t[2]);
					getLinearInterpolationPlane_2 (smallerIntPoints, interpolationCoef);
					a->matrix->rightCol[i] += (1./192) * smallTriangleArea * (interpolationCoef[0] * 1 + interpolationCoef[1] * 5 + interpolationCoef[2] * 8);

					smallerIntPoints[0] = .5 * (t[0] + t[2]);
					smallerIntPoints[1] = .5 * (t[1] + t[2]);
					smallerIntPoints[2] = t[2];
					getLinearInterpolationPlane_3 (smallerIntPoints, interpolationCoef);
					a->matrix->rightCol[i] += (1./192) * smallTriangleArea * (interpolationCoef[0] * 5 + interpolationCoef[1] * 1 + interpolationCoef[2] * 8);

					smallerIntPoints[0] = .5 * (t[0] + t[1]);
					smallerIntPoints[1] = .5 * (t[1] + t[2]);
					smallerIntPoints[2] = .5 * (t[2] + t[0]);
					getLinearInterpolationPlane_4 (smallerIntPoints, interpolationCoef);
					a->matrix->rightCol[i] += (1./192) * smallTriangleArea * (interpolationCoef[0] * 5 + interpolationCoef[1] * 5 + interpolationCoef[2] * 16);
					
					//Though, it sort of works
				}
			}
			else{
				neighborCount = getCommonNeighborCount (a->calcSegments, i, j);
				if (neighborCount != 0){
					a->matrix->elements[ind] = neighborCount * smallTriangleArea * (1./12);
					a->matrix->indices[ind] = j;
					++ind;
				}
			}
		}
	}
	a->matrix->indices[msize] = ind;
}

void taskFillValues(Args *a) {
	const quint32 meshDrawSize = (a->drawSegments + 1) * (a->drawSegments + 1);
	const quint32 meshCalcSize = (a->calcSegments + 1) * (a->calcSegments + 1);
	qreal phi (0);
	QPointF point;
	for (quint32 i = a->thread; i < meshDrawSize; i += a->threads) {
		a->values[i] = 0.;
		point = getPointByMeshIndex(a->points, a->drawSegments, i);
		for (quint32 j = 0; j < meshCalcSize; ++j) {
			phi = phiFunctionGetByVertexIndex(a->points, a->calcSegments, point, j);
			a->values[i] += a->alphas[j] * phi;
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
