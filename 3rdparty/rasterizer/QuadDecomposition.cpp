#include "QuadDecomposition.h"

#include "VectorMath.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <queue>
#include <random>
#include <tuple>
#include <unordered_map>
#include <vector>

typedef int Vertex;

typedef std::vector<Vertex> Path;

struct Graph
{
  size_t numVertices() const
  {
    return m_adjacencyList.size();
  }

  std::vector<std::vector<Vertex>> m_adjacencyList;
};

class Matching
{
public:
  Matching(const Graph& graph) : m_graph(graph), m_matchedVertex(graph.numVertices(), -1), m_bridges(graph.numVertices()), m_clearToken(0), m_tree(graph.numVertices())
  {
    std::vector<Vertex> unmatchedVertices;

    // Start with a greedy maximal matching
    for (Vertex v = 0; v < m_graph.numVertices(); ++v)
    {
      if (m_matchedVertex[v] == -1)
      {
        bool found = false;
        for (auto w : m_graph.m_adjacencyList[v])
        {
          if (m_matchedVertex[w] == -1)
          {
            match(v, w);
            found = true;
            break;
          }
        }

        if (!found)
        {
          unmatchedVertices.push_back(v);
        }
      }
    }

    std::vector<Vertex> path;
    for (auto v : unmatchedVertices)
    {
      if (m_matchedVertex[v] == -1)
      {
        if (findAugmentingPath(v, path))
        {
          augment(path);
          path.clear();
        }
      }
    }
  }

  Vertex getMatchedVertex(Vertex v)
  {
    return m_matchedVertex[v];
  }

private:
  void match(Vertex v, Vertex w)
  {
    m_matchedVertex[v] = w;
    m_matchedVertex[w] = v;
  }

  void augment(std::vector<Vertex>& path)
  {
    for (int i = 0; i < path.size(); i += 2)
    {
      match(path[i], path[i + 1]);
    }
  }

  bool findAugmentingPath(Vertex root, std::vector<Vertex> & path)
  {
    // Clear out the forest
    size_t numVertices = m_graph.numVertices();

    m_clearToken++;

    // Start our tree root
    m_tree[root].m_depth = 0;
    m_tree[root].m_parent = -1;
    m_tree[root].m_clearToken = m_clearToken;
    m_tree[root].m_blossom = root;

    m_queue.push(root);

    while (!m_queue.empty())
    {
      Vertex v = m_queue.front();
      m_queue.pop();

      for (auto w : m_graph.m_adjacencyList[v])
      {
        if (examineEdge(root, v, w, path))
        {
          while (!m_queue.empty())
          {
            m_queue.pop();
          }

          return true;
        }
      }
    }

    return false;
  }

  bool examineEdge(Vertex root, Vertex v, Vertex w, std::vector<Vertex> & path)
  {
    Vertex vBar = find(v);
    Vertex wBar = find(w);

    if (vBar != wBar)
    {
      if (m_tree[wBar].m_clearToken != m_clearToken)
      {
        if (m_matchedVertex[w] == -1)
        {
          buildAugmentingPath(root, v, w, path);
          return true;
        }
        else
        {
          extendTree(v, w);
        }
      }
      else if (m_tree[wBar].m_depth % 2 == 0)
      {
        shrinkBlossom(v, w);
      }
    }

    return false;
  }

  void buildAugmentingPath(Vertex root, Vertex v, Vertex w, std::vector<Vertex> & path)
  {
    path.push_back(w);
    findPath(v, root, path);
  }

  void extendTree(Vertex v, Vertex w)
  {
    Vertex u = m_matchedVertex[w];

    Node& nodeV = m_tree[v];
    Node& nodeW = m_tree[w];
    Node& nodeU = m_tree[u];

    nodeW.m_depth = nodeV.m_depth + 1 + (nodeV.m_depth & 1);	// Must be odd, so we add either 1 or 2
    nodeW.m_parent = v;
    nodeW.m_clearToken = m_clearToken;
    nodeW.m_blossom = w;

    nodeU.m_depth = nodeW.m_depth + 1;
    nodeU.m_parent = w;
    nodeU.m_clearToken = m_clearToken;
    nodeU.m_blossom = u;

    m_queue.push(u);
  }

