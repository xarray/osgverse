/**
 * \file Poisson.cpp
 * \brief
 *
 * Poisson Disk Points Generator
 *
 * \version 1.1.3
 * \date 10/03/2016
 * \author Sergey Kosarevsky, 2014-2016
 * \author support@linderdaum.com   http://www.linderdaum.com   http://blog.linderdaum.com
 */

 /*
     Usage example:

         PoissonGenerator::DefaultPRNG PRNG;
         const auto Points = PoissonGenerator::GeneratePoissonPoints( NumPoints, PRNG );
 */

 // Fast Poisson Disk Sampling in Arbitrary Dimensions
 // http://people.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf

 // Implementation based on http://devmag.org.za/2009/05/03/poisson-disk-sampling/

 /* Versions history:
  *		1.1.3		Mar 10, 2016		Header-only library, no global mutable state
  *		1.1.2		Apr  9, 2015		Output a text file with XY coordinates
  *		1.1.1		May 23, 2014		Initialize PRNG seed, fixed uninitialized fields
  *    1.1		May  7, 2014		Support of density maps
  *		1.0		May  6, 2014
 */

#include <vector>
#include <random>
#include <iostream>
#include <stdint.h>
#include <time.h>

#define POISSON_PROGRESS_INDICATOR 0

namespace PoissonGenerator
{

    const char* Version = "1.1.3 (10/03/2016)";

    class DefaultPRNG
    {
    public:
        DefaultPRNG()
            : m_RD()
            , m_Gen(m_RD())
            , m_Dis(0.0f, 1.0f)
        {
            // prepare PRNG
            m_Gen.seed(time(nullptr));
        }

        float RandomFloat()
        {
            return static_cast<float>(m_Dis(m_Gen));
        }

        int RandomInt(int Max)
        {
            std::uniform_int_distribution<> DisInt(0, Max);
            return DisInt(m_Gen);
        }

    private:
        std::random_device m_RD;
        std::mt19937 m_Gen;
        std::uniform_real_distribution<float> m_Dis;
    };

    struct sPoint
    {
        sPoint()
            : x(0)
            , y(0)
            , m_Valid(false)
        {}
        sPoint(float X, float Y)
            : x(X)
            , y(Y)
            , m_Valid(true)
        {}
        float x;
        float y;
        bool m_Valid;
        //
        bool IsInRectangle() const
        {
            return x >= 0 && y >= 0 && x <= 1 && y <= 1;
        }
        //
        bool IsInCircle() const
        {
            float fx = x - 0.5f;
            float fy = y - 0.5f;
            return (fx*fx + fy * fy) <= 0.25f;
        }
    };

    struct sGridPoint
    {
        sGridPoint(int X, int Y)
            : x(X)
            , y(Y)
        {}
        int x;
        int y;
    };

    float GetDistance(const sPoint& P1, const sPoint& P2)
    {
        return sqrt((P1.x - P2.x) * (P1.x - P2.x) + (P1.y - P2.y) * (P1.y - P2.y));
    }

    sGridPoint ImageToGrid(const sPoint& P, float CellSize)
    {
        return sGridPoint((int)(P.x / CellSize), (int)(P.y / CellSize));
    }

    struct sGrid
    {
        sGrid(int W, int H, float CellSize)
            : m_W(W)
            , m_H(H)
            , m_CellSize(CellSize)
        {
            m_Grid.resize(m_H);

            for (auto i = m_Grid.begin(); i != m_Grid.end(); i++) { i->resize(m_W); }
        }
        void Insert(const sPoint& P)
        {
            sGridPoint G = ImageToGrid(P, m_CellSize);
            m_Grid[G.x][G.y] = P;
        }
        bool IsInNeighbourhood(sPoint Point, float MinDist, float CellSize)
        {
            sGridPoint G = ImageToGrid(Point, CellSize);

            // number of adjucent cells to look for neighbour points
            const int D = 5;

            // scan the neighbourhood of the point in the grid
            for (int i = G.x - D; i < G.x + D; i++)
            {
                for (int j = G.y - D; j < G.y + D; j++)
                {
                    if (i >= 0 && i < m_W && j >= 0 && j < m_H)
                    {
                        sPoint P = m_Grid[i][j];

                        if (P.m_Valid && GetDistance(P, Point) < MinDist) { return true; }
                    }
                }
            }


            return false;
        }

