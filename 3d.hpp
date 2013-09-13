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
	Vertex (const Vertex& v):
		index(v.index), row(v.row), column(v.column)
	{}
//	inline Vertex& operator= (const Vertex & v);
};
/*
inline Vertex& Vertex::operator= (const Vertex &v){
	index = v.index;
	row = v.row;
	column = v.column;
	return *this;
}
*/
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
	double c;
};
//c*x^(xPow)*y^(yPow)
typedef QList<Triangle> TriangleList;
typedef QList<Monom> Polynom;

void getPoints (QPointF **points);

double phiFunctionGetByVertexIndex (QPointF * const points, const quint32 segments, const QPointF point, const quint32 vertexNumber);

quint32 getVertexIndex (const quint32 segments, const quint32 row, const quint32 column);

quint32 getIndexFromVertex (Vertex v);

QPointF getPointByMeshCoordinates (QPointF * const points, const quint32 segments, const quint32 row, const quint32 column);

Vertex getVertex (const quint32 row, const quint32 column, const quint32 index);

Vertex getVertexByIndex (const quint32 segments, const quint32 index);

Triangle getTriangleByIndex (QPointF *points, const quint32 segments, const quint32 triangle_index);

Polynom psiFunctionPolynom (const Triangle &triangle, const short n);

double psiFunction (const QPointF point, const Triangle &triangle, const short n);

bool pointInTriangle (const QPointF point, const Triangle &triangle);

double phiFunctionGetByTriangle (const QPointF point, const Triangle &triangle, const QPointF vertex);

Polynom phiFunctionPolynom (const Triangle &triangle, const QPointF vertex);

TriangleList getCommonTriangles (QPointF * const points, const quint32 segments, const Vertex v1, const Vertex v2);

TriangleList getSurroundingTriangles (QPointF * const points, const quint32 segments, const Vertex v1);

Triangle getTriangleByVertexAndLocalIndex (QPointF * const points, const quint32 segments, const Vertex v, const quint32 index);

double determinant (const double a11, const double a12, const double a13, const double a21,
const double a22, const double a23, const double a31, const double a32, const double a33);

Polynom getLinearInterpolation (QPointF * const p);

void addToPolynom(Polynom &p, const Monom &m);

void addToPolynom(Polynom &p, quint32 xPow, quint32 yPow, double c);

Polynom polynomMultiply(const Polynom &p1, const Polynom &p2);

double polynomIntegral(const Polynom &p);

double getIntegral(QPointF * const points, const Polynom &polynom);

double getIntegral(const Triangle &triangle, const Polynom &polynom);

double getIntegralForGoodTriangle(QPointF * const triangle, const Polynom &polynom);