  void shrinkBlossom(Vertex v, Vertex w)
  {
    Vertex b = findCommonAncestor(v, w);

    shrinkPath(b, v, w);
    shrinkPath(b, w, v);
  }

  void shrinkPath(Vertex b, Vertex v, Vertex w)
  {
    Vertex u = find(v);

    while (u != b)
    {
      makeUnion(b, u);
      assert(u != -1);
      assert(m_matchedVertex[u] != -1);
      u = m_matchedVertex[u];
      makeUnion(b, u);
      makeRepresentative(b);
      m_queue.push(u);
      m_bridges[u] = std::make_pair(v, w);
      u = find(m_tree[u].m_parent);
    }
  }

  Vertex findCommonAncestor(Vertex v, Vertex w)
  {
    while (w != v)
    {
      if (m_tree[v].m_depth > m_tree[w].m_depth)
      {
        v = m_tree[v].m_parent;
      }
      else
      {
        w = m_tree[w].m_parent;
      }
    }

    return find(v);
  }

  void findPath(Vertex s, Vertex t, Path & path)
  {
    if (s == t)
    {
      path.push_back(s);
    }
    else if (m_tree[s].m_depth % 2 == 0)
    {
      path.push_back(s);
      path.push_back(m_matchedVertex[s]);
      findPath(m_tree[m_matchedVertex[s]].m_parent, t, path);
    }
    else
    {
      Vertex v, w;
      std::tie(v, w) = m_bridges[s];

      path.push_back(s);

      size_t offset = path.size();
      findPath(v, m_matchedVertex[s], path);
      std::reverse(path.begin() + offset, path.end());

      findPath(w, t, path);
    }
  }

  void makeUnion(int x, int y)
  {
    int xRoot = find(x);
    m_tree[xRoot].m_blossom = find(y);
  }

  void makeRepresentative(int x)
  {
    int xRoot = find(x);
    m_tree[xRoot].m_blossom = x;
    m_tree[x].m_blossom = x;
  }

  int find(int x)
  {
    if (m_tree[x].m_clearToken != m_clearToken)
    {
      return x;
    }

    if (x != m_tree[x].m_blossom)
    {
      // Path compression
      m_tree[x].m_blossom = find(m_tree[x].m_blossom);
    }

    return m_tree[x].m_blossom;
  }


private:
  int m_clearToken;

  const Graph& m_graph;

  std::queue<Vertex> m_queue;
  std::vector<Vertex> m_matchedVertex;

  struct Node
  {
    Node()
    {
      m_clearToken = 0;
    }

    int m_depth;
    Vertex m_parent;
    Vertex m_blossom;

    int m_clearToken;
  };

  std::vector<Node> m_tree;

  std::vector<std::pair<Vertex, Vertex>> m_bridges;
};


namespace
{
  struct PairHash
  {
  public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U>& x) const
    {
      auto hashT = std::hash<T>{}(x.first);
      auto hashU = std::hash<U>{}(x.second);
      return hashT ^ (hashU + 0x9e3779b9 + (hashT << 6) + (hashT >> 2));
    }
  };

  bool canMergeTrianglesToQuad(__m128 v0, __m128 v1, __m128 v2, __m128 v3)
  {
    // Maximum distance of vertices from original plane in world space units
    float maximumDepthError = 0.5f;

    __m128 n0 = normalize(normal(v0, v1, v2));
    __m128 n2 = normalize(normal(v2, v3, v0));

    __m128 planeDistA = _mm_andnot_ps(_mm_set1_ps(-0.0f), _mm_dp_ps(n0, _mm_sub_ps(v1, v3), 0x7F));
    __m128 planeDistB = _mm_andnot_ps(_mm_set1_ps(-0.0f), _mm_dp_ps(n2, _mm_sub_ps(v1, v3), 0x7F));

    if (_mm_comigt_ss(planeDistA, _mm_set1_ps(maximumDepthError)) || _mm_comigt_ss(planeDistB, _mm_set1_ps(maximumDepthError)))
    {
      return false;
    }

    return true;
  }
}