    private:
        int m_W;
        int m_H;
        float m_CellSize;

        std::vector< std::vector<sPoint> > m_Grid;
    };

    template <typename PRNG>
    sPoint PopRandom(std::vector<sPoint>& Points, const PRNG& Generator)
    {
        PRNG& GeneratorN = const_cast<PRNG&>(Generator);
        const int Idx = GeneratorN.RandomInt(Points.size() - 1);
        const sPoint P = Points[Idx];
        Points.erase(Points.begin() + Idx);
        return P;
    }

    template <typename PRNG>
    sPoint GenerateRandomPointAround(const sPoint& P, float MinDist, const PRNG& Generator)
    {
        // start with non-uniform distribution
        PRNG& GeneratorN = const_cast<PRNG&>(Generator);
        float R1 = GeneratorN.RandomFloat();
        float R2 = GeneratorN.RandomFloat();

        // radius should be between MinDist and 2 * MinDist
        float Radius = MinDist * (R1 + 1.0f);

        // random angle
        float Angle = 2 * 3.141592653589f * R2;

        // the new point is generated around the point (x, y)
        float X = P.x + Radius * cos(Angle);
        float Y = P.y + Radius * sin(Angle);

        return sPoint(X, Y);
    }

    /**
        Return a vector of generated points

        NewPointsCount - refer to bridson-siggraph07-poissondisk.pdf for details (the value 'k')
        Circle  - 'true' to fill a circle, 'false' to fill a rectangle
        MinDist - minimal distance estimator, use negative value for default
    **/
    template <typename PRNG = DefaultPRNG>
    std::vector<sPoint> GeneratePoissonPoints(
        size_t NumPoints,
        const PRNG& Generator,
        int NewPointsCount = 30,
        bool Circle = true,
        float MinDist = -1.0f
    )
    {
        if (MinDist < 0.0f)
        {
            MinDist = sqrt(float(NumPoints)) / float(NumPoints);
        }

        std::vector<sPoint> SamplePoints;
        std::vector<sPoint> ProcessList;

        // create the grid
        float CellSize = MinDist / sqrt(2.0f);

        int GridW = (int)ceil(1.0f / CellSize);
        int GridH = (int)ceil(1.0f / CellSize);

        PRNG& GeneratorN = const_cast<PRNG&>(Generator);
        sGrid Grid(GridW, GridH, CellSize);

        sPoint FirstPoint;
        do {
            FirstPoint = sPoint(GeneratorN.RandomFloat(), GeneratorN.RandomFloat());
        } while (!(Circle ? FirstPoint.IsInCircle() : FirstPoint.IsInRectangle()));

        // update containers
        ProcessList.push_back(FirstPoint);
        SamplePoints.push_back(FirstPoint);
        Grid.Insert(FirstPoint);

        // generate new points for each point in the queue
        while (!ProcessList.empty() && SamplePoints.size() < NumPoints)
        {
#if POISSON_PROGRESS_INDICATOR
            // a progress indicator, kind of
            if (SamplePoints.size() % 100 == 0) std::cout << ".";
#endif // POISSON_PROGRESS_INDICATOR

            sPoint Point = PopRandom<PRNG>(ProcessList, Generator);

            for (int i = 0; i < NewPointsCount; i++)
            {
                sPoint NewPoint = GenerateRandomPointAround(Point, MinDist, Generator);

                bool Fits = Circle ? NewPoint.IsInCircle() : NewPoint.IsInRectangle();

                if (Fits && !Grid.IsInNeighbourhood(NewPoint, MinDist, CellSize))
                {
                    ProcessList.push_back(NewPoint);
                    SamplePoints.push_back(NewPoint);
                    Grid.Insert(NewPoint);
                    continue;
                }
            }
        }

#if POISSON_PROGRESS_INDICATOR
        std::cout << std::endl << std::endl;
#endif // POISSON_PROGRESS_INDICATOR

        return SamplePoints;
    }
}
