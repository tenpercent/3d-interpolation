#include "msr.hpp"
#include <QtCore/qmath.h>

MsrMatrix::MsrMatrix(quint32 size, quint32 nzcount):
	size(size),
	nzcount(nzcount),
	indices(new quint32[size+nzcount+1]),
	elements(new qreal[size+nzcount+1]),
	rightCol(new qreal[size])
{
	for (quint32 i = 0; i < size + nzcount + 1; ++i){
		indices[i] = 1111;
		elements[i] = 1111.;
	}
}

MsrMatrix::~MsrMatrix() {
	delete[] elements;
	delete[] indices;
	delete[] rightCol;
}

qreal MsrMatrix::getElement(quint32 i, quint32 j) const {
	quint32 index;
	if (i == j)
		return elements[i];
	else
		for (index = indices[i]; index < indices[i+1]; ++index)
			if (indices[index] == j)
				return elements[index];
	return 0;
}

void MsrMatrix::applyToVector(qreal *vector, qreal *newVector) const {
	quint32 i, ind;
	for (i = 0; i < size; ++i) {
		newVector[i] = elements[i] * vector[i];
		for (ind = indices[i]; ind < indices[i+1]; ++ind)
			newVector[i] += elements[ind] * vector[indices[ind]];
	}
}
qreal MsrMatrix::getResidual(qreal *vector) const {
	qreal result(0.), c;
	quint32 i, ind;
	for (i = 0; i < size; ++i) {
		c = elements[i] * vector[i];
		for (ind = indices[i]; ind < indices[i+1]; ++ind)
			c += elements[ind] * vector[indices[ind]];
		result += (rightCol[i] - c) * (rightCol[i] - c);
	}
	return qSqrt(result);
}

qreal MsrMatrix::scalarProduct(qreal *vector1, qreal *vector2) const {
	qreal result = 0;
	for (quint32 i = 0; i < size; ++i)
		result += vector1[i] * vector2[i];
	return result;
}

quint32 MsrMatrix::solve(qreal *result) const {
	qreal *r = new qreal[size];
	qreal *p = new qreal[size];
	qreal prevResidual(0), residual, alpha;
	quint32 i;
	for (i = 0; i < size; ++i) {
		result[i] = 0;
		r[i] = rightCol[i];
	}
	applyToVector(r, p);
	residual = getResidual(result);
	quint32 iterations = 0;
	while (qAbs(prevResidual - residual) > 1e-14) {
		++iterations;
		alpha = scalarProduct(p, r) / scalarProduct(p, p);
		for (i = 0; i < size; ++i) {
			result[i] += alpha * r[i];
			r[i] -= alpha * p[i];
		}
		applyToVector(r, p);
		prevResidual = residual;
		residual = getResidual(result);
	}
	delete[] r;
	delete[] p;
	return iterations;
}