std::vector<uint32_t> QuadDecomposition::decompose(const std::vector<uint32_t>& indices, const std::vector<__m128>& vertices)
{
  std::vector<uint32_t> result;

  size_t triangleCount = indices.size() / 3;

  Graph candidateGraph;
  candidateGraph.m_adjacencyList.resize(triangleCount);

  std::unordered_map<std::pair<uint32_t, uint32_t>, std::vector<std::pair<uint32_t, uint32_t>>, PairHash> edgeToTriangle;

  for (uint32_t triangleIdx = 0; triangleIdx < triangleCount; ++triangleIdx)
  {
    uint32_t i[3];
    i[0] = indices[3 * triangleIdx + 0];
    i[1] = indices[3 * triangleIdx + 1];
    i[2] = indices[3 * triangleIdx + 2];

    edgeToTriangle[std::make_pair(i[0], i[1])].push_back(std::make_pair(triangleIdx, i[2]));
    edgeToTriangle[std::make_pair(i[1], i[2])].push_back(std::make_pair(triangleIdx, i[0]));
    edgeToTriangle[std::make_pair(i[2], i[0])].push_back(std::make_pair(triangleIdx, i[1]));

    for (int edgeIdx = 0; edgeIdx < 3; ++edgeIdx)
    {
      auto f = std::make_pair(i[(edgeIdx + 1) % 3], i[edgeIdx]);

      auto & neighbors = edgeToTriangle[f];
      for (auto pair : neighbors)
      {
        uint32_t neighborTriangle = pair.first;
        uint32_t apex = pair.second;

        uint32_t quad[] = { i[edgeIdx], apex, i[(edgeIdx + 1) % 3], i[(edgeIdx + 2) % 3] };

        if (canMergeTrianglesToQuad(vertices[quad[0]], vertices[quad[1]], vertices[quad[2]], vertices[quad[3]]))
        {
          candidateGraph.m_adjacencyList[triangleIdx].push_back(neighborTriangle);
          candidateGraph.m_adjacencyList[neighborTriangle].push_back(triangleIdx);
        }
      }
    }
  }


  uint32_t quadCount = 0;
  uint32_t trigleCount = 0;

  Matching matching(candidateGraph);

  for (uint32_t triangleIdx = 0; triangleIdx < triangleCount; ++triangleIdx)
  {
    int neighbor = matching.getMatchedVertex(triangleIdx);

    // No quad found
    if (neighbor == -1)
    {
      auto i0 = indices[3 * triangleIdx + 0];
      auto i1 = indices[3 * triangleIdx + 1];
      auto i2 = indices[3 * triangleIdx + 2];

      result.push_back(i0);
      result.push_back(i2);
      result.push_back(i1);
      result.push_back(i0);
    }
    else if (triangleIdx < uint32_t(neighbor))
    {
      uint32_t i[3];
      i[0] = indices[3 * triangleIdx + 0];
      i[1] = indices[3 * triangleIdx + 1];
      i[2] = indices[3 * triangleIdx + 2];

      // Find out which edge was matched
      for (uint32_t edgeIdx = 0; edgeIdx < 3; ++edgeIdx)
      {
        auto f = std::make_pair(i[(edgeIdx + 1) % 3], i[edgeIdx]);
        auto & neighbors = edgeToTriangle[f];
        for (auto pair : neighbors)
        {
          if (pair.first == neighbor)
          {
            result.push_back(i[edgeIdx]);
            result.push_back(i[(edgeIdx + 2) % 3]);
            result.push_back(i[(edgeIdx + 1) % 3]);
            result.push_back(pair.second);

            quadCount++;

            goto nextTriangle;
          }
        }
      }
    }

  nextTriangle:
    continue;
  }

  return result;
}
