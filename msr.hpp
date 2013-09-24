#include <QtGlobal>
#define RIGHT_PART_QUAD_INTERPOLATION
class MsrMatrix {
public:
	const quint32 size;
	const quint32 nzcount;
	quint32 *indices;
	qreal *elements;
	qreal *rightCol;

	MsrMatrix(const quint32 size, const quint32 nzcount);
	~MsrMatrix();
	qreal getElement(quint32 i, quint32 j) const;
	quint32 solve(qreal *result) const;

private:
	qreal scalarProduct(qreal *vector1, qreal *vector2) const;
	void applyToVector(qreal *vector, qreal *newVector) const;
	qreal getResidual(qreal *vector) const;
};
