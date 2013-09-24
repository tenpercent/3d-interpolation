#include <QPointF>
#include <QList>
#include "msr.hpp"

class Vertex {
public:
	quint32 index;
	quint32 row;
	quint32 column;
	Vertex(): 
		index (0), row (0), column (0)
	{}
	Vertex (const quint32 row, const quint32 column, const quint32 index): 
		index(index), row(row), column(column)
	{}
};

QPointF getPointFromVertex (QPointF * const points, const quint32 segments, const Vertex v);

class Triangle {
private:
	QPointF *points;
	quint32 segments;
public:
	Vertex a, b, c;
	Triangle ()
	{}
	Triangle (QPointF * const points, const quint32 segments, const Vertex a, const Vertex b, const Vertex c):
		points(points), segments(segments), a(a), b(b), c(c)
	{}
	QPointF operator[] (const quint32 i) const {
		Vertex v;
		switch (i) {
			case 0:
				v = a; break;
			case 1:
				v = b; break;
			case 2:
			 	v = c; break;
			default: 
				return QPointF();
		}
		return getPointFromVertex (points, segments, v);
	}
};
struct Monom {
	quint32 xPow;
	quint32 yPow;
	qreal c;
};
//c*x^(xPow)*y^(yPow)
typedef QList<Triangle> TriangleList;
typedef QList<Monom> Polynom;

quint32 getPoints (QPointF **points);

qreal phiFunctionGetByVertexIndex (QPointF * const points, const quint32 segments, const QPointF point, const quint32 vertexNumber);

quint32 getVertexIndex (const quint32 segments, const quint32 row, const quint32 column);

quint32 getIndexFromVertex (Vertex v);

QPointF getPointByMeshCoordinates (QPointF * const points, const quint32 segments, const quint32 row, const quint32 column);

QPointF getPointByMeshIndex (QPointF * const points, const quint32 segments, const quint32 index);

Vertex getVertex (const quint32 row, const quint32 column, const quint32 index);

Vertex getVertexByIndex (const quint32 segments, const quint32 index);

Triangle getTriangleByIndex (QPointF *points, const quint32 segments, const quint32 triangle_index);

Polynom psiFunctionPolynom (const Triangle &triangle, const short n);

qreal psiFunction (const QPointF point, const Triangle &triangle, const short n);

bool pointInTriangle (const QPointF point, const Triangle &triangle);

qreal phiFunctionGetByTriangle (const QPointF point, const Triangle &triangle, const QPointF vertex);

Polynom phiFunctionPolynom (const Triangle &triangle, const QPointF vertex);

TriangleList getCommonTriangles (QPointF * const points, const quint32 segments, const Vertex v1, const Vertex v2);

TriangleList getSurroundingTriangles (QPointF * const points, const quint32 segments, const Vertex v1);

Triangle getTriangleByVertexAndLocalIndex (QPointF * const points, const quint32 segments, const Vertex v, const quint32 index);

qreal determinant (const qreal a11, const qreal a12, const qreal a13, const qreal a21,
const qreal a22, const qreal a23, const qreal a31, const qreal a32, const qreal a33);

Polynom getLinearInterpolation (QPointF * const p);

void addToPolynom(Polynom &p, const Monom &m);

void addToPolynom(Polynom &p, quint32 xPow, quint32 yPow, qreal c);

Polynom polynomMultiply(const Polynom &p1, const Polynom &p2);

qreal polynomIntegral(const Polynom &p);

qreal getIntegral(QPointF * const points, const Polynom &polynom);

qreal getIntegral(const Triangle &triangle, const Polynom &polynom);

qreal getIntegralForGoodTriangle(QPointF * const triangle, const Polynom &polynom);

quint32 getSurroundingNeighborCount (const quint32 segments, const quint32 index);

quint32 getCommonNeighborCount (const quint32 segments, const quint32 i_index, const quint32 j_index);

void getLinearInterpolationPlane (QPointF* p, qreal* coef);

void getLinearInterpolationPlane_1 (QPointF* p, qreal* coef);

void getLinearInterpolationPlane_2 (QPointF* p, qreal* coef);

void getLinearInterpolationPlane_3 (QPointF* p, qreal* coef);

void getLinearInterpolationPlane_4 (QPointF* p, qreal* coef);
